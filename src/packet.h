#ifndef __LIBX_PACKET_H
#define __LIBX_PACKET_H

#include <stdint.h>

struct packhdr {
	uint16_t cmd;
	char data[0];
};

#define PACK_CMD_LEN (sizeof(struct packhdr))
#define PACK_HDR_LEN PACK_CMD_LEN
#define PACK_LEN(pack_type) (PACK_CMD_LEN + sizeof(pack_type))

#define PACK_HDR(hdrname, ptr) struct packhdr *hdrname = \
		(struct packhdr*)((char*)ptr)
#define PACK_DATA(dataname, ptr, type) type *dataname = \
		((type*)((char*)ptr + PACK_CMD_LEN))

#define PACK_INIT(packname, hdrname, dataname, type) \
	char packname[CMD_LEN + sizeof(type)]; \
	PACK_HDR(hdrname, packname); \
	PACK_DATA(dataname, packname, type);

#define PACK_ALIVE 0x2480 // 0x80, 0x24

struct pack_alive {
	char trash[24];
} __attribute__((packed));

#endif
