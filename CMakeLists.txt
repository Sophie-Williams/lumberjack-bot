project (lumberjack-bot)

cmake_minimum_required(VERSION 3.2)

find_package(X11 REQUIRED)

add_executable(lumberjack lumberjack.cpp)
target_link_libraries(lumberjack ${X11_LIBRARIES})
