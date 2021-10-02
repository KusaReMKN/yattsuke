#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUFFERSIZE 4096

static void sig_handler(int sig);
static int server_socket(const char *service);
static void echo_service(int sock);

static jmp_buf env;

int main(int argc, char *argv[])
{
	int sock;
	struct sigaction sa;

	/* Disable buffering */
	setbuf(stdout, NULL);

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

	if (setjmp(env) == 0) {
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;
		sa.sa_handler = sig_handler;
		if (sigaction(SIGINT, &sa, NULL) == -1) {
			perror("sigaction(SIGINT)");
			exit(EXIT_FAILURE);
		}
		if (sigaction(SIGTERM, &sa, NULL) == -1) {
			perror("sigaction(SIGTERM)");
			exit(EXIT_FAILURE);
		}

		while (1)
			echo_service(sock);
	}

	/* Close socket */
	close(sock);

	return 0;
}

void sig_handler(int sig)
{
	printf("Caught signal %d\n", sig);
	longjmp(env, 1);
}

static int server_socket(const char *service)
{
	int sock;
	int err, yes;
	struct addrinfo hints = { 0 };
	struct addrinfo *result, *rp;

	hints.ai_family   = AF_UNSPEC;
	hints.ai_socktype = SOCK_DGRAM;
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
			goto next;
		if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
			break;	/* success */
next:		close(sock);
	}
	freeaddrinfo(result);

	if (rp == NULL) {
		fprintf(stderr, "%s: Could not bind\n", __FUNCTION__);
		return -1;
	}
	return sock;
}

static void echo_service(int sock)
{
	ssize_t readlen;
	char buf[BUFFERSIZE];
	int err;
	struct sockaddr clsa;
	socklen_t clsalen;
	char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

	/* Wait connection */
	clsalen = sizeof(clsa);
	readlen = recvfrom(sock, buf, sizeof(buf), 0, &clsa, &clsalen);
	if (readlen == -1) {
		perror("recvfrom");
		return;
	}
	buf[readlen] = '\0';
	printf("Client says: %s\n", buf);

	/* Get the numeric hostname and service name */
	err = getnameinfo(&clsa, clsalen, hbuf, sizeof(hbuf),
			sbuf, sizeof(sbuf), NI_NUMERICHOST | NI_NUMERICSERV);
	if (err != 0)
		fprintf(stderr, "getnameinfo: %s\n", gai_strerror(err));
	else
		printf("hostname: %s, service: %s\n", hbuf, sbuf);

	/* Reply a message */
	if (sendto(sock, buf, readlen, 0, &clsa, clsalen) == -1) {
		perror("sendto");
		return;
	}

	return;
}
