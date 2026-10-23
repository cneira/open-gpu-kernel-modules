# Build in a native illumos zone; run on a host whose /dev/fb is NVIDIA.

CC = gcc
CFLAGS = -m64 -std=gnu11 -O2 -Wall -Wextra -Werror

all: vis_user_ioctl_test

vis_user_ioctl_test: vis_user_ioctl_test.c
	$(CC) $(CFLAGS) -o $@ $<

check: vis_user_ioctl_test
	./vis_user_ioctl_test

clean:
	rm -f vis_user_ioctl_test

.PHONY: all check clean
