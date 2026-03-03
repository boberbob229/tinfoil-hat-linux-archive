add_executable(blinker blinker.c)

target_compile_definitions(blinker PRIVATE
    _GNU_SOURCE
)

install(TARGETS blinker DESTINATION bin)
