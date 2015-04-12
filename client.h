
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

#ifndef __CLIENT_H
#define __CLIENT_H

typedef struct {
    int             stream_fd;
    char            addr[INET_ADDRSTRLEN];
    int             port;
    char            method[10];
    char            http_ver[10];
    char            uri[1024];
    int             running;
    pthread_t       thread;
    int             jpeg_ready;
} client_t;

#endif /* __CLIENT_H */


void       * handle_client(client_t *client);
