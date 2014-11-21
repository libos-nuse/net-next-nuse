#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>

int main(int argc, char *argv[])
{
	int sock;

	sock = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2000);
	addr.sin_addr.s_addr = 0;

	if (argc == 1) {
		printf("%s [dest hostname]\n", argv[0]);
		exit(0);
	}

	struct hostent *host = gethostbyname(argv[1]);
	memcpy(&addr.sin_addr.s_addr, host->h_addr_list[0], host->h_length);

	int result;
	result = connect(sock, (const struct sockaddr *)&addr, sizeof(addr));
	if (result)
		perror("connect");

	uint8_t buf[10240];

	memset(buf, 0x66, 20);
	memset(buf + 20, 0x67, 1004);

	for (uint32_t i = 0; i < 1000; i++) {
		ssize_t n;
		n = write(sock, buf, 1024);
		if (n < 0) {
			printf("write: %s %d\n", strerror(errno), errno);
			break;
		}
	}
	printf("did write all buffers\n");

	close(sock);

	return 0;
}
