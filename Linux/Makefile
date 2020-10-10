
ifeq ($(CC), cc)
CC = gcc
endif

ifndef CFLAGS
CFLAGS = -MMD -O2 -Wall -g -fPIC -pie
endif

ifeq ($(CC), gcc)
CFLAGS += -I/usr/include/glib-2.0
CFLAGS += -I/usr/lib/x86_64-linux-gnu/glib-2.0/include
LDFLAGS += -L./thirdpart/gcc/bluez-5.52/lib/.libs
LDFLAGS += -L./thirdpart/gcc/bluez-5.52/src/.libs
LDFLAGS += -L./libsharekit/gcc
endif

ifeq ($(CC), aarch64-linux-gnu-gcc)
CFLAGS += -I../src/thirdpart/openssl/aarch64/include
CFLAGS += -I./bluez_inc/aarch64/include
CFLAGS += -I./bluez_inc/aarch64/include/glib-2.0
CFLAGS += -I./bluez_inc/aarch64/lib/glib-2.0/include
CFLAGS += -I./thirdpart/aarch64/bluez-5.52/lib
LDFLAGS += -L../src/thirdpart/openssl/aarch64/lib
LDFLAGS += -L./bluez_inc/aarch64/lib/
LDFLAGS += -L./bluez_inc/aarch64/lib64/
LDFLAGS += -L./thirdpart/aarch64/bluez-5.52/lib/.libs/
LDFLAGS += -L./thirdpart/aarch64/bluez-5.52/src/.libs/
LDFLAGS += -L./libsharekit/aarch64
endif

LDFLAGS += -L./
LDFLAGS += -Wl,-rpath,./

CFLAGS += $(EXTRA_CFLAGS)
CFLAGS += -I$(abspath ../src/sharekit/inc)
CFLAGS += -I$(abspath ../src/drivers)
CFLAGS += -I./bluez

ALL = sharekitdtx sharekitdrx

all: $(ALL)

LIBS_W += -lpthread -lshared-mainloop -lbluetooth-internal -lcrypto -lsharekit

# for tx
OBJS_TX = sharekitdtx.o driver_ble_tx.o

# for rx
OBJS_RX = sharekitdrx.o driver_ble_rx.o

E=echo

sharekitdtx: $(OBJS_TX)
	#$(MAKE) -C ./ lib
	@$(CC) -o sharekitdtx $(OBJS_TX) $(LIBS_W) $(LDFLAGS)
	@$(E) "  LD " $@
	
sharekitdrx: $(OBJS_RX)
	@$(CC) -o sharekitdrx $(OBJS_RX) $(LIBS_W) $(LDFLAGS)
	@$(E) "  LD " $@

%.o: %.c
	@$(CC) -c -o $@ $(CFLAGS) $<
	@$(E) "  CC " $<

clean:
	#$(MAKE) -C ./src clean
	find ./bluez -name "*.o"  | xargs rm -f
	find ./bluez -name "*.d"  | xargs rm -f
	rm -f *~ *.o *.d *.jpg sharekitdtx sharekitdrx

-include $(OBJS:%.o=%.d)
