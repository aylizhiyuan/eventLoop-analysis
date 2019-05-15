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

// --------------------------------------- 辅助参数宏 ----------------------------------------------
/*
 * c 如果是空白字符返回 true, 否则返回false
 * c : 必须是 int 值,最好是 char 范围
 */
#define sh_isspace(c) \
    ((c==' ')||(c>='\t'&&c<='\r'))
    
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
        
// --------------------------------------- 辅助变量宏 和 声明 ------------------------------------------

// char[]缓冲区大小
#define _INT_BUF (1024)
// listen监听队列的大小
#define _INT_LIS (7)

/*
 * 读取文件描述符 fd 一行的内容,保存在buf中,返回读取内容长度 
 * fd        : 文件描述符
 * buf        : 保存的内容
 * sz        : buf 的大小
 *            : 返回读取的长度
 */
int getfdline(int fd, char buf[], int sz);

// 返回400 请求解析失败,客户端代码错误
extern inline void response_400(int cfd);

// 返回404 文件内容, 请求文件没有找见
extern inline void response_404(int cfd);

// 返回501 错误, 不支持的请求
extern inline void response_501(int cfd);

// 服务器内部错误,无法处理等
extern inline void response_500(int cfd);

// 返回200 请求成功 内容, 后面可以加上其它参数,处理文件输出
extern inline void response_200(int cfd);

/*
 * 将文件 发送给客户端
 * cfd        : 客户端文件描述符
 * path        : 发送的文件路径
 */
void response_file(int cfd, const char* path);

/*
 * 返回启动的服务器描述符(句柄), 这里没有采用8080端口,防止冲突,用了随机端口
 * pport     : 输出参数和输出参数, 如果传入NULL,将不返回自动分配的端口
 *             : 返回 启动的文件描述符
 */
int serstart(uint16_t* pport);

/*
 * 在客户端链接过来,多线程处理的函数
 * arg        : 传入的参数, 客户端文件描述符 (int)arg
 *             : 返回处理结果,这里默认返回 NULL
 */
void* request_accept(void* arg);

/*
 * 处理客户端的http请求.
 * cfd        : 客户端文件描述符
 * path        : 请求的文件路径
 * type        : 请求类型,默认是POST,其它是GET
 * query    : 请求发送的过来的数据, url ? 后面那些数据
 */
void request_cgi(int cfd, const char* path, const char* type, const char* query);

/*
 * 主逻辑,启动服务,可以做成守护进程.
 * 具体的实现逻辑, 启动小型玩乐级别的httpd 服务
 */
int main(int argc, char* argv[])
{
    pthread_attr_t attr;
    uint16_t port = 0;
    //传入传出参数，用于服务器自动生成端口号，并socket---> bind -----> listen 
    int sfd = serstart(&port);
    
    printf("httpd running on port %u.\n", port);
    // 初始化线程属性，调用后，attr中包含的就是操作系统实现支持的线程所有属性的默认值
    pthread_attr_init(&attr);
    //设置线程的分离状态，默认是不分离的状态，改为分离状态。这样当线程结束后，马上释放系统资源，否则要等到pthread_join函数返回时
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    for(;;){
        pthread_t tid;
        struct sockaddr_in caddr;
        socklen_t clen = sizeof caddr;
        int cfd = accept(sfd, (struct sockaddr*)&caddr, &clen);
        if(cfd < 0){
            CERR("accept sfd = %d is error!", sfd);
            break;
        }
        if(pthread_create(&tid, &attr, request_accept, (void*)cfd) < 0)
            CERR("pthread_create run is error!");
    }
    // 销毁吧, 一切都结束了
    pthread_attr_destroy(&attr);
    close(sfd);
    return 0;
}

// ----------------------------------------- 具体的函数实现过程 ------------------------------------------------

/*
 * 读取文件描述符 fd 一行的内容,保存在buf中,返回读取内容长度 
 * fd        : 文件描述符
 * buf        : 保存的内容
 * sz        : buf 的大小
 *            : 返回读取的长度
 */
int 
getfdline(int fd, char buf[], int sz)
{
    //buf是指向数组的指针，tp也是指向数组的指针
    char* tp = buf;
    char c;
    //sz的值从1024开始减少
    --sz;
    while((tp-buf)<sz){
        //读取从客户端发过来的数据，一个字节一个字节的读
        if(read(fd, &c, 1) <= 0) //伪造结束条件
            break;
        if(c == '\r'){ //全部以\r分割
            if(recv(fd, &c, 1, MSG_PEEK)>0 && c == '\n')
                read(fd, &c, 1);
            else //意外的结束,填充 \n 结束读取
                *tp++ = '\n';
            break;
        }
        //不断的将读取的内容赋值给tp数组，tp数组内的指针不断的移动
        *tp++ = c;
    }
    *tp = '\0';
    return tp - buf;
}

// 返回400 请求解析失败,客户端代码错误
inline void 
response_400(int cfd)
{
    const char* estr = "HTTP/1.0 400 BAD REQUEST\r\n"
    "Server: wz simple httpd 1.0\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<p>你的请求有问题,请检查语法!</p>\r\n";
    
    write(cfd, estr, strlen(estr));
}

// 返回404 文件内容, 请求文件没有找见
inline void 
response_404(int cfd)
{
    const char* estr = "HTTP/1.0 404 NOT FOUND\r\n"
    "Server: wz simple httpd 1.0\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
    "<head><title>你请求的界面被查水表了!</title></head>\r\n"
    "<body><p>404: 估计是回不来了</p></body>"
    "</html>";
    
    //开始发送数据
    write(cfd, estr, strlen(estr));
}

// 返回501 错误, 请求解析失败,不支持的请求
inline void 
response_501(int cfd)
{
    const char* estr = "HTTP/1.0 501 Method Not Implemented\r\n"
    "Server: wz simple httpd 1.0\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
    "<head><title>小伙子不要乱请求</title></head>\r\n"
    "<body><p>too young too simple, 年轻人别总想弄出个大新闻.</p></body>"
    "</html>";
    
    //这里还有一个好的做法是将这些内容定义在文件中输出文件
    write(cfd, estr, strlen(estr));
}


// 服务器内部错误,无法处理等
inline void 
response_500(int cfd)
{
    const char* estr = "HTTP/1.0 500 Internal Server Error\r\n"
    "Server: wz simple httpd 1.0\r\n"
    "Content-Type: text/html\r\n"
    "\r\n"
    "<html>"
    "<head><title>Sorry </title></head>\r\n"
    "<body><p>最近有点方了!</p></body>"
    "</html>";
    
    write(cfd, estr, strlen(estr));
}

// 返回200 请求成功 内容, 后面可以加上其它参数,处理文件输出
inline void 
response_200(int cfd)
{
    // 打印返回200的报文头
    const char* str = "HTTP/1.0 200 OK\r\n"
    "Server: wz simple httpd 1.0\r\n"
    "Content-Type: text/html\r\n"
    "\r\n";
    
    write(cfd, str, strlen(str));
}

/*
 * 将文件 发送给客户端
 * cfd        : 客户端文件描述符
 * path        : 发送的文件路径
 */
void 
response_file(int cfd, const char* path)
{
    FILE* txt;
    char buf[_INT_BUF];
    
    // 读取报文头,就是过滤
    while(getfdline(cfd, buf, sizeof buf)>0 && strcmp("\n", buf))
        ;
    // 这里开始处理 文件内容
    if((txt = fopen(path, "r")) == NULL) //文件解析错误,给它个404
        response_404(cfd);
    else{
        response_200(cfd); //发送给200的报文头过去
        // 先判断文件内容存在
        while(!feof(txt) && fgets(buf, sizeof buf, txt))
            write(cfd, buf, strlen(buf));
    }
    fclose(txt);
}

/*
 * 返回启动的服务器描述符(句柄)
 * pport     : 输出参数和输出参数, 如果传入NULL,将不返回自动分配的端口
 *             : 返回 启动的文件描述符
 */
int 
serstart(uint16_t* pport)
{
    int sfd;
    struct sockaddr_in saddr = { AF_INET };
    //创建一个socket连接,PF_INET代表的含义是IPV4协议,sock_stream代表的是TCP的数据流
    //0代表的是协议，使用0代表根据前两个参数的默认协议走
    IF_CHECK(sfd = socket(PF_INET, SOCK_STREAM, 0)); //返回socket文件描述符sfd
    //将端口号转化为网络字节序
    saddr.sin_port = !pport || !*pport ? 0 : htons(*pport);
    //地址设置为本机的地址,INADDR_ANY 就是0.0.0.0
    saddr.sin_addr.s_addr = INADDR_ANY;
    // 绑定一下端口信息
    IF_CHECK(bind(sfd, (struct sockaddr*)&saddr, sizeof saddr));
    if(pport && !*pport){
        socklen_t clen = sizeof saddr;
        //在以端口号为0调用bind（告知内核去选择本地临时端口号）后，getsockname用于返回由内核赋予的本地端口号
        IF_CHECK(getsockname(sfd, (struct sockaddr*)&saddr, &clen));
        //将端口号转化成网络字节序
        *pport = ntohs(saddr.sin_port);
    }
    // 开启监听任务
    IF_CHECK(listen(sfd, _INT_LIS));
    return sfd;
}

/*
 * 在客户端链接过来,多线程处理的函数
 * arg        : 传入的参数, 客户端文件描述符 (int)arg
 *             : 返回处理结果,这里默认返回 NULL
 */
void* 
request_accept(void* arg)
{
    char buf[_INT_BUF], path[_INT_BUF>>1], type[_INT_BUF>>5];
    char *lt, *rt, *query, *nb = buf;
    struct stat st;
    int iscgi, cfd = (int)arg;
    //获取我们的请求头数据，例如"GET /index.html HTTP/1.0"
    if(getfdline(cfd, buf, sizeof buf) <= 0){ //请求错误
        response_501(cfd);
        close(cfd);
        return NULL;
    }
    // 合法请求处理
    //lt是一个32个元素的数组，rt是一个存储了HTTP头信息的1024的数组
    //将请求的方式放入type数组里面
    for(lt=type, rt=nb; !sh_isspace(*rt) && (lt-type)< sizeof type - 1; *lt++ = *rt++)
        ;
    *lt = '\0'; //已经将 buf中开始不为empty 部分塞入了 type 中
    //同样处理合法与否判断, 出错了直接返回错误结果
    if((iscgi = strcasecmp(type, "POST")) && strcasecmp(type, "GET")){
        response_501(cfd);
        close(cfd);
        return NULL;
    }
    // 在buf中 去掉空字符
    while(*rt && sh_isspace(*rt))
        ++rt;
    // 这里得到路径信息
    *path = '.';
    for(lt = path + 1; (lt-path)<sizeof path - 1 && !sh_isspace(*rt); *lt++ = *rt++)
        ;
    *lt = '\0'; //query url路径就拼接好了
    
    //单独处理 get 获取 ? 后面数据, 不是POST那就是GET
    if(iscgi != 0){
        for(query = path; *query && *query != '?'; ++query)
            ;
        if(*query == '?'){
            iscgi = 0;
            *query++ = '\0';
        }
    }
    
    // type , path 和 query 已经构建好了
    if(stat(path, &st) < 0){
        while(getfdline(cfd, buf, sizeof buf)>0 && strcmp("\n", buf))// 读取内容直到结束
            ;
        response_404(cfd);
        close(cfd);
        return NULL;
    }
    // 合法情况, 执行,写入,读取权限
    if ((st.st_mode & S_IXUSR) ||(st.st_mode & S_IXGRP) ||(st.st_mode & S_IXOTH))
        iscgi = 0;
    if(iscgi) //没有cgi
        response_file(cfd, path);
    else
        request_cgi(cfd, path, type, query);
    
    close(cfd);
    return NULL;
}

/*
 * 处理客户端的http请求.
 * cfd        : 客户端文件描述符
 * path        : 请求的文件路径
 * type        : 请求类型,默认是POST,其它是GET
 * query    : 请求发送的过来的数据, url ? 后面那些数据
 */
void 
request_cgi(int cfd, const char* path, const char* type, const char* query)
{
    char buf[_INT_BUF];
    int pocgi[2], picgi[2];
    pid_t pid;
    int contlen = -1; //报文长度
    char c;
    
    if(strcasecmp(type, "POST") == 0){
        while(getfdline(cfd, buf, sizeof buf)>0 && strcmp("\n", buf)){
            buf[15] = '\0';
            if(!strcasecmp(buf, "Content-Length:"))
                contlen = atoi(buf + 16);
        }
        if(contlen == -1){ //错误的报文,直接返回错误结果
            response_400(cfd);
            return;
        }
    } 
    else{ // 读取报文头,就是过滤, 后面就假定是 GET
        while(getfdline(cfd, buf, sizeof buf)>0 && strcmp("\n", buf))
            ;
    }
    
    //这里处理请求内容, 先处理错误信息
    if(pipe(pocgi) < 0){
        response_500(cfd);
        return;
    }
    if(pipe(picgi) < 0){ //管道 是 0读取, 1写入
        close(pocgi[0]), close(pocgi[1]);
        response_500(cfd);
        return;
    }
    if((pid = fork())<0){
        close(pocgi[0]), close(pocgi[1]);
        close(picgi[0]), close(picgi[1]);
        response_500(cfd);
        return;
    }
    // 这里就是多进程处理了, 先处理子进程
    if(pid == 0) {
        // dup2 让 前者共享后者同样的文件表
        dup2(pocgi[1], STDOUT_FILENO); //标准输出算作 pocgi管道 的写入端
        dup2(picgi[0], STDIN_FILENO); //标准输入做为picgif管道的读取端
        close(pocgi[0]);
        close(pocgi[1]);
        
        // 添加环境变量,用于当前会话中
        sprintf(buf, "REQUEST_METHOD=%s", type);
        putenv(buf);
        // 继续凑环境变量串,放到当前会话种
        if(strcasecmp(buf, "POST") == 0)
            sprintf(buf, "CONTENT_LENGTH=%d", contlen);
        else
            sprintf(buf, "QUERY_STRING=%s", query);
        putenv(buf);
        // 成功的话调到 新的执行体上
        execl(path, path, NULL);
        
        // 这行代码原本是不用的, 但是防止 execl执行失败, 子进程没有退出.妙招
        exit(EXIT_SUCCESS);
    }
    // 父进程, 为所欲为了,先发送个OK
    write(cfd, "HTTP/1.0 200 OK\r\n", 17);
    close(pocgi[1]);
    close(picgi[0]);
    
    if(strcasecmp(type, "POST") == 0){
        int i; //将数据都写入到 picgi 管道中, 让子进程在 picgi[0]中读取 => STDIN_FILENO
        for(i=0; i<contlen; ++i){
            read(cfd, &c, 1);
            write(picgi[1], &c, 1);
        }
    }
    //从子进程中 读取数据 发送给客户端, 多线程跨进程阻塞模型
    while(read(pocgi[0], &c, 1) > 0)
        write(cfd, &c, 1);
    
    close(pocgi[0]);
    close(picgi[1]);
    //等待子进程结束
    waitpid(pid, NULL, 0);
}