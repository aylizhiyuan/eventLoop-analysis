#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

//线程池的队列
//这里需要用到一个数据结构是队列

typedef struct queue *queue_t;
struct node {
    void *element; //该任务的执行函数
    struct node *next; //这个指针指向下一个任务
};
struct queue {
    struct node front; //队头
    struct node *tail; //对尾
};
//创建一个队列的函数
queue_t queue_create(){
    queue_t q;
    //为队列创建空间
    q = (queue_t)malloc(sizeof(struct queue));
    q->front.element = NULL;
    q->front.next = NULL;
    //队尾指向队头
    q->tail = &q->front;
    return q;
}
//检查队列是否为空的函数
int queue_isempty(queue_t q){
    return &q->front == q->tail;
}
//向队列中添加任务
void *queue_enqueue(queue_t q,unsigned int bytes){
    //为队尾的任务创建空间
    //第一个任务q->front ---> 第二个任务q->front->next --->第三个任务q->front->next->next 
    //队尾指向最后一个任务 q->tail指向最后一个任务方便操作
    q->tail->next = (struct node*)malloc(sizeof(struct node));
    q->tail->next->element = malloc(bytes);
    q->tail->next->next = NULL;
    q->tail = q->tail->next;
    return q->tail->element;
}
//队列中拿出任务
void* queue_dequeue(queue_t q){
    //拿出队头的下一个任务
    struct node *tmp = q->front.next;
    void *element;
    if(tmp == NULL){
        return NULL;
    }
    element= tmp->element;
    q->front.next = tmp->next;
    free(tmp);
    if(q->front.next == NULL){
        q->tail = &q->front;
    }
    return element;
}
//删除队列
void queue_destroy(queue_t q){
    struct node *tmp, *p=q->front.next;
	while(p!=NULL) {
		tmp=p;
		p=p->next;
		free(tmp);
	}
	free(q);
}

//线程池的代码
typedef struct thread_pool *thread_pool_t;
struct thread_pool {
    //线程池的中线程的数量
    unsigned int thread_count;
    //线程
    pthread_t *threads;
    //任务队列
    queue_t tasks;
    //线程的互斥锁
    pthread_mutex_t lock;
    //条件变量
    pthread_cond_t task_ready;
};
struct task {
    void* (*routine)(void *arg);
    void *arg;
};

static void cleanup(pthread_mutex_t *lock){
    pthread_mutex_unlock(lock);
};
static void * worker(thread_pool_t pool) {
	struct task *t;
	while(1) {
		pthread_mutex_lock(&pool->lock);
		pthread_cleanup_push((void(*)(void*))cleanup, &pool->lock);
		while(queue_isempty(pool->tasks)) {
			pthread_cond_wait(&pool->task_ready, &pool->lock);
			/*A  condition  wait  (whether  timed  or  not)  is  a  cancellation point ... a side-effect of acting upon a cancellation request  while in a condition wait is that the mutex is (in  effect)  re-acquired  before  calling  the  first  cancellation  cleanup  handler.*/
		}
		t=(struct task*)queue_dequeue(pool->tasks);
		pthread_cleanup_pop(0);
		pthread_mutex_unlock(&pool->lock);
		t->routine(t->arg);/*todo: report returned value*/
		free(t);
	}
	return NULL;
}
 
thread_pool_t thread_pool_create(unsigned int thread_count) {
	unsigned int i;
	thread_pool_t pool=NULL;
	pool=(thread_pool_t)malloc(sizeof(struct thread_pool));
	pool->thread_count=thread_count;
	pool->threads=(pthread_t*)malloc(sizeof(pthread_t)*thread_count);
	
	pool->tasks=queue_create();
	
	pthread_mutex_init(&pool->lock, NULL);
	pthread_cond_init(&pool->task_ready, NULL);
	
	for(i=0; i<thread_count; i++) {
		pthread_create(pool->threads+i, NULL, (void*(*)(void*))worker, pool);
	}
	return pool;
}
 
void thread_pool_add_task(thread_pool_t pool, void* (*routine)(void *arg), void *arg) {
	struct task *t;
	pthread_mutex_lock(&pool->lock);
	t=(struct task*)queue_enqueue(pool->tasks, sizeof(struct task));
	t->routine=routine;
	t->arg=arg;
	pthread_cond_signal(&pool->task_ready);
	pthread_mutex_unlock(&pool->lock);
}
 
void thread_pool_destroy(thread_pool_t pool) {
	unsigned int i;
	for(i=0; i<pool->thread_count; i++) {
		pthread_cancel(pool->threads[i]);
	}
	for(i=0; i<pool->thread_count; i++) {
		pthread_join(pool->threads[i], NULL);
	}
	pthread_mutex_destroy(&pool->lock);
	pthread_cond_destroy(&pool->task_ready);
	queue_destroy(pool->tasks);
	free(pool->threads);
	free(pool);
}
void *test(void *arg){
    int i;
    for(i=0;i<5;i++){
        printf("tid:%ld task:%ld\n",pthread_self(),(long)arg);
        //强制输出，立即输出
        fflush(stdout);
        //睡眠2秒的时间
        sleep(2);
    }
    return NULL;
}
int main(){
    long i=0;
	thread_pool_t pool;
	pool=thread_pool_create(2);
	for(i=0; i<5; i++) {
		thread_pool_add_task(pool, test, (void*)i);
	}
	puts("press enter to terminate ...");
	getchar();
	thread_pool_destroy(pool);
	return 0;
}