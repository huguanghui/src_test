#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "common_util.h"

/*************************************** base64 ***************************************/

static const char * base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

char * base64Encode( const unsigned char * bindata, char * base64, int binlength )
{
    if (!bindata || !base64)
    {
        fprintf(stderr, "NULL POINTER EXCEPTION!\n");
        return NULL;
    }

    int i = 0;
    int j = 0;
    unsigned char current = '\0';

    for ( i = 0, j = 0 ; i < binlength ; i += 3 )
    {
        current = (bindata[i] >> 2) ;
        current &= (unsigned char)0x3F;
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i] << 4 ) ) & ( (unsigned char)0x30 ) ;
        if ( i + 1 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+1] >> 4) ) & ( (unsigned char) 0x0F );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)(bindata[i+1] << 2) ) & ( (unsigned char)0x3C ) ;
        if ( i + 2 >= binlength )
        {
            base64[j++] = base64char[(int)current];
            base64[j++] = '=';
            break;
        }
        current |= ( (unsigned char)(bindata[i+2] >> 6) ) & ( (unsigned char) 0x03 );
        base64[j++] = base64char[(int)current];

        current = ( (unsigned char)bindata[i+2] ) & ( (unsigned char)0x3F ) ;
        base64[j++] = base64char[(int)current];
    }
    base64[j] = '\0';

    return base64;
}

//返回解码后数据长度
int base64Decode( const char * base64, unsigned char * bindata )
{
    if (!base64 || !bindata)
    {
        fprintf(stderr, "NULL POINTER EXCEPTION!\n");
        return -1;
    }

    int i = 0;
    int j = 0;
    unsigned char k = '\0';
    unsigned char temp[4] = {0};

    for ( i = 0, j = 0; base64[i] != '\0' ; i += 4 )
    {
        memset( temp, 0xFF, sizeof(temp) );
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i] )
            {
                temp[0]= k;
            }
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+1] )
            {
                temp[1]= k;
            }
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+2] )
            {
                temp[2]= k;
            }
        }
        for ( k = 0 ; k < 64 ; k ++ )
        {
            if ( base64char[k] == base64[i+3] )
            {
                temp[3]= k;
            }
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[0] << 2))&0xFC)) |
                ((unsigned char)((unsigned char)(temp[1]>>4)&0x03));
        if ( base64[i+2] == '=' )
        {
            break;
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[1] << 4))&0xF0)) |
                ((unsigned char)((unsigned char)(temp[2]>>2)&0x0F));
        if ( base64[i+3] == '=' )
        {
            break;
        }

        bindata[j++] = ((unsigned char)(((unsigned char)(temp[2] << 6))&0xF0)) |
                ((unsigned char)(temp[3]&0x3F));
    }

    return j;
}

/*************************************** mbuf ***************************************/
void mbuf_init(struct mbuf *mbuf, size_t initial_size) 
{
	mbuf->len = mbuf->size = 0;
	mbuf->buf = NULL;
	mbuf_resize(mbuf, initial_size);
	return;
}

void mbuf_free(struct mbuf *mbuf) 
{
	if (mbuf->buf != NULL) {
		free(mbuf->buf);
		mbuf_init(mbuf, 0);
	}
	return;
}

void mbuf_resize(struct mbuf *a, size_t new_size) 
{
	if (new_size > a->size || (new_size < a->size && new_size >= a->len)) 
	{
		char *buf = (char *) realloc(a->buf, new_size);
		/*
			* In case realloc fails, there's not much we can do, except keep things as
			* they are. Note that NULL is a valid return value from realloc when
			* size == 0, but that is covered too.
			*/
		if (buf == NULL && new_size != 0)
		{
				return;
		}
		a->buf = buf;
		a->size = new_size;
	}
	return;
}

void mbuf_trim(struct mbuf *mbuf) 
{
	mbuf_resize(mbuf, mbuf->len);
	return;
}

size_t mbuf_insert(struct mbuf *a, size_t off, const void *buf, size_t len) 
{
	if ((NULL == a)||
		(a->len > a->size)||
		(off > a->len))
	{
			return 0;
	}

	char *p = NULL;
	/* check overflow */
	if (~(size_t) 0 - (size_t) a->buf < len)
	{
		return 0;
	}

	if (a->len + len <= a->size) 
	{
		memmove(a->buf + off + len, a->buf + off, a->len - off);
		if (buf != NULL) 
		{
			memcpy(a->buf + off, buf, len);
		}
		a->len += len;
	} 
	else 
	{
		size_t min_size = (a->len + len);
		size_t new_size = (size_t)(min_size * MBUF_SIZE_MULTIPLIER);
		if (new_size - min_size > MBUF_SIZE_MAX_HEADROOM) 
		{
			new_size = min_size + MBUF_SIZE_MAX_HEADROOM;
		}
		p = (char *) realloc(a->buf, new_size);
		if (p == NULL && new_size != min_size) 
		{
			new_size = min_size;
			p = (char *) realloc(a->buf, new_size);
		}
		if (p != NULL) 
		{
			a->buf = p;
			if (off != a->len) 
			{
				memmove(a->buf + off + len, a->buf + off, a->len - off);
			}
			if (buf != NULL) memcpy(a->buf + off, buf, len);
			a->len += len;
			a->size = new_size;
		} 
		else 
		{
			len = 0;
		}
	}

	return len;
}

size_t mbuf_append(struct mbuf *a, const void *buf, size_t len) 
{
	return mbuf_insert(a, a->len, buf, len);
}

size_t mbuf_append_and_free(struct mbuf *a, void *data, size_t len) 
{
	size_t ret;
	/* Optimization: if the buffer is currently empty,
		* take over the user-provided buffer. */
	if (a->len == 0) 
	{
		if (a->buf != NULL) free(a->buf);
		a->buf = (char *) data;
		a->len = a->size = len;
		return len;
	}
	ret = mbuf_insert(a, a->len, data, len);
	free(data);
	return ret;
}

void mbuf_remove(struct mbuf *mb, size_t n) 
{
	if (n > 0 && n <= mb->len) 
	{
		memmove(mb->buf, mb->buf + n, mb->len - n);
		mb->len -= n;
	}
	return;
}

void mbuf_clear(struct mbuf *mb) 
{
	mb->len = 0;
	return;
}

void mbuf_move(struct mbuf *from, struct mbuf *to) 
{
	memcpy(to, from, sizeof(*to));
	memset(from, 0, sizeof(*from));
	return;
}

#define MG_BUFFER_SIZE 1000

static int mbuf_avprintf(char **buf, int size, const char *fmt, va_list ap)
{
	va_list ap_copy;
	int len;

	va_copy(ap_copy, ap);
	len = vsnprintf(*buf, size, fmt, ap_copy);
	if (len >= size)
	{
		*buf = (char *)malloc(len+1);
		va_copy(ap_copy, ap);
		len = vsnprintf(*buf, len+1, fmt, ap_copy);
		va_end(ap_copy);
	}
	return len;
}

static int mbuf_vprintf(struct mbuf *m, const char *fmt, va_list ap)
{
	int len;
	char mem[MG_BUFFER_SIZE], *buf = mem;

	if ((len = mbuf_avprintf(&buf, sizeof(mem), fmt, ap)) > 0)
	{
		mbuf_append(m, buf, len);
	}
	if (buf != mem && buf != NULL)
	{
		free(buf);
	}

	return len;
}

int mbuf_printf(struct mbuf *m, const char *fmt, ...)
{
	int len;
	va_list ap;
	va_start(ap, fmt);
	len = mbuf_vprintf(m, fmt, ap);
	va_end(ap);

	return len;
}
