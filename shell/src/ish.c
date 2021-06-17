#include "parse.h"
#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// max line size including newline
#define MAX_LINE_SIZE 1023

enum ProcState {
  UNDEF,
  FOREGROUND,
  BACKGROUND,
  TERMINATED,
};

struct Job {
  pid_t pid; // process id
  int jid;   // job id
  enum ProcState state;
};

DynArray_T jobs;
char *prompt = "% ";
pid_t fg_pid;

static int allocate_job_id() {
  static int job_id = 1;
  return job_id++;
}

void evaluate(char *);
bool handle_if_builtin(DynArray_T);
void count_pipes(void *, void *);
pid_t run_command(int, int, char **);
char **construct_args(DynArray_T, int *);

struct Job *addjob(pid_t, enum ProcState);
bool deletejob(pid_t);
void clearjob(struct Job *);

void sigchld_handler(int);

int main(int argc, char **argv) {
  // one space for null, another for checking overflow
  char cmd[MAX_LINE_SIZE + 2];
  int length;

  jobs = DynArray_new(0);
  fg_pid = 0;

  signal(SIGCHLD, sigchld_handler);

  while (true) {
    printf("%s", prompt);

    if (fgets(cmd, MAX_LINE_SIZE, stdin) == NULL) {
      // EOF, if code has reached here

      printf ("\n"); // print newline before exiting
      exit(0);
    }

    length = strlen(cmd);
    if (length > MAX_LINE_SIZE) {
      printf("file name too long\n");
      continue;
    }

    // remove newline for ease of parsing
    if (cmd[length - 1] == '\n')
      cmd[length - 1] = '\0';

    evaluate(cmd);

    fflush(stdout);
    fflush(stdout);
  }

  return 0;
}

void evaluate(char *cmd) {
  DynArray_T tokens;
  enum ParseResult result;
  bool is_builtin;
  int pipes, fd_in = STDIN_FILENO, fd_out = 0, token_cursor = 0;
  int fd[2];
  char **argv;
  /* pid_t pid; */
  sigset_t mask_all, mask_child, mask_prev;

  sigfillset(&mask_all);
  sigemptyset(&mask_child);
  sigaddset(&mask_child, SIGCHLD);

  tokens = DynArray_new(0);
  if (tokens == NULL) {
    fprintf(stderr, "Cannot allocate memory\n");
    exit(EXIT_FAILURE);
  }

  result = parse_line(cmd, tokens);

  // 1. tokens must not be empty
  // 2. first token MUST be a command
  if (!DynArray_getLength(tokens) ||
      ((struct Token *) DynArray_get(tokens, 0))->type != TOKEN_WORD) {
    free_line(tokens);
    return;
  }

  if (result != PARSE_SUCCESS) {
    free_line(tokens);
    return;
  }

  /* if (result == PARSE_SUCCESS) { */
  /*   DynArray_map(tokens, printToken, NULL); */
  /* } */

  is_builtin = handle_if_builtin(tokens);
  if (is_builtin) {
    free_line(tokens);
    return;
  }

  pipes = 0;
  DynArray_map(tokens, count_pipes, &pipes);

  /* printf ("# of pipes: %d\n", pipes); */

  for (int i = 0; i <= pipes; i++) {
    struct Job *job;
    pid_t pid;

    pipe(fd);
    argv = construct_args(tokens, &token_cursor);
    // the last command needs to print out to stdout
    fd_out = i == pipes ? STDOUT_FILENO : fd[1];

    sigprocmask(SIG_BLOCK, &mask_child, &mask_prev);
    pid = run_command (fd_in, fd_out, argv);
    if (pid < 0) {
      // TODO: error handling!
    }

    // block signals before adding job
    sigprocmask(SIG_BLOCK, &mask_all, NULL);
    job = addjob(pid, FOREGROUND);
    sigprocmask(SIG_SETMASK, &mask_prev, NULL);

    // parent does not use argv
    free(argv);
    close(fd[1]);
    fd_in = fd[0];

    while (job->state == FOREGROUND)
      sleep (1);
  }

  // close the last pipe's input fd, as no process uses it
  close(fd_in);

  free_line(tokens);
}

pid_t run_command(int fd_in, int fd_out, char **argv) {
  pid_t pid = fork();
  if (pid == 0) {
    fg_pid = getpid();

    if (fd_in != STDIN_FILENO) {
      dup2(fd_in, 0);
      close(fd_in);
    }

    if (fd_out != STDOUT_FILENO) {
      dup2(fd_out, 0);
      close(fd_out);
    }

    if (execvp(argv[0], argv) < 0)
      return -1;
  }

  return pid;
}

bool handle_if_builtin(DynArray_T tokens) {
  // length of more than 0 should be guaranteed
  assert(DynArray_getLength(tokens));

  bool is_built_in = false;
  int length = DynArray_getLength(tokens);
  struct Token *first = DynArray_get(tokens, 0);
  if (first->type != TOKEN_WORD)
    return false;

  if (!strcmp(first->value, "setenv")) {
    char *var, *val;

    assert(length >= 2);
    var = ((struct Token *)DynArray_get(tokens, 1))->value;
    val = length == 2 ? "" : ((struct Token *)DynArray_get(tokens, 2))->value;
    setenv(var, val, 1);

    return true;
  } else if (!strcmp(first->value, "unsetenv")) {
    char *var;

    assert(length >= 2);
    var = ((struct Token *)DynArray_get(tokens, 1))->value;
    unsetenv(var);

    return true;
  } else if (!strcmp(first->value, "cd")) {
    char *dir = length == 2 ? ((struct Token *)DynArray_get(tokens, 1))->value
                            : getenv("HOME");
    if (!chdir(dir)) {
      // TODO: print error message
    }

    return true;
  } else if (!strcmp(first->value, "exit")) {
    exit(0);
  } else if (!strcmp(first->value, "fg")) {
    return true;
  }

  return is_built_in;
}

void count_pipes(void *element, void *extra) {
  struct Token *token = element;
  int *sum = extra;

  if (token->type == TOKEN_PIPE)
    (*sum)++;
}

// construct an array of strings from a tokens dynarray
// tokenCursor will be updated to point to the next command token
char **construct_args(DynArray_T tokens, int *token_cursor) {
  int i = *token_cursor, j;
  int length = DynArray_getLength(tokens);
  int argc = 0;
  struct Token *token;

  while (i < length && (token = DynArray_get(tokens, i))->type == TOKEN_WORD) {
    argc++;
    i++;
  }

  char **argv = calloc(argc + 1, sizeof(char *));

  // iterate again
  i = *token_cursor;
  j = 0;
  while (i < length && (token = DynArray_get(tokens, i))->type == TOKEN_WORD) {
    argv[j] = token->value;
    i++;
    j++;
  }
  argv[argc] = NULL;

  while (i < length && (token = DynArray_get(tokens, i))->type != TOKEN_WORD) {
    argv[j] = token->value;
    i++;
    j++;
  }

  *token_cursor = i;

  return argv;
}

void sigchld_handler(int sig) {
  int status;
  pid_t pid;
  sigset_t mask_all, mask_original;

  sigfillset(&mask_all);

  while ((pid = waitpid(-1, &status, 0)) > 0) {
    sigprocmask(SIG_BLOCK, &mask_all, &mask_original);
    deletejob(pid);
    sigprocmask(SIG_SETMASK, &mask_original, NULL);

  if (errno == ECHILD)
    return;
}

struct Job *addjob(pid_t pid, enum ProcState state) {
  struct Job *new_job = calloc(1, sizeof(struct Job));
  if (new_job == NULL) {
    // TODO: error handling!
  }

  new_job->pid = pid;
  new_job->jid = allocate_job_id();
  new_job->state = state;

  // return null if it fails
  if (!DynArray_add(jobs, new_job)) {
    free (new_job);
    return NULL;
  }

  return new_job;
}

bool deletejob(pid_t pid) {
  for (int i = 0; i < DynArray_getLength(jobs); i++) {
    struct Job *job = DynArray_get(jobs, i);
    if (job->pid != pid) continue;

    // (job->pid == pid) from here!
    job->state = TERMINATED;
    return true;
  }

  return false;
}
