#include "ish.h"
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

enum BuiltinType {
  IS_BUILTIN,
  IS_EXTERNAL,
  IS_EXIT
};

DynArray_T jobs;
char * const prompt = "% ";
char * const rcfilename = ".ishrc";
char *shell_name;
bool sigquit_active;

bool evaluate(char *);
enum BuiltinType handle_if_builtin(DynArray_T);
int construct_exec_unit(DynArray_T, int, struct ExecUnit *);
pid_t run_command(int, int, struct ExecUnit *);
void cleanup();
void run_rc_file();
void wait_fg(struct Job *);

void sigchld_handler(int);
void sigint_handler(int);
void sigquit_handler(int);
void sigalrm_handler(int);

// main procedure
int main(int argc, char **argv) {
  // one space for null, another for checking overflow
  char cmd[MAX_LINE_SIZE + 2];
  int length;
  bool should_continue = true;

  sigquit_active = false;
  shell_name = argv[0];
  init_jobs(&jobs);

  signal(SIGCHLD, sigchld_handler);
  signal(SIGINT, sigint_handler);
  signal(SIGQUIT, sigquit_handler);
  signal(SIGALRM, sigalrm_handler);

  run_rc_file();

  while (should_continue) {
    printf("%s", prompt);

    if (fgets(cmd, MAX_LINE_SIZE, stdin) == NULL) {
      // EOF, if code has reached here
      printf("\n"); // print newline before exiting
      break;
    }

    length = strlen(cmd);

    // command is too long
    if (length > MAX_LINE_SIZE) {
      fprintf(stderr, "%s: command too long\n", shell_name);
      continue;
    }

    // remove newline for ease of parsing
    if (cmd[length - 1] == '\n')
      cmd[length - 1] = '\0';

    // evaluate the command
    should_continue = evaluate(cmd);

    fflush(stdout);
    fflush(stdout);
  }

  // clean global resources
  cleanup();

  return 0;
}

/**
 * run_rc_file - read and run the commands in the rc file
 */
void run_rc_file() {
  char cmd[MAX_LINE_SIZE + 2];
  int length;
  FILE *rcfile = NULL;
  bool should_continue = true;
  char *homedir = getenv("HOME");
  char *path_to_rc = malloc(strlen(homedir) + strlen(rcfilename) + 2);

  sprintf(path_to_rc, "%s/%s", homedir, rcfilename);

  rcfile = fopen(path_to_rc, "r");
  free(path_to_rc);

  if (rcfile == NULL)
    return;

  while (should_continue && fgets(cmd, MAX_LINE_SIZE, rcfile)) {
    length = strlen(cmd);

    if (length > MAX_LINE_SIZE) {
      fprintf(stderr, "%s: command too long\n", shell_name);
      continue;
    }

    printf("%s%s", prompt, cmd);

    if (cmd[length - 1] == '\n')
      cmd[length - 1] = '\0';

    fflush(rcfile);
    should_continue = evaluate(cmd);

    fflush(stdout);
    fflush(stdout);
  }

  fclose(rcfile);
  rcfile = NULL;

  if (!should_continue) {
    cleanup();
    exit(0);
  }
}

/**
 * evaluate - accept a command string and run it
 *
 * accepts:
 *  cmd: c string that contains the whole command
 *
 * returns:
 *  whether or not the shell should continue running
 */
bool evaluate(char *cmd) {
  DynArray_T tokens;
  enum ParseResult result;
  enum BuiltinType builtin_type = IS_BUILTIN;
  bool is_bg;
  int pipes, fd_in = STDIN_FILENO, fd_out = STDOUT_FILENO, token_cursor = 0;
  int fd[2];
  sigset_t mask_all, mask_child, mask_prev;

  sigfillset(&mask_all);
  sigemptyset(&mask_child);
  sigaddset(&mask_child, SIGCHLD);

  tokens = DynArray_new(0);
  if (tokens == NULL) {
    fprintf(stderr, "Cannot allocate memory\n");
    return false;
  }

  result = parse_line(cmd, tokens);

  // 1. tokens must not be empty
  // 2. first token MUST be a command
  if (!DynArray_getLength(tokens) ||
      ((struct Token *) DynArray_get(tokens, 0))->type != TOKEN_WORD) {
    free_line(tokens);
    return true;
  }

  if (result != PARSE_SUCCESS) {
    if (result == PARSE_NO_QUOTE_PAIR)
      fprintf(stderr, "%s: Could not find quote pair\n", shell_name);

    free_line(tokens);
    return true;
  }

  builtin_type = handle_if_builtin(tokens);
  if (builtin_type == IS_BUILTIN || builtin_type == IS_EXIT)
    goto done;

  pipes = count_pipes(tokens);

  // check if command is background
  is_bg = is_background(tokens);
  if (is_bg && pipes != 0) {
    fprintf(stderr, "Mixture of pipes and background process!\n");
    goto done;
  }

  for (int i = 0; i <= pipes; i++) {
    // execute command in between pipe
    struct Job *job;
    struct ExecUnit exec_unit;
    pid_t pid;

    token_cursor = construct_exec_unit(tokens, token_cursor, &exec_unit);

    /* ------ error handling start ------ */
    if (token_cursor == -1) {
      free(exec_unit.argv);
      goto done;
    }

    if (i > 0) {
      if (exec_unit.argc == 0) {
        fprintf(stderr, "%s: Pipe or redirection destination is not specified\n", shell_name);
        free(exec_unit.argv);
        goto done;
      }

      if (exec_unit.infile != NULL) {
        fprintf(stderr, "%s: Multiple redirection of standard input\n", shell_name);
        free(exec_unit.argv);
        goto done;
      }
    }

    if (i < pipes && exec_unit.outfile != NULL) {
      fprintf(stderr, "%s: Multiple redirection of standard out\n", shell_name);
      free(exec_unit.argv);
      goto done;
    }
    /* ------ error handling end ------ */

    // no need to pipe if last command
    if (i != pipes) {
      pipe(fd);
      fd_out = fd[1];
    } else {
      fd_out = STDOUT_FILENO;
    }

    // block SIGCHLD, since we want to add a job first
    sigprocmask(SIG_BLOCK, &mask_child, &mask_prev);

    // fork a new process and call the command in it
    pid = run_command (fd_in, fd_out, &exec_unit);

    if (pid < 0) {
      // run_command error! free resources and return
      sigprocmask(SIG_SETMASK, &mask_prev, NULL);
      free(exec_unit.argv);
      if (i != pipes) close(fd[1]);
      goto done;
    }

    // block all signals before adding job
    sigprocmask(SIG_BLOCK, &mask_all, NULL);
    job = add_job(jobs, pid, is_bg ? BACKGROUND : FOREGROUND);
    sigprocmask(SIG_SETMASK, &mask_prev, NULL);

    // parent does not use argv
    free(exec_unit.argv);

    // close write fd in parent, and set up next read fd
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

  return builtin_type != IS_EXIT;
}

/**
 * wait_fg - wait for a job to finish running
 *
 * accepts:
 *  job: the job to wait for
 */
void wait_fg(struct Job *job) {
  while (job->state == FOREGROUND)
    sleep(1);
}

/**
 * run_command - run a command with the specified file descriptors
 *
 * accepts:
 *  fd_in: file descriptor to read input from
 *  fd_out: file descriptor to write output to
 *  e: struct that contains parsed command information
 *
 * returns:
 *  pid of the created child process
 */
pid_t run_command(int fd_in, int fd_out, struct ExecUnit *e) {
  char **argv = e->argv;
  int open_fd;

  pid_t pid = fork();
  if (pid == 0) {
    // this_process.gid <- this_process.pid
    // this is done to set this process's group id to be different from the shell
    /* setpgid(0, 0); */

    if (e->infile != NULL) {
      if ((open_fd = open(e->infile, O_RDONLY)) < 0) {
        fprintf(stderr, "%s: No such file or directory\n", shell_name);
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

/**
 * handle_if_builtin - accept a dynarray of tokens and execute it if it is
 * builtin
 *
 * this function "executes" the command if the command is built in, and then
 * returns if it is a builtin command or not
 *
 * accepts:
 *  tokens: a DynArray_T of tokens that comprise the command
 *
 * returns:
 *  IS_BUILTIN if the command is built in
 *  IS_EXIT if the builtin command is "exit"
 *  IS_EXTERNAL if the command is not built in
 */
enum BuiltinType handle_if_builtin(DynArray_T tokens) {
  // length of more than 0 should be guaranteed
  if (!DynArray_getLength(tokens))
    return IS_EXTERNAL;

  int length = DynArray_getLength(tokens);
  struct Token *first = DynArray_get(tokens, 0);
  if (first->type != TOKEN_WORD)
    return IS_EXTERNAL;

  if (!strcmp(first->value, "setenv")) {
    char *var, *val;

    if (length < 2) {
      fprintf(stderr, "%s: setenv takes one or two parameters\n", shell_name);
      return IS_BUILTIN;
    }

    var = ((struct Token *)DynArray_get(tokens, 1))->value;
    val = length == 2 ? "" : ((struct Token *)DynArray_get(tokens, 2))->value;
    setenv(var, val, 1);

    return IS_BUILTIN;
  } else if (!strcmp(first->value, "unsetenv")) {
    char *var;

    if (length < 2) {
      fprintf(stderr, "%s: unsetenv takes one parameter\n", shell_name);
      return IS_BUILTIN;
    }

    var = ((struct Token *)DynArray_get(tokens, 1))->value;
    unsetenv(var);

    return IS_BUILTIN;
  } else if (!strcmp(first->value, "cd")) {
    char *dir = length == 2 ? ((struct Token *)DynArray_get(tokens, 1))->value
                            : getenv("HOME");

    if (chdir(dir) < 0)
      fprintf(stderr, "%s: No such file or directory\n", shell_name);

    return IS_BUILTIN;
  } else if (!strcmp(first->value, "exit")) {
    return IS_EXIT;
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
      return IS_BUILTIN;
    }

    job->state = FOREGROUND;
    sigprocmask(SIG_SETMASK, &prev, NULL);

    printf("[%d] Latest background process is executing\n", job->pid);
    wait_fg(job);
    printf("[%d]                           Done\n", job->pid);

    return IS_BUILTIN;
  }

  return IS_EXTERNAL;
}

/**
 * sigchld_handler - handler for SIGCHLD
 *
 * reap terminated children
 */
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

/**
 * sigint_handler - handler for SIGINT
 *
 * send a SIGINT signal to the current foreground job
 */
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

/**
 * sigquit_handler - handler for SIGQUIT
 *
 * set the `sigquit_active` flag to true, and start an alarm
 * if handler is invoked again in that time frame, quit.
 */
void sigquit_handler(int sig) {
  if (sigquit_active)
    exit(0);

  sigquit_active = true;
  printf("\nType Ctrl-\\ again within 5 seconds to exit.\n");
  alarm(5);
}

/**
 * sigalrm_handler - handler for SIGALRM
 *
 * sets the global `sigquit_active` variable to false
 */
void sigalrm_handler(int sig) {
  sigquit_active = false;
}

/**
 * cleanup - clean up global resources
 */
void cleanup() {
  if (jobs) {
    free_jobs(jobs);
    jobs = NULL;
  }
}
