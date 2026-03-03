add_executable(ree ree.c)

target_compile_definitions(ree PRIVATE
    _GNU_SOURCE
)

install(TARGETS ree DESTINATION bin)
