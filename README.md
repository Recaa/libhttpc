# 一、介绍

​	libhttpc 是在学习 HTTP 协议的过程中，用来练手的一个小项目（加深理解）。
   
   目前支持的平台：
   1） Linux 平台。

​	目前所支持的功能：（参考 example 目录）
​	1）支持 GET、POST 方法
​	2）支持 chunk 传输

# 二、编译和执行

​	该工程是基于 CMake 来编译的，需要先安装 CMake 工具。

```shell
# ubuntu
sudo apt-get update
sudo apt-get install -y build-essential
sudo apt-get install -y cmake
```

​	Demo 编译和执行命令如下：

```shell
cd example

# 修改该目录下的 CMakeLists.txt 文件，选择 Demo
add_executable(Demo get_request.c)
# add_executable(Demo post_request.c)

# 编译
./make.sh
# 执行 demo
./build/Demo
```

# 三、移植

   1）直接拷贝 src 目录
   2）如果使用的是 PSRAM，可修改 src/httpc_opts.h 文件
