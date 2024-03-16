# flags
_CFLAGS := -O2 -std=c99 -pipe -I src \
           -Wall -Wextra -Wformat    \
           -Werror=implicit-function-declaration -Werror=int-conversion -Werror=format-security

# files
SRC := src/smap.c src/hash.c src/resize.c src/del.c src/scan.c
OBJ := $(SRC:.c=.o)
LIB := libsmap.a

# targets
.PHONY: lib clean stat test

# compiler
# CC := clang

# library target
lib: $(LIB)

# library
$(LIB): override CFLAGS := $(_CFLAGS) $(CFLAGS)
$(LIB): $(OBJ)
	$(AR) $(ARFLAGS) $@ $?

# library dependencies
$(OBJ): src/impl.h smap.h
src/hash.o: wyhash/wyhash.h wyhash/wyhash32.h

# testing
TBIN := test-smap
TSRC := mite/mite.c mite/mite.h src/test.c smap.h src/impl.h

test: $(TBIN)
	./$(TBIN) -f '_test$$'

$(TBIN): $(TSRC) $(LIB)
	$(CC) $(_CFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(filter-out %.h,$^)

# clean-up
clean:
	$(RM) $(LIB) $(OBJ) $(TBIN)

# statistics
stat:
	@echo SRC = $(SRC)
	@echo LIB = $(LIB)
	@[ -f $(LIB) ] && nm $(LIB) || true
