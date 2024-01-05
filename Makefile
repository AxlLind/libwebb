.PHONY: build run test fmt fmt-check lint clean help
.DEFAULT_GOAL  := help
.EXTRA_PREREQS := $(MAKEFILE_LIST)

LIB   := out/libwebb.a
OBJS  := $(sort $(patsubst src/%.c,out/obj/%.o,$(wildcard src/*.c)))
BINS  := $(sort $(patsubst bin/%.c,out/bin/%,$(wildcard bin/*.c)))
TESTS := $(sort $(patsubst tests/%.c,out/tests/%,$(wildcard tests/*.c)))
FILES := $(sort $(wildcard src/* tests/* bin/* include/webb/*))

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow -pthread

out:
	mkdir -p out/obj out/tests out/bin

out/obj/%.o: src/%.c src/internal.h include/webb/webb.h | out
	$(CC) $(CFLAGS) -Iinclude -o $@ $< -c

out/tests/%: tests/%.c $(LIB)
	$(CC) $(CFLAGS) -Iinclude -o $@ $^ -Isrc

out/bin/%: bin/%.c $(LIB)
	$(CC) $(CFLAGS) -Iinclude -o $@ $^

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
	python3 clang-tidy.py $(FILES) -- -std=gnu99 -Isrc -Iinclude

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
