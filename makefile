all: rm
	g++ server.c -o server && \
	g++ client.c -o client

rm:
	rm -f server && rm -f client