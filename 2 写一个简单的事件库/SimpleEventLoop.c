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


typedef struct __el_loop {
  int event_count;
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
  event *ret = list->head->next;
  list->head->next = ret->next;
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
      ret = each->next;
      ret->next->prev = each;
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
int main(){
    printf("hello world");
    //定义好Loop之后，这个loop中有一个等待的事件队列，和一个就绪的事件队列
    el_loop *loop = (el_loop*) malloc(sizeof(el_loop));
    //下面我们来分别进行赋值
    loop->active_events = event_list_init();
    loop->ready_events = event_list_init();
    //创建一个新的事件，并把它添加到事件的队列里面去
    event *e = (event*) malloc(sizeof(event));
    event_list_put(loop->active_events,e);
}
