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

// read�����ķ�װ����ȫ��ȡ����ֹ�жϡ�
int read_tcp(int fd, char *buf);

// write�����ķ�װ����ȫд�룬��ֹ�жϡ�
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
		memset(&addr_client, 0, sizeof(addr_client));
		addr_client.sin_family = AF_INET;
		addr_client.sin_port = port_client;
		if (inet_pton(AF_INET, ip_client, &addr_client.sin_addr) <= 0) {			//ת��ip��ַ:���ʮ����-->������
			printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}
		bind(sockfd, (struct sockaddr*)&addr_client, sizeof(addr_client));

		//����Ŀ��������ip��port
		memset(&addr, 0, sizeof(addr));
		addr.sin_family = AF_INET;
		addr.sin_port = port_server;
		if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {			//ת��ip��ַ:���ʮ����-->������
			printf("Error inet_pton(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		//������Ŀ����������socket��Ŀ���������ӣ�-- ��������
		// TODO 220
		if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			printf("Error connect(): %s(%d)\n", strerror(errno), errno);
			return 1;
		}

		//��ȡ��������
		printf("ftp:> ");
		fgets(sentence, 4096, stdin);
		if (strcmp(sentence, "quit\n") == 0){
			break;
		}
		len = strlen(sentence);
		
		//�Ѽ�������д��socket
		// write_tcp(sockfd, sentence, len);
		// p = 0;
		// while (p < len) {
		// 	int n = write(sockfd, sentence + p, len - p);		//write��������֤���е�����д�꣬������;�˳�
		// 	if (n < 0) {
		// 		printf("Error write(): %s(%d)\n", strerror(errno), errno);
		// 		return 1;
		// 	} else {
		// 		p += n;
		// 	}
		// }

		//ե��socket���յ�������
		int res = read_tcp(sockfd, sentence);

		// p = 0;
		// while (1) {
		// 	int n = read(sockfd, sentence + p, 8191 - p);
		// 	if (n < 0) {
		// 		printf("Error read(): %s(%d)\n", strerror(errno), errno);	//read����֤һ�ζ��꣬������;�˳�
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

		//ע�⣺read�����Ὣ�ַ�������'\0'����Ҫ�ֶ����
		// sentence[p - 1] = '\0';

		// ����������������Ϣ�����ж�Ӧ����
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