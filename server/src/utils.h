#pragma once

#include <sys/socket.h>
#include <sys/stat.h>
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

#define ERR_RETURN { \
	printf("%s(%d)\n", strerror(errno), errno); \
	return 1; \
}

#define ERR_CONTINUE { \
	printf("%s(%d)\n", strerror(errno), errno); \
	continue; \
}

#define ERR_BREAK { \
	printf("%s(%d)\n", strerror(errno), errno); \
	break; \
}

enum Trans_mode {
	UNDEFINED_MODE,
	PORT_MODE,
	PASV_MODE
};

enum Trans_cmd{
	UNDEFINED_TRANS_CMD,
	RETR,
	STOR,
	LIST
};

// 用于防止缓冲区溢出
const int small_len = 100;
const int cap_buff_file = 10000;

/**
 * @brief Create an addr
 * 
 * @param ip 
 * @param port 
 * @param addr 
 * @return int 
 */
int create_addr(char ip[], int port, struct sockaddr_in *addr){
	memset(addr, 0, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {			//转换ip地址:点分十进制-->二进制
		printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
		return 0;
	}
	return 1;
}

// TODO 深入了解read函数
/**
 * @brief 从fd中读取最多1000字节到buff，读到\0时read终止
 * 会在buf结尾添加\0
 * read遇到\0会停止，但是不会读入buff，因此要靠\n来判断是否读完
 * @pre fd中的数据必须以\n结尾，否则循环无法终止
 * 对应要求使用write函数需要考虑\n
 * 
 * @param fd 
 * @param buf 
 * @return int 
 */
int tcp_read_line(int fd, char *buf){
	const int len = 1000;
	int p = 0;

	while (1) {
		if (p >= len) break;
		int n = read(fd, buf + p, len - p);
		// int n = read(fd, buf + p, 1000 - p);
		if (n < 0){
			printf("Error read(): %s(%d)\n", strerror(errno), errno);
			return 0;
		}
		else if (n == 0) break;
		else{
			p += n;
			if (buf[p - 1] == '\n') break;
		}
	}
  buf[p] = '\0';
	return 1;
}

/**
 * @brief 将字符串buf写入fd
 * @pre buf以\0结尾
 * 
 * @param fd 
 * @param buf 
 * @return int 
 */
int tcp_write_str(int fd, char *buf){
	int p = 0;
	int len = strlen(buf);
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

/**
 * @brief 将buf的至多len写入fd
 * 
 * @param fd 
 * @param buf 
 * @param len 
 * @return int 
 */
int tcp_write_len(int fd, char *buf, size_t len){
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

/**
 * @brief 将字符串str末尾的"\n"替换成"\r\n"
 * 
 * @param str 
 */
void add_return(char str[]){
	int len = strlen(str);
	if (len <= 0 || str[len - 1] != '\n') return;
	str[len - 1] = '\r';
	str[len] = '\n';
	str[len + 1] = '\0';
}