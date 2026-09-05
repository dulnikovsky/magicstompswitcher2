cmake_minimum_required(VERSION 3.16)

project(magicstompswitcher2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(magicstompswitcher2 main.cpp magicstomp.h)

target_link_libraries(magicstompswitcher2 PRIVATE asound)
install(TARGETS magicstompswitcher2
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
