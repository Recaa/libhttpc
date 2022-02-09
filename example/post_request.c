/*
 * @Author       : Zeepunt.H
 * @Date         : 2022-02-09 20:39:29
 * @LastEditTime : 2022-02-09 22:02:05
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/Recaa
 *  
 * Copyright (c) 2022 by Zeepunt.H, All Rights Reserved. 
 */

#include <http_client.h>

/* 测试链接 */
#define TEST_URL        "http://httpbin.org/post"

/* 头部缓存区大小 */
#define HEADER_BUF_SIZE 4096

/* 自定义缓冲区大小 */
#define BUF_SIZE        4096

void default_post_request(void)
{
    int count = 0;

    char *recv_buf = NULL;
    char *send_buf = NULL;

    httpc_t httpc;

    httpc_init(&httpc, TEST_URL, HEADER_BUF_SIZE);

    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;

    send_buf = calloc(1, BUF_SIZE);
    if (NULL == send_buf)
        goto end;

    sprintf(send_buf, "just for post data test.");

    httpc_post(&httpc, send_buf, strlen(send_buf));

    count = httpc_normal_data_get(&httpc, recv_buf, BUF_SIZE);
    printf("recv : %s\n.", recv_buf);

end:
    safe_free(recv_buf);
    safe_free(send_buf);
    httpc_deinit(&httpc);
}

void diy_post_request(void)
{
    int count = 0;
    int fd = -1;

    char *recv_buf = NULL;
    char *send_buf = NULL;

    httpc_t httpc;

    httpc_init(&httpc, TEST_URL, HEADER_BUF_SIZE);

    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;

    send_buf = calloc(1, BUF_SIZE);
    if (NULL == send_buf)
        goto end;

    sprintf(send_buf, "just for post data test.");

    httpc_header_set(&httpc, "POST %s HTTP/1.1\r\n", httpc.uri);
    httpc_header_set(&httpc, "Host: %s\r\n", httpc.host);
    httpc_header_set(&httpc, "Content-Length: %d\r\n", strlen(send_buf));
    httpc_header_set(&httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(&httpc, "\r\n");

    httpc_send_request(&httpc, send_buf, BUF_SIZE);

    count = httpc_normal_data_get(&httpc, recv_buf, BUF_SIZE);
    printf("recv : %s\n.", recv_buf);

end:
    safe_free(recv_buf);
    safe_free(send_buf);
    httpc_deinit(&httpc);  
}

void print_usage(void)
{
    printf("usage: Demo default.\n");
    printf("       Demo diy.\n");
}

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        print_usage();
        return -1;
    }

    if (0 == strcmp(argv[1], "default")) 
    {
        default_post_request();
        return 0;
    }
    else if (0 == strcmp(argv[1], "diy"))
    {
        diy_post_request();
        return 0;
    }
    else 
    {
        print_usage();
        return -1;
    }
}