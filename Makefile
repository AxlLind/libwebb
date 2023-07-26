.PHONY: run clean
.DEFAULT_GOAL := out/webc
.EXTRA_PREREQS := $(MAKEFILE_LIST)

SOURCES := $(filter-out src/main.c,$(wildcard src/*.c))
TESTS   := $(wildcard tests/test_*.c)

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow

$(SOURCES:src/%.c=out/%.o): out/%.o: src/%.c src/%.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TESTS:tests/test_%.c=out/test_%): out/test_%: tests/test_%.c $(SOURCES:src/%.c=out/%.o)
	$(CC) $(CFLAGS) $^ -Isrc -o $@

$(TESTS:tests/test_%.c=run-test-%): run-test-%: out/test_%
	./$<

out/webc: src/main.c $(SOURCES:src/%.c=out/%.o)
	$(CC) $(CFLAGS) $^ -o $@

run: out/webc
	./$< .

run-tests: $(TESTS:tests/test_%.c=run-test-%)

clang-format:
	clang-format -i -style=file $$(find . -name '*.c' -or -name '*.h')

clean:
	rm -rf out

#@ Print this help text
help:
	@echo 'usage: make [TARGET..]'
	@echo 'Makefile used to build and lint WebC'
	@echo
	@echo 'TARGET:'
	@awk ' \
		/(^| )[a-z-%]+:/ { \
      if (desc !~ /^#@ /) next;	\
			for (i=1; i <= NF; i++) \
			  if ($$i ~ /:$$/) target = $$i; \
			printf "  %s%s\n", target, substr(desc, 4, 100) \
	  } \
	  { desc = $$0 } \
	' $(MAKEFILE_LIST) | column -t -s ':'
