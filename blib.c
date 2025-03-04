#include <blib.h>

size_t strlen(const char *s) {
    int i = 0;
    while (*(s+i) != '\0') {
	i++;
    }
    return i;
}

char *strcpy(char *dst, const char *src) {
    char *res = dst;
    while ((*dst++ = *src++))
        ;
    return res;
}

char *strncpy(char *dst, const char *src, size_t n) {
    char *res = dst;
    while (*src && n--) {
        *dst++ = *src++;
    }
    *dst = '\0';
    return res;
}

int strcmp(const char *str1, const char *str2) {
    while (*str1 && (*str1 == *str2)) {
        str1++;
        str2++;
    }
    return *(unsigned char *)str1 - *(unsigned char *)str2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    while (n--) {
        if (*s1 != *s2) {
            return *s1 - *s2;
        }
        if (*s1 == 0) {
            break;
        }
        s1++;
        s2++;
    }
    return 0;
}

char *strcat(char *dst, const char *src) {
    char *tem = dst;

    while (*tem) {
    	tem++;
    }

    while ((*tem++ = *src++) != '\0');
	
    return dst;
}

char *strncat(char *dst, const char *src, size_t n){
    char *tem = dst;
    int i = 1;

    while (*tem) {
	tem++;
    }

    while ((i++ <= n) && ((*tem++ = *src++) != '\0'));
    
    *tem = '\0';

    return dst; 
}

char *strchr(const char *str, int character){
    while (*str != '\0') {
        if (*str == (char)character) {
            return (char *)str;
        }
        str++;
    }

    return NULL;
}

char* strsep(char** stringp, const char* delim){
    if (*stringp == NULL) {
	return NULL;
    }

    char *tem = *stringp;
    char *ret = *stringp;
    const char *tem_delim = delim;
    char sc,c;

    for (;*tem;){
	c = *tem++;
	tem_delim = delim;
	sc = *tem_delim;
	while (sc != '\0'){
	   if ( (sc = *tem_delim++) == c ) {
		if (c == '\0'){
			return NULL;
		}
	   } else {
		tem[-1] = '\0';
		*stringp = tem;
		return ret;
	   }
	}
    }

    return NULL;
}


void *memset(void *s, int c, size_t n) {
    unsigned char *p = s;
    while (n--) {
        *p++ = (unsigned char)c;
    }
    return s;
}

void *memcpy(void *out, const void *in, size_t n) {
    char *csrc = (char *)in;
    char *cdst = (char *)out;
    for (size_t i = 0; i < n; i++) {
        cdst[i] = csrc[i];
    }
    return out;
}

int memcmp(const void *s1, const void *s2, size_t n) {
    const unsigned char *p1 = s1, *p2 = s2;
    while (n--) {
        if (*p1 != *p2) {
            return *p1 - *p2;
        }
        p1++, p2++;
    }
    return 0;
}
