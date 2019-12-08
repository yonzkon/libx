#ifndef __LIBX_PLUGIN_H
#define __LIBX_PLUGIN_H

#include "list.h"

#ifdef __cplusplus
extern "C" {
#endif

#define LIBX_PLUGIN extern "C"

struct plugin {
	char filename[256];
	void *handle;
	struct list_head node;
};

extern struct list_head plugin_head;

struct plugin *load_plugin(const char *filename);
void unload_plugin(struct plugin *p);

#ifdef __cplusplus
}
#endif
#endif
