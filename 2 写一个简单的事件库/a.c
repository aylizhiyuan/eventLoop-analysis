#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<time.h>
#include<arpa/inet.h>
#include<errno.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<signal.h>
#define LISTENQ 1024
#define TRUE 1
#define FALSE 0
#define MAXLINE 1024
//callback
typedef void (*cb_func) (int fd, int size, void *arg);
typedef void (*io_init) (struct __el_loop *);
typedef void (*io_add) (struct __el_loop *,  struct __event *);
typedef void (*io_del) (struct __el_loop *,  struct __event *);
typedef void (*io_dispatch) (struct __el_loop *);
typedef void sigfunc(int);


typedef enum {
  DEFAULT,
  SIGNAL,
  TIMER
} EVENT_TYPE;

typedef struct __event {
  int fd;
  int flags;
  int size;
  void *arg;
  cb_func cb;
  EVENT_TYPE type;
  struct __event *next;
  struct __event *prev;
} event;

typedef struct __el_event_list {
  int count;
  event *head;
} el_event_list;

typedef struct __io_multi {
  const char *name;
  io_init init;
  io_add add;
  io_del del;
  io_dispatch dispatch;
} io_multi;


typedef struct __el_loop {
  int event_count;
  io_multi io;
  int ioid;
  int timeout; //ms
  el_event_list *active_events;
  el_event_list *ready_events;
} el_loop;



void error(const char *msg) {
  fprintf(stderr, "%s, error information : \n", msg);
  fprintf(stderr, "%s\n", (char*)strerror(errno));
  exit(-1);
}
//事件初始化
event *event_init(int fd, int flags, cb_func cb, void *arg) {
  event *e = (event*) malloc(sizeof(event));
  e->fd = fd;
  e->flags = flags;
  e->arg = arg;
  e->cb = cb;
  e->type = DEFAULT;
  return e;
}
//释放事件的内存空间
void event_free(event *ev) {
  free(ev);
}



/**
   init event list
 **/
el_event_list *event_list_init() {
  event *new_event = (event*) malloc(sizeof(event));
  new_event->fd = -1;
  new_event->flags = 0;
  new_event->size = 0;
  new_event->arg = NULL;
  new_event->cb = NULL;
  new_event->next = new_event;
  new_event->prev = new_event;
  el_event_list *list = (el_event_list*) malloc(sizeof(el_event_list));
  list->count = 0;
  list->head = new_event;
  return list;
}



void event_list_put(el_event_list *list, event *item) {
  list->count++;

  item->prev = list->head->prev;
  list->head->prev->next = item;
  item->next = list->head;
  list->head->prev = item;
}

event *event_list_get(el_event_list *list) {
  if (list->count <= 0)
    error("event list is empty!");
  list->count--;
  //拿到下一个任务
  event *ret = list->head->next;
  //将链表的下一个任务指向下下个任务
  list->head->next = ret->next;
  //将下下个任务的上一个任务指向头部
  ret->next->prev = list->head;
  return ret;
}

event *event_list_delete(el_event_list *list, int fd) {
  event *each = list->head;
  int size = list->count;
  int i;
  event *ret;
  for (i = 0; i < size; i++) {
    if (each->next->fd == fd) {
      //ret就是当前要删除的任务
      ret = each->next;
      //将删除任务的上一个任务
      ret->next->prev = each;
      //将任务替换掉
      each->next = ret->next;
      break;
    }
    each = each->next;
  }
  if (i == size)
    error("event-list not contain item!");
  list->count--;
  return ret;
}

int event_list_size(el_event_list *list) {
  return list->count;
}

int event_list_is_empty(el_event_list *list) {
  return (list->count > 0) ? FALSE : TRUE;
}

void event_list_free(el_event_list *list) {
  free(list->head);
  free(list);
}
void onaccept(int fd, int size, void *arg) {
    printf("hello e!!!!");
}
int main(){
    printf("hello world");
    //定义好Loop之后，这个loop中有一个等待的事件队列，和一个就绪的事件队列
    el_loop *loop = (el_loop*) malloc(sizeof(el_loop));
    //下面我们来分别进行赋值
    loop->event_count = 0;
    loop->timeout = -1;
    loop->active_events = event_list_init();
    loop->ready_events = event_list_init();

    //给loop添加
    // loop->io.name = "kqueue";
    // loop->io.init = kqueue_init;
    // loop->io.add = kqueue_add;
    // loop->io.del = kqueue_del;
    // loop->io.dispatch = kqueue_dispatch;
    // loop->io.init(loop);
    
    //创建一个新的事件，并把它添加到事件的队列里面去
    event *e = event_init(1, 1, onaccept, loop);
    
    //新添加的事件跟初始化的事件交替连接
    //每新添加一个事件count值就加1，这是等待列表中的事件数量
    event_list_put(loop->active_events,e);
    //循环体上的event_count加1
    loop->event_count++;
    //这里还需要将该事件添加到kqueue里面去
    //loop->io.add(loop, e);

    //整体的大概思路是明白了，loop维护了两个事件列表,一个是等待列表，一个是就绪列表
    //所有的事件首先进入等待列表中，记录长度和形成一个链表的结构
    //下面开始循环，根据event_count中的数量进行外层的循环，这里不清楚含义
    //里面的循环非常的简单，将就绪列表中的任务一个个的取出来，执行回调函数
    //当就绪列表空了跳出内层循环
    //当event_count值为0的时候跳出外层循环
    //但我感觉这个循环应该是跳不出的，它应该是个无限的死循环，这得看我们下次修改的结果了
    while (loop->event_count > 0) {
      while (event_list_size(loop->ready_events) > 0) {
        event *e = event_list_get(loop->ready_events);
        //call the callback function
        e->cb(e->fd, e->size, e->arg);
        event_free(e);
        loop->event_count--;
      }
  }
  return 0;
}
