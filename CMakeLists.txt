cmake_minimum_required(VERSION 3.5)

project(posix_client LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(X11 REQUIRED)
find_package(PNG REQUIRED)

add_executable(posix_client main.cpp image.h socket.h)

target_include_directories(posix_client PUBLIC ${X11_INCLUDE_DIR} ${PNG_INCLUDE_DIRS})
target_link_libraries(posix_client ${X11_LIBRARIES} ${PNG_LIBRARIES})

install(TARGETS posix_client
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR})
