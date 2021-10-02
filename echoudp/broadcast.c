#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFERSIZE 4096

static int client_socket(const char *host, const char *service);

int main(int argc, char *argv[])
{
	int sock;
	int i;

	/* Disable buffering */
	setbuf(stdout, NULL);

	/* Check the number of arguments */
	if (argc < 4) {
		fprintf(stderr, "Usage: %s host port msg ...\n", argv[0]);
		exit(EXIT_FAILURE);
	}

	/* Create new socket */
	sock = client_socket(argv[1], argv[2]);
	if (sock == -1) {
		fprintf(stderr, "client_socket: Failed.\n");
		exit(EXIT_FAILURE);
	}

	for (i = 3; i < argc; i++) {
		/* Check length */
		if (strlen(argv[i]) + 1 > BUFFERSIZE) {
			fprintf(stderr, "too long message in argv[%i]\n", i);
			continue;
		}

		/* Send a message */
		if (write(sock, argv[i], strlen(argv[i])) == -1) {
			perror("write");
			exit(EXIT_FAILURE);
		}
	}

	/* Close socket */
	close(sock);

	return 0;
}

static int client_socket(const char *host, const char *service)
{
	int sock;
	int err, yes;
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
		yes = 1;
		err = setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &yes,
								sizeof(yes));
		if (err == -1)
			goto next;
		if (connect(sock, rp->ai_addr, rp->ai_addrlen) == 0)
			break;	/* success */
next:		close(sock);
	}
	freeaddrinfo(result);

	if (rp == NULL) {
		fprintf(stderr, "%s: Could not connect\n", __FUNCTION__);
		return -1;
	}
	return sock;
}
