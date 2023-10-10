#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <ctype.h>
#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// TODO 处理\r\n

int create_addr(char ip[], int port, struct sockaddr_in *addr){
	memset(addr, 0, sizeof(addr));
	addr->sin_family = AF_INET;
	addr->sin_port = port;
	if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {			//转换ip地址:点分十进制-->二进制
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 0;
	}
	return 1;
}

/**
 * @brief 从socket中读取一行到buf，包括换行符，在结尾添加\0
 * 
 * @param fd 
 * @param buf 
 * @return int 是否成功
 */
int read_tcp(int fd, char *buf){
	int p = 0;
	while (1) {
		int n = read(fd, buf + p, 1000 - p);
		if (n < 0){
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
			return 0;
		}
		else if (n == 0){
			break;
		}
		else{
			p += n;
			if (buf[p - 1] == '\n'){
				break;
			}
		}
	}
  buf[p] = '\0';
	return 1;
}

/**
 * @brief 将buf写入socket，直到写完len个字节
 * 
 * @param fd 
 * @param buf 
 * @param len 
 * @return int 是否成功
 */
int write_tcp(int fd, char *buf, int len){
	int p = 0;
	while (p < len){
		int n = write(fd, buf + p, len - p);
		if (n < 0){
			printf("Error write(): %s(%d)\n", strerror(errno), errno);
			return 0;
		}
		else{
			p += n;
		}
	}
  return 1;
}

// void reconnect(int connfd, int listenfd, ){
// 	close(connfd);
// 	if ((connfd = accept(listenfd, SOCK_STREAM, IPPROTO_TCP)) == -1) {
// 		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
// 		return;
// 	}
// }

// void dot_to_comma(char *ip, int port){}