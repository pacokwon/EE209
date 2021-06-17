#ifndef __ISH_JOB_H
#define __ISH_JOB_H

#include <stdbool.h>
#include <unistd.h>
#include "dynarray.h"

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

void init_jobs(DynArray_T *);
struct Job *add_job(DynArray_T, pid_t, enum ProcState);
struct Job *get_latest_job(DynArray_T);
bool delete_job(DynArray_T, pid_t);
void free_jobs(DynArray_T);

#endif
