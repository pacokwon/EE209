#include "job.h"
#include "dynarray.h"
#include <stdio.h>
#include <stdlib.h>

static int allocate_job_id(void);
static void free_single_job(void *, void *);

/**
 * init_jobs - initialize list of jobs
 *
 * accepts:
 *  jobs: pointer to DynArray_T of job structs
 */
void init_jobs(DynArray_T *jobs) {
  *jobs = DynArray_new(0);
}

/**
 * add_job - add a job to the given jobs array
 *
 * accepts:
 *  jobs: DynArray_T of job structs
 *  pid: pid of new job
 *  state: the state of job (fg, bg, etc..)
 *
 * returns:
 *  pointer to newly created job
 *  NULL on error
 */
struct Job *add_job(DynArray_T jobs, pid_t pid, enum ProcState state) {
  struct Job *new_job = calloc(1, sizeof(struct Job));
  if (new_job == NULL)
    return NULL;

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

/**
 * delete_job - delete a job that has the given pid
 *
 * accepts:
 *  jobs: DynArray_T of job structs
 *  pid: pid of job to be deleted
 *
 * returns:
 *  true if successfully deleted
 *  false otherwise
 */
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
      return false;
    }

    return true;
  }

  return false;
}

/**
 * free_jobs - free all jobs in the jobs array
 *
 * accepts:
 *  jobs: DynArray_T of job structs
 */
void free_jobs(DynArray_T jobs) {
  DynArray_map(jobs, free_single_job, NULL);
  DynArray_free(jobs);
}

/**
 * get_latest_job - get the last job that was created
 *
 * accepts:
 *  jobs: DynArray_T of job structs
 *
 * returns:
 *  NULL if no job exists
 *  the pointer to the last job that was created
 */
struct Job *get_latest_job(DynArray_T jobs) {
  int length = DynArray_getLength(jobs);
  struct Job *latest = NULL;

  for (int i = 0; i < length; i++) {
    struct Job *job = DynArray_get(jobs, i);
    if (!job) continue;

    // jid is an indicator of time, so we use this as a metric
    if (!latest || latest->jid < job->jid)
      latest = job;
  }

  return latest;
}

/**
 * get_foreground_job - get the currently running foreground job
 *
 * accepts:
 *  jobs: DynArray_T of job structs
 *
 * returns:
 *  NULL if no foreground job is running
 *  the pointer to the foreground job otherwise
 */
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

/**
 * free_single_job - free function for job struct
 *
 * the job struct does not dynamically allocate any of its fields,
 * so simply call `free` on the item
 */
static void free_single_job(void *pvItem, void *pvExtra) {
  free(pvItem);
}

/**
 * allocate_job_id - allocate job id (not much to say here)
 *
 * only the shell will use this, so no concurrency issues
 */
static int allocate_job_id() {
  static int job_id = 1;
  return job_id++;
}
