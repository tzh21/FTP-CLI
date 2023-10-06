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

	// server�Ķ˿ں�ip
	// TODO �Ϸ��Լ��
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

	// ͨ����ѭ��
	while (1){
		int sockfd;
		struct sockaddr_in addr;
		struct sockaddr_in addr_client;
		int len;
		int p;

		//����socket
		// ��socket���ں�Ŀ��������������ͨ�š���Ҫ�����socket�����ļ����䡣
		if ((sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
			printf("Error socket(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		//���ñ�����ip��port
		create_addr(ip_client, port_client, &addr_client);
		bind(sockfd, (struct sockaddr*)&addr_client, sizeof(addr_client));

		//����Ŀ��������ip��port
		create_addr(ip, port_server, &addr);

		//������Ŀ����������socket��Ŀ���������ӣ�-- ��������
		// TODO 220
		if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		// ��ȡ��������
		printf("ftp:> ");
		fgets(sentence, 4096, stdin);
		if (strcmp(sentence, "quit\n") == 0) break;
		len = strlen(sentence);
		
		// д��socket
		write_tcp(sockfd, sentence, len);

		// ե��socket���յ�������
		int res = read_tcp(sockfd, sentence);

		char code[100];
		sscanf(sentence, "%s", code);
		// ������Ҫ�����ļ��������µ�socket�������ļ�����
		if (strcmp(code, "50") == 0){
			// �����õ�socket
			int listenfd;
			if ((listenfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1) {
				printf("Error socket(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// TODO ��ͨ�������ö˿�
			int port_client_trans = 1357;
			struct sockaddr_in addr_client;
			create_addr("0.0.0.0", 1357, &addr_client);
			addr_client.sin_addr.s_addr = htonl(INADDR_ANY);	//����"0.0.0.0"
			
			if (bind(listenfd, (struct sockaddr*)&addr_client, sizeof(addr_client)) == -1) {
				printf("Error bind(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			if (listen(listenfd, 10) == -1) {
				printf("Error listen(): %s(%d)\n", strerror(errno), errno);
				return 1;
			}

			// �����ļ�ʹ�õ�socket
			int socknd_trans;
			socklen_t addr_client_len = sizeof(addr_client);
			if ((socknd_trans = accept(listenfd, (struct sockaddr *)&addr_client, &addr_client_len)) == -1) {
				printf("Error accept(): %s(%d)\n", strerror(errno), errno);
				continue;
			}

			read_tcp(socknd_trans, sentence);

			close(listenfd);
		}

		// ����������������Ϣ
		printf("SERVER: %s", sentence);

		close(sockfd);
	}
	
	return 0;
}