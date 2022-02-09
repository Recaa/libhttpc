/*
 * @Author       : Zeepunt.H
 * @Date         : 2022-02-09 20:43:08
 * @LastEditTime : 2022-02-09 22:00:58
 * Gitee : https://gitee.com/zeepunt
 * Github: https://github.com/Recaa
 *  
 * Copyright (c) 2022 by Zeepunt.H, All Rights Reserved. 
 */

#ifndef __HTTPC_OPTS_H__
#define __HTTPC_OPTS_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stddef.h>
#include <string.h>
#include <unistd.h>

#define LOG printf

#define DEBUG  1
#define INFO   1
#define WARN   1
#define ERROR  1

#define LOG_DEBUG(fmt, ...)														    	\
	do {																		    	\
		LOG("\033[37m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__);	    \
	} while (0)

#define LOG_INFO(fmt, ...)														    	\
	do {																		   	 	\
		LOG("\033[32m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__);	    \
	} while (0)

#define LOG_WARN(fmt, ...)														    	\
	do {																		    	\
		LOG("\033[33m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__);	    \
	} while (0)

#define LOG_ERROR(fmt, ...)														    	\
	do {																		    	\
		LOG("\033[31m[%s:%d]" fmt "\033[0m\n", __func__, __LINE__, ##__VA_ARGS__);	    \
	} while (0)

#if DEBUG
    #define httpc_debug LOG_DEBUG
#else
    #define httpc_debug
#endif

#if INFO
    #define httpc_info LOG_INFO
#else
    #define httpc_info
#endif

#if WARN
    #define httpc_warn LOG_WARN
#else
    #define httpc_warn
#endif

#if ERROR
    #define httpc_error LOG_ERROR
#else
    #define httpc_error
#endif

#define httpc_calloc  calloc
#define httpc_free    free
#define httpc_memset  memset

#define WARN_NULL(p, v)                                                          \
	do {																		 \
		if (NULL == p) {                                                         \
			LOG("\033[31m[%s:%d]pointer is NULL.\033[0m\n", __func__, __LINE__); \
			return v;                                                            \
		}                                                                        \
	} while(0)

#define safe_free(p) 		\
	do {					\
		if(p) {				\
			httpc_free(p);	\
			p = NULL;	    \
		}					\
	} while(0)

#endif