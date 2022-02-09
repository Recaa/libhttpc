/*
 * @Author       : Zeepunt.H
 * @Date         : 2022-02-09 20:38:07
 * @LastEditTime : 2022-02-09 22:02:01
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/Recaa
 *  
 * Copyright (c) 2022 by Zeepunt.H, All Rights Reserved. 
 */

#include <errno.h>
#include <fcntl.h>

#include <http_client.h>

#define DEBUG  1
#define INFO   1
#define WARN   1
#define ERROR  1

/* 测试链接 */
#define TEST_URL                "http://httpbin.org/get"

/* chunk 测试链接 */
#define CHUNK_URL               "http://www.httpwatch.com/httpgallery/chunked/chunkedimage.aspx"

/* range 测试链接 */
#define RANGE_URL               "http://httpbin.org/range/%d"

/* 保存的文件名 */
#define GET_CHUNK_FILE_PATH     "tmp_file.aspx"

/* 头部缓存区大小 */
#define HEADER_BUF_SIZE         4096

/* 自定义缓冲区大小 */
#define BUF_SIZE        4096

void default_get_request(void)
{
    char *recv_buf = NULL;
    
    httpc_t httpc;
    
    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;   

    httpc_init(&httpc, TEST_URL, HEADER_BUF_SIZE);

    httpc_get(&httpc);

    httpc_normal_data_get(&httpc, recv_buf, BUF_SIZE);
    printf("recv : %s\n.", recv_buf);

end:
    safe_free(recv_buf);
    httpc_deinit(&httpc);
}

void diy_get_request(void)
{
    char *recv_buf = NULL;

    httpc_t httpc;

    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;

    httpc_init(&httpc, TEST_URL, HEADER_BUF_SIZE);

    httpc_header_set(&httpc, "GET %s HTTP/1.1\r\n", httpc.uri);
    httpc_header_set(&httpc, "Host: %s\r\n", httpc.host);
    httpc_header_set(&httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(&httpc, "\r\n");

    httpc_send_request(&httpc, NULL, 0);
    
    httpc_normal_data_get(&httpc, recv_buf, BUF_SIZE);
    printf("recv : %s\n.", recv_buf);

end:
    safe_free(recv_buf);
    httpc_deinit(&httpc);  
}

void chunk_get_request(void)
{
    int count = 0;
    int fd = -1;

    char *buf = NULL;

    httpc_t httpc;

    httpc_init(&httpc, CHUNK_URL, HEADER_BUF_SIZE);

    httpc_get(&httpc);

    buf = calloc(1, BUF_SIZE);
    if (NULL == buf)
        goto end;

    /* 因为只有该demo使用这个文件，所以这里是表示删除 */
    unlink(GET_CHUNK_FILE_PATH);

    fd = open(GET_CHUNK_FILE_PATH, O_RDWR | O_CREAT);
    if (fd < 0)
    {
        printf("open %s fail, [%s].\n", GET_CHUNK_FILE_PATH, strerror(errno));
        goto end;
    } 

    printf("start download...\n");
    while (1)
    {
        count = httpc_chunked_data_get(&httpc, buf, BUF_SIZE);
        if (count < 0)
            break;
        write(fd, buf, count);
        memset(buf, 0, BUF_SIZE);
    }
    close(fd);
    printf("done.\n");

end:
    safe_free(buf);
    httpc_deinit(&httpc);
}

void range_get_request(void)
{
    int ret          = -1;
    int offset       = 10;
    char tmp_url[32] = {0};
    char *recv_buf   = NULL;
    
    httpc_t httpc;
    
    recv_buf = calloc(1, BUF_SIZE);
    if (NULL == recv_buf)
        goto end;   


    sprintf(tmp_url, RANGE_URL, offset);
    httpc_init(&httpc, tmp_url, HEADER_BUF_SIZE);

    ret = httpc_get_position(&httpc, 0, offset);
    if (ret < 0)
        goto end;

    httpc_normal_data_get(&httpc, recv_buf, BUF_SIZE);
    printf("recv : %s.\n", recv_buf);

end:
    safe_free(recv_buf);
    httpc_deinit(&httpc);
}

void print_usage(void)
{
    printf("usage: Demo default.\n");
    printf("       Demo diy.\n");
    printf("       Demo chunk.\n");
    printf("       Demo range.\n");
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
        default_get_request();
        return 0;
    }
    else if (0 == strcmp(argv[1], "diy"))
    {
        diy_get_request();
        return 0;
    }
    else if (0 == strcmp(argv[1], "chunk"))
    {
        chunk_get_request();
        return 0;
    }
    else if (0 == strcmp(argv[1], "range"))
    {
        range_get_request();
        return 0;
    }
    else 
    {
        print_usage();
        return -1;
    }
}