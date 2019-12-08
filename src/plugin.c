#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include "plugin.h"
#include "showmsg.h"

typedef int (*plugin_init_fn)();
typedef void (*plugin_exit_fn)();

LIST_HEAD(plugin_head);

struct plugin *load_plugin(const char *filename)
{
	void *handle = dlopen(filename, RTLD_LAZY);
	if (handle == NULL) {
		show_error("(dlopen) %s\n", dlerror());
		return NULL;
	}

	plugin_init_fn func = dlsym(handle, "plugin_init");
	if (!func) {
		dlclose(handle);
		show_error("(dlsym) %s\n", dlerror());
		return NULL;
	}
	if ((*func)()) {
		dlclose(handle);
		show_error("plugin_init of %s failed!\n", filename);
		return NULL;
	}

	struct plugin *p = malloc(sizeof(struct plugin));
	snprintf(p->filename, sizeof(p->filename), "%s", filename);
	p->handle = handle;
	INIT_LIST_HEAD(&p->node);
	list_add(&p->node, &plugin_head);
	return p;
}

void unload_plugin(struct plugin *p)
{
	plugin_exit_fn func = dlsym(p->handle, "plugin_exit");
	if (!func)
		show_error("(dlsym) %s\n", dlerror());
	else
		(*func)();

	if (dlclose(p->handle)) {
		show_fatal("%s\n", dlerror());
		exit(EXIT_FAILURE);
	}

	list_del(&p->node);
	free(p);
}
