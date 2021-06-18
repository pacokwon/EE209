#include "job.h"
#include "parse.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

// max line size including newline
#define MAX_LINE_SIZE 1023

// NOTE: echo "foo" | cat < Makefile -> prints contents of Makefile
struct ExecUnit {
  int argc;
  char **argv;
  char *outfile;
  char *infile;
};

DynArray_T jobs;
char * const prompt = "% ";
char * const rcfile = ".ishrc";
char *filename;
bool sigquit_active;

void wait_fg(struct Job *);
void run_rc_file();
void evaluate(char *);
bool handle_if_builtin(DynArray_T);
pid_t run_command(int, int, struct ExecUnit *);
int construct_exec_unit(DynArray_T, int, struct ExecUnit *);

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

  run_rc_file();

  while (true) {
    printf("%s", prompt);

    if (fgets(cmd, MAX_LINE_SIZE, stdin) == NULL) {
      // EOF, if code has reached here
      printf("\n"); // print newline before exiting
      return 0;
    }

    length = strlen(cmd);
    if (length > MAX_LINE_SIZE) {
      fprintf(stderr, "%s: command too long\n", filename);
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

void run_rc_file() {
  FILE *rc;
  char cmd[MAX_LINE_SIZE + 2];
  int length;
  char *homedir = getenv("HOME");
  char *path_to_rc = malloc(strlen(homedir) + strlen(rcfile) + 2);

  sprintf(path_to_rc, "%s/%s", homedir, rcfile);

  rc = fopen(path_to_rc, "r");
  free(path_to_rc);

  if (rc == NULL)
    return;

  while (fgets(cmd, MAX_LINE_SIZE, rc)) {
    length = strlen(cmd);

    if (length > MAX_LINE_SIZE) {
      fprintf(stderr, "%s: command too long\n", filename);
      continue;
    }

    printf("%s%s", prompt, cmd);

    if (cmd[length - 1] == '\n')
      cmd[length - 1] = '\0';

    fflush(rc);
    evaluate(cmd);

    fflush(stdout);
    fflush(stdout);
  }

  fclose(rc);
}

void evaluate(char *cmd) {
  DynArray_T tokens;
  enum ParseResult result;
  bool is_builtin, is_bg;
  int pipes, fd_in = STDIN_FILENO, fd_out = STDOUT_FILENO, token_cursor = 0;
  int fd[2];
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

    if (result == PARSE_NO_QUOTE_PAIR)
      fprintf(stderr, "%s: Could not find quote pair\n", filename);

    free_line(tokens);
    return;
  }

  is_builtin = handle_if_builtin(tokens);
  if (is_builtin)
    goto done;

  pipes = count_pipes(tokens);

  is_bg = is_background(tokens);
  if (is_bg && pipes != 0) {
    fprintf(stderr, "Mixture of pipes and background process!\n");
    goto done;
  }

  for (int i = 0; i <= pipes; i++) {
    struct Job *job;
    struct ExecUnit exec_unit;
    pid_t pid;

    token_cursor = construct_exec_unit(tokens, token_cursor, &exec_unit);

    if (token_cursor == -1) {
      free(exec_unit.argv);
      goto done;
    }

    if (i > 0) {
      if (exec_unit.argc == 0) {
        fprintf(stderr, "%s: Pipe or redirection destination is not specified\n", filename);
        free(exec_unit.argv);
        goto done;
      }

      if (exec_unit.infile != NULL) {
        fprintf(stderr, "%s: Multiple redirection of standard input\n", filename);
        free(exec_unit.argv);
        goto done;
      }
    }

    if (i < pipes && exec_unit.outfile != NULL) {
      fprintf(stderr, "%s: Multiple redirection of standard out\n", filename);
      free(exec_unit.argv);
      goto done;
    }

    if (i != pipes) {
      pipe(fd);
      fd_out = fd[1];
    } else {
      fd_out = STDOUT_FILENO;
    }

    sigprocmask(SIG_BLOCK, &mask_child, &mask_prev);
    pid = run_command (fd_in, fd_out, &exec_unit);
    if (pid < 0) {
      // TODO: error handling!
      sigprocmask(SIG_SETMASK, &mask_prev, NULL);
      free(exec_unit.argv);
      if (i != pipes) close(fd[1]);
      goto done;
    }

    // block signals before adding job
    sigprocmask(SIG_BLOCK, &mask_all, NULL);
    job = add_job(jobs, pid, is_bg ? BACKGROUND : FOREGROUND);
    sigprocmask(SIG_SETMASK, &mask_prev, NULL);

    // parent does not use argv
    free(exec_unit.argv);

    if (i != pipes) {
      close(fd[1]);
      fd_in = fd[0];
    }

    // busy wait if foreground job
    if (!is_bg) {
      wait_fg(job);

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

pid_t run_command(int fd_in, int fd_out, struct ExecUnit *e) {
  char **argv = e->argv;
  int open_fd;

  pid_t pid = fork();
  if (pid == 0) {
    // this_process.gid <- this_process.pid
    // this is done to set this process's group id to be different from the shell
    /* setpgid(0, 0); */

    if (e->infile != NULL) {
         /* (open_fd = open(e->infile, O_RDONLY)) >= 0) { */
      if ((open_fd = open(e->infile, O_RDONLY)) < 0) {
        fprintf(stderr, "%s: No such file or directory\n", filename);
        exit(-1);
      }

      // close the pipe and use the file instead
      close(fd_in);
      fd_in = open_fd;
    }

    if (e->outfile != NULL
        && (open_fd = open(e->outfile, O_CREAT | O_WRONLY, S_IRUSR | S_IWUSR)) >= 0) {
      // close the pipe and use the file instead
      close(fd_out);
      fd_out = open_fd;
    }

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
  if (!DynArray_getLength(tokens))
    return false;

  bool is_built_in = false;
  int length = DynArray_getLength(tokens);
  struct Token *first = DynArray_get(tokens, 0);
  if (first->type != TOKEN_WORD)
    return false;

  if (!strcmp(first->value, "setenv")) {
    char *var, *val;

    if (length < 2) {
      fprintf(stderr, "%s: setenv takes one or two parameters\n", filename);
      return true;
    }

    var = ((struct Token *)DynArray_get(tokens, 1))->value;
    val = length == 2 ? "" : ((struct Token *)DynArray_get(tokens, 2))->value;
    setenv(var, val, 1);

    return true;
  } else if (!strcmp(first->value, "unsetenv")) {
    char *var;

    if (length < 2) {
      fprintf(stderr, "%s: unsetenv takes one parameter\n", filename);
      return true;
    }

    var = ((struct Token *)DynArray_get(tokens, 1))->value;
    unsetenv(var);

    return true;
  } else if (!strcmp(first->value, "cd")) {
    char *dir = length == 2 ? ((struct Token *)DynArray_get(tokens, 1))->value
                            : getenv("HOME");

    if (chdir(dir) < 0)
      fprintf(stderr, "%s: No such file or directory\n", filename);

    return true;
  } else if (!strcmp(first->value, "exit")) {
    exit(0);
  } else if (!strcmp(first->value, "fg")) {
    struct Job *job;
    sigset_t sset, prev;

    sigfillset(&sset);
    sigprocmask(SIG_SETMASK, &sset, &prev);
    job = get_latest_job(jobs);

    // early return if no job exists
    if (job == NULL) {
      fprintf(stderr, "fg: no child processes\n");
      sigprocmask(SIG_SETMASK, &prev, NULL);
      return true;
    }

    job->state = FOREGROUND;
    sigprocmask(SIG_SETMASK, &prev, NULL);

    printf("[%d] Latest background process is executing\n", job->pid);
    wait_fg(job);
    printf("[%d]                           Done\n", job->pid);

    return true;
  }

  return is_built_in;
}

// construct an array of strings from a tokens dynarray
// tokenCursor will be updated to point to the next command token
int construct_exec_unit(DynArray_T tokens, int token_cursor, struct ExecUnit *e) {
  int i = token_cursor, j;
  int length = DynArray_getLength(tokens);
  int argc = 0;
  struct Token *token;

  e->infile = NULL;
  e->outfile = NULL;

  // iteration #1, to compute `argc`
  while (i < length && ((struct Token *) DynArray_get(tokens, i))->type == TOKEN_WORD) {
    argc++;
    i++;
  }

  char **argv = calloc(argc + 1, sizeof(char *));
  // iteration #2, to construct `argv`
  i = token_cursor;
  j = 0;
  while (i < length && (token = DynArray_get(tokens, i))->type == TOKEN_WORD) {
    argv[j] = token->value;
    i++;
    j++;
  }
  argv[argc] = NULL;
  e->argc = argc;
  e->argv = argv;

  // the ith token is a token that is not a word. so one of:
  // 1) redir in | 2) redir out | 3) pipe | 4) background

  // if this loop ends,
  // its either because the token is a pipe,
  // or its the end of the list
  while (i < length && (token = DynArray_get(tokens, i))->type != TOKEN_PIPE) {
    enum TokenType type = token->type;

    if (type == TOKEN_REDIRECT_IN || type == TOKEN_REDIRECT_OUT) {
      token = DynArray_get(tokens, ++i);

      if (token->type != TOKEN_WORD) {
        if (type == TOKEN_REDIRECT_IN)
          fprintf(stderr, "%s: Standard input redirection without file name\n", filename);

        if (type == TOKEN_REDIRECT_OUT)
          fprintf(stderr, "%s: Standard output redirection without file name\n", filename);

        return -1;
      }

      // NOTE: from here, `token` is a word
      if (type == TOKEN_REDIRECT_IN) {
        if (e->infile != NULL) {
          fprintf(stderr, "%s: Multiple redirection of standard input\n", filename);
          return -1;
        }

        e->infile = token->value;
      } else {
        if (e->outfile != NULL) {
          fprintf(stderr, "%s: Multiple redirection of standard out\n", filename);
          return -1;
        }

        e->outfile = token->value;
      }
    }

    i++;
  }

  // ignore anything that comes in between a pipe and the next word
  while (i < length && ((token = DynArray_get(tokens, i))->type != TOKEN_WORD))
    i++;

  // i will point to a word or an end of the token list
  return i;
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
    kill(job->pid, SIGINT);
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
