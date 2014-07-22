cmake_minimum_required(VERSION 2.8)
project (imagecache)

set (PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

MESSAGE( STATUS ${NATIVE_CORE_SOURCE_DIR}/deps)
include_directories("${PROJECT_INCLUDE_DIR}")
include_directories("${NATIVE_CORE_SOURCE_DIR}/deps" "${NATIVE_CORE_SOURCE_DIR}/include")

set (IMAGE_CACHE_SRCS ${PROJECT_SOURCE_DIR}/src/image_cache.c
                      ${PROJECT_SOURCE_DIR}/src/murmur.c)

add_library(${PROJECT_NAME} ${IMAGE_CACHE_SRCS})
