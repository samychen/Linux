
#include "thread_pool.h"
#include "async_queue_interner.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

extern const async_queue_op_t async_eventfd_op;
extern const async_queue_op_t async_cond_op;

static const async_queue_op_t* async_queue_ops[] =
{
    &async_cond_op,
    &async_eventfd_op
};

static threadpool_type_t threadpool_current = threadpool_cond_type;

/**
 * @function void *threadpool_thread(void *threadpool)
 * @brief the worker thread
 * @param threadpool the pool which own the thread
 */
static void* threadpool_thread(void *threadpool);

int threadpool_free(threadpool_t *pool);


BOOL threadpool_config(threadpool_type_t type)
{
    threadpool_current = type;
    return TRUE;
}

threadpool_t* threadpool_create(int thread_count, int queue_size, int flags)
{
    threadpool_t* pool;

    if ((thread_count <=0) || (thread_count > MAX_THREAD_POOL_SIZE) || (queue_size <=0) || (queue_size > MAX_QUEUE_SIZE))
    {
        printf("please check your input parameters.\n");
        return NULL;
    }

    if ((pool = (threadpool_t*)malloc(sizeof(threadpool_t))) == NULL)
    {
        printf("malloc threadpool memory failed.\n");
        return NULL;
    }

    pool->qsize    = queue_size;
    pool->tsize    = thread_count;
    pool->qop      = async_queue_ops[threadpool_current];
    pool->threads  = (pthread_t*)malloc(sizeof(pthread_t) * thread_count);
    pool->shutdown = 0;

#ifdef ENABLE_MULTI_THREAD_QUEUES
    pool->pick_thread_num = 0;  /* default there is one producer thread */
    pool->queue = (async_queue_t **)malloc(sizeof(async_queue_t*) * thread_count);
    for (int i = 0; i < thread_count ; i ++){
        pool->queue[i] = (pool->qop->create)(queue_size);
	}
#else
    pool->queue    =  (pool->qop->create)(queue_size);
#endif

    // create worker thread
    for(int i = 0; i < pool->tsize; i ++)
    {
        if(pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void*)pool) != 0)
        {
            printf("create thread failed.\n");
            threadpool_destroy(pool, 0);
            return NULL;
        }
    }

    pool->started = TRUE;

    return pool;
}

int threadpool_add(threadpool_t *pool, void* (*routine)(void *), void *arg)
{
    int err = 0;

    if(pool == NULL || routine == NULL)
    {
        return threadpool_invalid;
    }

    do {
        /* Are we shutting down ? */
        if(pool->shutdown)
        {
            err = threadpool_shutdown;
            break;
        }

        task_t task;
        task.run  = routine;
        task.argv = arg;

#ifdef ENABLE_MULTI_THREAD_QUEUES
        /*
         * default there is one producer thread, so we should not care about 
         * thread-safe scene, round robin, 罗宾轮询
         */
        pool->pick_thread_num ++;  
        int thread_index = pool->pick_thread_num % pool->tsize;
		
        if (!pool->qop->push(pool->queue[thread_index], &task))
        {
            printf("push task to queue failed.\n");
        }
#else

        if (!pool->qop->push(pool->queue, &task))
        {
            printf("push task to queue failed.\n");
        }

#endif

    } while(0);

    return err;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, err = 0;

    if(pool == NULL)
    {
        return threadpool_invalid;
    }

    do {
        /* Already shutting down */
        if(pool->shutdown)
        {
            err = threadpool_shutdown;
            break;
        }

        pool->shutdown = (flags & threadpool_graceful) ?
            graceful_shutdown : immediate_shutdown;

#ifdef ENABLE_MULTI_THREAD_QUEUES
        /* Destory all worker threads */
        for (int i = 0; i < pool->tsize; i++){
            pool->qop->destory(pool->queue[i]);
		}
#else
        /* Destory all worker threads */
        pool->qop->destory(pool->queue);
#endif

        /* Join all worker thread */
        for(i = 0; i < pool->tsize; i++)
        {
            if(pthread_join(pool->threads[i], NULL) != 0)
            {
                err = threadpool_thread_failure;
            }
        }
    } while(0);

    /* Only if everything went well do we deallocate the pool */
    if(!err)
    {
        threadpool_free(pool);
    }

    return err;
}

int threadpool_free(threadpool_t *pool){

    if(pool == NULL || pool->started > 0){
        return -1;
    }

    if(pool->threads){
        free(pool->threads);
#ifdef ENABLE_MULTI_THREAD_QUEUES
        for(int i = 0; i < pool->tsize; i++){
            pool->qop->free(pool->queue[i]);
		}   
#else
        pool->qop->free(pool->queue);
#endif
    }
    free(pool);

    return 0;
}

static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;


	/* get thread queue if enable multi thread queue feature */
#ifdef ENABLE_MULTI_THREAD_QUEUES
	pthread_t tid = pthread_self();
    int queue_index = 0;
    for (int i = 0; i < pool->tsize ; i ++){
        if (pool->threads[i] == tid){
            queue_index = i;
			break;
		}
	}
	async_queue_t *queue = pool->queue[queue_index];
#else
    async_queue_t *queue = pool->queue;
#endif

    for(;;){

        task_t* task = pool->qop->pop(queue, 50);
	
        if (task != NULL)
        {
            task->run(task->argv);
        }

        if(((pool->shutdown == immediate_shutdown) || (pool->shutdown == graceful_shutdown) )
            && (pool->qop->empty(queue)))
        {
            //printf("--- thread %d is exit.\n", pthread_self());
            break;
        }
    }
	
    return(NULL);
}




