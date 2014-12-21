project(software-renderer)

add_executable(software-renderer main.cpp)

install(TARGETS software-renderer RUNTIME DESTINATION bin)
