.PHONY: clean all doc install

LIB_OBJS := ble_adv.o
SCANNER_OBJS := scanner.o
LYWSD03MMC_DUMPER_OBJS := lywsd03mmc_dumper.o
OBJS := $(SCANNER_OBJS) $(LYWSD03MMC_DUMPER_OBJS) $(LIB_OBJS)
HEADERS := ble_adv.h
BINARIES := scanner lywsd03mmc_dumper
LIB := libble_adv.so
CC := gcc
DOXYGEN := doxygen
CFLAGS := -fPIE -Wall -Werror -Wextra -pedantic -std=c11 -O0 -g -Wcast-align -D_DEFAULT_SOURCE -Iinclude
ifeq (clang,$(CC))
  CFLAGS += -Weverything
  CFLAGS += -Wno-padded
  CFLAGS += -Wno-disabled-macro-expansion
  CFLAGS += -Wno-unused-macros
  CFLAGS += -Wno-unsafe-buffer-usage
  CFLAGS += -Wno-declaration-after-statement
endif
LDFLAGS := -lbluetooth
PREFIX := /usr/local
DESTDIR :=

all: $(BINARIES) $(LIB)

clean:
	rm -f $(BINARIES) $(OBJS) $(LIB)
	rm -rf doc

scanner: $(SCANNER_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) $(LDFLAGS_EXAMPLES)

lywsd03mmc_dumper: $(LYWSD03MMC_DUMPER_OBJS) $(LIB_OBJS)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(LIB): $(LIB_OBJS)
	$(CC) -shared -o $@ $^

doc:
	$(DOXYGEN)

install: $(LIB)
	install -Dm644 -t "$(DESTDIR)$(PREFIX)/include" ble_adv.h
	install -Dm755 -t "$(DESTDIR)$(PREFIX)/lib" libble_adv.so
