# client

## 运行方法

因为使用了linux库函数，本程序只能在linux和wsl中运行

在src目录下，运行make，然后执行client程序即可

linux和wsl的ip机制不同，所以在执行client程序时需要根据环境进行调整

在linux中，如果希望client在port模式下使用公网ip，需要在client后配置参数-local，如下

```bash
client -local r
```

这种情况下，client在port模式下会使用linux中的 `ip a` 命令获取本机ip，并且发送给服务器

在wsl中，由于wsl使用的ip地址是windows分配的局域网地址，所以不能通过简单方式获取公网ip，只能使用 `127.0.0.1` 作为自身ip，要求server和client运行在同一机器上

## 参数

`-ip` 服务器的ip地址，默认为127.0.0.1 （本机）

`-port` 服务器用于接受命令的端口，默认为21

`-root` 客户端文件根目录，用于 `RETR` 和 `STOR`

`-local` 在 `PORT` 中使用公网ip还是127.0.0.1。 `r` 表示公网， `l` 表示本机。默认为本机

## 文件目录

默认目录为当前工作目录的 `client_root` 文件夹
