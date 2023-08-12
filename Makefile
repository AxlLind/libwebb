.PHONY: help build run test fmt fmt-check lint clean
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

LIB   := out/libwebb.a
OBJS  := $(patsubst src/%.c,out/obj/%.o,$(wildcard src/*.c))
BINS  := $(patsubst bin/%.c,out/bin/%,$(wildcard bin/*.c))
TESTS := $(patsubst tests/%.c,out/tests/%,$(wildcard tests/*.c))
FILES := $(wildcard src/* tests/* bin/* include/webb/*)

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow

out/obj/%.o: src/%.c src/internal.h include/webb/webb.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

out/tests/%: tests/%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -Isrc -Iinclude -o $@

out/bin/%: bin/%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -Iinclude -o $@

$(LIB): $(OBJS)
	ar -rc $@ $^

#@ Compile everything
build: $(LIB) $(TESTS) $(BINS)

#@ Run the web server
run: out/bin/webb
	./$< .

#@ Run all tests
test: $(TESTS)
	$(subst $() $(), && ,$(^:%=./%))

#@ Format all source files, in place
fmt:
	clang-format -style=file -i $(FILES)

#@ Check if sources files are formatted
fmt-check:
	clang-format -style=file --dry-run -Werror $(FILES)

#@ Lint all source files, using clang-tidy
lint:
	clang-tidy $(FILES) -- -Isrc -Iinclude -std=gnu99

#@ Remove all make artifacts
clean:
	rm -rf out
