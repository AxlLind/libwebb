.PHONY: help run run-test-% test fmt fmt-check lint clean
.DEFAULT_GOAL := help
.EXTRA_PREREQS := $(MAKEFILE_LIST)

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

SOURCES   := $(filter-out src/main.c,$(wildcard src/*.c))
TESTS     := $(wildcard tests/test_*.c)
ALL_FILES := $(wildcard src/*.c src/*.h tests/*.c)

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow

$(SOURCES:src/%.c=out/%.o): out/%.o: src/%.c src/%.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(TESTS:tests/test_%.c=out/test_%): out/test_%: tests/test_%.c $(SOURCES:src/%.c=out/%.o)
	$(CC) $(CFLAGS) $^ -Isrc -o $@

out/webc: src/main.c $(SOURCES:src/%.c=out/%.o)
	$(CC) $(CFLAGS) $^ -o $@

$(TESTS:tests/test_%.c=run-test-%): run-test-%: out/test_%
	./$<

#@ Run the web server
run: out/webc
	./$< .

#@ Run all tests
test: $(TESTS:tests/test_%.c=run-test-%)

#@ Format all source files, in place
fmt:
	clang-format -style=file -i $(ALL_FILES)

#@ Check if sources files are formatted
fmt-check:
	clang-format -style=file --dry-run -Werror $(ALL_FILES)

#@ Lint all source files, using clang-tidy
lint:
	clang-tidy $(ALL_FILES) -- -Isrc -std=gnu99

#@ Remove all make artifacts
clean:
	rm -rf out
