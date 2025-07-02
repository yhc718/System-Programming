#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "routine.h"
#include "thread_tool.h"

struct sleeping_set sleeping_set;

// TODO::
// Prints out the signal you received.
// This function should not return. Instead, jumps to the scheduler.
void sighandler(int signum) {
    if (signum == SIGTSTP)
        printf("caught SIGTSTP\n");
    else if (signum == SIGALRM)
        printf("caught SIGALRM\n");
    siglongjmp(sched_buf, 1);
    // Your code here
}

// TODO::
// Perfectly setting up your scheduler.
void scheduler() {
    // Your code here
    // 1
    thread_create(idle, 0, NULL);
    int val = sigsetjmp(sched_buf, 1);
    alarm(time_slice);

    // 2
    sigset_t sigset, oldset;
    struct sigaction sa_ignore, sa_restore;

    sa_ignore.sa_handler = SIG_IGN;
    sigemptyset(&sa_ignore.sa_mask);
    sa_ignore.sa_flags = 0;

    sigaction(SIGTSTP, &sa_ignore, &sa_restore);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTSTP);
    sigprocmask(SIG_UNBLOCK, &sigset, &oldset);
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    sigaction(SIGTSTP, &sa_restore, NULL);

    sigaction(SIGALRM, &sa_ignore, &sa_restore);

    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_UNBLOCK, &sigset, &oldset);
    sigprocmask(SIG_SETMASK, &oldset, NULL);

    sigaction(SIGALRM, &sa_restore, NULL);

    // 3
    
    if (val == 1){
        int temp_arr[THREAD_MAX];
        int temp_size = 0;
        for (int i = 0; i < sleeping_set.size; i++){
            sleeping_set.elements[i] -> sleeping_time -= time_slice;
            if (sleeping_set.elements[i] -> sleeping_time <= 0){
                sleeping_set.elements[i] -> sleeping_time = 0;
                temp_arr[temp_size] = sleeping_set.elements[i] -> id;
                temp_size ++;
            }
        }
        
        for (int i = 0; i < temp_size - 1; i++){
            for (int j = 0; j < temp_size - i - 1; j++){
                if (temp_arr[j] > temp_arr[j + 1]) {
                    int temp = temp_arr[j];
                    temp_arr[j] = temp_arr[j + 1];
                    temp_arr[j + 1] = temp;
                }
            }
        }
        for (int i = 0; i < temp_size; i++){
            thread_awake(temp_arr[i]);
        }
    }
    

    // 4
    while (waiting_queue.size != 0){
        if (waiting_queue.arr[waiting_queue.head] -> waiting_for == 1){
            
            if (rwlock.write_count == 0){
                ready_queue.arr[(ready_queue.head + ready_queue.size) % THREAD_MAX] = waiting_queue.arr[waiting_queue.head];
                ready_queue.size += 1;
                waiting_queue.head = (waiting_queue.head + 1) % THREAD_MAX;
                waiting_queue.size -= 1;
            }
            else 
                break;
        }
        else if (waiting_queue.arr[waiting_queue.head] -> waiting_for == 2){
            if (rwlock.write_count == 0 && rwlock.read_count == 0){
                ready_queue.arr[(ready_queue.head + ready_queue.size) % THREAD_MAX] = waiting_queue.arr[waiting_queue.head];
                ready_queue.size += 1;
                waiting_queue.head = (waiting_queue.head + 1) % THREAD_MAX;
                waiting_queue.size -= 1;
            }
            else 
                break;
        }
    }

    // 5
    if (val == 1){
        if (current_thread -> id != 0){ // from signal handler
            ready_queue.arr[(ready_queue.head + ready_queue.size) % THREAD_MAX] = current_thread;
            ready_queue.size += 1;
        }
    }
    else if (val == 2){ // from read_write_lock
        waiting_queue.arr[(waiting_queue.head + waiting_queue.size) % THREAD_MAX] = current_thread;
        waiting_queue.size += 1;
    }
    else if (val == 3){ // from exit
        free(current_thread -> args);
        free(current_thread);
    }

    // 6
    if (ready_queue.size == 0){
        if (sleeping_set.size == 0){
            free(idle_thread);
            return;
        }
        else 
            current_thread = idle_thread;
    }
    else {
        current_thread = ready_queue.arr[ready_queue.head];
        ready_queue.head = (ready_queue.head + 1) % THREAD_MAX;
        ready_queue.size -= 1;
    }
    siglongjmp(current_thread -> env, 1);
}
