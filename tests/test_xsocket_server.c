#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include "xsocket.h"
#include "showmsg.h"

static uint16_t port = 1224;

static int on_packet(struct xsocket *sender)
{
	const char *buffer;
	size_t len;

	len = xsocket_rbuf_get(sender, &buffer);
	if (len < 2)
		return 0;

	if (CONV_W(buffer) == 0x3131) {
		xsocket_rbuf_head(sender, 26);
	} else {
		static char msg[1024];
		int nread = xsocket_read(sender, msg, sizeof(msg));
		msg[nread] = 0;
		show_debug("%s", msg);

		xsocket_write(sender, msg, nread);
	}

	return 0;
}

static void on_connection(struct xsocket *sender, struct xsocket *conn)
{
	xsocket_on_packet(conn, on_packet);
}

static void *print_xsocket_state(void *arg)
{
	while (1) {
		show_status(xsocket_state());
		sleep(2);
	}

	return NULL;
}

static void test_xsocket()
{
	struct xsocket server;
	xsocket_init(&server);
	if (xsocket_listen(&server, htons(port), 0, NULL))
		exit(EXIT_FAILURE);
	xsocket_on_connection(&server, on_connection);

	pthread_t thread;
	pthread_create(&thread, NULL, print_xsocket_state, &server);
	xsocket_exec();
}

int main(int argc, char *argv[])
{
	if (argc >= 2)
		port = atoi(argv[1]);

	test_xsocket();
	return 0;
}
