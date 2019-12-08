//#define NDEBUG 1
#include <assert.h>
#include <string.h>
#include <sys/user.h>
#include "quebuf.h"
#include "showmsg.h"

void print_quebuf_state(quebuf_t *bq, char *header)
{
	show_debug("{size: %lu, garbage: %lu, used: %lu, spare: %lu} (%s)\n",
	           quebuf_size(bq), quebuf_garbage(bq), quebuf_used(bq),
	           quebuf_spare(bq), header);
}

void test_quebuf()
{
	quebuf_t *qbuf;
	char msg[1024];
	size_t nread;
	char hello_msg[] = "hello buffer";
	int write_time = PAGE_SIZE / sizeof(hello_msg) / 3 * 2;
	int read_time = write_time / 2;

	qbuf = quebuf_new(PAGE_SIZE);
	print_quebuf_state(qbuf, "state initial");
	assert(quebuf_size(qbuf) == PAGE_SIZE);
	assert(quebuf_garbage(qbuf) == 0);
	assert(quebuf_used(qbuf) == 0);
	assert(quebuf_spare(qbuf) == PAGE_SIZE);

	for (int i = 0; i < write_time; i++)
		quebuf_write(qbuf, (void*)hello_msg, sizeof(hello_msg));
	print_quebuf_state(qbuf, "state after write");
	assert(quebuf_size(qbuf) == PAGE_SIZE);
	assert(quebuf_garbage(qbuf) == 0);
	assert(quebuf_used(qbuf) == sizeof(hello_msg)*write_time);
	assert(quebuf_spare(qbuf) == PAGE_SIZE - sizeof(hello_msg)*write_time);

	for (int i = 0; i < read_time; i++)
		nread = quebuf_read(qbuf, msg, sizeof(hello_msg));
	print_quebuf_state(qbuf, "state after read");
	assert(quebuf_size(qbuf) == PAGE_SIZE);
	assert(quebuf_garbage(qbuf) == sizeof(hello_msg)*read_time);
	assert(quebuf_used(qbuf) == sizeof(hello_msg)*(write_time-read_time));
	assert(quebuf_spare(qbuf) == PAGE_SIZE - sizeof(hello_msg)*write_time);
	assert(memcmp(hello_msg, msg, strlen(hello_msg)) == 0);
	msg[nread] = 0;
	show_debug("%s (msg read)\n", msg);

	quebuf_collect(qbuf, QUEBUF_COLLECT_POLICY_NONE);
	print_quebuf_state(qbuf, "state after collect");
	assert(quebuf_size(qbuf) == PAGE_SIZE);
	assert(quebuf_garbage(qbuf) == 0);
	assert(quebuf_used(qbuf) == sizeof(hello_msg)*(write_time-read_time));
	assert(quebuf_spare(qbuf) ==
	       PAGE_SIZE - sizeof(hello_msg)*(write_time-read_time));

	quebuf_delete(qbuf);
}

int main()
{
	showmsg_level(MSG_DEBUG);
	test_quebuf();
}
