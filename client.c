
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
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "streameye.h"
#include "common.h"
#include "auth.h"


const char *RESPONSE_BASIC_AUTH_HEADER_TEMPLATE =
        "HTTP/1.1 401 Not Authorized\r\n"
        "Server: streamEye/%s\r\n"
        "Connection: close\r\n"
        "WWW-Authenticate: Basic realm=\"%s\"\r\n";

const char *RESPONSE_OK_HEADER_TEMPLATE =
        "HTTP/1.1 200 OK\r\n"
        "Server: streamEye/%s\r\n"
        "Connection: close\r\n"
        "Max-Age: 0\r\n"
        "Expires: 0\r\n"
        "Cache-Control: no-cache, private\r\n"
        "Pragma: no-cache\r\n"
        "Content-Type: multipart/x-mixed-replace; boundary=" BOUNDARY_SEPARATOR "\r\n";

const char *MULTIPART_HEADER =
        "\r\n" BOUNDARY_SEPARATOR "\r\n"
        "Content-Type: image/jpeg\r\n"
        "Content-Length: ";


static int          read_request(client_t *client);
static int          write_to_client(client_t *client, char *buf, int size);
static int          write_response_ok_header(client_t *client);
static int          write_response_auth_basic_header(client_t *client);
static int          write_multipart_header(client_t *client, int jpeg_size);


    /* client handling */

int read_request(client_t *client) {
    char buf[REQ_BUF_LEN];
    char *line_end, *header_mid;
    char *header_name, *header_value;
    char *auth_mode, *auth_basic_hash;
    char *strtok_ptr;
    int size, found, offs = 0;

    memset(buf, 0, REQ_BUF_LEN);

    while (running) {
        if (offs >= REQ_BUF_LEN) {
            ERROR_CLIENT(client, "request header too large");
            return -1;
        }

        size = read(client->stream_fd, buf + offs, REQ_BUF_LEN - offs);
        if (size < 0) {
            if (errno == EAGAIN) {
                ERROR_CLIENT(client, "timeout reading from client");
                return -1;
            }
            else if (errno == EINTR) {
                break;
            }
            else {
                ERRNO_CLIENT(client, "read() failed");
                return -1;
            }
        }
        else if (size == 0) {
            ERROR_CLIENT(client, "connection closed");
            return -1;
        }

        offs += size;

        line_end = strstr(buf, "\r\n\r\n");
        if (line_end) {
            /* two new lines end the request */
            line_end[4] = 0;
            break;
        }
    }

    DEBUG_CLIENT(client, "received request header");

    offs = 0;
    while (running && (line_end = strstr(buf + offs, "\r\n"))) {
        if (offs == 0) { /* first request line */
            found = sscanf(buf, "%9s %1023s %9s", client->method, client->uri, client->http_ver);
            if (found != 3) {
                ERROR_CLIENT(client, "invalid request line");
                return -1;
            }

            DEBUG_CLIENT(client, "%s %s %s", client->method, client->uri, client->http_ver);
        }
        else { /* subsequent line, request header */
            header_mid = strstr(buf + offs, ": ");
            if (header_mid && header_mid < line_end) {
                header_name = strndup(buf + offs, header_mid - buf - offs);
                header_mid += 1; /* skip ":" */
                while (header_mid < line_end && *header_mid == ' ') {
                    header_mid++; /* skip header value leading spaces */
                }
                header_value = strndup(header_mid, line_end - header_mid);

                if (!strcmp(header_name, "Authorization")) {
                    auth_mode = strtok_r(header_value, " ", &strtok_ptr);
                    if (!strcmp(auth_mode, "Basic")) {
                        DEBUG_CLIENT(client, "authorization header: Basic");
                        auth_basic_hash = strtok_r(NULL, " ", &strtok_ptr);
                        if (auth_basic_hash) {
                            client->auth_basic_hash = strdup(auth_basic_hash);
                        }
                        else {
                            ERROR_CLIENT(client, "missing authorization hash");
                        }
                    }
                    else {
                        ERROR_CLIENT(client, "unknown authorization header: %s", auth_mode);
                    }
                }
                else {
                    DEBUG_CLIENT(client, "header: %s: %s", header_name, header_value);
                }

                free(header_name);
                free(header_value);
            }
        }

        offs = line_end - buf + 2;
    }

    DEBUG_CLIENT(client, "request read");

    return 0;
}

int write_to_client(client_t *client, char *buf, int size) {
    int written = write(client->stream_fd, buf, size);

    if (written < 0) {
        if (errno == EPIPE || errno == EINTR) {
            return 0;
        }
        else {
            ERRNO_CLIENT(client, "write() failed");
            return -1;
        }
    }
    else if (written < size) {
        ERROR_CLIENT(client, "not all data could be written");
        return -1;
    }

    return written;
}

int write_response_ok_header(client_t *client) {
    char *data = malloc(strlen(RESPONSE_OK_HEADER_TEMPLATE) + 16);
    sprintf(data, RESPONSE_OK_HEADER_TEMPLATE, STREAM_EYE_VERSION);

    int r = write_to_client(client, data, strlen(data));
    free(data);

    return r;
}

int write_response_auth_basic_header(client_t *client) {
    char *realm = get_auth_realm();
    char *data = malloc(strlen(RESPONSE_BASIC_AUTH_HEADER_TEMPLATE) + 16 + strlen(realm));
    sprintf(data, RESPONSE_BASIC_AUTH_HEADER_TEMPLATE, STREAM_EYE_VERSION, realm);
    
    int r = write_to_client(client, data, strlen(data));
    free(data);
    
    return r;
}

int write_multipart_header(client_t *client, int jpeg_size) {
    static int multipart_header_len = 0;
    if (!multipart_header_len) {
        multipart_header_len = strlen(MULTIPART_HEADER);
    }

    int written = write_to_client(client, (char *) MULTIPART_HEADER, multipart_header_len);
    if (written <= 0) {
        return written;
    }

    char size_str[16];
    snprintf(size_str, 16, "%d\r\n\r\n", jpeg_size);

    return write_to_client(client, size_str, strlen(size_str));
}

void handle_client(client_t *client) {
    DEBUG_CLIENT(client, "reading client request");
    int result = read_request(client);
    if (result < 0) {
        ERROR_CLIENT(client, "failed to read client request");
        cleanup_client(client);
        
        return;
    }

    if (get_auth_mode() == AUTH_BASIC) {
        if (!client->auth_basic_hash || strcmp(client->auth_basic_hash, get_auth_basic_hash())) {
            if (client->auth_basic_hash) {
                ERROR_CLIENT(client, "authentication error");
            }
            else {
                DEBUG_CLIENT(client, "authentication required");
            }
            result = write_response_auth_basic_header(client);
            if (result < 0) {
                ERROR_CLIENT(client, "failed to write response header");
            }

            cleanup_client(client);
            
            return;
        }
        else {
            DEBUG_CLIENT(client, "authentication successful");
        }
    }

    DEBUG_CLIENT(client, "writing response header");
    result = write_response_ok_header(client);
    if (result < 0) {
        ERROR_CLIENT(client, "failed to write response header");
        cleanup_client(client);
        
        return;
    }

    client->last_frame_time = get_now();

    while (running) {
        if (pthread_mutex_lock(&jpeg_mutex)) {
            ERROR_CLIENT(client, "pthread_mutex_lock() failed");
            break;
        }

        while (!client->jpeg_ready) {
            if (pthread_cond_wait(&jpeg_cond, &jpeg_mutex)) {
                ERROR_CLIENT(client, "pthread_mutex_wait() failed");
                pthread_mutex_unlock(&jpeg_mutex);
                break;
            }
        }

        /* copy the jpeg buffer into the client's temporary buffer,
         * but first make sure there's enough space */
        if (jpeg_size > client->jpeg_tmp_buf_max_size) {
            DEBUG_CLIENT(client, "temporary buffer increased to %d bytes", jpeg_size);
            client->jpeg_tmp_buf_max_size = jpeg_size;
            client->jpeg_tmp_buf = realloc(client->jpeg_tmp_buf, client->jpeg_tmp_buf_max_size);
        }

        client->jpeg_tmp_buf_size = jpeg_size;
        memcpy(client->jpeg_tmp_buf, jpeg_buf, client->jpeg_tmp_buf_size);

        if (pthread_mutex_unlock(&jpeg_mutex)) {
            ERROR_CLIENT(client, "pthread_mutex_unlock() failed");
        }

        double now = get_now();
        client->frame_int = client->frame_int * 0.7 + (now - client->last_frame_time) * 0.3;
        client->last_frame_time = now;
        DEBUG_CLIENT(client, "current fps: %.01lf", 1 / client->frame_int);

        /* clear the ready flag for this client */
        client->jpeg_ready = 0;

        if (!running) {
            break; /* speeds up the shut down procedure a bit */
        }

        DEBUG_CLIENT(client, "writing multipart header");
        result = write_multipart_header(client, client->jpeg_tmp_buf_size);
        if (result < 0) {
            ERROR_CLIENT(client, "failed to write multipart header");
            break;
        }
        else if (result == 0) {
            INFO_CLIENT(client, "connection closed");
            break;
        }

        DEBUG_CLIENT(client, "writing jpeg data (%d bytes)", client->jpeg_tmp_buf_size);
        result = write_to_client(client, client->jpeg_tmp_buf, client->jpeg_tmp_buf_size);
        if (result < 0) {
            ERROR_CLIENT(client, "failed to write jpeg data");
            break;
        }
        else if (result == 0) {
            INFO_CLIENT(client, "connection closed");
            break;
        }
    }
    
    cleanup_client(client);
}

