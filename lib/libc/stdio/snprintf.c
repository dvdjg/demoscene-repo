/*
 * Minimal libc/runtime piece for bare-metal (stdio/snprintf.c).
 *
 * English tutorial supplement: HRM https://archive.org/details/amiga-hardware-reference-manual-3rd-edition
 * RKM https://archive.org/details/amiga-rom-kernel-reference-manual
 * HRM mirror http://amigadev.elowar.com/read/
 */
#include <stdio.h>

struct print_buf {
	char *buf;
	size_t size;
};

static void snprint_func(void *arg, char ch) {
	struct print_buf *pbuf = arg;

	if (pbuf->size < 2) {
    /* Reserve last buffer position for the terminating character: */
		return;
	}
	*(pbuf->buf)++ = ch;
	pbuf->size--;
}

int snprintf(char *buf, size_t size, const char *cfmt, ...) {
	int retval;
	va_list ap;
	struct print_buf arg;

	arg.buf = buf;
	arg.size = size;

	va_start(ap, cfmt);
	retval = kvprintf(&snprint_func, &arg, cfmt, ap);
	va_end(ap);

	if (arg.size >= 1)
		*(arg.buf)++ = 0;
	return retval;
}
