.PHONY: run clean
.DEFAULT_GOAL := help
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

#@ Run the web server
run: out/webc
	./$< .

#@ Run all tests
run-tests: $(TESTS:tests/test_%.c=run-test-%)

#@ Format all source files, in place
format:
	clang-format -style=file -i $$(find . -name '*.c' -or -name '*.h')

#@ Check if sources files are formatted
check-format:
	clang-format -style=file --dry-run -Werror $$(find . -name '*.c' -or -name '*.h')

#@ Lint all source files, using clang-tidy
lint:
	ls src/*.[ch] | xargs -i clang-tidy {} -- -Isrc -std=gnu99

#@ Remove all make artifacts
clean:
	rm -rf out

#@ Print this help text
help:
	@echo 'usage: make [TARGET..]'
	@echo 'Makefile used to build and lint WebC'
	@echo
	@echo 'TARGET:'
	@awk '                                              \
	  /(^| )[a-z-%]+:/ {                                \
	    if (desc !~ /^#@ /) next;                       \
	    for (i=1; i <= NF; i++)                         \
	      if ($$i ~ /:$$/) target = $$i;                \
	    printf "  %s%s\n", target, substr(desc, 4, 100) \
	  }                                                 \
	  { desc = $$0 }                                    \
	' $(MAKEFILE_LIST) | column -t -s ':'
