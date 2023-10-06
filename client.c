#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <errno.h>

#include <string.h>
#include <memory.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// read函数的封装，完全读取，防止中断。
int read_tcp(int fd, char *buf);

// write函数的封装，完全写入，防止中断。
int write_tcp(int fd, char *buf, int len);

int main(int argc, char **argv) {
	char sentence[8192];
	int port_server = 21;
	int port_client;
	// char ip[100] = "172.30.204.224";
	char ip[100] = "127.0.0.1";
	char ip_client[100] = "127.0.0.1";
	
	srand(time(NULL));
	port_client = rand() % (65500 - 20000 + 1) + 20000;
	// printf("port_client: %d\n", port_client);

	// server的端口和ip
	// TODO 合法性检查
	for (int i = 1; i < argc; i ++){
		if (strcmp(argv[i], "-port") == 0 && i + 1 < argc){
			port_server = atoi(argv[i + 1]);
			i ++;
		}
		else if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc){
			strcpy(ip, argv[i + 1]);
			i ++;
		}
		else{
			printf("Error: unknown option %s\n", argv[i]);
			return 1;
		}
	}

	while (1){
		int sockfd;
		struct sockaddr_in addr;
		struct sockaddr_in addr_client;
		int len;
		int p;

		//创建socket
		// 该socket用于和目标主机进行命令通信。需要另外的socket用于文件传输。
		if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
			printf("Error socket(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		//设置本机的ip和port
		memset(&addr_client, 0, sizeof(addr_client));
		addr_client.sin_family = AF_INET;
		addr_client.sin_port = port_client;
		if (inet_pton(AF_INET, ip_client, &addr_client.sin_addr) <= 0) {			//转换ip地址:点分十进制-->二进制
			printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}
		bind(sockfd, (struct sockaddr*)&addr_client, sizeof(addr_client));

		//设置目标主机的ip和port
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = port_server;
		if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {			//转换ip地址:点分十进制-->二进制
			printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		//连接上目标主机（将socket和目标主机连接）-- 阻塞函数
		// TODO 220
		if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		//获取键盘输入
		printf("ftp:> ");
		fgets(sentence, 4096, stdin);
		if (strcmp(sentence, "quit\n") == 0){
			break;
		}
		len = strlen(sentence);
		
		//把键盘输入写入socket
		// write_tcp(sockfd, sentence, len);
		// p = 0;
		// while (p < len) {
		// 	int n = write(sockfd, sentence + p, len - p);		//write函数不保证所有的数据写完，可能中途退出
		// 	if (n < 0) {
		// 		printf("Error write(): %s(%d)\n", strerror(errno), errno);
		// 		return 1;
		// 	} else {
		// 		p += n;
		// 	}
		// }

		//榨干socket接收到的内容
		int res = read_tcp(sockfd, sentence);

		// p = 0;
		// while (1) {
		// 	int n = read(sockfd, sentence + p, 8191 - p);
		// 	if (n < 0) {
		// 		printf("Error read(): %s(%d)\n", strerror(errno), errno);	//read不保证一次读完，可能中途退出
		// 		return 1;
		// 	} else if (n == 0) {
		// 		break;
		// 	} else {
		// 		p += n;
		// 		if (sentence[p - 1] == '\n') {
		// 			break;
		// 		}
		// 	}
		// }

		//注意：read并不会将字符串加上'\0'，需要手动添加
		// sentence[p - 1] = '\0';

		// 解析服务器返回信息，进行对应操作
		printf("SERVER: %s", sentence);

		close(sockfd);
	}
	
	return 0;
}

int read_tcp(int fd, char *buf){
	int p = 0;
	while (1) {
		int n = read(fd, buf + p, 8191 - p);
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
	return 1;
}

int write_tcp(int fd, char *buf, int len){
	int p = 0;
	while (p < len){
		int n = write(fd, buf + p, len + 1 - p);
		if (n < 0){
			printf("Error write(): %s(%d)\n", strerror(errno), errno);
			return 0;
		}
		else{
			p += n;
		}
	}
}