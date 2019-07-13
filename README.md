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



