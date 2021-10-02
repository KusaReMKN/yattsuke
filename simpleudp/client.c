#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFERSIZE 1024
#define MESSAGE "Hello!!"

static int client_socket(const char *host, const char *service);

int main(int argc, char *argv[])
{
	int sock;
	ssize_t readlen;
	char buf[BUFFERSIZE];

	/* Disable buffering */
	setbuf(stdout, NULL);

	/* Check the number of arguments */
	if (argc != 3) {
		fprintf(stderr, "Usage: %s host port\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Create new socket */
	sock = client_socket(argv[1], argv[2]);
	if (sock == -1) {
		fprintf(stderr, "client_socket: Failed.\n");
		exit(EXIT_FAILURE);
	}

	/* Send a message */
	if (write(sock, MESSAGE, strlen(MESSAGE)) == -1) {
		perror("write");
		exit(EXIT_FAILURE);
	}

	/* Receive a replies */
	printf("Waiting for server to reply ... ");
	readlen = read(sock, buf, sizeof(buf));
	if (readlen == -1) {
		perror("read");
		exit(EXIT_FAILURE);
	}
	buf[readlen] = '\0';
	printf("Server replies: %s\n", buf);

	/* Close socket */
	close(sock);

	return 0;
}

static int client_socket(const char *host, const char *service)
{
	int sock;
	int err;
	struct addrinfo hints = { 0 };
	struct addrinfo *result, *rp;

	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
	err = getaddrinfo(host, service, &hints, &result);
	if (err != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(err));
		return -1;
	}

	for (rp = result; rp != NULL; rp = rp->ai_next) {
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue;
		if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0)
			break;	/* success */
		close(sock);
	}
	freeaddrinfo(result);

	if (rp == NULL) {
		fprintf(stderr, "%s: Could not connect\n", __FUNCTION__);
		return -1;
	}
	return sock;
}
