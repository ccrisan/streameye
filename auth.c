
/*
 * Copyright (c) Calin Crisan
 * This file is part of streamEye.
 *
 * streamEye is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE64_LENGTH(src_len) (4 * (((src_len) + 2) / 3))

#include "common.h"
#include "auth.h"


    /* globals */

static int auth_mode = AUTH_OFF;
static char *auth_username = NULL;
static char *auth_password = NULL;
static char *auth_realm = NULL;
static char *auth_basic_hash = NULL;

static const char *_MODE_STR[] = {"off", "basic"};


    /* local functions */

static void base64_encode(const char *src, char *dest, int len);


void set_auth(int mode, char *username, char *password, char *realm) {
    DEBUG("setting authentication mode to %s", _MODE_STR[mode]);

    auth_mode = mode;
    auth_username = strdup(username);
    auth_password = strdup(password);
    auth_realm = strdup(realm);

    auth_basic_hash = NULL;
}

int get_auth_mode() {
    return auth_mode;
}

char* get_auth_realm() {
    return auth_realm;
}

char *get_auth_basic_hash() {
    if (auth_basic_hash) {
        return auth_basic_hash;
    }

    DEBUG("computing basic auth hash");

    char *userpass = malloc(strlen(auth_username) + strlen(auth_password) + 2);
    sprintf(userpass, "%s:%s", auth_username, auth_password);

    auth_basic_hash = malloc(BASE64_LENGTH(strlen(userpass)) + 1);
    base64_encode(userpass, auth_basic_hash, strlen(userpass));

    return auth_basic_hash;
}


void base64_encode(const char *src, char *dest, int len) {
    static const char conv_table[64] = {
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
        'w', 'x', 'y', 'z', '0', '1', '2', '3',
        '4', '5', '6', '7', '8', '9', '+', '/'
    };

    int i;
    unsigned char * p = (unsigned char *) dest;

    /* transform 3x8 -> 4x6 bits */
    for (i = 0; i < len; i += 3) {
        *p++ = conv_table[src[0] >> 2];
        *p++ = conv_table[((src[0] & 3) << 4) + (src[1] >> 4)];
        *p++ = conv_table[((src[1] & 0xf) << 2) + (src[2] >> 6)];
        *p++ = conv_table[src[2] & 0x3f];
        src += 3;
    }

    /* padding */
    if (i == len + 1) {
        *(p - 1) = '=';
    }
    else if (i == len + 2) {
        *(p - 1) = *(p - 2) = '=';
    }

    *p = '\0';
}
