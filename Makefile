##
## Code & Maintenance by Yonzkon <yonzkon@gmail.com>
##

all:
	$(MAKE) -C src $@

tests:
	$(MAKE) -C tests

clean:
	$(MAKE) -C src $@
	$(MAKE) -C tests $@

install:
	$(MAKE) -C src $@

uninstall:
	$(MAKE) -C src $@

.PHONY: all tests clean install uninstall
