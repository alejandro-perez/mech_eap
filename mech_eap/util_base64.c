/*
 * Copyright (c) 1995-2001 Kungliga Tekniska Högskolan
 * (Royal Institute of Technology, Stockholm, Sweden).
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * 3. Neither the name of the Institute nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE INSTITUTE AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE INSTITUTE OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

#include "gssapiP_eap.h"

static const char base64_chars[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static int
pos(char c)
{
    const char *p;
    for (p = base64_chars; *p; p++)
	if (*p == c)
	    return p - base64_chars;
    return -1;
}

ssize_t
base64Encode(const void *data, int size, char **str)
{
    char *s, *p;
    int i;
    int c;
    const unsigned char *q;

    if (size > INT_MAX/4 || size < 0) {
	*str = NULL;
	return -1;
    }

    p = s = (char *)GSSEAP_MALLOC(BASE64_EXPAND(size));
    if (p == NULL) {
        *str = NULL;
	return -1;
    }
    q = (const unsigned char *) data;

    for (i = 0; i < size;) {
	c = q[i++];
	c *= 256;
	if (i < size)
	    c += q[i];
	i++;
	c *= 256;
	if (i < size)
	    c += q[i];
	i++;
	p[0] = base64_chars[(c & 0x00fc0000) >> 18];
	p[1] = base64_chars[(c & 0x0003f000) >> 12];
	p[2] = base64_chars[(c & 0x00000fc0) >> 6];
	p[3] = base64_chars[(c & 0x0000003f) >> 0];
	if (i > size)
	    p[3] = '=';
	if (i > size + 1)
	    p[2] = '=';
	p += 4;
    }
    *p = 0;
    *str = s;
    return strlen(s);
}

#define DECODE_ERROR 0xffffffff

static unsigned int
token_decode(const char *token)
{
    int i;
    unsigned int val = 0;
    int marker = 0;
    if (strlen(token) < 4)
	return DECODE_ERROR;
    for (i = 0; i < 4; i++) {
	val *= 64;
	if (token[i] == '=')
	    marker++;
	else if (marker > 0)
	    return DECODE_ERROR;
	else
	    val += pos(token[i]);
    }
    if (marker > 2)
	return DECODE_ERROR;
    return (marker << 24) | val;
}

ssize_t
base64Decode(const char *str, void *data)
{
    const char *p;
    unsigned char *q;

    q = data;
    p = str;

    while (*p && (*p == '=' || strchr(base64_chars, *p) || isspace(*p))) {
	unsigned int val; 
	unsigned int marker; 
	if (isspace(*p)) {
            p++;
            continue;
        }
        val = token_decode(p);
        marker = (val >> 24) & 0xff;
	if (val == DECODE_ERROR)
	    return -1;
	*q++ = (val >> 16) & 0xff;
	if (marker < 2)
	    *q++ = (val >> 8) & 0xff;
	if (marker < 1)
	    *q++ = val & 0xff;
	p += 4;
    }
    return q - (unsigned char *) data;
}

int
base64Valid(const char *str)
{
    const char *p = str;
    int valid = 1;

    while (*p && *p && (*p == '=' || strchr(base64_chars, *p))) {
	unsigned int val = token_decode(p);
	if (val == DECODE_ERROR) {
            valid = 0;
	    break;
        }
	p += 4;
	if (*p == '\n')
	    p++;
    }
    return valid;
}
