#include "parse.h"
#include "job.h"
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

DynArray_T jobs;
char *prompt = "% ";
char *filename;
bool sigquit_active;

static void count_pipes(void *, void *);
static bool is_background(DynArray_T);

void wait_fg(struct Job *);
void evaluate(char *);
bool handle_if_builtin(DynArray_T);
pid_t run_command(int, int, char **);
char **construct_args(DynArray_T, int *);

void sigchld_handler(int);
void sigint_handler(int);
void sigquit_handler(int);
void sigalrm_handler(int);

int main(int argc, char **argv) {
  // one space for null, another for checking overflow
  char cmd[MAX_LINE_SIZE + 2];
  int length;

  sigquit_active = false;
  filename = argv[0];
  init_jobs(&jobs);

  signal(SIGCHLD, sigchld_handler);
  signal(SIGINT, sigint_handler);
  signal(SIGQUIT, sigquit_handler);
  signal(SIGALRM, sigalrm_handler);

  while (true) {
    printf("%s", prompt);

    if (fgets(cmd, MAX_LINE_SIZE, stdin) == NULL) {
      // EOF, if code has reached here
      printf("\n"); // print newline before exiting
      return 0;
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

  free_jobs(jobs);

  return 0;
}

void evaluate(char *cmd) {
  DynArray_T tokens;
  enum ParseResult result;
  bool is_builtin, is_bg;
  int pipes, fd_in = STDIN_FILENO, fd_out = STDOUT_FILENO, token_cursor = 0;
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

  is_builtin = handle_if_builtin(tokens);
  if (is_builtin)
    goto done;

  pipes = 0;
  DynArray_map(tokens, count_pipes, &pipes);

  is_bg = is_background(tokens);
  if (is_bg && pipes != 0) {
    fprintf(stderr, "Mixture of pipes and background process!\n");
    goto done;
  }

  /* printf ("# of pipes: %d\n", pipes); */

  for (int i = 0; i <= pipes; i++) {
    struct Job *job;
    pid_t pid;

    argv = construct_args(tokens, &token_cursor);

    if (i != pipes) {
      pipe(fd);
      fd_out = fd[1];
    } else {
      fd_out = STDOUT_FILENO;
    }

    sigprocmask(SIG_BLOCK, &mask_child, &mask_prev);
    pid = run_command (fd_in, fd_out, argv);
    if (pid < 0) {
      // TODO: error handling!
      sigprocmask(SIG_SETMASK, &mask_prev, NULL);
      free(argv);
      if (i != pipes) close(fd[1]);
      goto done;
    }

    // block signals before adding job
    sigprocmask(SIG_BLOCK, &mask_all, NULL);
    job = add_job(jobs, pid, is_bg ? BACKGROUND : FOREGROUND);
    sigprocmask(SIG_SETMASK, &mask_prev, NULL);

    // parent does not use argv
    free(argv);

    if (i != pipes) {
      close(fd[1]);
      fd_in = fd[0];
    }

    // busy wait if foreground job
    if (!is_bg) {
      /* wait_fg(job); */
      while (job->state == FOREGROUND)
        sleep(1);

      // free job after terminated
      free(job);
    }
  }

done:
  free_line(tokens);
}

void wait_fg(struct Job *job) {
  while (job->state == FOREGROUND)
    sleep(1);
}

pid_t run_command(int fd_in, int fd_out, char **argv) {
  pid_t pid = fork();
  if (pid == 0) {
    // this_process.gid <- this_process.pid
    // this is done to set this process's group id to be different from the shell
    setpgid(0, 0);
    if (fd_in != STDIN_FILENO) {
      dup2(fd_in, STDIN_FILENO);
      close(fd_in);
    }

    if (fd_out != STDOUT_FILENO) {
      dup2(fd_out, STDOUT_FILENO);
      close(fd_out);
    }

    if (execvp(argv[0], argv) < 0) {
      printf("%s: no such file or directory\n", argv[0]);
      exit(-1);
    }
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

    if (chdir(dir) < 0)
      printf("%s: No such file or directory\n", filename);

    return true;
  } else if (!strcmp(first->value, "exit")) {
    exit(0);
  } else if (!strcmp(first->value, "fg")) {
    struct Job *job;
    sigset_t sset, prev;

    sigfillset(&sset);
    sigprocmask(SIG_SETMASK, &sset, &prev);
    job = get_latest_job(jobs);
    assert(job);
    job->state = FOREGROUND;
    sigprocmask(SIG_SETMASK, &prev, NULL);

    wait_fg(job);

    return true;
  }

  return is_built_in;
}

static void count_pipes(void *element, void *extra) {
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
    delete_job(jobs, pid);
    sigprocmask(SIG_SETMASK, &mask_original, NULL);
  }

  if (errno == ECHILD)
    return;
}

void sigint_handler(int sig) {
  struct Job *job;
  sigset_t mask_all, mask_original;

  sigfillset(&mask_all);
  sigprocmask(SIG_BLOCK, &mask_all, &mask_original);
  job = get_foreground_job(jobs);
  sigprocmask(SIG_SETMASK, &mask_original, NULL);

  // send SIGINT signal to fg process
  if (job != NULL)
    kill(-job->pid, SIGINT);
}

void sigquit_handler(int sig) {
  if (sigquit_active)
    exit(0);

  sigquit_active = true;
  printf("\nType Ctrl-\\ again within 5 seconds to exit.\n");
  alarm(5);
}

void sigalrm_handler(int sig) {
  sigquit_active = false;
}

static bool is_background(DynArray_T tokens) {
  int length = DynArray_getLength(tokens);
  struct Token *token = DynArray_get(tokens, length - 1);

  return token && token->type == TOKEN_BACKGROUND;
}
