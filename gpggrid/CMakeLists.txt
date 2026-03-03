# gpggrid - grid-based GPG passphrase entry (anti-keystroke-logger)
# Original source recovered - written by Anonymous + Morten Poulsen, 2002
#
# Two versions:
#   gpggrid-original.c  - exact version shipped on the v1 floppy (messy, bugs noted in comments)
#   gpggrid.c           - cleaned up v1.1 by Morten Poulsen (this is what we build)

add_executable(gpggrid gpggrid.c)

target_compile_definitions(gpggrid PRIVATE _GNU_SOURCE)

# The original has a few authentic bugs worth preserving for reference:
#   - memset(&x, NULL, ...) - passes NULL pointer as int arg
#   - exit; instead of exit(1); - statement with no effect
#   - uninitialized y in getcords()
# Build the original too as a reference target (will warn)
add_executable(gpggrid-original gpggrid-original.c)
target_compile_options(gpggrid-original PRIVATE -w)  # suppress warnings on intentionally buggy original
target_compile_definitions(gpggrid-original PRIVATE _GNU_SOURCE)

install(TARGETS gpggrid DESTINATION bin)
