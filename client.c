#include "utils.h"

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

	// 通信主循环
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
		fgets(sentence, 4096, stdin);
		if (strcmp(sentence, "quit\n") == 0) break;
		len = strlen(sentence);
		
		// 写入socket
		write_tcp(sockfd, sentence, len);

		// 榨干socket接收到的内容
		int res = read_tcp(sockfd, sentence);

		char code[100];
		sscanf(sentence, "%s", code);
		// 服务器要传输文件。创建新的socket，用于文件传输
		if (strcmp(code, "50") == 0){
			// 监听用的socket
			int listenfd;
			if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// TODO 在通信中设置端口
			int port_client_trans = 1357;
			struct sockaddr_in addr_client;
			create_addr("0.0.0.0", 1357, &addr_client);
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
			int socknd_trans;
			socklen_t addr_client_len = sizeof(addr_client);
			if ((socknd_trans = accept(listenfd, (struct sockaddr *)&addr_client, &addr_client_len)) == -1) {
				printf("Error accept(): %s(%d)\n", strerror(errno), errno);
				continue;
			}

			read_tcp(socknd_trans, sentence);

			close(listenfd);
		}

		// 解析服务器返回信息
		printf("SERVER: %s", sentence);

		close(sockfd);
	}
	
	return 0;
}