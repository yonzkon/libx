#include "packet_parser.h"

#define NR_PACKET_PARSER 919
#define pp_hash_fn(tag) (tag % NR_PACKET_PARSER)

static struct packet_parser *pptab[NR_PACKET_PARSER];

struct packet_parser *find_packet_parser(unsigned short tag)
{
	struct packet_parser *p = pptab[pp_hash_fn(tag)];

	for (; p != NULL; p = p->hash_next) {
		if (p->tag == tag)
			return p;
	}

	return NULL;
}

void register_packet_parser(struct packet_parser *parser)
{
	struct packet_parser *p = pptab[pp_hash_fn(parser->tag)];

	if (!p) {
		pptab[pp_hash_fn(parser->tag)] = parser;
	} else {
		while (p->hash_next) {
			if (p == parser || p->tag == parser->tag)
				return;
			p = p->hash_next;
		}
		//parser->hash_next = p->hash_next;
		p->hash_next = parser;
		parser->hash_prev = p;
	}
}

void unregister_packet_parser(struct packet_parser *parser)
{
	struct packet_parser *p = pptab[pp_hash_fn(parser->tag)];

	if (p == parser)
		pptab[pp_hash_fn(parser->tag)] = parser->hash_next;

	if (parser->hash_prev)
		parser->hash_prev->hash_next = parser->hash_next;
	if (parser->hash_next)
		parser->hash_next->hash_prev = parser->hash_prev;
}
