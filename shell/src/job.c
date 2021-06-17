#include "job.h"
#include "dynarray.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

static int allocate_job_id(void);
static void free_single_job(void *, void *);

void init_jobs(DynArray_T *jobs) {
  *jobs = DynArray_new(0);
}

struct Job *add_job(DynArray_T jobs, pid_t pid, enum ProcState state) {
  struct Job *new_job = calloc(1, sizeof(struct Job));
  if (new_job == NULL) {
    // TODO: error handling!
    assert(true);
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

bool delete_job(DynArray_T jobs, pid_t pid) {
  int length = DynArray_getLength(jobs);

  for (int i = 0; i < length; i++) {
    struct Job *job = DynArray_get(jobs, i);
    if (job->pid != pid) continue;

    // job found!

    DynArray_removeAt(jobs, i);

    if (job->state == BACKGROUND) {
      // we don't need the object anymore; free it early
      printf("child %d terminated normally\n", pid);
      free(job);
    }
    else if (job->state == FOREGROUND) {
      // job is needed,
      // let the waiting part of the code free the job.
      job->state = TERMINATED;
    } else {
      // error! state is faulty
      assert(true);
    }

    return true;
  }

  return false;
}

void free_jobs(DynArray_T jobs) {
  DynArray_map(jobs, free_single_job, NULL);
  DynArray_free(jobs);
}

struct Job *get_latest_job(DynArray_T jobs) {
  int length = DynArray_getLength(jobs);
  struct Job *latest = NULL;

  for (int i = 0; i < length; i++) {
    struct Job *job = DynArray_get(jobs, i);
    if (!job) continue;

    if (!latest || latest->jid < job->jid)
      latest = job;
  }

  return latest;
}

struct Job *get_foreground_job(DynArray_T jobs) {
  int length = DynArray_getLength(jobs);

  for (int i = 0; i < length; i++) {
    struct Job *job = DynArray_get(jobs, i);
    if (!job) continue;

    if (job && job->state == FOREGROUND)
      return job;
  }

  return NULL;
}

static void free_single_job(void *pvItem, void *pvExtra) {
  free(pvItem);
}

// no concurrency issues here; only the shell calls this function
static int allocate_job_id() {
  static int job_id = 1;
  return job_id++;
}
