#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<sys/time.h>
#include<arpa/inet.h>
#include<errno.h>
#include<fcntl.h>
#include <unistd.h>
#include <sys/event.h>

#define LISTENQ 1024
#define TRUE 1
#define FALSE 0
/*
---------------
all kinds of io multi flags
 */
#define READ_EVENT EVFILT_READ
#define WRITE_EVENT EVFILT_WRITE
//---------------------
//max epoll or kqueue event's size
#define MAX_EVENT_COUNT 1024
//max line
#define MAXLINE 1024
//callback
typedef void (*cb_func) (int fd, int size, void *arg);
typedef void (*io_init) (struct __el_loop *);
typedef void (*io_add) (struct __el_loop *,  struct __event *);
typedef void (*io_del) (struct __el_loop *,  struct __event *);
typedef void (*io_dispatch) (struct __el_loop *);

/**
   事件
 **/
typedef struct __event {
  int fd;
  int flags;
  int size;
  void *arg;
  cb_func cb;
  struct __event *next;
  struct __event *prev;
} event;

/**
   事件队列
**/
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
  el_event_list *active_events;
  el_event_list *ready_events;
} el_loop;

//error.c
void error(const char *msg);
void error(const char *msg) {
  fprintf(stderr, "%s, error information : \n", msg);
  fprintf(stderr, "%s\n", (char*)strerror(errno));
  exit(-1);
}



//event.c
event *event_init(int fd, int flags, cb_func cb, void *arg);
void event_free(event *ev);


event *event_init(int fd, int flags, cb_func cb, void *arg) {
  event *e = (event*) malloc(sizeof(event));
  e->fd = fd;
  e->flags = flags;
  e->arg = arg;
  e->cb = cb;
  return e;
}

void event_free(event *ev) {
  free(ev);
}


//event_list.c
el_event_list *event_list_init();
void event_list_put(el_event_list *list, event *item);
event *event_list_get(el_event_list *list);
event *event_list_delete(el_event_list *list, int fd);
int event_list_size(el_event_list *list);
int event_list_is_empty(el_event_list *list);
void event_list_free(el_event_list *list);


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

//kqueue.c
void kqueue_init(el_loop *loop);
void kqueue_add(el_loop *loop, event *ev);
void kqueue_del(el_loop *loop, event *ev);
void kqueue_dispatch(el_loop *loop);
void using_kqueue(el_loop* loop);


void set_nonblock(int fd) {
  int flag;
  if ((flag = fcntl(fd, F_GETFL, 0)) < 0)
    error("fcntl get error!");
  if (fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
    error("fcntl set error!");
}

/**
regist event
 **/
int regist(int epollfd, int fd, int type) {
  struct kevent changes[1];
  EV_SET(&changes[0], fd, type, EV_ADD, 0, 0, NULL);
  int ret = kevent(epollfd, changes, 1, NULL, 0, NULL);
  return TRUE;
}

/*
  delete event
*/
int delete(int epollfd, int fd, int flags) {
  struct kevent changes[1];
  EV_SET(&changes[0], fd, flags, EV_DELETE, 0, 0, NULL);
  int ret = kevent(epollfd, changes, 1, NULL, 0, NULL);
  return TRUE;
}

/**
   init kqueue
**/
void kqueue_init(el_loop *loop) {
  loop->ioid = kqueue();
  if (loop->ioid < 0)
    error("kqueue error!\n");
}

void kqueue_add(el_loop *loop, event *ev) {
  regist(loop->ioid, ev->fd, ev->flags);
}

void kqueue_del(el_loop *loop, event *ev) {
  delete(loop->ioid, ev->fd, ev->flags);
}

void kqueue_dispatch(el_loop *loop) {
  struct kevent events[MAX_EVENT_COUNT];
  //首次是监听服务器有没有事件发生，如果有的话则返回ret
  int ret = kevent(loop->ioid, NULL, 0, events, MAX_EVENT_COUNT, NULL);
  int i;
  //循环发生事件的socket
  for (i = 0; i < ret; i++) {
    int sock = events[i].ident;
    int data = events[i].data;
    event *e = event_list_delete(loop->active_events, sock);
    delete(loop->ioid, e->fd, e->flags);
    e->size = data;
    event_list_put(loop->ready_events, e);
  }
}

void using_kqueue(el_loop *loop) {
  loop->io.name = "kqueue";
  loop->io.init = kqueue_init;
  loop->io.add = kqueue_add;
  loop->io.del = kqueue_del;
  loop->io.dispatch = kqueue_dispatch;
}




//loop.c
el_loop *loop_create();
void loop_free(el_loop *loop);
int loop_run(el_loop *loop);


el_loop *loop_create() {
  el_loop *loop = (el_loop*) malloc(sizeof(el_loop));
  loop->event_count = 0;
  loop->active_events = event_list_init();
  loop->ready_events = event_list_init();
  kqueue_init(loop);
  using_kqueue(loop);
  return loop;
}

void loop_free(el_loop *loop) {
  int nactive = event_list_size(loop->active_events);
  int nready = event_list_size(loop->ready_events);
  int i;
  for (i = 0; i < nactive; i++) {
    event *each = event_list_get(loop->active_events);
    event_free(each);
  }
  event_list_free(loop->active_events);
  for (i = 0; i < nready; i++) {
    event *each = event_list_get(loop->ready_events);
    event_free(each);
  }
  event_list_free(loop->ready_events);
  free(loop);
  return;
}

/**
   core part of all code
**/
int loop_run(el_loop *loop) {
  
  while (loop->event_count > 0) {
    loop->io.dispatch(loop);
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



//user.c
el_loop *el_loop_new();
event *el_event_new(int fd, int flags, cb_func cb, void *arg);
void el_event_add(el_loop *loop, event *e);
int el_loop_run(el_loop *loop);

el_loop *el_loop_new() {
  return loop_create();
}

event *el_event_new(int fd, int flags, cb_func cb, void *arg) {
  set_nonblock(fd); //must be no block io
  return event_init(fd, flags, cb, arg);
}

void el_event_add(el_loop *loop, event *e) {
  event_list_put(loop->active_events, e);
  loop->event_count++;
  kqueue_add(loop, e);
}

int el_loop_run(el_loop *loop) {
  return loop_run(loop);
}


/**
create listener
 **/
int create_listener() {
  int listenfd;
  struct sockaddr_in *servaddr = (struct sockaddr_in*) malloc(sizeof(struct sockaddr_in));
  if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    error("socket error!");
  servaddr->sin_family = AF_INET;
  servaddr->sin_port = htons(3333);
  if (inet_pton(AF_INET, "0.0.0.0", &servaddr->sin_addr) < 0)
    error("inet_pton error!");
  if (bind(listenfd, (struct sockaddr *)servaddr, sizeof(struct sockaddr_in)) < 0)
    error("bind error!");
  if (listen(listenfd, LISTENQ) < 0)
    error("listen error!");
  return listenfd;
}

void onread(int fd, int size, void *arg) {
  el_loop *loop = (el_loop*)arg;
  char buf[MAXLINE];
  int n;
  while ((n = read(fd, buf, MAXLINE)) > 0) {
    buf[n] = '\0';
    printf("%s", buf);
  }
  if (n == 0) {
    close(fd);
  } else if (n < 0) {
    if (errno == EWOULDBLOCK || errno == EAGAIN) {
      event *e = el_event_new(fd, READ_EVENT, onread, loop);
      el_event_add(loop, e);
      return;
    } else
      error("read from connected socket error!");
  }
}

void onaccept(int fd, int size, void *arg) {
  el_loop *loop = (el_loop*)arg;
  int i;
  for (i = 0; i < size; i++) {
    int connfd;
    if (( connfd = accept(fd, NULL, NULL)) < 0) {
      if (errno == EWOULDBLOCK || errno == ECONNABORTED
	  || errno == EINTR || errno == EPROTO) {
	continue;
      } else
	error("accept error!");
    }
    event *e = el_event_new(connfd, READ_EVENT, onread, loop);
    el_event_add(loop, e);
    event *old = el_event_new(fd, READ_EVENT, onaccept, loop);
    el_event_add(loop, old);
  }
}

int main() {
  //创建服务器的socket对象  
  int listenfd = create_listener();
  //创建一个loop对象
  //创建一个active_list队列和一个ready队列
  //在loop对象上添加对于kqueue的操作方法以及loop->ioid = kqueue函数
  el_loop *loop = el_loop_new();
  //以服务器的socket对象创建一个新的事件,存储该事件的回调函数和事件的类型
  event *e = el_event_new(listenfd, READ_EVENT, onaccept, loop);
  //将新的事件添加到loop->active_list队列中去，同时，将这个服务器的socket对象加入到kqueue中去
  //
  el_event_add(loop, e);
  //首先还是来看是否有客户端的连接上来，如果没有，该代码将会阻塞在这里
  //如果有的话，将发生事件的客户端放入loop->ready_list队列中去
  //从read_list中取出所有的事件，并调用这些事件的回调函数，直至完成
  //这里面的事件主要是服务器的读事件和多个客户端的读事件，无论是谁发生了，都会继续的调用Loop来进行循环
  //这就是一个简单的event loop
  return el_loop_run(loop);
}
