project(NATIVE_CORE)
cmake_minimum_required(VERSION 2.8)


include_directories("${NATIVE_CORE_SOURCE_DIR}/deps" "${NATIVE_CORE_SOURCE_DIR}/include")
#add_library(native_core ${core_src} ${deps_src} ${timestep_src})

add_subdirectory(src/core)
add_subdirectory("src/image-cache")
add_subdirectory(src/timestep)
