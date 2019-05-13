#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

//4.0 控制台打印错误信息, fmt必须是双引号括起来的宏
#define CERR(fmt, ...) \
    fprintf(stderr,"[%s:%s:%d][error %d:%s]" fmt "\r\n",\
         __FILE__, __func__, __LINE__, errno, strerror(errno),##__VA_ARGS__)

//4.1 控制台打印错误信息并退出, t同样fmt必须是 ""括起来的字符串常量
#define CERR_EXIT(fmt,...) \
    CERR(fmt,##__VA_ARGS__),exit(EXIT_FAILURE)

//4.3 if 的 代码检测
#define IF_CHECK(code)    \
    if((code) < 0) \
        CERR_EXIT(#code)

//待拼接的字符串
#define _STR_HTTP_1 "GET /index.html HTTP/1.0\r\nUser-Agent: Happy is good.\r\nHost: 127.0.0.1:"
#define _STR_HTTP_3 "\r\nConnection: close\r\n\r\n"

// 简单请求一下
int main(int argc, char* argv[])
{
    char buf[1024];
    int sfd;
    struct sockaddr_in saddr = { AF_INET };
    int len, port;
    // argc 默认为1 第一个参数 就是 执行程序串
    if((argc != 2) || (port=atoi(argv[1])) <= 0 )
        CERR_EXIT("Usage: %s [port]", argv[0]);
    
    // 开始了,就这样了    
    IF_CHECK(sfd = socket(PF_INET, SOCK_STREAM, 0));
    saddr.sin_port = htons(port);
    saddr.sin_addr.s_addr = INADDR_ANY;
    IF_CHECK(connect(sfd, (struct sockaddr*)&saddr, sizeof saddr));
    
    //开始发送请求
    strcpy(buf, _STR_HTTP_1);
    strcat(buf, argv[1]);
    strcat(buf, _STR_HTTP_3);
    write(sfd, buf, strlen(buf));

    //读取所哟内容
    while((len = read(sfd, buf, sizeof buf - 1))){
        buf[len] = '\0';
        printf("%s", buf);    
    }
    putchar('\n');    

    close(sfd);
    return 0;
}