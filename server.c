#include "utils.h"

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

	printf("Start listening\n");

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

		printf("Client connected\n");

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
				printf("%s:%d\n", ip_trans, port_trans);

				// 填充addr_trans
				create_addr(ip_trans, port_trans, &addr_trans);
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

		// 传输文件
		if (strcmp(buf, "RETR") == 0) {
			char msg[] = "50\n";
			write_tcp(connfd, msg, strlen(msg));
			
			// 新建socket，用于传输文件
			int sockfd_trans;
			if ((sockfd_trans = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// 设置目标用于文件传输的地址
			// TODO 根据RETR参数设置地址
			struct sockaddr_in addr_client_trans;
			create_addr("127.0.0.1", 1357, &addr_client_trans);

			// 连接
			if (connect(sockfd_trans, (struct sockaddr*)&addr_client_trans, sizeof(addr_client_trans)) < 0){
				printf("Error connect(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// 通过新socket发送信息
			// TODO broken pip 连接失败
			char file_msg[] = "run RETR\n";
			write_tcp(sockfd_trans, file_msg, strlen(file_msg));
		}

		close(connfd);
	}

	close(listenfd);
}