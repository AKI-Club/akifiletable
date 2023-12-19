# akifiletable makefile
AKIFILETABLE_TARGET := akifiletable
AKIFILETABLE_DEBUG_TARGET := akifiletable_d

AKIFILETABLE_SRC_FILES := frozen.c akifiletable.c
AKIFILETABLE_DEFAULT_OUTPUT_FILES := filetable.bin filetable.idx filetable.h

CFLAGS := -Wall -Wextra -s -O2
CFLAGS_DEBUG := -Wall -Wextra -g -Og -DDEBUG

default: release

all: release debug

release: $(AKIFILETABLE_TARGET)

debug: $(AKIFILETABLE_DEBUG_TARGET)

$(AKIFILETABLE_TARGET): $(AKIFILETABLE_SRC_FILES)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

$(AKIFILETABLE_DEBUG_TARGET): $(AKIFILETABLE_SRC_FILES)
	$(CC) $(CFLAGS_DEBUG) $^ $(LDFLAGS) -o $@

clean:
	rm -f $(AKIFILETABLE_TARGET) $(AKIFILETABLE_DEBUG_TARGET) $(AKIFILETABLE_DEFAULT_OUTPUT_FILES)

.PHONY: all clean default release debug
