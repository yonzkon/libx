#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "xsocket.h"
#include "showmsg.h"

// basic
static struct xsocket *stab[FD_SETSIZE];
static int stab_count;

// arg
static int count_max = 1000;
static char host[32] = "0";
static uint16_t port = 1224;

static int on_packet(struct xsocket *sender)
{
	static char msg[1024];
	int nread = xsocket_read(sender, msg, sizeof(msg));
	msg[nread] = 0;
	show_info("%s", msg);

	return 0;
}

static void create_clients(int count)
{
	for (int i = 0; i < count; i++) {
		struct xsocket *client = malloc(sizeof(struct xsocket));
		if (xsocket_connect(client, htons(port), inet_addr(host), NULL)) {
			xsocket_on_packet(client, on_packet);
			stab[xsocket_fd(client)] = client;
			stab_count++;
		}
	}
}

static void delete_clients(int count)
{
	if (count > stab_count)
		count = stab_count;

	for (int i = 0; count && i < FD_SETSIZE; i++) {
		if (!stab[i]) continue;

		if (rand() % stab_count <= count) {
			xsocket_close(stab[i]);
			stab[i] = NULL;
			stab_count--;
			count--;
		}
	}
}

static void send_packet(int count)
{
	char *msg = "yiendxie\n";

	if (count > stab_count)
		count = stab_count;

	for (int i = 0; count && i < FD_SETSIZE; i++) {
		if (!stab[i]) continue;

		if (rand() % stab_count <= count) {
			xsocket_write(stab[i], msg, strlen(msg));
			count--;
		}
	}
}

static void *attack(void *arg)
{
	srand(time(NULL));

	while (1) {
		sleep(1);
		create_clients((count_max - stab_count)>>1);
		delete_clients(stab_count>>2);
		send_packet(stab_count>>1);
	}

	return NULL;
}

static void test_xsocket()
{
	pthread_t thread;
	pthread_create(&thread, NULL, attack, NULL);

	xsocket_exec();
}

int main(int argc, char *argv[])
{
	if (argc >= 2)
		count_max = atoi(argv[1]);
	if (argc >= 3)
		snprintf(host, sizeof(host), "%s", argv[2]);
	if (argc >= 4)
		port = atoi(argv[3]);

	showmsg_level(MSG_DEBUG);
	test_xsocket();

	return 0;
}
