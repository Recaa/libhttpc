/*
 * @Author       : Zeepunt.H
 * @Date         : 2022-02-09 20:49:32
 * @LastEditTime : 2022-02-09 20:54:34
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/Recaa
 *  
 * Copyright (c) 2022 by Zeepunt.H, All Rights Reserved. 
 */

#ifndef __HTTP_CLIENT__
#define __HTTP_CLIENT__

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <httpc_opts.h>

typedef struct _http_client
{
    char *uri;                /*<! request address */
    char *host;               /*<! host address */

    char *header_buf;         /*<! http header buffer for request or response */
    int len_max;              /*<! malloc header buffer size */
    int len_used;             /*<! used header buffer size */

    int socket;
    int response_code;
    int time_out;             /*<! unit:s */

    int content_length;       /*<! conten legth */
    int chunked;              /*<! chunk flag */
} httpc_t;

int httpc_send_request(httpc_t *httpc, char *send_buf, int send_size);
int httpc_send_header(httpc_t *httpc);
int httpc_send_data(httpc_t *httpc, char *buf, int size);

int httpc_get(httpc_t *httpc);
int httpc_get_position(httpc_t *httpc, int start, int end);
int httpc_post(httpc_t *httpc, char *buf, int size);

int httpc_header_set(httpc_t *httpc, const char *fmt, ...);
char* httpc_header_get(httpc_t *httpc, const char *fields);

int httpc_normal_data_get(httpc_t *httpc, char *buf, int size);
int httpc_chunked_data_get(httpc_t *httpc, char *buf, int size);

int httpc_init(httpc_t *httpc, const char *uri, int size);
void httpc_deinit(httpc_t *httpc);

#endif