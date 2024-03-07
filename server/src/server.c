#include "utils.h"

int main(int argc, char **argv) {
	const int cap_buff_recv = 8192;
	const int cap_buff_send = 8192; 
	struct sockaddr_in addr_listen_cmd;     
	char buff_send[cap_buff_send];
	char buff_recv[cap_buff_recv];
	char cmd[100];
	char root[1000] = "/tmp";
	char wd[1000] = "/";
	char actual_pwd[sizeof(root) + sizeof(wd) + 1000];
	char ip_server_pasv[100] = "127.0.0.1";
	int sock_listen, sock_cmd;
	int res;
	int port_listen_cmd = 21;
	int is_pasv_local = 1;

	printf("Server started\n");

	srand(time(NULL));
	
	// 参数解析
	// 默认地址为127.0.0.1:21，默认文件根目录为/tmp
	for (int i = 1; i < argc; i ++){
		// pasv模式下server发送的ip
		if (strcmp(argv[i], "-ip") == 0 && i + 1 < argc){
			strcpy(ip_server_pasv, argv[i + 1]);
		}
		// 监听端口
		else if (strcmp(argv[i], "-port") == 0 && i + 1 < argc){
			port_listen_cmd = atoi(argv[i + 1]);
		}
		// 文件根目录
		else if (strcmp(argv[i], "-root") == 0 && i + 1 < argc){
			strcpy(root, argv[i + 1]);
		}
		// 本地调试
		else if (strcmp(argv[i], "-local") == 0 && i + 1 < argc){
			if (strcmp(argv[i + 1], "l") == 0){
				is_pasv_local = 1;
			}
			else if (strcmp(argv[i + 1], "r") == 0){
				is_pasv_local = 0;
			}
		}
	}

	// 初始化
	sprintf(actual_pwd, "%s%s", root, wd);

	create_addr("0.0.0.0", port_listen_cmd, &addr_listen_cmd);

	sock_listen = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sock_listen == -1) ERR_RETURN;
	res = bind(sock_listen, (struct sockaddr*)&addr_listen_cmd, sizeof(addr_listen_cmd));
	if (res == -1) ERR_RETURN;
	res = listen(sock_listen, 10);
	if (res == -1) ERR_RETURN;

	// sock_listen 监听循环
	while (1) {
		struct sockaddr_in addr_server_pasv;
		struct sockaddr_in addr_client_port;
		struct sockaddr_in addr_client;
		char old_name[100];
		int port_server_pasv;
		int is_logined = 0;
		int is_old_spec = 0;
		int sock_listen_pasv;
		enum Trans_mode trans_mode = UNDEFINED_MODE;
		socklen_t len = sizeof(addr_client);

		// 用于通信的socket 等待client的连接 阻塞
		sock_cmd = accept(sock_listen, (struct sockaddr*)&addr_client, &len);
		if (sock_cmd == -1) ERR_CONTINUE;

		int pid = fork();
		if (pid < 0) {
			exit(1);
		}
		if (pid > 0){
			close(sock_cmd);
			continue;
		}

		// 连接成功
		close(sock_listen);
		res = tcp_write_str(sock_cmd, "220 FTP server ready.\r\n");
		if (res == -1) ERR_CONTINUE;
		
		// sock_cmd 通信循环
		while (1) {
			res = tcp_read_line(sock_cmd, buff_recv);
			if (res == 0) ERR_BREAK;
			printf("From client: %s\n", buff_recv);

			// 处理命令
			res = sscanf(buff_recv, "%s", cmd);
			if (res == 0 || res == EOF) ERR_BREAK;
			if (strcmp(cmd, "QUIT") == 0 ||
				strcmp(cmd, "ABOR") == 0) {
				tcp_write_str(sock_cmd, "221 Goodbye\r\n");
				break;
			}
			if (strcmp(cmd, "USER") == 0) {
				char user_name[100];

				sscanf(buff_recv, "USER %s", user_name);
				if (strcmp(user_name, "anonymous") != 0) {
					tcp_write_str(sock_cmd, "530 Anonymous user only\r\n");
					continue;
				}

				tcp_write_str(sock_cmd, "331 Guest login ok, send your complete e-mail address as password.\r\n");

				// PASS
				res = tcp_read_line(sock_cmd, buff_recv);
				if (res == 0) ERR_BREAK;
				printf("From client: %s\n", buff_recv);

				sscanf(buff_recv, "%s", cmd);
				if (strcmp(cmd, "PASS") != 0) {
					tcp_write_str(sock_cmd, "530 Login required\r\n");
					continue;
				}
				char password[100];
				res = sscanf(buff_recv, "PASS %s", password);
				if (res <= 0){
					tcp_write_str(sock_cmd, "530 Login required\r\n");
					continue;
				}

				is_logined = 1;

				// welcome
				tcp_write_str(sock_cmd, "230 Guest login ok, access restrictions apply.\r\n");

				continue;
			}
			if (!is_logined) {
				tcp_write_str(sock_cmd, "530 Login required\r\n");
				continue;
			}

			if (strcmp(cmd, "SYST") == 0){
				tcp_write_str(sock_cmd, "215 UNIX Type: L8\r\n");
				continue;
			}
			else if (strcmp(cmd, "TYPE") == 0){
				char type[100];

				sscanf(buff_recv, "TYPE %s", type);
				if (
					strcmp(type, "I") != 0 &&
					strcmp(type, "A") != 0) {
					tcp_write_str(sock_cmd, "504 Error type\r\n");
					continue;
				}
				tcp_write_str(sock_cmd, "200 Type set to I.\r\n");
				continue;
			}
			// PASV server监听端口并发送，client主动连接
			else if (strcmp(cmd, "PASV") == 0){
				trans_mode = PASV_MODE;

				// 获取自身ip
				if (is_pasv_local){
					strcpy(ip_server_pasv, "127.0.0.1");
				}
				// 获取公网ip
				else {
					FILE *fp;
					
					fp = popen("ip a | grep 'inet ' | grep -v '127.0.0.1' | awk '{print $2}' | cut -d/ -f1", "r");
					if (fp == NULL) ERR_CONTINUE;

					fscanf(fp, "%s", ip_server_pasv);
				}

				// 随机生成端口号
				port_server_pasv = rand() % (60000 - 20000 + 1) + 20000;
				printf("%d\n", port_server_pasv);

				// 发送地址给client
				int h1,h2,h3,h4,p1,p2;
				sscanf(ip_server_pasv, "%d.%d.%d.%d", &h1, &h2, &h3, &h4);
				p1 = port_server_pasv / 256;
				p2 = port_server_pasv % 256;
				sprintf(buff_send, "227 Entering Passive Mode (%d,%d,%d,%d,%d,%d)\r\n", h1, h2, h3, h4, p1, p2);

				// PASV模式监听文件socket
				sock_listen_pasv = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
				if (sock_listen_pasv == -1) ERR_CONTINUE;
				create_addr(ip_server_pasv, port_server_pasv, &addr_server_pasv);
				res = bind(sock_listen_pasv, (struct sockaddr*)&addr_server_pasv, sizeof(addr_server_pasv));
				if (res == -1) ERR_BREAK;
				res = listen(sock_listen_pasv, 10);
				if (res == -1) ERR_BREAK;

				tcp_write_str(sock_cmd, buff_send);
				continue;
			}
			// PORT client监听端口并发送，server主动连接
			else if (strcmp(cmd, "PORT") == 0){
				char ip_client_port[100];
				int h1, h2, h3, h4, p1, p2;
				int port_client_port;

				trans_mode = PORT_MODE;

				sscanf(buff_recv, "PORT %d,%d,%d,%d,%d,%d", &h1, &h2, &h3, &h4, &p1, &p2);
				sprintf(ip_client_port, "%d.%d.%d.%d", h1, h2, h3, h4);
				port_client_port = p1 * 256 + p2;
				create_addr(ip_client_port, port_client_port, &addr_client_port);

				tcp_write_str(sock_cmd, "200 PORT command successful.\r\n");
				continue;
			}
			else if (
				strcmp(cmd, "RETR") == 0 ||
				strcmp(cmd, "STOR") == 0 ||
				strcmp(cmd, "LIST") == 0){
				char file_name[100];
				char buff_file[10000];
				char file_full_path[sizeof(actual_pwd) + 1 + sizeof(file_name)];
				size_t bytes_read = 0;
				int sock_trans;
				FILE *fp;
				enum Trans_cmd trans_cmd = UNDEFINED_TRANS_CMD;

				if (trans_mode == UNDEFINED_MODE) {
					tcp_write_str(sock_cmd, "425 Mode not specified\r\n");
					continue;
				}

				if (strcmp(cmd, "RETR") == 0) trans_cmd = RETR;
				else if (strcmp(cmd, "STOR") == 0) trans_cmd = STOR;
				else if (strcmp(cmd, "LIST") == 0) trans_cmd = LIST;

				// 获取文件名
				if (trans_cmd == LIST){
					// sprintf(file_full_path, "%s", actual_pwd);
					char list_cmd[sizeof(actual_pwd) + small_len];
					sprintf(list_cmd, "ls -l %s", actual_pwd);
					fp = popen(list_cmd, "r");
				}
				else if (trans_cmd == RETR){
					sscanf(buff_recv, "RETR %s", file_name);
					sprintf(file_full_path, "%s%s", actual_pwd, file_name);
					fp = fopen(file_full_path, "rb");
				}
				else if (trans_cmd == STOR){
					sscanf(buff_recv, "STOR %s", file_name);
					sprintf(file_full_path, "%s%s", actual_pwd, file_name);
					fp = fopen(file_full_path, "wb");
				}
				if (fp == NULL) {
					tcp_write_str(sock_cmd, "550 File not found\r\n");
					continue;
				}

				// 传输socket
				if (trans_mode == PASV_MODE) {
					sock_trans = accept(sock_listen_pasv, NULL, NULL);
				}
				else if (trans_mode == PORT_MODE) {
					sock_trans = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
					if (sock_trans == -1) ERR_CONTINUE;
					res = connect(sock_trans, (struct sockaddr*)&addr_client_port, sizeof(addr_client_port));
					if (res == -1) ERR_CONTINUE;
				}
				if (sock_trans == -1) {
					tcp_write_str(sock_cmd, "425 Cannot open data connection\r\n");
					continue;
				}

				// 发送150
				sprintf(buff_send, "150 Opening BINARY mode data connection for %s\r\n", file_name);
				tcp_write_str(sock_cmd, buff_send);
				
				// 传输文件
				if (trans_cmd == RETR){
					while((bytes_read = fread(buff_file, 1, sizeof(buff_file), fp)) > 0) {
						if (write(sock_trans, buff_file, bytes_read) <= 0) break;
					}
				}
				else if (trans_cmd == STOR){
					while ((bytes_read = read(sock_trans, buff_file, sizeof(buff_file))) > 0){
						if (fwrite(buff_file, 1, bytes_read, fp) <= 0) break;
					}
				}
				else if (trans_cmd == LIST){
					while((bytes_read = fread(buff_file, 1, sizeof(buff_file), fp)) > 0){
						if (write(sock_trans, buff_file, bytes_read) <= 0) break;
					}
				}

				// 传输完成回复
				tcp_write_str(sock_cmd, "226 Transfer complete.\r\n");

				// 关闭传输连接，还原传输模式
				if (trans_mode == PASV_MODE)
					close(sock_listen_pasv);
				fclose(fp);
				close(sock_trans);
				trans_mode = UNDEFINED_MODE;

				continue;
			}
			else if (strcmp(cmd, "PWD") == 0){
				sprintf(buff_send, "257 \"%s\" \r\n", wd);
				tcp_write_str(sock_cmd, buff_send);

				continue;
			}
			else if (strcmp(cmd, "CWD") == 0){
				char new_path[1000];

				res = sscanf(buff_recv, "CWD %s", new_path);
				if (res <= 0){
					tcp_write_str(sock_cmd, "500 Syntax error, command unrecognized\r\n");
					continue;
				}

				strcpy(wd, new_path);
				sprintf(actual_pwd, "%s%s", root, wd);

				sprintf(buff_send, "250 Directory successfully changed to \"%s\"\r\n", wd);
				tcp_write_str(sock_cmd, buff_send);

				continue;
			}
			else if (strcmp(cmd, "MKD") == 0){
				char new_path[1000];
				char new_path_full[sizeof(actual_pwd) + 1 + sizeof(new_path)];

				res = sscanf(buff_recv, "MKD %s", new_path);
				if (res <= 0){
					tcp_write_str(sock_cmd, "500 Syntax error, command unrecognized\r\n");
					continue;
				}
				sprintf(new_path_full, "%s/%s", actual_pwd, new_path);
				
				res = mkdir(new_path_full, 0777);
				if (res == -1){
					tcp_write_str(sock_cmd, "550 Create directory operation failed\r\n");
					continue;
				}

				tcp_write_str(sock_cmd, "257 Directory created\r\n");

				continue;
			}
			else if (strcmp(cmd, "RMD") == 0){
				char dir[1000];
				char dir_full[sizeof(actual_pwd) + 1 + sizeof(dir)];

				res = sscanf(buff_recv, "RMD %s", dir);
				if (res <= 0){
					tcp_write_str(sock_cmd, "500 Syntax error, command unrecognized\r\n");
					continue;
				}
				sprintf(dir_full, "%s/%s", actual_pwd, dir);
				
				rmdir(dir_full);
				tcp_write_str(sock_cmd, "250 Directory removed\r\n");

				continue;
			}
			else if (strcmp(cmd, "RNFR") == 0){
				is_old_spec = 1;

				res = sscanf(buff_recv, "RNFR %s", old_name);
				if (res <= 0){
					tcp_write_str(sock_cmd, "500 Syntax error, command unrecognized\r\n");
					continue;
				}

				tcp_write_str(sock_cmd, "350 Ready for destination name.\r\n");

				continue;
			}
			else if (strcmp(cmd, "RNTO") == 0){
				char old_name_full[sizeof(actual_pwd) + 1 + sizeof(old_name)];
				char new_name[1000];
				char new_name_full[sizeof(actual_pwd) + 1 + sizeof(new_name)];

				if (!is_old_spec){
					tcp_write_str(sock_cmd, "503 Bad sequence of commands\r\n");
					continue;
				}

				sscanf(buff_recv, "RNTO %s", new_name);

				sprintf(old_name_full, "%s/%s", actual_pwd, old_name);
				sprintf(new_name_full, "%s/%s", actual_pwd, new_name);

				rename(old_name_full, new_name_full);

				continue;
			}
			tcp_write_str(sock_cmd, "500 Syntax error, command unrecognized\r\n");
		}

		close(sock_cmd);
		break;
	}

	// close(sock_listen);

	exit(0);
}

