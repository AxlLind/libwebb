.PHONY: run clean
.DEFAULT_GOAL := out/webc
.EXTRA_PREREQS := $(MAKEFILE_LIST)

SOURCES := $(filter-out src/main.c,$(wildcard src/*.c))

CC     := gcc
CFLAGS := -std=gnu99 -pedantic -O3 -Wall -Wextra -Werror -Wcast-qual -Wcast-align -Wshadow

$(SOURCES:src/%.c=out/%.o): out/%.o: src/%.c src/%.h
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

out/webc: src/main.c $(SOURCES:src/%.c=out/%.o)
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $^ -o $@

run: out/webc
	./$< .

clean:
	rm -rf out
