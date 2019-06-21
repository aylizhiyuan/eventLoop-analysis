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