#ifndef __LIBX_XSOCKET_H
#define __LIBX_XSOCKET_H

#include <stddef.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/syscall.h>
#include "quebuf.h"
#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CONV_B(ptr) (*(uint8_t*)(ptr))
#define CONV_W(ptr) (*(uint16_t*)(ptr))
#define CONV_L(ptr) (*(uint32_t*)(ptr))
#define CONV_Q(ptr) (*(uint64_t*)(ptr))

#define READ_QUEBUF_SIZE 8192
#define WRITE_QUEBUF_SIZE 8192
#define STALL_INTERVAL 60 * 5

enum __xsocket_core_runflag_type {
	XSOCKET_ST_RUN = 0,
	XSOCKET_ST_STOP,
};

typedef enum __xsocket_type {
	XSOCKET_CLIENT = 0,
	XSOCKET_SERVER = 1,
	XSOCKET_CONNECTION = 2,
} xsocket_type_t;

struct xsocket;

struct xsocket_operations {
	void (*idle)(struct xsocket *sock);
	void (*close)(struct xsocket *sock);
	int (*packet)(struct xsocket *sock);
	// client
	void (*connected)(struct xsocket *client);
	// server
	void (*listening)(struct xsocket *server);
	void (*connection)(struct xsocket *server, struct xsocket *conn);
};

struct xsocket {
	int eof;
	int fd;
	struct sockaddr_in laddr; // for listen
	struct sockaddr_in raddr; // for accpet & connect
	xsocket_type_t type;
	time_t tick; // time of last recv(for detecting timeouts),
	             // zero when timeout is disabled
	time_t stall;

	quebuf_t *rbuf;
	quebuf_t *wbuf;
	void *sdata; // stores application-specific data related to the xsocket

	struct list_head node;

	void *private_data;
	struct xsocket_operations ops;
};

void xsocket_init(struct xsocket *sock);
void xsocket_close(struct xsocket *sock);
int xsocket_connect(struct xsocket *client, uint16_t port, uint32_t host,
                    void (*connected)(struct xsocket *));
int xsocket_reconnect(struct xsocket *client,
                      void (*connected)(struct xsocket *));
int xsocket_listen(struct xsocket *server, uint16_t port, uint32_t host,
                   void (*listening)(struct xsocket *));
void xsocket_on_idle(struct xsocket *sock, void (*idle)(struct xsocket *));
void xsocket_on_close(struct xsocket *sock, void (*close)(struct xsocket *));
void xsocket_on_packet(struct xsocket *sock, int (*packet)(struct xsocket *));
void xsocket_on_connection(struct xsocket *server,
                           void (*connection)(struct xsocket *, struct xsocket *));
struct xsocket *xsocket_find_by_fd(int fd);
struct xsocket *xsocket_check(struct xsocket *sock);

int xsocket_fd(struct xsocket *sock);
int xsocket_eof(struct xsocket *sock);
void *xsocket_sdata(struct xsocket *sock);
void *xsocket_sdata_alloc(struct xsocket *sock, size_t size);
void xsocket_sdata_free(struct xsocket *sock);

size_t xsocket_peek(struct xsocket *sock, void *ptr, size_t len);
size_t xsocket_read(struct xsocket *sock, void *ptr, size_t len);
size_t xsocket_write(struct xsocket *sock, const void *ptr, size_t len);

size_t xsocket_rbuf_get(struct xsocket *sock, const char **rbuf);
size_t xsocket_rbuf_head(struct xsocket *sock, size_t len);
size_t xsocket_rbuf_head_v2(struct xsocket *sock, const char **rbuf, size_t len);

int xsocket_loop(int timeout);
int xsocket_exec();
void xsocket_stop();
const char *xsocket_state();
const char *xsocket_status();

#ifdef __cplusplus
}
#endif
#endif
