cmake_minimum_required(VERSION 3.16)

project(magicstompswitcher2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(WITH_SSD1306_DISPLAY "Determinates build with or without SSD 1306 OLED display support" OFF)

add_executable(magicstompswitcher2 main.cpp magicstomp.h)

if(${WITH_SSD1306_DISPLAY})
    add_compile_definitions(WITH_SSD1306_DISPLAY)
    target_sources(magicstompswitcher2 PRIVATE ssd1306_display.h ssd1306_display.cpp)
endif()

target_link_libraries(magicstompswitcher2 PRIVATE asound)
if(${WITH_SSD1306_DISPLAY})
    target_link_libraries(magicstompswitcher2 PRIVATE ssd1306_i2c)
endif()


install(TARGETS magicstompswitcher2
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
