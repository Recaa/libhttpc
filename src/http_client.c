/*
 * @Author       : Zeepunt.H
 * @Date         : 2022-02-09 20:53:42
 * @LastEditTime : 2022-02-09 22:08:14
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/Recaa
 *  
 * Copyright (c) 2022 by Zeepunt.H, All Rights Reserved. 
 */

#include <http_client.h>

int httpc_handle_response(httpc_t *httpc);

/**
 * @brief http header 缓冲区初始化
 * 
 * @param httpc: http client 对象
 * @param size: 缓冲区的大小
 * 
 * @return 0:成功, -1:失败
 */
static int httpc_header_init(httpc_t *httpc, int size)
{
    WARN_NULL(httpc, -1);

    if (size <= 0)
    {
        httpc_error("invalid buffer len.");
        return -1;
    }

    httpc->header_buf = httpc_calloc(1, size * sizeof(char));
    if (NULL == httpc->header_buf)
    {
        httpc_error("header buffer malloc fail.");
        return -1;
    }
    httpc->len_max = size;
    httpc->len_used = 0;
    
    return 0;
}

/**
 * @brief http header 缓冲区清零
 * 
 * @param httpc: http client 对象
 */
static void httpc_header_reset(httpc_t *httpc)
{
    WARN_NULL(httpc, );

    httpc_memset(httpc->header_buf, 0, httpc->len_max);
    httpc->len_used = 0;
}

/**
 * @brief http header 缓冲区释放
 * 
 * @param httpc: http client 对象
 */
static void httpc_header_deinit(httpc_t *httpc)
{
    WARN_NULL(httpc, );

    safe_free(httpc->header_buf);
    httpc->len_max = 0;
    httpc->len_used = 0;
}

/**
 * @brief 将数据发送到服务器
 * 
 * @param httpc: http client 对象
 * @param buf: 发送数据的缓冲区
 * @param size: 缓冲区的大小
 * 
 * @return >=0:发送的数据大小, -1:失败
 */
static int httpc_write(httpc_t *httpc, char *buf, int size)
{
    WARN_NULL(httpc, -1);
    WARN_NULL(buf, -1);

    if (httpc->socket < 0)
    {
        httpc_error("invalid socket.");
        return -1;
    }

    return send(httpc->socket, buf, size, 0);
}

/**
 * @brief 接收服务器的数据
 * 
 * @param httpc: http client 对象
 * @param buf: 接收数据的缓冲区
 * @param size: 缓冲区的大小
 * 
 * @return >=0:接收的数据大小, -1:失败
 */
static int httpc_read(httpc_t *httpc, char *buf, int size)
{
    WARN_NULL(httpc, -1);
    WARN_NULL(buf, -1);

    if (httpc->socket < 0)
    {
        httpc_error("invalid socket.");
        return -1;
    }

    return recv(httpc->socket, buf, size, 0);
}

/**
 * @brief 解析传进来的 uri 连接
 * 
 * @param httpc: http client 对象
 * 
 * @return 0:成功, -1:失败
 */
static int httpc_parse_uri(httpc_t *httpc)
{
    int ret              = -1;
    char port_str[5]     = "80";
    char *host_ptr       = NULL;
    char *ptr            = NULL;

    WARN_NULL(httpc, -1);
    WARN_NULL(httpc->uri, -1);

    /* example: https://gitee.com/zeepunt/httpclient */
    if (0 == strncmp(httpc->uri, "http://", 7))
    {
        host_ptr = httpc->uri + 7;
    }
    else if (0 == strncmp(httpc->uri, "https://", 8))
    {
        host_ptr = httpc->uri + 8;
        httpc_memset(port_str, 0, 4);
        strncpy(port_str, "443", 3);
        port_str[3] = '\0';
    }
    else
    {
        httpc_error("unknown uri.");
        goto invalid;
    }

    httpc->host = NULL;
    ptr = strstr(host_ptr, "/");
    if (NULL == ptr)
        httpc->host = strdup(host_ptr);
    else
        httpc->host = strndup(host_ptr, ptr - host_ptr);

    if (NULL == httpc->host)
    {
        httpc_error("host buffer malloc fail.");
        goto invalid;
    }
    httpc_debug("host: %s.", httpc->host);

    /* 找到主机地址对应的 IP 地址 */
    struct addrinfo hints;
    struct addrinfo *result  = NULL;
    struct addrinfo *cur     = NULL;

    httpc_memset(&hints, 0, sizeof(struct addrinfo));
    ret = getaddrinfo(httpc->host, 
                      port_str,
                      &hints,
                      &result);
    if (ret < 0)
    {
        httpc_error("getaddrinfo fail. [%d]", ret);
        goto addr_fail;
    }
    
    char ip_str[INET_ADDRSTRLEN] = {0};

    for (cur = result; cur != NULL; cur = cur->ai_next) 
    {
        if (cur->ai_family == AF_INET)
        {
            ret = connect(httpc->socket, cur->ai_addr, sizeof(struct sockaddr));
            if (0 == ret)
            {
                inet_ntop(AF_INET, &((struct sockaddr_in *)cur->ai_addr)->sin_addr, ip_str, INET_ADDRSTRLEN);
                httpc_info("connected ip addr: %s.", ip_str);
                break;
            }
        }
    }

    freeaddrinfo(result);
    ret = 0;
    return ret;

addr_fail:
    safe_free(httpc->host);
invalid:
    return ret;
}

/**
 * @brief 接收服务器的数据
 * 
 * @param httpc: http client 对象
 * @param buf: 接收数据的缓冲区
 * @param size: 缓冲区大小
 * 
 * @return >0:接收的数据大小, 0:失败
 */
static int httpc_read_one_line(httpc_t *httpc, char *buf, int size)
{
    int ret        = -1;
    int count      = 0;
    char byte      = 0;
    char last_byte = 0;

    WARN_NULL(httpc, -1);
    WARN_NULL(buf, -1);

    while (1)
    {
        ret = httpc_read(httpc, &byte, 1);
        if (ret <= 0)
            break;

        if ((byte == '\n') && (last_byte == '\r'))    /* \r\n 在 HTTP 中表示一行结束 */
            break;
        
        last_byte = byte;
        buf[count++] = byte;
        if (count >= size)
        {
            httpc_warn("buffer is full, maybe data is incomplete.[%d >= %d]", count, size);
            break;
        }
    }

    return count;    /* 这里返回的长度 = 数据 + '\r' + 1 , 所以调用函数需要做相应处理 */
}

/**
 * @brief 发送报文头部数据
 * 
 * @param httpc: http client 对象
 * 
 * @return >=0:发送的数据大小, -1:失败
 */
int httpc_send_header(httpc_t *httpc)
{
    WARN_NULL(httpc, -1);
    WARN_NULL(httpc->header_buf, -1);

    httpc_debug("http request header:\n%s", httpc->header_buf);
    return httpc_write(httpc, httpc->header_buf, httpc->len_used);
}

/**
 * @brief 发送报文主体数据
 * 
 * @param httpc: http client 对象
 * @param buf: 发送数据的缓冲区
 * @param size: 缓冲区的大小
 * 
 * @return >=0:发送的数据大小, -1:失败
 */
int httpc_send_data(httpc_t *httpc, char *buf, int size)
{
    WARN_NULL(httpc, -1);
    WARN_NULL(buf, -1);

    httpc_debug("http request data:\n%s", buf);
    return httpc_write(httpc, buf, size);
}

/**
 * @brief 发送 http 请求, 并且自动解析响应报文的头部信息
 * 
 * @param httpc: http client 对象
 * @param send_buf: 发送数据的缓冲区
 * @param send_size: 缓冲区的大小
 * 
 * @return 0:成功, -1:失败
 */
int httpc_send_request(httpc_t *httpc, char *send_buf, int send_size)
{
    int ret = -1;

    WARN_NULL(httpc, -1);
    WARN_NULL(httpc->header_buf, -1);

    ret = httpc_send_header(httpc);
    if (ret < 0)
    {
        httpc_error("http send header fail.");
        return -1;
    }

    if (NULL != send_buf)
    {
        ret = httpc_send_data(httpc, send_buf, send_size);
        if (ret < 0)
        {
            httpc_error("http send data fail.");
            return -1;
        }
    }

    ret = httpc_handle_response(httpc);
    if (ret < 0)
    {
        httpc_error("http handle response header fail.");
        return -1;
    }

    return ret;
}

/**
 * @brief 处理从服务器接收到的数据, 获得响应的报文头部, 但不包括响应的报文主体
 * 
 * @param httpc: http client 对象
 * @param fields: 关键字
 * 
 * @return 0:成功, -1:失败
 */
int httpc_handle_response(httpc_t *httpc)
{
    int count = -1;
    char *buf = NULL;
    char *ptr = NULL;

    WARN_NULL(httpc, -1);
    WARN_NULL(httpc->header_buf, -1);

    httpc_header_reset(httpc);

    httpc_debug("http response header:");
    while (1)
    {
        buf = httpc->header_buf + httpc->len_used;

        count = httpc_read_one_line(httpc, buf, httpc->len_max-httpc->len_used);
        if (count <= 0)
            break;

        /* 报文头部和报文主体之间存在 "\r\n" 换行符 */
        if ((buf[0] == '\r') && (1 == count))
        {
            buf[0] = '\0';
            break;
        }

        buf[count - 1] = '\0';
        httpc_debug("%s.", buf);

        httpc->len_used += count;  /* 这里不用 -1, 因为下次循环会用到 */
        if (httpc->len_used >= httpc->len_max)
        {
            httpc_warn("buffer is full, maybe data is incomplete.");
            break;
        }
    }

    ptr = strstr(httpc->header_buf, "HTTP/1.");
    if (NULL == ptr)
    {
        httpc_warn("can not find response code.");
        return -1;
    }
    ptr += strlen("HTTP/1.1");

    while ((NULL != ptr) && (*ptr == ' '))
        ptr++;
    httpc->response_code = strtoul(ptr, NULL, 10);  /* 不做处理, 因为 strtoul 遇到非数字时自动停止 */

    httpc->content_length = 0;
    ptr = NULL;
    ptr = httpc_header_get(httpc, "Content-Length");
    if (NULL != ptr)
        httpc->content_length = strtoul(ptr, NULL, 10);
    
    httpc->chunked = 0;
    if (0 != httpc->content_length) 
        return 0;    /* chunked 和 Content-Length 是互斥的 */
        
    ptr = NULL;
    ptr = httpc_header_get(httpc, "Transfer-Encoding");
    if (NULL != ptr)
    {
        if (0 == strcmp(ptr, "chunked"))
        {
            httpc_info("chunked mode.");
            httpc->chunked = 1;
        }
        return 0;
    }
    else
        return -1;    
}

/**
 * @brief 根据关键字从响应中找出对应的数据
 * 
 * @param httpc: http client 对象
 * @param fields: 关键字
 * 
 * @return 地址:成功, NULL:失败
 */
char* httpc_header_get(httpc_t *httpc, const char *fields)
{
    int count     = 0;
    char *str_ptr = NULL;
    char *ptr     = NULL;

    WARN_NULL(httpc, NULL);
    WARN_NULL(httpc->header_buf, NULL);
    WARN_NULL(fields, NULL);

    str_ptr = httpc->header_buf;
    while (count <= httpc->len_used)
    {
        ptr = strstr(str_ptr+count, fields);
        if (NULL == ptr) 
        {
            ptr = strchr(str_ptr+count, '\0');
            if (NULL == ptr)
                break;
            count += (ptr - (str_ptr+count)) + 1;
            ptr = NULL;
        }
        else
        {
            break; 
        }   
    }

    if (NULL == ptr)
    {
        httpc_error("can not find field[%d]: %s.", count, fields);
        return NULL;
    }
    
    ptr += strlen(fields) + 1;  /* +1 对应的是 ":" */

    if (ptr[0] == ' ')
        ptr += 1;

    return ptr;
}

/**
 * @brief 自定义填充 header 缓冲区
 * 
 * @param httpc: http client 对象
 * @param fmt: 填充的参数
 * 
 * @return >=0:成功, -1:失败
 */
int httpc_header_set(httpc_t *httpc, const char *fmt, ...)
{
    int len = 0;

    va_list args;

    WARN_NULL(httpc, -1);
    WARN_NULL(httpc->header_buf, -1);

    va_start(args, fmt);
    len = vsnprintf(httpc->header_buf + httpc->len_used, 
                    httpc->len_max - httpc->len_used, 
                    fmt, 
                    args);
    va_end(args);

    httpc->len_used += len;
    if (httpc->len_used >= httpc->len_max)
    {
        httpc_warn("buffer is full.");
        return -1;
    }

    return len;
}

/**
 * @brief 接收 chunk 模式的报文主体数据
 * 
 * @param httpc: http client 对象
 * @param buf: 接收数据的缓冲区
 * @param size: 缓冲区的大小
 * 
 * @return >=0:接收的数据大小, -1:失败
 */
int httpc_chunked_data_get(httpc_t *httpc, char *buf, int size)
{
    int ret       = -1;
    int count     = 0;
    int len       = -1;
    char *tmp_buf = NULL;

    WARN_NULL(httpc, -1);
    WARN_NULL(buf, -1);

    tmp_buf = httpc_calloc(1, 1024 * sizeof(char));
    if (NULL == tmp_buf)
        return -1;
    
    count = httpc_read_one_line(httpc, tmp_buf, 1024);
    if (count < 0)
    {
        httpc_error("chunk length invalid.");
        goto fail;
    }
    buf[count - 1] = '\0';

    len = strtoul(tmp_buf, NULL, 16);
    if (len < 0)
    {
        httpc_error("chunk data invalid.");
        goto fail;
    }
    else if (len == 0)
    {
        httpc_debug("no more chunk data.");
        ret = -1;
        goto fail;
    }

    if (len > size)
    {
        httpc_error("buffer is smaller than chunck size");
        goto fail;
    }

    count = httpc_read(httpc, buf, len);
    if (count < 0)
    {
        httpc_error("recv chunk data fail.");
        goto fail;
    }
    
    len = httpc_read(httpc, tmp_buf, 2);    /* 对于 CRLF, 不做处理 */
    if (len < 0)
    {
        httpc_error("recv chunk CRLF fail.");
    }
    ret = count;

fail:
    safe_free(tmp_buf);
    return ret;
}

/**
 * @brief 接收报文主体数据
 * 
 * @param httpc: http client 对象
 * @param buf: 接收数据的缓冲区
 * @param size: 缓冲区的大小
 * 
 * @return >=0:接收的数据大小, -1:失败
 */
int httpc_normal_data_get(httpc_t *httpc, char *buf, int size)
{
    int ret = -1;

    WARN_NULL(httpc, -1);
    WARN_NULL(buf, -1);

    ret = httpc_read(httpc, buf, size);
    if (ret < 0)
    {
        httpc_error("recv normal data fail.");
    }
    return ret;
}

/**
 * @brief 发送一个 HTTP GET 请求
 * 
 * @param httpc: http client 对象
 * 
 * @return 0:成功, -1:失败
 */
int httpc_get(httpc_t *httpc)
{
    WARN_NULL(httpc, -1);

    httpc_header_set(httpc, "GET %s HTTP/1.1\r\n", httpc->uri);
    httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
    httpc_header_set(httpc, "Accept: */*\r\n");
    httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(httpc, "\r\n");    

    return httpc_send_request(httpc, NULL, 0);
}

/**
 * @brief 发送一个带范围请求的 HTTP GET 请求
 * 
 * @param httpc: http client 对象
 * @param start: 请求的开始位置, >=0
 * @param end: 请求的结束位置, >=0
 * 
 * @return 0:成功, -1:失败
 */
int httpc_get_position(httpc_t *httpc, int start, int end)
{
    int ret   = -1;
    char *ptr = NULL;

    WARN_NULL(httpc, -1);

    ret = httpc_get(httpc);
    if (ret < 0)
        return -1;

    ptr = httpc_header_get(httpc, "Accept-Ranges: bytes");
    if (NULL == ptr)
    {
        httpc_error("not support range request.");
        return -1;
    }
    httpc_header_reset(httpc);

    httpc_header_set(httpc, "GET %s HTTP/1.1\r\n", httpc->uri);
    httpc_header_set(httpc, "Host: %s\r\n", httpc->host);

    if ((start >= 0) && (end >= 0))
    {
        httpc_header_set(httpc, "Range: bytes=%d-%d\r\n", start, end);
    }
    else if ((start >= 0) && (end < 0))
    {
        httpc_header_set(httpc, "Range: bytes=%d-\r\n", start);
    }
    else if ((start < 0) && (end >= 0))
    {
        httpc_header_set(httpc, "Range: bytes=-%d\r\n", end);
    }
    else
    {
        httpc_error("invalid range.");
        return -1;
    }
    
    httpc_header_set(httpc, "Accept: */*\r\n");
    httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(httpc, "\r\n");    
}

/**
 * @brief 发送一个 HTTP POST 请求
 * 
 * @param httpc: http client 对象
 * @param buf: 发送缓冲区
 * @param size: 缓冲区大小
 * 
 * @return 0:成功, -1:失败 
 */
int httpc_post(httpc_t *httpc, char *buf, int size)
{
    WARN_NULL(httpc, -1);

    httpc_header_set(httpc, "POST %s HTTP/1.1\r\n", httpc->uri);
    httpc_header_set(httpc, "Host: %s\r\n", httpc->host);
    httpc_header_set(httpc, "Content-Length: %d\r\n", size);
    httpc_header_set(httpc, "Accept: */*\r\n");
    httpc_header_set(httpc, "User-Agent: Tiny HTTPClient Agent\r\n");
    httpc_header_set(httpc, "\r\n");

    if (NULL != buf)
        return httpc_send_request(httpc, buf, size);
    else
        return httpc_send_request(httpc, NULL, 0);
}

/**
 * @brief 初始化一个 http client 对象
 * 
 * @param httpc: http client 对象
 * @param url: 要访问的网络地址
 * @param size: header 缓冲区的大小, 用于存放 request 和 response
 * 
 * @return 0:成功, -1:失败
 */
int httpc_init(httpc_t *httpc, const char *uri, int size)
{
    int ret = -1;

    struct timeval timeout;

    WARN_NULL(httpc, -1);
    WARN_NULL(uri, -1);

    httpc_memset(httpc, 0, sizeof(httpc_t));
    
    ret = httpc_header_init(httpc, size);
    if (ret < 0)
        goto invalid;

    httpc->uri = NULL;
    httpc->uri = strdup(uri);
    if (NULL == httpc->uri)
    {
        httpc_error("uri buffer malloc fail.");
        goto uri_fail;
    }

    httpc->socket = -1;
    httpc->socket = socket(AF_INET, SOCK_STREAM, 0);
    if (httpc->socket < 0)
    {
        httpc_error("socket create fail.");
        goto parse_fail;
    }
    
    if (httpc->time_out > 0)
    {
        timeout.tv_sec = httpc->time_out;
        timeout.tv_usec = 0;
    }
    else
    {
        timeout.tv_sec = 10;
        timeout.tv_usec = 0;
    }

    ret = setsockopt(httpc->socket, SOL_SOCKET, SO_SNDTIMEO, (void *)&timeout, (socklen_t)(sizeof(timeout)));
    if (-1 == ret)
    {
        httpc_warn("set send timeout fail.");
    }

    ret = setsockopt(httpc->socket, SOL_SOCKET, SO_RCVTIMEO, (void *)&timeout, (socklen_t)(sizeof(timeout)));
    if (-1 == ret)
    {
        httpc_warn("set recv timeout fail.");
    }

    ret = httpc_parse_uri(httpc);
    if (ret < 0)
        goto parse_fail;

    ret = 0;
    return ret;

parse_fail:
    close(httpc->socket);
    safe_free(httpc->host);
    safe_free(httpc->uri);
uri_fail:
    httpc_header_deinit(httpc);
invalid:
    return ret;
}

/**
 * @brief 注销一个 http client 对象
 * 
 * @param httpc: http client 对象
 */
void httpc_deinit(httpc_t *httpc)
{
    close(httpc->socket);
    safe_free(httpc->host);
    safe_free(httpc->uri);
    httpc_header_deinit(httpc);
}