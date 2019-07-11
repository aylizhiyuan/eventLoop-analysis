#include "el.h"

//设置非阻塞的socket连接
//简单的理解一下，就是立即返回该函数，但是会不断的轮询
//满足条件后再次调用该函数
void set_nnblock(int fd){
    int flag;
    //返回文件的性质
    if((flag=fcntl(fd,F_GETFL,0)) < 0){
        error("fcntl get error");
    }
    //其实这里就是设置socket的非阻塞方式
    if(fcntl(fd,F_SETFL,flag | O_NONBLOCK) < 0){
        error("fcntl set error!");
    }
}
//注册事件的函数
//kevent是unix下的一个IO复用的函数，跟epoll一样
int regist(int epollfd,int fd,int type){
    //创建一个事件的数组
    struct kevent changes[1];
    //赋值并向kqueue中添加一个事件
    //changes[0]是标准输入流,changes[1]是标准的输出流
    EV_SET(&changes[0],fd,type,EV_ADD,0,0,NULL);
    int ret = kevent(epollfd,changes,1,NULL,0,NULL);
    return TRUE;
}
//删除事件的函数
int delete(int epollfd,int fd,int flags){
    struct kevent changes[1];
    EV_SET(&changes[0],fd,flags,EV_DELETE,0,0,NULL);
    int ret = kevent(epollfd,changes,1,NULL,0,NULL);
    return TRUE;
}
//初始化kqueue
void kqueue_init(el_loop *loop){
    loop->ioid = kqueue();
    if(loop->ioid < 0){
        error("kqueue error\n");
    }
}
void kqueue_add(el_loop *loop,event *ev){
    regist(loop->ioid,ev->fd,ev->flags);
}
void kqueue_del(el_loop *loop,event *ev){
    delete(loop->ioid,ev->fd,ev->flags);
}
void kqueue_dispatch(el_loop *loop){
    struct kevent events[MAX_EVENT_COUNT];
    int ret = kevent(loop->ioid,NULL,0,events,MAX_EVENT_COUNT,NULL);
    int i;
    for(i=0;i<ret;i++){
        int sock = events[i].ident;
        int data = events[i].data;
        event *e = event_list_delete(loop->active_events,sock);
        delete(loop->ioid,e->fd,e->flags);
        e->size = data;
        event_list_put(loop->ready_events,e);
    }
}
void using_kqueue(el_loop *loop){
    loop->io.name = "kqueue";
    loop->io.init = kqueue_init;
    loop->io.add = kqueue_add;
    loop->io.del = kqueue_del;
    loop->io.dispatch = kqueue_dispatch;
}

