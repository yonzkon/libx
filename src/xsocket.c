#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <sys/time.h>
#include "xsocket.h"
#include "showmsg.h"

// basic
static int nfds;
static fd_set fds;
static LIST_HEAD(xsocket_head);

// thread control
static int runflag = XSOCKET_ST_RUN;
#ifdef __linux
static pid_t tid = 0;
#endif

// statistics
static int nr_xsocket_current;
static int nr_xsocket_total;
static int nr_xsocket_closed;

void xsocket_init(struct xsocket *sock)
{
	// ** init static fileds
	if (list_empty(&xsocket_head)) {
		nfds = 0;
		FD_ZERO(&fds);
	}

	sock->eof = 1;
	sock->fd = -1;
	memset(&sock->laddr, 0, sizeof(sock->laddr));
	memset(&sock->raddr, 0, sizeof(sock->raddr));
	sock->type = XSOCKET_CLIENT;
	sock->tick = time(NULL);
	sock->stall = STALL_INTERVAL;

	INIT_LIST_HEAD(&sock->node);

	sock->rbuf = NULL;
	sock->wbuf = NULL;
	sock->sdata = NULL;

	sock->private_data = NULL;
	sock->ops.idle = NULL;
	sock->ops.close = NULL;
	sock->ops.packet = NULL;
	sock->ops.connected = NULL;
	sock->ops.listening = NULL;
	sock->ops.connection = NULL;

	nr_xsocket_current++;
	nr_xsocket_total++;
}

void xsocket_close(struct xsocket *sock)
{
	if (sock->type == XSOCKET_CLIENT)
		show_debug("Socket (C #%d) closed %s:%d\n",
		           sock->fd, inet_ntoa(sock->raddr.sin_addr),
		           ntohs(sock->raddr.sin_port));
	else if (sock->type == XSOCKET_SERVER)
		show_debug("Socket (S #%d) closed %s:%d\n",
		           sock->fd, inet_ntoa(sock->laddr.sin_addr),
		           ntohs(sock->laddr.sin_port));
	else if (sock->type == XSOCKET_CONNECTION)
		show_debug("Socket (A #%d) closed %s:%d\n",
		           sock->fd, inet_ntoa(sock->raddr.sin_addr),
		           ntohs(sock->raddr.sin_port));

#ifdef __linux
	if (tid && tid != syscall(SYS_gettid)) {
		sock->eof = 1;
		return;
	}
#endif

	/*
	 * we use eof and xsocket_head to manage xsocket
	 * 1. eof => close => list_del
	 * 2. close => eof => list_del
	 */
	sock->eof = 1;
	list_del(&sock->node);
	if (sock->type == XSOCKET_SERVER) {
		struct xsocket *pos;
		list_for_each_entry(pos, &xsocket_head, node) {
			if (pos->laddr.sin_port == sock->laddr.sin_port)
				xsocket_close(pos);
		}
	}

	shutdown(sock->fd, SHUT_RDWR);
	close(sock->fd);
	FD_CLR(sock->fd, &fds);
	sock->fd = -1;

	if (sock->rbuf)
		quebuf_delete(sock->rbuf);
	sock->rbuf = NULL;
	if (sock->wbuf)
		quebuf_delete(sock->wbuf);
	sock->wbuf = NULL;
	if (sock->sdata)
		xsocket_sdata_free(sock);
	sock->sdata = NULL;

	sock->private_data = NULL;
	sock->ops.idle = NULL;
	sock->ops.close = NULL;
	sock->ops.packet = NULL;
	sock->ops.connected = NULL;
	sock->ops.listening = NULL;
	sock->ops.connection = NULL;

	nr_xsocket_current--;
	nr_xsocket_closed++;
}

int xsocket_connect(struct xsocket *client, uint16_t port, uint32_t host,
                    void (*connected)(struct xsocket *))
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) return -1;

	struct sockaddr_in raddr;
	raddr.sin_family = AF_INET;
	raddr.sin_port = port;
	raddr.sin_addr.s_addr = host;
	if (connect(fd, (struct sockaddr*)&raddr, sizeof(raddr)) == -1) {
		close(fd);
		return -1;
	}
	show_debug("Socket (C #%d) connected to %s:%d\n", fd,
	           inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port));

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in laddr;
	socklen_t lsocklen = sizeof(laddr);
	getsockname(fd, (struct sockaddr*)&laddr, &lsocklen);

	if (nfds < fd + 1)
		nfds = fd + 1;
	FD_SET(fd, &fds);
	client->fd = fd;
	client->raddr = raddr;
	client->laddr = laddr;
	client->type = XSOCKET_CLIENT;
	client->tick = time(NULL); // update tick here, supporting reconnect
	client->eof = 0;
	client->ops.connected = connected;
	list_add(&client->node, &xsocket_head);

	if (connected)
		connected(client);
	return 0;
}

int xsocket_reconnect(struct xsocket *client, void (*connected)(struct xsocket *))
{
	return xsocket_connect(client, client->raddr.sin_port,
	                       client->raddr.sin_addr.s_addr,
	                       connected);
}

int xsocket_listen(struct xsocket *server, uint16_t port, uint32_t host,
                   void (*listening)(struct xsocket *))
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1) return -1;

	struct sockaddr_in laddr;
	laddr.sin_family = AF_INET;
	laddr.sin_port = port;
	laddr.sin_addr.s_addr = host;

	if (bind(fd, (struct sockaddr*)&laddr, sizeof(laddr)) == -1) {
		close(fd);
		return -1;
	}
	listen(fd, 100);
	show_debug("Socket (L #%d) listened at %s:%d\n", fd,
	           inet_ntoa(laddr.sin_addr), ntohs(laddr.sin_port));

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	if (nfds < fd + 1)
		nfds = fd + 1;
	FD_SET(fd, &fds);
	server->fd = fd;
	server->laddr = laddr;
	server->type = XSOCKET_SERVER;
	server->stall = LONG_MAX;
	server->eof = 0;
	server->ops.listening = listening;
	list_add(&server->node, &xsocket_head);

	if (listening)
		listening(server);
	return 0;
}

static struct xsocket *xsocket_accept(struct xsocket *server)
{
	struct xsocket *conn = calloc(1, sizeof(struct xsocket));
	if (conn == NULL) return NULL;
	xsocket_init(conn);

	struct sockaddr_in raddr;
	socklen_t rsocklen = sizeof(raddr);
	int fd = accept(server->fd, (struct sockaddr*)&raddr, &rsocklen);
	if (fd == -1) goto errout;
	show_debug("Socket (A #%d) accepted from %s:%d\n", fd,
	           inet_ntoa(raddr.sin_addr), ntohs(raddr.sin_port));

	int opt = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

	struct sockaddr_in laddr;
	socklen_t lsocklen = sizeof(laddr);
	getsockname(fd, (struct sockaddr*)&laddr, &lsocklen);

	if (nfds < fd + 1)
		nfds = fd + 1;
	FD_SET(fd, &fds);
	conn->fd = fd;
	conn->raddr = raddr;
	conn->laddr = laddr;
	conn->type = XSOCKET_CONNECTION;
	conn->eof = 0;
	list_add(&conn->node, &xsocket_head);
	return conn;

errout:
	xsocket_close(conn);
	free(conn);
	return NULL;
}

void xsocket_on_idle(struct xsocket *sock, void (*idle)(struct xsocket *))
{
	sock->ops.idle = idle;
}

void xsocket_on_close(struct xsocket *sock, void (*close)(struct xsocket *))
{
	sock->ops.close = close;
}

void xsocket_on_packet(struct xsocket *sock, int (*packet)(struct xsocket *))
{
	sock->ops.packet = packet;
}

void xsocket_on_connection(struct xsocket *server,
                           void (*connection)(struct xsocket *, struct xsocket *))
{
	server->ops.connection = connection;
}

struct xsocket *xsocket_find_by_fd(int fd)
{
	struct xsocket *pos;
	list_for_each_entry(pos, &xsocket_head, node) {
		if (pos->fd == fd)
			return pos;
	}
	return NULL;
}

struct xsocket *xsocket_check(struct xsocket *sock)
{
	struct xsocket *pos;
	list_for_each_entry(pos, &xsocket_head, node) {
		if (pos == sock)
			return pos;
	}
	return NULL;
}

int xsocket_fd(struct xsocket *sock)
{
	return sock->fd;
}

int xsocket_eof(struct xsocket *sock)
{
	return sock->eof;
}

void* xsocket_sdata(struct xsocket *sock)
{
	return sock->sdata;
}

void* xsocket_sdata_alloc(struct xsocket *sock, size_t size)
{
	if (sock->sdata)
		return NULL;

	sock->sdata = calloc(size, 1);
	return sock->sdata;
}

void xsocket_sdata_free(struct xsocket *sock)
{
	free(sock->sdata);
	sock->sdata = NULL;
}

size_t xsocket_peek(struct xsocket *sock, void *ptr, size_t len)
{
	assert(sock->rbuf);
	return quebuf_peek(sock->rbuf, ptr, len);
}

size_t xsocket_read(struct xsocket *sock, void *ptr, size_t len)
{
	assert(sock->rbuf);
	return quebuf_read(sock->rbuf, ptr, len);
}

size_t xsocket_write(struct xsocket *sock, const void *ptr, size_t len)
{
	// lazy allocate wbuf
	if (!sock->wbuf) {
		sock->wbuf = quebuf_new(WRITE_QUEBUF_SIZE);
		if (!sock->wbuf)
			return 0;
	}

	// we can write safely now
	return quebuf_write(sock->wbuf, ptr, len);
}

size_t xsocket_rbuf_get(struct xsocket *sock, const char **rbuf)
{
	assert(sock->rbuf);
	*rbuf = quebuf_rawbuf_out_pos(sock->rbuf);
	return quebuf_used(sock->rbuf);
}

size_t xsocket_rbuf_head(struct xsocket *sock, size_t len)
{
	assert(sock->rbuf);
	return quebuf_offset_out_head(sock->rbuf, len);
}

size_t xsocket_rbuf_head_v2(struct xsocket *sock, const char **rbuf, size_t len)
{
	xsocket_rbuf_head(sock, len);
	return xsocket_rbuf_get(sock, rbuf);
}

static int xsocket_poll_recv(int timeout)
{
	struct timeval tv = { timeout / 1000, timeout % 1000 * 1000 };
	fd_set readfds;
	memcpy(&readfds, &fds, sizeof(fd_set));

	int nr_readfds_selected = select(nfds, &readfds, NULL, NULL, &tv);
	if (nr_readfds_selected == -1) {
		if (errno == EINTR)
			return 0;
		show_fatal("[select] (%d) %s\n", errno, strerror(errno));
		return -1;
	}

	struct xsocket *pos;
	list_for_each_entry(pos, &xsocket_head, node) {
		if (!FD_ISSET(pos->fd, &readfds))
			continue;

		if (!nr_readfds_selected)
			break;
		// should after FD_ISSET, before other failed conditions
		--nr_readfds_selected;

		if (pos->eof)
			continue;

		pos->tick = time(NULL);

		if (pos->type == XSOCKET_SERVER) {
			struct xsocket *conn = xsocket_accept(pos);
			if (conn && pos->ops.connection)
				pos->ops.connection(pos, conn);
			continue;
		}

		// lazy allocate rbuf
		if (!pos->rbuf) {
			pos->rbuf = quebuf_new(READ_QUEBUF_SIZE);
			if (!pos->rbuf)
				continue;
		}

		// we can recv safely now
		int nrecv = recv(pos->fd, quebuf_rawbuf_in_pos(pos->rbuf),
		                 quebuf_spare(pos->rbuf), 0);
		if (nrecv == -1) {
			pos->eof = 1;
			show_debug("Socket #%d read error.\n", pos->fd);
		} else if (nrecv == 0) {
			pos->eof = 1;
			show_debug("Socket #%d read finished.\n", pos->fd);
		} else {
			quebuf_offset_in_head(pos->rbuf, nrecv);

			if (pos->ops.packet && pos->ops.packet(pos))
				pos->eof = 1;

			quebuf_collect(pos->rbuf, QUEBUF_COLLECT_POLICY_LESS_SPARE);

			if (!quebuf_spare(pos->rbuf) &&
			    quebuf_realloc(pos->rbuf, quebuf_size(pos->rbuf) << 1)) {
				show_warn("Socket #%d don't have enough spare"
				          " of rbuf.\n", pos->fd);
				pos->eof = 1;
			}
		}
	}

	return 0;
}

static int xsocket_poll_send(int timeout)
{
	struct timeval tv = { timeout / 1000, timeout % 1000 * 1000 };
	fd_set writefds;
	memcpy(&writefds, &fds, sizeof(fd_set));

	int nr_writefds_selected = select(nfds, NULL, &writefds, NULL, &tv);
	if (nr_writefds_selected == -1) {
		if (errno == EINTR)
			return 0;
		show_fatal("[select] (%d) %s\n", errno, strerror(errno));
		return -1;
	}

	struct xsocket *pos;
	list_for_each_entry(pos, &xsocket_head, node) {
		if (!FD_ISSET(pos->fd, &writefds))
			continue;

		if (!nr_writefds_selected)
			break;
		// should after FD_ISSET, before other failed conditions
		--nr_writefds_selected;

		if (pos->eof)
			continue;
		if (!pos->wbuf || !quebuf_used(pos->wbuf))
			continue;

		int nsend = send(pos->fd, quebuf_rawbuf_out_pos(pos->wbuf),
		                 quebuf_used(pos->wbuf), 0);
		if (nsend == -1) {
			pos->eof = 1;
		} else {
			quebuf_offset_out_head(pos->wbuf, nsend);
			quebuf_collect(pos->wbuf, QUEBUF_COLLECT_POLICY_LESS_SPARE);

			if (!quebuf_spare(pos->wbuf)) {
				show_warn("Socket #%d don't have enough spare"
				          " of wbuf.\n", pos->fd);
				pos->eof = 1;
			}
		}
	}

	return 0;
}

static void xsocket_poll_close()
{
	struct xsocket *pos, *n;
	list_for_each_entry_safe(pos, n, &xsocket_head, node) {
		// stall of XSOCKET_SERVER is initialed as LONG_MAX,
		// so it can't add nr anymore
		if (time(NULL) - pos->tick > pos->stall) {
			pos->eof = 1;
			show_debug("Socket #%d time out.\n", pos->fd);
		}

		if (pos->eof) {
			if (pos->ops.close)
				pos->ops.close(pos);
			xsocket_close(pos);
			if (pos->type == XSOCKET_CONNECTION)
				free(pos);
		}
	}
}

static void xsocket_poll_idle()
{
	struct xsocket *pos;
	list_for_each_entry(pos, &xsocket_head, node) {
		if (pos->ops.idle)
			pos->ops.idle(pos);
	}
}

int xsocket_loop(int timeout)
{
	int rc;

	if (timeout < 100)
		timeout = 100;

	if ((rc = xsocket_poll_recv(timeout)) == -1)
		return rc;

	if ((rc = xsocket_poll_send(timeout)) == -1)
		return rc;

	xsocket_poll_close();
	xsocket_poll_idle();

	return rc;
}

int xsocket_exec()
{
	int rc = 0;
#ifdef __linux
	tid = syscall(SYS_gettid);
	show_debug("xsocket thread tid = %lu\n", tid);
#endif

	while (runflag == XSOCKET_ST_RUN) {
		if ((rc = xsocket_loop(-1)) != 0)
			break;
	}

	return rc;
}

void xsocket_stop()
{
	runflag = XSOCKET_ST_STOP;
}

const char *xsocket_state()
{
	static char state[1024];

	snprintf(state, sizeof(state),
	         "xsocket_current: %d, xsocket_total: %d, xsocket_closed: %d\n",
	         nr_xsocket_current, nr_xsocket_total, nr_xsocket_closed);
	return state;
}

const char *xsocket_status()
{
	static char status[4096];
	char buffer[256];

	memset(status, 0, sizeof(status));

	struct xsocket *pos;
	list_for_each_entry(pos, &xsocket_head, node) {
		if (strlen(status) > 4000)
			break;
		if (pos->type == XSOCKET_SERVER) {
			snprintf(buffer, sizeof(buffer), "#%d %s (%s)\n", pos->fd,
			         inet_ntoa(pos->laddr.sin_addr), "server");
			strcat(status, buffer);
		} else if (pos->type == XSOCKET_CONNECTION) {
			snprintf(buffer, sizeof(buffer), "#%d %s (%s)\n", pos->fd,
			         inet_ntoa(pos->raddr.sin_addr), "connection");
			strcat(status, buffer);
		} else {
			snprintf(buffer, sizeof(buffer), "#%d %s (%s)\n", pos->fd,
			         inet_ntoa(pos->raddr.sin_addr), "client");
			strcat(status, buffer);
		}
	}
	return status;
}
