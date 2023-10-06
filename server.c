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

// read函数的封装，完全读取，防止中断。
int read_tcp(int fd, char *buf);

// write函数的封装，完全写入，防止中断。
int write_tcp(int fd, char *buf, int len);

int main(int argc, char **argv) {
	int listenfd, connfd;		//监听socket和连接socket不一样，后者用于数据传输
	struct sockaddr_in addr;
	char sentence[1000];
	int p;
	int len;
	int port = 21;
	char root[4096] = "/tmp";
	const int cap_user_list = 10;
	struct sockaddr_in user_list[cap_user_list];
	int n_users = 0;

	printf("Server start\n");

	// 解析argc
	// TODO 合法性检查
	for (int i = 1; i < argc; i ++){
		if (strcmp(argv[i], "-port") == 0 && i + 1 < argc){
			port = atoi(argv[i + 1]);
			i ++;
		}
		else if (strcmp(argv[i], "-root") == 0 && i + 1 < argc){
			strcpy(root, argv[i + 1]);
			i ++;
		}
		else{
			printf("Error: unknown option %s\n", argv[i]);
			return 1;
		}
	}

	//创建监听socket
	if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
		printf("Error socket(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_port = port;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);	//监听"0.0.0.0"

	// socket监听本主机的任何ip（localhost，公网，局域网）的port端口，默认为21
	if (bind(listenfd, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
		printf("Error bind(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	// 开始监听socket
	if (listen(listenfd, 10) == -1) {
		printf("Error listen(): %s(%d)\n", strerror(errno), errno);
		return 1;
	}

	printf("Socket established\n");

	// 循环处理client的请求
	while (1) {
		// 用于接收命令的socket
		// TODO 使用select改为非阻塞式。只使用accept，导致一直等待直到listenfd有连接，期间无法处理其他client的请求。
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}

		// 读取client的命令
		read_tcp(connfd, sentence);

		// 检查是否为新用户
		int is_new_user = 1;
		for (int i = 0; i < n_users; i ++){
			if (user_list[i].sin_addr.s_addr == client_addr.sin_addr.s_addr &&
				user_list[i].sin_port == client_addr.sin_port){
				is_new_user = 0;
				break;
			}
		}
		// 新用户需要登录，然后加入用户列表，
		if (is_new_user){
			// read_tcp(connfd, sentence);
			char buf[100];
			int res = sscanf(sentence, "USER %s", buf);
			
			if (res != 1){
				printf("Error: new user should send USER command first\n");
				char msg_login[] = "403 new user should send USER command first\n\n";
				write(connfd, msg_login, strlen(msg_login));
				close(connfd);
				continue;
			}
			else if (strcmp(buf, "anonymous") != 0){
				printf("Error: only anonymous user is supported\n");
				char msg_login[] = "403 only anonymous user is supported\n\n";
				write(connfd, msg_login, strlen(msg_login));
				close(connfd);
				continue;
			}

			char msg_login[] = "331 Guest login ok, send your complete e-mail address as password.\n";
			write_tcp(connfd, msg_login, strlen(msg_login));
			close(connfd);

			user_list[n_users] = client_addr;
			n_users ++;
			
			continue;
		}
		
		char buf[100];
		struct sockaddr_in addr_trans;
		int is_addr_specified = 0;
		char ip_trans[100];
		int port_trans = 20;
		int res = sscanf(sentence, "%s", buf);
		int continue_main_loop = 0;
		// 读取指令，直到不是PORT或PASV
		// 如果是PORT就设置传输文件的addr，然后可以执行后续命令
		while (strcmp(buf, "PORT") == 0 || strcmp(buf, "PASV") == 0){
			if (strcmp(buf, "PORT") == 0){
				int h1,h2,h3,h4,p1,p2;
				res = sscanf(sentence, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);

				// 重新进行主循环
				if (res != 6){
					printf("Error: PORT command format error\n");
					char msg_login[] = "403 PORT command format error\n\n";
					write_tcp(connfd, msg_login, strlen(msg_login));

					close(connfd);

					continue_main_loop = 1;
					break;
				}
				
				// 计算实际ip,port
				sprintf(ip_trans, "%d.%d.%d.%d", h1, h2, h3, h4);
				port_trans = p1 * 256 + p2; // 可以反推出p1的最大值是256
				printf("%s:%d", ip_trans, port_trans);

				// 填充addr_trans
				memset(&addr_trans, 0, sizeof(addr));
				addr_trans.sin_family = AF_INET;
				if (inet_pton(AF_INET, ip_trans, &addr_trans.sin_addr) <= 0) {			//转换ip地址:点分十进制-->二进制
					printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
					continue;
				}
				addr_trans.sin_port = port_trans;
				is_addr_specified = 1;

				continue_main_loop = 0;
				break;
				// 终止PORT循环，继续主循环
			}
			else if (strcmp(buf, "PASV") == 0){
				// TODO
			}
			
			read_tcp(connfd, sentence);
		}
		if (continue_main_loop){
			close(connfd);
			continue;
		}

		char msg[] = "port sepicified\n";
		write_tcp(connfd, msg, strlen(msg));

		// 重启socket
		close(connfd);
		if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}

		// 重新读取指令
		read_tcp(connfd, sentence);
		res = sscanf(sentence, "%s", buf);

		if (strcmp(buf, "RETR")) {
			char msg[] = "run RETR";
			write_tcp(connfd, msg, strlen(msg));
		}

		//榨干socket传来的内容
		// p = 0;
		// int res = read_tcp(connfd, sentence);
		// if (res == 0){
		// 	close(connfd);
		// 	continue;
		// }
		// len = strlen(sentence);
		//字符串处理
		// for (p = 0; p < len; p++) {
		// 	sentence[p] = toupper(sentence[p]);
		// }
		//发送字符串到socket
 		// p = 0;
		// while (p < len) {
		// 	int n = write(connfd, sentence + p, len + 1 - p);
		// 	if (n < 0) {
		// 		printf("Error write(): %s(%d)\n", strerror(errno), errno);
		// 		return 1;
	 	// 	} else {
		// 		p += n;
		// 	}
		// }

		close(connfd);
	}

	close(listenfd);
}

// read函数的封装，完全读取，防止中断。
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