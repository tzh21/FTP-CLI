#include "utils.h"

int main(int argc, char **argv) {
	const int cap_buff_recv = 8192;
	const int cap_buff_send = 8192;
	struct sockaddr_in addr_server;
	struct sockaddr_in addr_server_pasv;
	struct sockaddr_in addr_client_port;
	char buff_recv[cap_buff_recv];
	char buff_send[cap_buff_send];
	char cmd[100];
	char code[100];
	char ip_server[100] = "127.0.0.1";
	char ip_server_pasv[100];
	char path_root[2000];
	int sock_cmd;
	int res;
	int port_server = 21;
	int port_server_pasv;
	int sock_client_port;
	int is_port_local = 1; // 在port模式下发送公网ip还是127.0.0.1，可通过-local选项设置
	enum Trans_mode trans_mode = UNDEFINED_MODE;
	enum Trans_cmd trans_cmd = UNDEFINED_TRANS_CMD;

	puts("Client started");

	// 设置默认根目录
	char wd[1000];
	if (getcwd(wd, sizeof(wd)) != NULL){
		getcwd(wd, sizeof(wd));
		sprintf(path_root, "%s/client_root", wd);

		// 不存在则创建
		struct stat st;
		if (stat(path_root, &st) == -1){
			mkdir(path_root, 0777);
		}
	}
	else ERR_RETURN;

	// 参数解析
	// 默认服务器地址为127.0.0.1:21，默认文件根目录为程序所在目录的一个子目录
	for (int i = 1; i < argc; i ++){
		// 服务器ip
		if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc){
			strcpy(ip_server, argv[i + 1]);
		}
		// 服务器端口
		else if (strcmp(argv[i], "-port") == 0 && i + 1 < argc){
			port_server = atoi(argv[i + 1]);
		}
		// 文件根目录
		else if (strcmp(argv[i], "-root") == 0 && i + 1 < argc){
			strcpy(path_root, argv[i + 1]);
		}
		else if (strcmp(argv[i], "-local") == 0 && i + 1 < argc){
			if (strcmp(argv[i + 1], "l") == 0){
				is_port_local = 1;
			}
			if (strcmp(argv[i + 1], "r") == 0){
				is_port_local = 0;
			}
		}
	}

	//连接目标主机 阻塞
	sock_cmd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_cmd == -1) ERR_RETURN;
	res = create_addr(ip_server, port_server, &addr_server);
	if (res == 0) ERR_RETURN;
	res = connect(sock_cmd, (struct sockaddr*)&addr_server, sizeof(addr_server));
	if (res == -1) ERR_RETURN;

	// 成功连接，开始通信
	res = tcp_read_line(sock_cmd, buff_recv);
	if (res == 0) ERR_RETURN;
	printf("%s\n", buff_recv);

	while (1) {
		printf("FTP:> ");
		fgets(buff_send, 4096, stdin);
		add_return(buff_send);
		tcp_write_str(sock_cmd, buff_send);

		sscanf(buff_send, "%s", cmd);

		if (strcmp(cmd, "QUIT") == 0) break;

		if (strcmp(cmd, "PASV") == 0){
			trans_mode = PASV_MODE;
		}
		else if (strcmp(cmd, "PORT") == 0){
			char ip_client_port[100];
			int h1, h2, h3, h4, p1, p2;
			int port_client_port;

			trans_mode = PORT_MODE;

			// 获取自身ip
			if (is_port_local){
				strcpy(ip_client_port, "127.0.0.1");
			}
			// 获取公网ip
			else {
				FILE *fp;
				
				fp = popen("ip a | grep 'inet ' | grep -v '127.0.0.1' | awk '{print $2}' | cut -d/ -f1", "r");
				if (fp == NULL) ERR_CONTINUE;

				fscanf(fp, "%s", ip_client_port);
			}

			sscanf(buff_send, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
			sprintf(ip_client_port, "%d.%d.%d.%d", h1, h2, h3, h4);
			port_client_port = p1 * 256 + p2;
			create_addr(ip_client_port, port_client_port, &addr_client_port);

			sock_client_port = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			if (sock_client_port == -1) ERR_CONTINUE;
			res = bind(sock_client_port, (struct sockaddr*)&addr_client_port, sizeof(addr_client_port));
			if (res == -1) ERR_CONTINUE;
			res = listen(sock_client_port, 10);
			if (res == -1) ERR_CONTINUE;
		}
		else if (
			strcmp(cmd, "RETR") == 0 ||
			strcmp(cmd, "STOR") == 0){
			char file_name[100];
			char buff_file[1000];
			char file_full_path[sizeof(path_root) + 1 + sizeof(file_name)];
			FILE *fp;
			int bytes_read = 0;
			int sock_trans;
			
			if (trans_mode == UNDEFINED_MODE) {
				puts("Error: Mode not specified");
				continue;
			}

			if (strcmp(cmd, "RETR") == 0){
				trans_cmd = RETR;
				sscanf(buff_send, "RETR %s", file_name);
			}
			else if (strcmp(cmd, "STOR") == 0){
				trans_cmd = STOR;
				sscanf(buff_send, "STOR %s", file_name);
			}

			if (trans_mode == PASV_MODE) {
				sock_trans = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (sock_trans == -1) ERR_CONTINUE;
				sleep(0.5);
				res = connect(sock_trans, (struct sockaddr*)&addr_server_pasv, sizeof(addr_server_pasv));
				if (res == -1) ERR_CONTINUE;
			}
			else if (trans_mode == PORT_MODE) {
				sock_trans = accept(sock_client_port, NULL, NULL);
				if (sock_trans == -1) ERR_CONTINUE;
			}

			// 150
			res = tcp_read_line(sock_cmd, buff_recv);
			if (res == 0) ERR_CONTINUE;
			puts(buff_recv);
			sscanf(buff_recv, "%s", code);
			if (strcmp(code, "550") == 0) {
				close(sock_trans);
				continue;
			}

			// 读取文件
			sprintf(file_full_path, "%s/%s", path_root, file_name);
			if (trans_cmd == RETR){
				fp = fopen(file_full_path, "wb");
				while ((bytes_read = read(sock_trans, buff_file, sizeof(buff_file))) > 0) {
					if (fwrite(buff_file, 1, bytes_read, fp) <= 0) break;
				}
			}
			else if (trans_cmd == STOR){
				fp = fopen(file_full_path, "rb");
				while ((bytes_read = fread(buff_file, 1, sizeof(buff_file), fp)) > 0) {
					if (write(sock_trans, buff_file, bytes_read) <= 0) break;
				}
			}

			fclose(fp);
			close(sock_trans);

			// 226
			res = tcp_read_line(sock_cmd, buff_recv);
			if (res == 0) ERR_CONTINUE;
			puts(buff_recv);

			continue;
		}
		// 服务器对LIST不回复代码
		else if (strcmp(cmd, "LIST") == 0){
			int sock_trans;
			int bytes_read = 0;
			char buff_file[1000];
			
			trans_cmd = LIST;

			if (trans_mode == UNDEFINED_MODE) {
				puts("Error: Mode not specified");
				continue;
			}
			
			if (trans_mode == PASV_MODE) {
				sock_trans = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (sock_trans == -1) ERR_CONTINUE;
				sleep(0.5);
				res = connect(sock_trans, (struct sockaddr*)&addr_server_pasv, sizeof(addr_server_pasv));
				if (res == -1) ERR_CONTINUE;
			}
			else if (trans_mode == PORT_MODE) {
				sock_trans = accept(sock_client_port, NULL, NULL);
				if (sock_trans == -1) ERR_CONTINUE;
			}

			// 150
			res = tcp_read_line(sock_cmd, buff_recv);
			if (res == 0) ERR_CONTINUE;
			printf("%s\n", buff_recv);

			while ((bytes_read = read(sock_trans, buff_file, sizeof(buff_file))) > 0) {
				if (fwrite(buff_file, 1, bytes_read, stdout) <= 0) break;
			}
			puts("");

			close(sock_trans);

			// 226
			res = tcp_read_line(sock_cmd, buff_recv);
			if (res == 0) ERR_CONTINUE;
			printf("%s\n", buff_recv);

			continue;
		}

		// 读取服务器回应（单行）
		res = tcp_read_line(sock_cmd, buff_recv);
		if (res == 0) ERR_CONTINUE;
		printf("%s\n", buff_recv);
		sscanf(buff_recv, "%s", code);

		// PASV 接受服务器提供的地址
		if (strcmp(code, "227") == 0) {
			// 地址
			int h1, h2, h3, h4, p1, p2;
			sscanf(buff_recv, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)", &h1, &h2, &h3, &h4, &p1, &p2);
			sprintf(ip_server_pasv, "%d.%d.%d.%d", h1, h2, h3, h4);
			port_server_pasv = p1 * 256 + p2;
			create_addr(ip_server_pasv, port_server_pasv, &addr_server_pasv);
		}
	}
	
	close(sock_cmd);

	return 0;
}
