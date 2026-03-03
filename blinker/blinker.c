/*
 * blinker.c - Keyboard LED controller for Tinfoil Hat Linux
 * Part of morse2led-1.0
 * Compiled with GCC 2.96 (Red Hat Linux 7.1), statically linked against uClibc
 *
 * Manipulates keyboard LEDs (capslock, numlock, scrolllock) using ioctl.
 * Designed to pipe output from text2morse to blink morse code on LEDs.
 *
 * Reconstructed from binary (version 01.01, stripped ELF32/i386).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

/* LED bit flags (from linux/kd.h) */
#define LED_SCR  0x01   /* scroll lock */
#define LED_NUM  0x02   /* num lock    */
#define LED_CAP  0x04   /* caps lock   */

static int console_fd = -1;
static int original_leds = 0;

static void restore_leds(void)
{
    if (console_fd >= 0) {
        ioctl(console_fd, KDSETLED, original_leds);
        close(console_fd);
        console_fd = -1;
    }
}

static void signal_handler(int sig)
{
    (void)sig;
    restore_leds();
    fprintf(stderr, "Program (and child) terminated.\n");
    exit(0);
}

static int open_console(void)
{
    int fd;
    /* Try /dev/console, /dev/tty0, /dev/tty in order */
    const char *devices[] = { "/dev/console", "/dev/tty0", "/dev/tty", NULL };
    int i;
    for (i = 0; devices[i]; i++) {
        fd = open(devices[i], O_RDWR);
        if (fd >= 0)
            return fd;
    }
    return -1;
}

static void usage(void)
{
    fprintf(stderr,
        "USAGE: blinker [-help] [-d millisecs] [-ld millisecs]\n"
        " [-cd millisecs] { \"tokens\" | - } ( - for stdin )\n"
        "HELP:\n"
        "This program manipulates keyboard lights.\n"
        "  It accepts a stream of characters either from the command line\n"
        "  or STDIN. Leds are represented by characters:\n"
        "    \"c\" (capslock), \"n\" (number lock), and \"s\" (scroll lock)\n"
        "  The first occurence of a character activates the led,\n"
        "  the next deactivates it. The character \"d\" will signal the\n"
        "  program to sleep for the time specified by the -cd switch.\n"
        "  -      read characters from STDIN, ignoring cmdline string\n"
        "  -d x   sleep x msecs after interpreting each character\n"
        "  -ld y  run in loop, sleeps y milsecs between each loop\n"
        "         * -ld will only work on string passed from cmdline\n"
        "  -cd z  sleep z msecs for every \"d\" in character sequence\n"
        "    blinker -cd 1000 cnsdddcns\n"
        "  This will turn all leds on, wait 3 seconds, then turn all leds off.\n"
    );
}

static void msleep(int ms)
{
    if (ms > 0)
        usleep((useconds_t)ms * 1000);
}

int main(int argc, char *argv[])
{
    int delay_ms     = 0;   /* -d  : delay after each token */
    int loop_delay   = 0;   /* -ld : loop delay             */
    int char_delay   = 0;   /* -cd : delay per 'd' token    */
    int use_stdin    = 0;
    const char *token_str = NULL;
    int current_leds = 0;
    int i;

    /* Parse arguments */
    for (i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-help") == 0 || strcmp(argv[i], "--help") == 0) {
            usage();
            return 0;
        } else if (strcmp(argv[i], "-d") == 0 && i + 1 < argc) {
            delay_ms = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-ld") == 0 && i + 1 < argc) {
            loop_delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-cd") == 0 && i + 1 < argc) {
            char_delay = atoi(argv[++i]);
        } else if (strcmp(argv[i], "-") == 0) {
            use_stdin = 1;
        } else {
            token_str = argv[i];
        }
    }

    if (!use_stdin && !token_str) {
        usage();
        return 1;
    }

    /* Open console for ioctl */
    console_fd = open_console();
    if (console_fd < 0) {
        fprintf(stderr, "blinker: KDGETLED\nMaybe stdin is not a VT?\n");
        return 1;
    }

    /* Save original LED state */
    if (ioctl(console_fd, KDGETLED, &original_leds) < 0) {
        fprintf(stderr, "Error reading current led setting.\n");
        close(console_fd);
        return 1;
    }
    current_leds = original_leds;

    /* Register cleanup */
    if (atexit(restore_leds) != 0) {
        fprintf(stderr, "blinker: atexit() failed\n");
        close(console_fd);
        return 1;
    }
    signal(SIGINT,  signal_handler);
    signal(SIGTERM, signal_handler);

    /* Process token stream */
    do {
        FILE *src = use_stdin ? stdin : NULL;
        const char *p = token_str;
        int ch;

        while (1) {
            if (use_stdin) {
                ch = fgetc(src);
                if (ch == EOF) break;
            } else {
                if (!p || *p == '\0') break;
                ch = *p++;
            }

            switch (ch) {
                case 'c':
                    current_leds ^= LED_CAP;
                    break;
                case 'n':
                    current_leds ^= LED_NUM;
                    break;
                case 's':
                    current_leds ^= LED_SCR;
                    break;
                case 'd':
                    msleep(char_delay);
                    break;
                default:
                    if (ch > ' ') {
                        fprintf(stderr, "ERROR: blinker: illegal token: %c\n", ch);
                        restore_leds();
                        return 1;
                    }
                    break;
            }

            /* Check LED value is valid (3 bits only) */
            if (current_leds & ~(LED_CAP | LED_NUM | LED_SCR)) {
                fprintf(stderr, "blinker: wrong led-value\n");
                restore_leds();
                return 1;
            }

            if (ioctl(console_fd, KDSETLED, current_leds) < 0) {
                fprintf(stderr, "blinker: KDSETLED\n");
                restore_leds();
                return 1;
            }

            msleep(delay_ms);
        }

        if (loop_delay > 0 && !use_stdin)
            msleep(loop_delay);

    } while (loop_delay > 0 && !use_stdin);

    fprintf(stderr, "Bye-Bye !\n");
    return 0;
}
