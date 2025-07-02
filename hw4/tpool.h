#pragma once

#include <pthread.h>

/* You may define additional structures / typedef's in this header file if
 * needed.
 */

typedef int** Matrix;
typedef int* Vector;

struct frontend_queue {
  Matrix a;
  Matrix b;
  Matrix c;
  int num_works;
  struct frontend_queue *next;
};

struct work_queue {
  Matrix a;
  Matrix b;
  Matrix c;
  long long start_idx;
  long long end_idx;
  struct work_queue *next;
};

struct thread_queue {
  pthread_t tid;
  struct thread_queue *next;
};

struct tpool {
  struct frontend_queue *frontend_head;
  struct frontend_queue *frontend_rear;
  struct work_queue *work_head;
  struct work_queue *work_rear;

  pthread_t frontend_thread;
  struct thread_queue *thread_head;
  struct thread_queue *thread_rear;

  pthread_mutex_t frontend_mutex;
  pthread_mutex_t work_mutex;
  pthread_mutex_t sync_mutex;
  pthread_cond_t frontend_cond;
  pthread_cond_t work_cond;
  pthread_cond_t sync_cond;

  int n;
  int num_threads;
  int requirement;
  int subtasks;
  int frontend_terminate;
  int work_terminate;
  // Add things you need here
};

struct tpool* tpool_init(int num_threads, int n);
void tpool_request(struct tpool*, Matrix a, Matrix b, Matrix c, int num_works);
void tpool_synchronize(struct tpool*);
void tpool_destroy(struct tpool*);
int calculation(int n, Vector, Vector);  // Already implemented
