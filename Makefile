.PHONY: help build run test fmt fmt-check lint clean
.DEFAULT_GOAL  := help
.EXTRA_PREREQS := $(MAKEFILE_LIST)

LIB   := out/libwebb.a
OBJS  := $(patsubst src/%.c,out/obj/%.o,$(wildcard src/*.c))
BINS  := $(patsubst bin/%.c,out/bin/%,$(wildcard bin/*.c))
TESTS := $(patsubst tests/%.c,out/tests/%,$(wildcard tests/*.c))
FILES := $(wildcard src/* tests/* bin/* include/webb/*)

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow

out/obj/%.o: src/%.c src/internal.h include/webb/webb.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $< -Iinclude -c

out/tests/%: tests/%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ -Iinclude -Isrc

out/bin/%: bin/%.c $(LIB)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $^ -Iinclude

$(LIB): $(OBJS)
	$(AR) rc $@ $^

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
	clang-format -style=file $(FILES) -i

#@ Check if sources files are formatted
fmt-check:
	clang-format -style=file $(FILES) --dry-run -Werror

#@ Lint all source files, using clang-tidy
lint:
	clang-tidy $(FILES) -- -Isrc -Iinclude -std=gnu99

#@ Remove all make artifacts
clean:
	rm -rf out

#@ Print help text
help:
	@echo 'usage: make [TARGET..]'
	@echo 'Makefile used to build and lint libwebb'
	@echo
	@echo 'TARGET:'
	@awk '{                                           \
	  if (desc ~ /^#@ /)                              \
	    printf "  %s%s\n", $$1, substr(desc, 4, 100); \
	  desc = $$0                                      \
	}' $(MAKEFILE_LIST) | column -t -s ':'
