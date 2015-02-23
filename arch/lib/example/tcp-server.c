#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

int main(int argc, char *argv[])
{
	int sock;

	sock = socket(PF_INET, SOCK_STREAM, 0);

	struct sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(2000);
	addr.sin_addr.s_addr = 0;

	int result;
	result = bind(sock, (const struct sockaddr *)&addr, sizeof(addr));
	if (result)
		perror("bind");

	result = listen(sock, 1);
	if (result)
		perror("listen");


	int fd = accept(sock, 0, 0);

	uint8_t buf[10240];

	for (uint32_t i = 0; i < 100; i++) {
		ssize_t n;
		n = read(fd, buf, 1024);
		/*      printf ("read:  %d\n", n); */
		if (n <= 0) {
			printf("read: %s %d\n", strerror(errno), errno);
			break;
		}
	}
	printf("did read all buffers\n");

	close(sock);

	return 0;
}
