cmake_minimum_required(VERSION 2.8)
project (core)

set (PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

include_directories("${PROJECT_INCLUDE_DIR}")
set (CORE_SRCS ${PROJECT_SOURCE_DIR}/events.c
${PROJECT_SOURCE_DIR}/object_pool.c
${PROJECT_SOURCE_DIR}/texture_2d.c
${PROJECT_SOURCE_DIR}/benchmark.c
${PROJECT_SOURCE_DIR}/geometry.c
${PROJECT_SOURCE_DIR}/rgba.c
${PROJECT_SOURCE_DIR}/texture_manager.c
${PROJECT_SOURCE_DIR}/config.c
${PROJECT_SOURCE_DIR}/graphics_utils.c
${PROJECT_SOURCE_DIR}/tealeaf_canvas.c
${PROJECT_SOURCE_DIR}/timer.c
${PROJECT_SOURCE_DIR}/core.c
${PROJECT_SOURCE_DIR}/image_loader.c
${PROJECT_SOURCE_DIR}/tealeaf_context.c
${PROJECT_SOURCE_DIR}/url_loader.c
${PROJECT_SOURCE_DIR}/draw_textures.c
${PROJECT_SOURCE_DIR}/image_writer.c
${PROJECT_SOURCE_DIR}/tealeaf_shaders.c)

add_library(${PROJECT_NAME} ${CORE_SRCS})
