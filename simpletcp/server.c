#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BACKLOG 5
#define BUFFERSIZE 1024
#define MESSAGE "Bye!!"

static int server_socket(const char *service);

int main(int argc, char *argv[])
{
	int sock, wsock;
	ssize_t readlen;
	char buf[BUFFERSIZE];
	int err;
	struct sockaddr clsa;
	socklen_t clsalen;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	/* Check the number of arguments */
	if (argc != 2) {
		fprintf(stderr, "Usage: %s port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Create named socket */
	sock = server_socket(argv[1]);
	if (sock == -1) {
		fprintf(stderr, "server_socket: Failed.\n");
		exit(EXIT_FAILURE);
	}

	/* Wait connection (Create working socket) */
	clsalen = sizeof(clsa);
	wsock = accept(sock, &clsa, &clsalen);
	if (wsock == -1) {
		perror("accept");
		exit(EXIT_FAILURE);
	}

	/* Get the numeric hostname and service name */
	err = getnameinfo(&clsa, clsalen, hbuf, sizeof(hbuf),
			sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	if (err != 0)
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
	else
		printf("hostname: %s, service: %s\n", hbuf, sbuf);

	/* Receive a message */
	readlen = read(wsock, buf, sizeof(buf));
	if (readlen == -1) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	buf[readlen] = '\0';
	printf("Client says: %s\n", buf);

	/* Reply a message */
	if (write(wsock, MESSAGE, strlen(MESSAGE)) == -1) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	/* Shutdown connection */
	shutdown(wsock, SHUT_RDWR);

	/* Close socket */
	close(wsock);
	close(sock);

	return 0;
}

static int server_socket(const char *service)
{
	int sock;
	int err, yes;
	struct addrinfo hints = { 0 };
	struct addrinfo *result, *rp;

	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags    = AI_PASSIVE;
	err = getaddrinfo(NULL, service, &hints, &result);
	if (err != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue;
		yes = 1;
		err = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &yes,
								sizeof(yes));
		if (err == -1)
			goto replay;
		err = bind(sock, rp->ai_addr, rp->ai_addrlen);
		if (err == -1)
			goto replay;
		if (listen(sock, BACKLOG) == 0)
			break;	/* success */
replay:		close(sock);
	}
	freeaddrinfo(result);

	if (rp == NULL) {
		fprintf(stderr, "%s: Could not listen\n", __FUNCTION__);
		return -1;
	}
	return sock;
}
