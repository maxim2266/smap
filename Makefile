# Disable built-in rules and variables
MAKEFLAGS += --no-builtin-rules
MAKEFLAGS += --no-builtin-variables

# flags
CFLAGS_BASE := -pipe -std=c11 -Wall -Wextra -Werror=implicit-function-declaration -Wformat \
		-Werror=format-security -Werror=int-conversion -DSMAP_TEST

SAN_FLAGS := -fsanitize=address -fsanitize=leak -fsanitize=undefined	\
		-fsanitize-address-use-after-scope	\
		-ggdb -fno-omit-frame-pointer

# source files
SRC32 := smap32.c test32.c test.c mite/mite.c
SRC64 := smap64.c test64.c test.c mite/mite.c
HDR := smap.h mite/mite.h

# targets
.PHONY: clean test test32 test64 bench bench32 bench64

test: test32 test64
bench: bench32 bench64

# compiler
CC := gcc
#CC := clang-11

# binaries
test-san-64: $(HDR) $(SRC64)
	$(CC) -m64 $(CFLAGS_BASE) $(SAN_FLAGS) -o $@ $(SRC64)

test-san-32: $(HDR) $(SRC32)
	$(CC) -m32 $(CFLAGS_BASE) $(SAN_FLAGS) -o $@ $(SRC32)

test-opt-64: $(HDR) $(SRC64)
	$(CC) -O2 -m64 $(CFLAGS_BASE) -o $@ $(SRC64)

test-opt-32: $(HDR) $(SRC32)
	$(CC) -O2 -m32 $(CFLAGS_BASE) -o $@ $(SRC32)

# benchmaks
bench32: test-opt-32
	@echo "32 bit hashing:" && ./test-opt-32 -f '_bench$$'

bench64: test-opt-64
	@echo "64 bit hashing:" && ./test-opt-64 -f '_bench$$'

# tests
test32: test-san-32 test-opt-32
	@echo "Test with sanitiser, 32 bits:" && ./test-san-32 -f '_test$$'
	@echo "Test optimised, 32 bits:" && ./test-opt-32 -f '_test$$'

test64: test-san-64 test-opt-64
	@echo "Test with sanitiser, 64 bits:" && ./test-san-64 -f '_test$$'
	@echo "Test optimised, 64 bits:" && ./test-opt-64 -f '_test$$'

# clean-up
clean:
	rm -f test-san-32 test-opt-32 test-san-64 test-opt-64
