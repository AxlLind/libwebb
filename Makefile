.PHONY: help build run run-test-% test fmt fmt-check lint clean
.DEFAULT_GOAL := help
.EXTRA_PREREQS := $(MAKEFILE_LIST)

help:
	@echo 'usage: make [TARGET..]'
	@echo 'Makefile used to build and lint libwebb'
	@echo
	@echo 'TARGET:'
	@awk '                                              \
	  /(^| )[a-z%-]+:/ {                                \
	    if (desc !~ /^#@ /) next;                       \
	    for (i=1; i <= NF; i++)                         \
	      if ($$i ~ /:$$/) target = $$i;                \
	    printf "  %s%s\n", target, substr(desc, 4, 100) \
	  }                                                 \
	  { desc = $$0 }                                    \
	' $(MAKEFILE_LIST) | column -t -s ':'

SOURCES   := $(wildcard src/*.c)
BINARIES  := $(wildcard bin/*.c)
TESTS     := $(wildcard tests/*.c)
ALL_FILES := $(wildcard src/* tests/* bin/* include/webb/*)

LIB := out/libwebb.a

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow

out/obj/%.o: src/%.c src/internal.h include/webb/webb.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

out/test/%: tests/%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -Isrc -Iinclude -o $@

out/bin/%: bin/%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -Iinclude -o $@

$(LIB): $(SOURCES:src/%.c=out/obj/%.o)
	ar -rc $@ $^

$(TESTS:tests/test_%.c=run-test-%): run-test-%: out/test/test_%
	@./$<

#@ Compile everything
build: $(LIB) $(BINARIES:bin/%.c=out/bin/%) $(TESTS:tests/test_%.c=out/test/test_%)

#@ Run the web server
run: out/bin/webb
	./$< .

#@ Run all tests
test: $(TESTS:tests/test_%.c=out/test/test_%) $(TESTS:tests/test_%.c=run-test-%)

#@ Format all source files, in place
fmt:
	clang-format -style=file -i $(ALL_FILES)

#@ Check if sources files are formatted
fmt-check:
	clang-format -style=file --dry-run -Werror $(ALL_FILES)

#@ Lint all source files, using clang-tidy
lint:
	clang-tidy $(ALL_FILES) -- -Isrc -Iinclude -std=gnu99

#@ Remove all make artifacts
clean:
	rm -rf out
