#include "tpool.h"

#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

void *frontend(void *arg){
    struct tpool *pool = (struct tpool *)arg;
    while (1) {
        pthread_mutex_lock(&pool -> frontend_mutex);
        while (pool -> frontend_head == NULL && pool -> frontend_terminate == 0){
            pthread_cond_wait(&pool -> frontend_cond, &pool -> frontend_mutex);
        }

        if (pool -> frontend_terminate == 1){
            pthread_mutex_unlock(&pool -> frontend_mutex);
            break;
        }


        struct frontend_queue *temp = pool -> frontend_head;
        pool -> frontend_head = temp -> next;
        if (pool -> frontend_head == NULL)
            pool -> frontend_rear = NULL;

        pthread_mutex_unlock(&pool -> frontend_mutex);

        pthread_mutex_lock(&pool -> sync_mutex);
        pool -> requirement -= 1;
        pool -> subtasks += temp -> num_works;
        if (pool -> requirement == 0 && pool -> subtasks == 0)
            pthread_cond_signal(&pool -> sync_cond);
        pthread_mutex_unlock(&pool -> sync_mutex);

        
        for (int i = 0; i < pool -> n; i++) {
            for (int j = i + 1; j < pool -> n; j++) {
                int temp_num = (temp -> b)[i][j];
                (temp -> b)[i][j] = (temp -> b)[j][i];
                (temp -> b)[j][i] = temp_num;
            }
        }

        int total = pool -> n * pool -> n;
        int size = total / temp -> num_works;
        int remainders = total % temp -> num_works;
        int start = 0;
        int end;
        for (int i = 0; i < temp -> num_works; i++){
            end = start + size + ((i < remainders)? 1: 0);
            struct work_queue * new_node = (struct work_queue *) malloc (sizeof(struct work_queue));
            new_node -> next = NULL;
            new_node -> a = temp -> a;
            new_node -> b = temp -> b;
            new_node -> c = temp -> c;
            new_node -> start_idx = start;
            new_node -> end_idx = end;
            pthread_mutex_lock(&pool -> work_mutex);
            if (pool -> work_rear != NULL){
                pool -> work_rear -> next = new_node;
                pool -> work_rear = new_node;
            }
            else {
                pool -> work_head = new_node;
                pool -> work_rear = new_node;
            }
            pthread_cond_signal(&pool -> work_cond);
            pthread_mutex_unlock(&pool -> work_mutex);
            start = end;
        }
        
        free(temp);
    }
    return NULL;
}

void *backend(void *arg){
    struct tpool *pool = (struct tpool *)arg;
    while (1) {
        pthread_mutex_lock(&pool -> work_mutex);
        while (pool -> work_head == NULL && pool -> work_terminate == 0){
            pthread_cond_wait(&pool -> work_cond, &pool -> work_mutex);
        }

        if (pool -> work_terminate == 1){
            pthread_mutex_unlock(&pool -> work_mutex);
            break;
        }

        struct work_queue *temp = pool -> work_head;
        pool -> work_head = temp -> next;
        if (pool -> work_head == NULL)
            pool -> work_rear = NULL;

        pthread_mutex_unlock(&pool -> work_mutex);

        for (int i = temp -> start_idx; i < temp -> end_idx; i++){
            int row = i / pool -> n;
            int col = i % pool -> n;
            temp -> c[row][col] = calculation(pool -> n, temp -> a[row], temp -> b[col]);        
        }

        pthread_mutex_lock(&pool -> sync_mutex);
        pool -> subtasks -= 1;
        if (pool -> requirement == 0 && pool -> subtasks == 0)
            pthread_cond_signal(&pool -> sync_cond);
        pthread_mutex_unlock(&pool -> sync_mutex);
        free(temp);
    }
    return NULL;
}

struct tpool *tpool_init(int num_threads, int n) {
    
    struct tpool *new_pool = (struct tpool *) malloc (sizeof(struct tpool));
    new_pool -> frontend_head = NULL;
    new_pool -> frontend_rear = NULL; 
    new_pool -> work_head = NULL;
    new_pool -> work_rear = NULL;
    new_pool -> thread_head = NULL;
    new_pool -> thread_rear = NULL;
    new_pool -> num_threads = num_threads;
    new_pool -> n = n;
    new_pool -> requirement = 0;
    new_pool -> subtasks = 0;
    new_pool -> frontend_terminate = 0;
    new_pool -> work_terminate = 0;
    pthread_mutex_init(&new_pool->frontend_mutex, NULL);
    pthread_cond_init(&new_pool->frontend_cond, NULL);
    pthread_mutex_init(&new_pool->work_mutex, NULL);
    pthread_cond_init(&new_pool->work_cond, NULL);
    pthread_mutex_init(&new_pool->sync_mutex, NULL);
    pthread_cond_init(&new_pool->sync_cond, NULL);
    pthread_create(&new_pool -> frontend_thread, NULL, frontend, (void *)new_pool);
    pthread_t tid;
    for (int i = 0; i < num_threads; i++){
        pthread_create(&tid, NULL, backend, (void *)new_pool);
        struct thread_queue *new_node = (struct thread_queue *) malloc (sizeof(struct thread_queue));
        new_node -> tid = tid;
        new_node -> next = NULL;

        if (new_pool -> thread_head == NULL){
            new_pool -> thread_head = new_node;
            new_pool -> thread_rear = new_node;
        }
        else {
            new_pool -> thread_rear -> next = new_node;
            new_pool -> thread_rear = new_node;
        }
    }
    return new_pool;
}

void tpool_request(struct tpool *pool, Matrix a, Matrix b, Matrix c,
                   int num_works) {
    struct frontend_queue *new_node = (struct frontend_queue *) malloc (sizeof(struct frontend_queue));
    new_node -> a = a;
    new_node -> b = b;
    new_node -> c = c;
    new_node -> num_works = num_works;
    new_node -> next = NULL;

    pthread_mutex_lock(&pool -> sync_mutex);
    pool -> requirement += 1;
    pthread_mutex_unlock(&pool -> sync_mutex);
    
    pthread_mutex_lock(&pool -> frontend_mutex);
    if (pool -> frontend_rear != NULL){
        pool -> frontend_rear -> next = new_node;
        pool -> frontend_rear = new_node;
    }
    else {
        pool -> frontend_head = new_node;
        pool -> frontend_rear = new_node;
    }
    pthread_cond_signal(&pool -> frontend_cond);
    pthread_mutex_unlock(&pool -> frontend_mutex);
    
    return;
}

void tpool_synchronize(struct tpool *pool) {
    
    pthread_mutex_lock(&pool -> sync_mutex);
    while (pool -> requirement != 0 || pool -> subtasks != 0){
        
        pthread_cond_wait(&pool -> sync_cond, &pool -> sync_mutex);
    }
    pthread_mutex_unlock(&pool -> sync_mutex);
}

void tpool_destroy(struct tpool *pool) {

    pthread_mutex_lock(&pool->frontend_mutex);
    pool -> frontend_terminate = 1;
    pthread_cond_broadcast(&pool -> frontend_cond);
    pthread_mutex_unlock(&pool -> frontend_mutex);
    pthread_join(pool -> frontend_thread, NULL);


    pthread_mutex_lock(&pool -> work_mutex);
    pool -> work_terminate = 1;
    pthread_cond_broadcast(&pool -> work_cond);
    pthread_mutex_unlock(&pool -> work_mutex);
    while (pool -> thread_head != NULL){
        pthread_join(pool -> thread_head -> tid, NULL);
        struct thread_queue *temp = pool -> thread_head;
        pool -> thread_head = temp -> next;
        free(temp);
    }
    
    pthread_mutex_destroy(&pool -> frontend_mutex);
    pthread_mutex_destroy(&pool -> work_mutex);
    pthread_mutex_destroy(&pool -> sync_mutex);

    pthread_cond_destroy(&pool -> frontend_cond);
    pthread_cond_destroy(&pool -> work_cond);
    pthread_cond_destroy(&pool -> sync_cond);

    free(pool);
    
}
