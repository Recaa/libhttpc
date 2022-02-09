# set file path
aux_source_directory(. CUR_DIR_SRCS)

# build library
add_library(httpc STATIC ${CUR_DIR_SRCS})