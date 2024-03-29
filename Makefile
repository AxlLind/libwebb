.PHONY: build run test fmt fmt-check lint %-lint clean help
.DEFAULT_GOAL  := help
.EXTRA_PREREQS := $(MAKEFILE_LIST)

SLIB  := out/libwebb.a
DLIB  := out/libwebb.so
OBJS  := $(sort $(patsubst src/%.c,out/obj/%.o,$(wildcard src/*.c)))
BINS  := $(sort $(patsubst bin/%.c,out/bin/%,$(wildcard bin/*.c)))
TESTS := $(sort $(patsubst tests/%.c,out/tests/%,$(wildcard tests/*.c)))
FILES := $(sort $(wildcard src/* tests/* bin/* include/webb/*))

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow -pthread -fPIC

out:
	mkdir -p out/obj out/tests out/bin

out/obj/%.o: src/%.c src/internal.h include/webb/webb.h | out
	$(CC) $(CFLAGS) -Iinclude -c $< -o $@

out/bin/%: bin/%.c $(SLIB)
	$(CC) $(CFLAGS) -Iinclude $^ -o $@

out/tests/%: tests/%.c $(DLIB)
	$(CC) $(CFLAGS) -Iinclude -Isrc $^ -o $@

$(SLIB): $(OBJS)
	$(AR) rc $@ $^

$(DLIB): $(OBJS)
	$(CC) -shared $^ -o $@

%-lint: %
	clang-tidy $* -- -std=gnu99 -Isrc -Iinclude 2>/dev/null

#@ Compile everything
build: $(SLIB) $(DLIB) $(BINS) $(TESTS)

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
lint: $(FILES:=-lint)

#@ Remove all make artifacts
clean:
	rm -rf out

#@ Display help information
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
