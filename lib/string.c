#include <types.h>
#include <print.h>
#include <stream.h>

void *memcpy(void *dst, const void *src, size_t n) {
	void *dstaddr = dst;
	void *max = dst + n;

	if (((u_long)src & 3) != ((u_long)dst & 3)) {
		while (dst < max) {
			*(char *)dst++ = *(char *)src++;
		}
		return dstaddr;
	}

	while (((u_long)dst & 3) && dst < max) {
		*(char *)dst++ = *(char *)src++;
	}

	// copy machine words while possible
	while (dst + 4 <= max) {
		*(uint32_t *)dst = *(uint32_t *)src;
		dst += 4;
		src += 4;
	}

	// finish the remaining 0-3 bytes
	while (dst < max) {
		*(char *)dst++ = *(char *)src++;
	}
	return dstaddr;
}

void *memset(void *dst, int c, size_t n) {
	void *dstaddr = dst;
	void *max = dst + n;
	u_char byte = c & 0xff;
	uint32_t word = byte | byte << 8 | byte << 16 | byte << 24;

	while (((u_long)dst & 3) && dst < max) {
		*(u_char *)dst++ = byte;
	}

	// fill machine words while possible
	while (dst + 4 <= max) {
		*(uint32_t *)dst = word;
		dst += 4;
	}

	// finish the remaining 0-3 bytes
	while (dst < max) {
		*(u_char *)dst++ = byte;
	}
	return dstaddr;
}

size_t strlen(const char *s) {
	int n;

	for (n = 0; *s; s++) {
		n++;
	}

	return n;
}

char *strcpy(char *dst, const char *src) {
	char *ret = dst;

	while ((*dst++ = *src++) != 0) {
	}

	return ret;
}

const char *strchr(const char *s, int c) {
	for (; *s; s++) {
		if (*s == c) {
			return s;
		}
	}
	return 0;
}

int strcmp(const char *p, const char *q) {
	while (*p && *p == *q) {
		p++, q++;
	}

	if ((u_int)*p < (u_int)*q) {
		return -1;
	}

	if ((u_int)*p > (u_int)*q) {
		return 1;
	}

	return 0;
}

FILE *fmemopen(FILE *stream, void *buf, const char *mode) {
	if (strcmp(mode, "w") == 0) {
		stream->ptr = buf;
		stream->base = buf;
		stream->end = buf;
		return stream;
	} else if (strcmp(mode, "a") == 0) {
		stream->base = buf;
		char *end = buf + strlen(buf);
		stream->ptr = end;
		stream->end = end;
		return stream;
	} else {
		return NULL;
	}
}

void myoutputk(void *data, const char *buf, size_t len) {
	char *tem = ((FILE *)data)->ptr;
	for (int i = 0; i < len; i++) {
		tem++;
		*tem = buf[i];
	}
	((FILE *)data)->ptr = tem;
}

int fmemprintf(FILE *stream, const char *fmt, ...) {
	char *base = stream->ptr;

	va_list ap;
	va_start(ap, fmt);
	vprintfmt(myoutputk, stream, fmt, ap);
	va_end(ap);

	if (stream->end < stream->ptr) {
		stream->end = stream->ptr;
	}

	return stream->ptr - base;
}

int fseek(FILE *stream, long offset, int fromwhere) {
	char *target;
	if (fromwhere == SEEK_SET) {
		target = stream->base + offset;
	} else if (fromwhere == SEEK_CUR) {
		target = stream->ptr + offset;
	} else if (fromwhere == SEEK_END) {
		target = stream->end + offset;
	} else {
		return 1;
	}

	if (target >= stream->base && target <= stream->end) {
		stream->ptr = target;
		return 0;
	} else {
		return 1;
	}
}

int fclose(FILE *stream) {
	char *end = stream->end;
	*end = '\0';

	return 0;
}
