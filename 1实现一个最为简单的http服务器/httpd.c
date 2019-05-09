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


