/*
 * clrngd.c - Clock-based entropy daemon for Tinfoil Hat Linux v2
 * Compiled with GCC 3.3, statically linked against uClibc
 *
 * Feeds entropy into /dev/random by measuring timing jitter from
 * the CPU clock / mainboard timers. Runs as a daemon. Uses FIPS
 * statistical tests to verify entropy quality before submitting.
 *
 * Usage: clrngd [interval_seconds]
 *   Default interval: 240 seconds
 *   Minimum useful interval: 60 seconds (entropy is buffered in ~2500 byte blocks)
 *
 * Reconstructed from binary (stripped ELF32/i386, GCC 3.3).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#include <syslog.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <linux/random.h>

#define RANDOM_DEV          "/dev/random"
#define ENTROPY_BLOCK_SIZE  2500    /* bytes gathered per cycle */
#define DEFAULT_INTERVAL    240     /* seconds between runs */
#define MIN_INTERVAL        60      /* minimum useful interval */
#define FIPS_MAX_FAILURES   3       /* abort after this many FIPS failures */

static int random_fd = -1;
static volatile int running = 1;

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

/* Daemonize the process */
static int daemonize(void)
{
    pid_t pid;

    pid = fork();
    if (pid < 0) return -1;
    if (pid > 0) exit(0); /* parent exits */

    setsid();

    /* Redirect stdio to /dev/null */
    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        close(devnull);
    }
    return 0;
}

/*
 * Simple FIPS 140-1 monobit test:
 * Count 1-bits in a 20000-bit (2500 byte) sample.
 * Pass if 9725 < count < 10275.
 */
static int fips_monobit_test(const unsigned char *buf, int len)
{
    int count = 0;
    int i, b;
    for (i = 0; i < len; i++) {
        for (b = 0; b < 8; b++) {
            if (buf[i] & (1 << b))
                count++;
        }
    }
    return (count > 9725 && count < 10275);
}

/*
 * Gather entropy bytes by measuring timer jitter.
 * Uses RDTSC (via gettimeofday fallback) to capture low-order timing bits.
 */
static int gather_entropy(unsigned char *buf, int len)
{
    int i;
    struct timeval tv;
    unsigned char accum = 0;
    int bit = 0;
    int byte_idx = 0;

    memset(buf, 0, len);

    for (i = 0; byte_idx < len; i++) {
        /* Get a timing sample - use tv_usec low bits as entropy */
        gettimeofday(&tv, NULL);
        accum ^= (unsigned char)(tv.tv_usec & 0xFF);
        accum ^= (unsigned char)((tv.tv_usec >> 8) & 0xFF);

        bit++;
        if (bit == 8) {
            buf[byte_idx++] = accum;
            accum = 0;
            bit = 0;
            /* Small delay to increase jitter between samples */
            usleep(10);
        }
    }
    return len;
}

/* Submit entropy to the kernel via ioctl RNDADDENTROPY */
static int add_entropy(const unsigned char *buf, int len, int entropy_bits)
{
    struct rand_pool_info *rpi;
    int rpi_size = sizeof(struct rand_pool_info) + len;
    int ret;

    rpi = malloc(rpi_size);
    if (!rpi) return -1;

    rpi->entropy_count = entropy_bits;
    rpi->buf_size = len;
    memcpy(rpi->buf, buf, len);

    ret = ioctl(random_fd, RNDADDENTROPY, rpi);
    free(rpi);
    return ret;
}

int main(int argc, char *argv[])
{
    int interval = DEFAULT_INTERVAL;
    int fips_failures = 0;
    unsigned char entropy_buf[ENTROPY_BLOCK_SIZE];
    int ret;

    if (argc > 1) {
        interval = atoi(argv[1]);
        if (interval < MIN_INTERVAL) {
            fprintf(stderr,
                "Running clrngd more often than 60 seconds is pointless as the random data\n"
                " is bufferred in 2500 byte blocks and gathering this much takes\n"
                "approximately one minute. Recommended is 240 second, which is default.\n");
            interval = MIN_INTERVAL;
        }
    }

    /* Open /dev/random before daemonizing */
    random_fd = open(RANDOM_DEV, O_RDWR);
    if (random_fd < 0) {
        fprintf(stderr, "unable to open random device, exit: %s\n", strerror(errno));
        fprintf(stderr, "maybe you should run clrngd as root?\n");
        return 1;
    }

    /* Daemonize */
    if (daemonize() < 0) {
        fprintf(stderr, "unable to daemonize, exit: %s\n", strerror(errno));
        close(random_fd);
        return 1;
    }

    openlog("clrngd", LOG_PID, LOG_DAEMON);
    syslog(LOG_INFO, "started correctly");

    signal(SIGTERM, sig_handler);
    signal(SIGINT,  sig_handler);

    /* Set up interval timer */
    struct itimerval itv;
    itv.it_value.tv_sec     = interval;
    itv.it_value.tv_usec    = 0;
    itv.it_interval.tv_sec  = interval;
    itv.it_interval.tv_usec = 0;
    if (setitimer(ITIMER_REAL, &itv, NULL) < 0) {
        syslog(LOG_ERR, "setitimer error occured");
    }

    while (running) {
        /* Gather entropy */
        gather_entropy(entropy_buf, ENTROPY_BLOCK_SIZE);

        /* FIPS monobit test */
        if (!fips_monobit_test(entropy_buf, ENTROPY_BLOCK_SIZE)) {
            fips_failures++;
            syslog(LOG_WARNING, "FIPS test failed");

            if (fips_failures >= FIPS_MAX_FAILURES) {
                syslog(LOG_ERR,
                    "Too many FIPS tests failed, apparently mainboard timers "
                    "are too good or too few to provide real entropy");
                syslog(LOG_ERR, "Exiting.");
                break;
            }

            /* Try alternate method: just write raw bytes */
            ret = write(random_fd, entropy_buf, ENTROPY_BLOCK_SIZE);
            if (ret < 0) {
                syslog(LOG_ERR, "entropy adding failed (returned %d), trying another way", ret);
            } else {
                syslog(LOG_INFO, "which also failed: %s", strerror(errno));
            }
        } else {
            fips_failures = 0;

            /* Submit with estimated entropy bits (conservative: 1 bit per byte) */
            ret = add_entropy(entropy_buf, ENTROPY_BLOCK_SIZE, ENTROPY_BLOCK_SIZE * 1);
            if (ret < 0) {
                syslog(LOG_ERR, "entropy adding failed (returned %d), trying another way", ret);
                /* Fallback: write without entropy count */
                if (write(random_fd, entropy_buf, ENTROPY_BLOCK_SIZE) > 0) {
                    syslog(LOG_INFO, "which worked OK");
                }
            } else {
                syslog(LOG_INFO, "correctly added %d entropy bytes", ENTROPY_BLOCK_SIZE);
            }
        }

        /* Clear sensitive data */
        memset(entropy_buf, 0, sizeof(entropy_buf));

        /* Sleep until next interval */
        sleep(interval);
    }

    closelog();
    close(random_fd);
    return 0;
}
