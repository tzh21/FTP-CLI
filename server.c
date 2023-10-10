#include "utils.h"

int main(int argc, char **argv) {
	// TODO 自定义端口、根目录
	const int recv_buff_cap = 2000; // 接收缓冲区大小
	const int user_list_cap = 100; // 用户列表容量
	struct sockaddr_in addr; // 监听地址
	struct sockaddr_in user_list[user_list_cap]; // 用户列表
	char recv_buff[recv_buff_cap]; // 接收缓冲区
	char root[1000] = "/tmp";
	int listenfd; // 监听socket
	int connfd;		// 连接socket，用于传输请求和响应
	int port = 21;
	int n_users = 0; // 用户数量

	printf("Server start\n");

	// TODO 合法性检查
	// 解析argc
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

	create_addr("0.0.0.0", port, &addr);
	addr.sin_addr.s_addr = htonl(INADDR_ANY);	// 监听"0.0.0.0"

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
		struct sockaddr_in addr_client_trans;
		socklen_t client_addr_len = sizeof(client_addr);
		char ip_trans[100] = "127.0.0.1";
		char client_cmd[100];
		int is_addr_specified = 0;
		int port_trans = 1357;
		int res = 0;
		int continue_main_loop = 0;
		int is_new_user = 1;

		if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
			printf("Error accept(): %s(%d)\n", strerror(errno), errno);
			continue;
		}

		printf("Client connected\n");

		// 读取client的命令
		read_tcp(connfd, recv_buff);

		// 检查是否为新用户
		for (int i = 0; i < n_users; i ++){
			if (user_list[i].sin_addr.s_addr == client_addr.sin_addr.s_addr &&
				user_list[i].sin_port == client_addr.sin_port){
				is_new_user = 0;
				break;
			}
		}

		// 新用户需要登录，然后加入用户列表
		// TODO 密码
		if (is_new_user){
			char buf[100];
			int res = sscanf(recv_buff, "USER %s", buf);
			
			// 处理异常
			if (res != 1){
				printf("Error: new user should send USER command first\n");
				char msg[] = "403 new user should send USER command first\r\n";
				write(connfd, msg, strlen(msg));
				close(connfd);
				continue;
			}
			else if (strcmp(buf, "anonymous") != 0){
				printf("Error: only anonymous user is supported\n");
				char msg[] = "403 only anonymous user is supported\n\n";
				write(connfd, msg, strlen(msg));
				close(connfd);
				continue;
			}

			char msg[] = "331 Guest login ok, send your complete e-mail address as password.\n";
			write_tcp(connfd, msg, strlen(msg));
			close(connfd);

			user_list[n_users] = client_addr;
			n_users ++;
			
			continue;
		}
		
		// TODO 根据client的参数修改
		res = sscanf(recv_buff, "%s", client_cmd);
		// 读取指令，直到不是PORT或PASV
		// 如果是PORT就设置传输文件的addr，然后可以执行后续命令
		while (strcmp(client_cmd, "PORT") == 0 || strcmp(client_cmd, "PASV") == 0){
			if (strcmp(client_cmd, "PORT") == 0){
				int h1,h2,h3,h4,p1,p2;
				
				res = sscanf(recv_buff, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);

				// 处理异常
				if (res != 6){
					printf("Error: PORT command format error\n");
					char msg[] = "403 PORT command format error\n";
					write_tcp(connfd, msg, strlen(msg));

					close(connfd);

					continue_main_loop = 1;
					break;
				}
				
				// 计算实际ip,port
				sprintf(ip_trans, "%d.%d.%d.%d", h1, h2, h3, h4);
				port_trans = p1 * 256 + p2; // 可以反推出p1的最大值是256
				printf("%s:%d\n", ip_trans, port_trans);

				char msg[] = "200 PORT command successful.\r\n";
				write_tcp(connfd, msg, strlen(msg));

				create_addr(ip_trans, port_trans, &addr_client_trans);
				is_addr_specified = 1;

				// 终止PORT循环，继续主循环
				continue_main_loop = 0;
				break;
			}
			// TODO 是否需要指定ip
			else if (strcmp(client_cmd, "PASV") == 0){
				char msg[] = "227 Entering Passive Mode (127,0,0,1,5,77)\r\n"; // 默认端口1357对应5,77
				write_tcp(connfd, msg, strlen(msg));

				create_addr(ip_trans, port_trans, &addr_client_trans);
				is_addr_specified = 1;

				continue_main_loop = 0;
				break;
			}

			// 重新读取指令
			
			close(connfd);
			if ((connfd = accept(listenfd, (struct sockaddr *)&client_addr, &client_addr_len)) == -1) {
				printf("Error accept(): %s(%d)\n", strerror(errno), errno);
				continue;
			}

			read_tcp(connfd, recv_buff);
			res = sscanf(recv_buff, "%s", client_cmd);
		}
		if (continue_main_loop){
			close(connfd);
			continue;
		}

		// TODO 自定义文件路径

		// 传输文件
		if (strcmp(client_cmd, "RETR") == 0) {

			// 返回代码50
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
			// struct sockaddr_in addr_client_trans;
			// create_addr("127.0.0.1", 1357, &addr_client_trans);

			// TODO 将sleep改为触发机制
			sleep(3);

			// 连接
			if (connect(sockfd_trans, (struct sockaddr*)&addr_client_trans, sizeof(addr_client_trans)) < 0){
				printf("Error connect(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// 通过新socket发送信息
			// 获取文件完整路径
			char file_name[100];
			res = sscanf(recv_buff, "RETR %s", file_name);
			char file_path[2000];
			sprintf(file_path, "%s/%s", root, file_name);
			FILE *file = fopen(file_path, "rb");
			if (file == NULL){
				perror("File open failed in server");
				continue;
			}
			char file_buff[1000];
			size_t bytes_read;
			while ((bytes_read = fread(file_buff, 1, sizeof(file_buff), file)) > 0) {
				if (write_tcp(sockfd_trans, file_buff, bytes_read) == 0){
					break;
				}
			}

			fclose(file);
			close(sockfd_trans);
			close(connfd);
			continue;
		}

		close(connfd);
	}

	close(listenfd);
}