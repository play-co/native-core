cmake_minimum_required(VERSION 2.8)
project (timestep)

set (PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)

include_directories("${PROJECT_INCLUDE_DIR}")

set (TIMESTEP_SRCS ${PROJECT_SOURCE_DIR}/timestep_events.cpp
${PROJECT_SOURCE_DIR}/timestep_text_data.cpp
${PROJECT_SOURCE_DIR}/timestep_animate.cpp
${PROJECT_SOURCE_DIR}/timestep_image_map.cpp
${PROJECT_SOURCE_DIR}/timestep_view.cpp
)

add_library(${PROJECT_NAME} ${TIMESTEP_SRCS})
