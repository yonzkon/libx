#ifndef __LIBX_PACKET_PARSER_H
#define __LIBX_PACKET_PARSER_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

enum packet_parser_result {
	PACKET_PARSED = 0,
	PACKET_BROKEN,
	PACKET_INCOMPLETE,
};

typedef int (*do_parse_fn)(const char *buffer, size_t buflen,
                           size_t packlen, void *data);

struct packet_parser {
	unsigned short tag;
	size_t len;
	do_parse_fn do_parse;
	struct packet_parser *hash_next;
	struct packet_parser *hash_prev;
};

#define PACKET_PARSER_INIT(tag, len, do_parse) {tag, len, do_parse, NULL, NULL}

struct packet_parser *find_packet_parser(unsigned short tag);
void register_packet_parser(struct packet_parser *parser);
void unregister_packet_parser(struct packet_parser *parser);

#ifdef __cplusplus
}
#endif
#endif
