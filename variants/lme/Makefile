# PSP build configuration for joesnose_nods_free.prx
# Build from the repository root with: make -B release
TARGET = joesnose_nods_free_LME
OBJS = src/main.o src/completion_bridge.o src/zeemote_hid.o src/ds4_basic.o src/sony_controls.o src/zeemote_hooks.o src/zeemote_input.o
PSP_FW_VERSION = 660
BUILD_PRX = 1
USE_KERNEL_LIBC = 1
USE_KERNEL_LIBS = 1
LIBS = -lpsppower_driver -lpspsystemctrl_kernel -lgcc
CFLAGS = -O2 -G0 -std=gnu11 -Wall -Wextra -Werror -fno-pic
PSPSDK = $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build_prx.mak

.PHONY: release
release: all
