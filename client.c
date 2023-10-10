#include "utils.h"

int main(int argc, char **argv) {
	const int send_buff_cap = 2000;
	const int recv_buff_cap = 2000;
	char send_buff[send_buff_cap];
	char recv_buff[recv_buff_cap];
	char code[100];
	char ip[100] = "127.0.0.1";
	char ip_client[100] = "127.0.0.1";
	int port_server = 21;
	int port_client;
	int port_client_trans = 1357; // 传输文件的端口
	
	srand(time(NULL));
	port_client = rand() % (65500 - 20000 + 1) + 20000;

	// 解析命令行参数
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

	// 通信主循环
	while (1){
		struct sockaddr_in addr;
		struct sockaddr_in addr_client;
		int sockfd;
		int res = 0;

		//创建socket
		// 该socket用于和目标主机进行命令通信。需要另外的socket用于文件传输。
		if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
			printf("Error socket(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		//设置本机的ip和port
		create_addr(ip_client, port_client, &addr_client);
		bind(sockfd, (struct sockaddr*)&addr_client, sizeof(addr_client));

		//设置目标主机的ip和port  
		create_addr(ip, port_server, &addr);

		//连接上目标主机（将socket和目标主机连接）-- 阻塞函数
		// TODO 220
		if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		// 获取键盘输入
		printf("ftp:> ");
		fgets(send_buff, 4096, stdin);
		if (strcmp(send_buff, "quit\n") == 0) break;
		
		// 写入socket
		write_tcp(sockfd, send_buff, strlen(send_buff));

		// TODO 若使用PORT指令指定端口，需要解析发出的端口，用于监听
		res = sscanf(send_buff, "%s", code);
		if (strcmp(code, "PORT") == 0){
			int h1,h2,h3,h4,p1,p2;

			res = sscanf(send_buff, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
			port_client_trans = p1 * 256 + p2;
		}

		// 榨干socket接收到的内容
		res = read_tcp(sockfd, recv_buff);

		sscanf(recv_buff, "%s", code);

		// 若收到PASV命令，解析ip和port
		if (strcmp(code, "227") == 0){
			int h1,h2,h3,h4,p1,p2;

			sscanf(recv_buff, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
			port_client_trans = p1 * 256 + p2;
		}
		// 服务器要传输文件。创建新的socket，用于文件传输
		else if (strcmp(code, "50") == 0){
			struct sockaddr_in addr_client;
			socklen_t addr_client_len = sizeof(addr_client);
			FILE *file;
			size_t bytes_recv;
			int listenfd; // 监听文件传输
			int socknd_trans;
			char file_buff[1500]; // 接收文件的缓存比传入文件的缓存大，避免数据丢失

			if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// TODO 在通信中设置端口
			create_addr("0.0.0.0", port_client_trans, &addr_client);
			addr_client.sin_addr.s_addr = htonl(INADDR_ANY);	//监听"0.0.0.0"
			
			if (bind(listenfd, (struct sockaddr*)&addr_client, sizeof(addr_client)) == -1) {
				printf("Error bind(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			if (listen(listenfd, 10) == -1) {
				printf("Error listen(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// 传输文件使用的socket
			if ((socknd_trans = accept(listenfd, (struct sockaddr *)&addr_client, &addr_client_len)) == -1) {
				printf("Error accept(): %s(%d)\n", strerror(errno), errno);
				continue;
			}

			file = fopen("/home/tzh/ftp/downloads/ex.txt", "wb");
			if (file == NULL){
				perror("File open failed in client");
				continue;
			}

			// 读取文件
			while((bytes_recv = recv(socknd_trans, file_buff, sizeof(file_buff), 0)) > 0){
				fwrite(file_buff, 1, bytes_recv, file);
			}

			fclose(file);
			close(socknd_trans);
			close(listenfd);
		}

		// 输出服务器返回信息
		printf("SERVER: %s", recv_buff);

		close(sockfd);
	}
	
	return 0;
}