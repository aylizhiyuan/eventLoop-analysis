# nginx-analysis

## 1. nginx 安装

1.1 Mac下的安装

brew update

brew info nginx

brew install nginx

-------------------------------------------


1.2 liunx下的安装

yum -y install gcc gcc-g++ autoconf pcre pcre-devel make automake
yum -y install wget httpd-tools vim

创建一个配置文件

/etc/yum.repos.d/nginx.repo

[nginx]
name=nginx repo
baseurl=http://nginx.org/packages/centos/7/$basearch/
gpgcheck=0
enabled=1

yum install nginx -y
nginx -v

## 2. 安装目录和配置文件

2.1 liunx下

查看配置文件和目录  rpm -ql nginx

配置文件在/etc/nginx/nginx.conf;/etc/nginx/conf.d/*.conf;etc/nginx/conf.d/default.conf


启动nginx 使用 

- systemctl restart nginx.service

- systemctl reload nginx.service

- nginx -S reload

--------------------------------------------

2.2 Mac下

mac下的配置文件在/usr/local/etc/nginx/nginx.conf
默认的打开的文件所在的位置是： /usr/local/var/www
启动nginx 使用 nginx 命令即可

2.3 配置文件 /user/local/etc/nginx目录下文件的介绍：

    /etc/logrotate.d/nginx  用于logrotate服务的日志切割
    /etc/nginx;/etc/nginx/nginx.conf;/etc/nginx/conf.d;/etc/nginx/conf.d/default.conf 主配置文件
    /etc/nginx/fastcgi_params;/etc/nginx/scgi_params;/etc/nginx/uswgi_params cgi配置，fastcgi配置
    /etc/nginx/koi-utf;/etc/nginx/koi-win;/etc/nginx/win-utf 编码转换映射转化文件
    /etc/nginx/mine.types 设置Http协议的content-type与扩展名对应的关系
    /usr/lib/systemd/system/nginx-debug.service;/usr/lib/systemd/system/nginx.service;/etc/sysconfig/nginx;/etc/sysconfig/nginx;/etc/sysconfig/nginx-debug 用于配置系统守护进程管理器管理方式
    /etc/nginx/modules;/usr/lib64/nginx/modules nginx模块
    /var/cache/nginx 缓存
    /var/log/nginx 日志

## 3.nginx配置语法    

- 配置文件由指令与指令块构成
- 每条指令以;分号结尾，指令与参数间以空格符号分割
- 指令块以{}大括号将多条指令组织在一起
- include语句允许组合多个配置文件以提升可维护性
- 使用#符号添加注释，提高可读性
- 使用$符号使用变量
- 部分指令的参数支持正则表达式

## 4. nginx命令行

1. 格式: nginx -s reload
2. 帮助: -? -h
3. 使用指定的配置文件 -c
4. 指定配置指令 -g
5. 指定运行目录 -p
6. 发送信号 -s
    - 立刻停止服务 stop
    - 优雅的停止服务 quit
    - 重载配置文件 reload
    - 重新开始记录日志文件 reopen
7. 测试配置文件是否有语法错误 -t -T
8. 打印nginx版本信息，编译信息 -v -V




## 5. nginx的理解

网络处理高并发请求的思路一般都是从如何解决多个请求开始的，当服务器new Socket()---> bind() ---> listen() .每个客户端调用connection进行连接，服务器则调用accept接受这个连接

        while(1){
            //accept每次都会返回一个新的文件描述符代表着与客户端的连接
            int client = accept(listenfd);
        }

这时候你想想，能用一个循环就解决所有的问题了嘛？

然而我们不可能把所有的网络请求都放在一起对吗？这时候是不是可以考虑使用多进程的技术呢？

    while(1){
        cfd = Accept(lfd,(struct sockaddr *)&clie_addr,&clie_addr_len);
        //连接的客户端
        //多进程版的服务器
        pid = fork();
        if(pid == 0){
            //这是新创建的子进程
            while(1){
                //读取客户端的数据
                n = Read(cfd,buf,sizeof(buf));
                //写回给客户端
                write(cfd,buf,n);
            }
        }
    }

多进程的优点很明显，每个客户端都独立占据一部分的内存空间，处理单个请求不会影响其他的请求，但是，问题也是非常的大，原因是因为多进程的服务器会占用更多的系统资源。

那么可以扩展到多线程的模式，多线程的程序会共享内存空间，线程和线程之间会进行争抢，会存在处理顺序上的问题
    void *do_work(void *arg){
        while (1) {
            n = Read(ts->connfd, buf, MAXLINE);
            Write(ts->connfd, buf, n);
        }
    }
    while (1) {
		connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &cliaddr_len);
		/* 达到线程最大数时，pthread_create出错处理, 增加服务器稳定性 */
        //ts代表的就是服务器的请求，会发送给线程内部
		pthread_create(&tid, NULL, do_work, (void*)&ts[i]);
	}

多线程中依然会存在各种各样的性能问题，每次请求过来，计算机就要为线程单独开辟一个空间，依然是非常消耗的，来一个开一个，过大的连接会把计算机的资源耗尽

epoll是目前大量采用的技术，核心的优点是在于队列在内核，并且epoll有一个等待的就绪队列，直接循环就绪队列，有新的请求来的时候会调用。

epoll在于提供了一个单一的入口来管理这些请求，多路IO复用，简单来看的话，通过一个单一的入口管理所有的网络请求

    //创建我们的epoll模型，传递建议最大的监听数量
	efd = epoll_create(OPEN_MAX);
    //调用ctl函数进行监听的参数设置,将listenfd添加到监听节点中去
	res = epoll_ctl(efd, EPOLL_CTL_ADD, listenfd, &tep);
    while(1){
        //刚开始的时候只监听服务器的读事件
		//eq是我们返回的发生事件的所有fd的集合，刚开始的时候是0
		//下次我们监听的就是服务器和客户端所有的读事件了
		nready = epoll_wait(efd, ep, OPEN_MAX, -1); /* 阻塞监听 */
        //循环监听的请求
        for (i = 0; i < nready; i++) {
            //服务器的读事件，处理新加入的请求
            if (ep[i].data.fd == listenfd) {
                //接收连接，并且将我们的客户端文件描述符放到client数组里面
				connfd = Accept(listenfd, (struct sockaddr *)&cliaddr, &clilen);
                for (j = 0; j < OPEN_MAX; j++) {
					if (client[j] < 0) {
						client[j] = connfd; /* save descriptor */
						break;
					}
				}
                //将客户端的连接加入到我们的监听集合中去
				res = epoll_ctl(efd, EPOLL_CTL_ADD, connfd, &tep);

            }else{
                //如果是客户端的读事件发生的话
				sockfd = ep[i].data.fd;
				n = Read(sockfd, buf, MAXLINE);
				//客户端关闭了连接，读到了0字节
				if (n == 0) {
					for (j = 0; j <= maxi; j++) {
						if (client[j] == sockfd) {
							client[j] = -1;
							break;
						}
					}
					//将客户端从监听节点中去掉
					res = epoll_ctl(efd, EPOLL_CTL_DEL, sockfd, NULL);
					if (res == -1)
						perr_exit("epoll_ctl");

					Close(sockfd);
					printf("client[%d] closed connection\n", j);
				} else {
					//否则读取客户端的数据，写回给客户端
					for (j = 0; j < n; j++)
						buf[j] = toupper(buf[j]);
					Writen(sockfd, buf, n);
				}
            }
        }
    }

epoll维护了一个监听的数组，当数组中的读事件发生的时候，再去调用对应的处理函数处理，注意，这时候epoll_wait类似于accept都是阻塞监听的,实质上依然是同步的把，但是可以设置超时的时间，让它不阻塞

那我们总结一下epoll的好处把，epoll_create的时候创建的监听队列是在内核中的，如果发现有读事件发生的时候，直接在内核态传递给用户态，减少了一次用户态到内核态的复制

并且epoll队列维护了一个就绪的队列nready，只需要轮询所有的就绪队列中的事件即可，不需要将所有的请求都轮询一遍判断当前哪个请求发生了读事件

下一步我们就需要使用eventLoop实现epoll的异步了，epoll_wait依然是阻塞的，如何让它变得不阻塞呢？


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

服务器创建socket ---> 将服务器的socket封装到一个事件中去，并给它注册一个回调函数---> 将该事件首先放入active_list中(待触发的双向链表中),event_count数量+1，然后开启一个循环----> 首次监听服务器的事件发否发生了，如果没有发生阻塞，如果发生了的话，那么它会把事件从active_list扔到ready_list中去 ----> 再一个循环，内循环中会找到在ready_list中的所有事件循环来执行回调函数，执行完后event_count-- ----> 按说怎么会无线循环呢？秘密就在回调函数中

服务器的回调函数onaccpet ----> 调用accpet获取客户端的socket ----> 将客户端的socket封装成一个事件，注册回调函数onread----> 这次再将客户端和服务器端的socket再次加入到active_list中 ---> active_list中始终大于0，所以，循环始终不会终止

如果客户端的连接结束后 ----> 将服务器的事件继续放入到active_list中，循环不终止

你会发现eventloop实质上就是来在外边来了两个循环

    while(1){
        //外层循环不断的循环监听服务器/客户端的事件
        //感觉必须要设置超时的时间才行，一定不能阻塞
        while(1){
            //内层循环将就绪的服务器/客户端的回调函数执行
            //如果事件没有发生的话，继续将服务器/客户端的事件加入到外层循环中去
        }
    }

eventLoop是如何做到非阻塞的呢？个人感觉一定要设置超时的时间，当该事件没有发生的时候，我们直接返回一个一个结果，让程序继续，然后依然把任务扔到eventLoop中，直到任务事件发生了，我们才去执行它。

当然，真正用的时候还是要配合多出一个单独的线程的,例如nodeJS中用来单独处理异步任务的eventLoop


我们来看看nginx的做法

每个worker进程都是从master进程fork出来的，在master进程中，先建立好需要listen的socket之后，然后再fork出多个worker进程,所有的worker进程的listenfd会在新连接到来的时候变得可读，为保证只有一个进程处理该连接,所有的worker进程都会抢accept_mutex,抢到互斥锁的进程注册listenfd读事件，调用accept接受连接。

多个请求 ---> worker进程争抢(并发执行) ----> worker进程中采用异步非阻塞方式处理请求 ---> eventLoop ----> 请求事件

注意，我们之前写的很多异步的代码，例如eventEmitter和promise都是属于手动触发的异步，实际上更多应用场景中，我们需要的是自动触发的，例如ajax,onClick事件这些，在浏览器中，他们其实就是一个eventLoop来负责触发的

6. nginx的连接

nginx在启动阶段，会解析配置文件(nginx.conf),得到需要监听的端口与Ip地址，然后在nginx的master进程里面，先初始化好这个监控的socket,然后再fork出多个子进程出来，子进程会竞争accept新的连接，此时，客户端可以向nginx发起连接了。当客户端与服务器端三次握手建立好一个连接之后，nginx中的一个子进程会accept成功，得到客户端的socket,然后创建ngx_connection_t结构体。接着，设置读写事件处理函数并添加读写事件来与客户端进行数据的交换。最后nginx或客户端主动关掉连接，到此，一个连接完毕


7. nginx的请求处理


8. nginx基本的数据结构
















