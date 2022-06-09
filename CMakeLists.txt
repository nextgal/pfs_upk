cmake_minimum_required (VERSION 3.11)
project (pfs_upk)

set(CMAKE_CXX_STANDARD 17)
add_definitions(-DUNICODE -D_UNICODE)

add_executable(pfs_upk pfs_upk.cpp pack.cpp unpack.cpp xorcrypt.cpp sha1.c MemoryStream.cpp csharp_types.h MemoryStream.h)

if (CMAKE_SYSTEM_NAME MATCHES "Linux")
target_link_libraries(pfs_upk stdc++fs)
endif()