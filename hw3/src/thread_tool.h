#ifndef THREAD_TOOL_H
#define THREAD_TOOL_H

#include <setjmp.h>
#include <signal.h>
#include <stdlib.h>

// The maximum number of threads.
#define THREAD_MAX 100


void sighandler(int signum);
void scheduler();

// The thread control block structure.
struct tcb {
    int id;
    int *args;
    // Reveals what resource the thread is waiting for. The values are:
    //  - 0: no resource.
    //  - 1: read lock.
    //  - 2: write lock.
    int waiting_for;
    int sleeping_time;
    jmp_buf env;  // Where the scheduler should jump to.
    int n, i, f_cur, f_prev, q_p, q_s, p_p, p_s; // TODO: Add some variables you wish to keep between switches.
};

// The only one thread in the RUNNING state.
extern struct tcb *current_thread;
extern struct tcb *idle_thread;

struct tcb_queue {
    struct tcb *arr[THREAD_MAX];  // The circular array.
    int head;                     // The index of the head of the queue
    int size;
};

extern struct tcb_queue ready_queue, waiting_queue;


// The rwlock structure.
//
// When a thread acquires a type of lock, it should increment the corresponding count.
struct rwlock {
    int read_count;
    int write_count;
};

extern struct rwlock rwlock;

// The remaining spots in classes.
extern int q_p, q_s;

// The maximum running time for each thread.
extern int time_slice;

// The long jump buffer for the scheduler.
extern jmp_buf sched_buf;

// TODO::
// You should setup your own sleeping set as well as finish the marcos below
// setup my own sleeping set
struct sleeping_set{
    struct tcb *elements[THREAD_MAX];
    int size;          
};

extern struct sleeping_set sleeping_set;

#define thread_create(func, t_id, t_args)                                              \
    ({                    \
        func(t_id, t_args);                                                             \
    })

#define thread_setup(t_id, t_args)                                                     \
    ({                 \
        struct tcb* new_tcb = (struct tcb*) malloc (sizeof (struct tcb)); \
        new_tcb -> id = t_id; \
        new_tcb -> args = t_args; \
        printf("thread %d: set up routine %s\n", t_id, __func__); \
        if (t_id == 0) \
            idle_thread = new_tcb; \
        else{ \
            ready_queue.arr[(ready_queue.head + ready_queue.size) % THREAD_MAX] = new_tcb; \
            ready_queue.size += 1; \
        } \
        int val = sigsetjmp(new_tcb -> env, 1); \
        if (val == 0) \
            return;                                           \
    })

#define thread_yield()                                  \
    ({               \
        sigset_t sigset; \
        sigsetjmp(current_thread -> env, 1); \
        sigemptyset(&sigset); \
        sigaddset(&sigset, SIGTSTP);    \
        sigprocmask(SIG_UNBLOCK, &sigset, NULL); \
        sigprocmask(SIG_BLOCK, &sigset, NULL); \
        sigsetjmp(current_thread -> env, 1); \
        sigemptyset(&sigset); \
        sigaddset(&sigset, SIGALRM);   \
        sigprocmask(SIG_UNBLOCK, &sigset, NULL); \
        sigprocmask(SIG_BLOCK, &sigset, NULL);  \
        sigsetjmp(current_thread -> env, 1); \
    })

#define read_lock()                                                      \
    ({             \
        sigsetjmp(current_thread -> env, 1);\
        if (rwlock.write_count != 0) {\
            current_thread -> waiting_for = 1; \
            siglongjmp(sched_buf, 2); \
        } \
        else \
            rwlock.read_count += 1;                                                      \
    })

#define write_lock()                                                     \
    ({              \
        sigsetjmp(current_thread -> env, 1); \
        if (rwlock.write_count != 0 || rwlock.read_count != 0){ \
            current_thread -> waiting_for = 2; \
            siglongjmp(sched_buf, 2); \
        } \
        else \
            rwlock.write_count += 1;                                                     \
    })

#define read_unlock()                                                                 \
    ({                 \
        rwlock.read_count -= 1;                                                               \
    })

#define write_unlock()                                                                \
    ({                  \
        rwlock.write_count -= 1;                                                              \
    })

#define thread_sleep(sec)                                            \
    ({              \
        current_thread -> sleeping_time = sec; \
        sleeping_set.elements[sleeping_set.size] = current_thread; \
        sleeping_set.size += 1;    \
        int val = sigsetjmp(current_thread -> env, 1); \
        if (val == 0) \
            siglongjmp(sched_buf, 5);                                              \
    })

#define thread_awake(t_id)                                                        \
    ({            \
        for (int j = 0; j < sleeping_set.size; j++){ \
            if (t_id == (sleeping_set.elements[j] -> id)){ \
                ready_queue.arr[(ready_queue.head + ready_queue.size) % THREAD_MAX] = sleeping_set.elements[j]; \
                ready_queue.size += 1; \
                sleeping_set.elements[j] = sleeping_set.elements[sleeping_set.size - 1]; \
                sleeping_set.size -= 1; \
                break;   \
            } \
        }                                                                \
    })

#define thread_exit()                                    \
    ({          \
        printf("thread %d: exit\n", current_thread -> id);   \
        siglongjmp(sched_buf, 3);                                      \
    })

#endif  // THREAD_TOOL_H
