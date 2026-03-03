add_executable(clrngd clrngd.c)

target_compile_definitions(clrngd PRIVATE
    _GNU_SOURCE
)

install(TARGETS clrngd DESTINATION bin)
