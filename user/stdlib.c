void *memcpy(void *restrict dest, const void *restrict src, unsigned long n)
{
	unsigned char *d = dest;
	const unsigned char *s = src;

	while(n--)
		*d++ = *s++;

	return dest;
}

int memcmp(void *d, void *s, unsigned long len)
{
        unsigned char *dd = d, *ss = s;

        while(len--)
        {
                if(*dd != *ss)
                        return *dd - *ss;
                dd++;
                ss++;
        }

        return 0;
}

void memset(void *dest, int c, unsigned long len)
{
	char *d = dest;

	while(len--)
		*d++ = c;
}
