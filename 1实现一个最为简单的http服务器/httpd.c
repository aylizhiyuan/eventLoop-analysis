#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
//辅助参数宏
#define sh_isspace(c) ((c==' ') || (c>='\t' && c<='\r'))
//控制台打印错误消息,fmt必须是双引号括起来的宏
#define CERR(fmt,...) \
fprintf(stderr,"[%s:%s:%d][error %d:%s]" fmt "\r\n",\
         __FILE__, __func__, __LINE__, errno, strerror(errno),##__VA_ARGS__)

//4.1 控制台打印错误信息并退出, t同样fmt必须是 ""括起来的字符串常量
#define CERR_EXIT(fmt,...) \
    CERR(fmt,##__VA_ARGS__),exit(EXIT_FAILURE)
//if的代码检测
#define IF_CHECK(code) \
    if((code) < 0) \
        CERR_EXIT(#code)

//辅助变量宏
#define _INT_BUF (1024)
#define _INT_LIS (7)

//读取文件描述符fd 一行的内容，保存在buf中，返回读取内容的长度
int getfdline(int fd,char buf[],int sz);
//返回400 请求解析失败,客户端代码错误
extern inline void response_400(int cfd);
//返回404 文件内容 请求文件没有找到
extern inline void response_404(int cfd);
//返回501 错误 不支持的请求
extern inline void response_501(int cfd);
//服务器内部错误，无法处理
extern inline void response_500(int cfd);
//返回200 请求成功 内容，后面可以加上其他的参数，处理文件输出
extern inline void response_200(int cfd);

//将文件发送给客户端
void response_file(int cfd,const char *path);
//返回启动服务器的描述符，这里灭有采用8080端口，防止冲突，用了随机端口
int serstart(uint16_t pport);
//在客户端链接过来，多线程处理的函数
void* request_accept(void * arg);
//处理客户端的http请求
void request_cgi(int cfd,const char *path,const char*type,const char*query);
//主逻辑，启动服务，可以做成守护进程
int main(int argc,char *argv[]){
    pthread_attr_t attr;
    uint16_t port = 0;
    int sfd = serstart(&port);


}


