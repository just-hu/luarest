cmake_minimum_required (VERSION 2.6)
project (luarest)

set (DEPS_DIR "${CMAKE_SOURCE_DIR}/deps")
set (SRC_DIR "${CMAKE_SOURCE_DIR}/src")

set (UV_INCLUDE_DIR "${DEPS_DIR}/libuv/include")
set (HTTP_PARSER_INCLUDE_DIR "${DEPS_DIR}/http-parser")
set (LUAJIT_INCLUDE_DIR "${DEPS_DIR}/luajit/src")

set (UV_LIBRARIES "${DEPS_DIR}/libuv/Debug/lib/uv.lib")
set (LUAJIT_LIBRARIES "${DEPS_DIR}/luajit/src/lua51.lib")

include_directories("${PROJECT_BINARY_DIR}" "${CMAKE_SOURCE_DIR}/include" "${HTTP_PARSER_INCLUDE_DIR}" 
	"${UV_INCLUDE_DIR}" "${LUAJIT_INCLUDE_DIR}")

if(WIN32)
	set (PLATFORM_LIBS Ws2_32.lib Psapi.lib Iphlpapi.lib)
endif(WIN32)

# HTTP-Parser dependency
set (HTTP_PARSER_SRC ${DEPS_DIR}/http-parser/http_parser.c)
# add library
add_library(http-parser ${HTTP_PARSER_SRC})

# Main
set (LIB_LIST ${UV_LIBRARIES} ${LUAJIT_LIBRARIES} http-parser)
set (LUAREST_SRC ${SRC_DIR}/main.c ${SRC_DIR}/app.c)

add_executable(luarest ${LUAREST_SRC})

# linking
target_link_libraries(luarest ${PLATFORM_LIBS} ${LIB_LIST})
