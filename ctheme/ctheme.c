/*
 * ctheme.c - Console palette theme loader for Tinfoil Hat Linux
 * From http://sourceforge.net/projects/ctheme
 * Compiled with GCC 2.96 (Red Hat Linux 7.1), statically linked against uClibc
 *
 * Reads theme script files and manipulates the Linux VGA console palette.
 * Used by THL to reduce screen contrast, making it harder to photograph
 * or shoulder-surf the display.
 *
 * Script syntax:
 *   set { <index> RRGGBB ... }   - set palette entries
 *   blend <idx> <steps> RRGGBB RRGGBB - blend between two colors
 *   fblend <idx> <steps> RRGGBB RRGGBB - flash blend
 *   flash <idx> <steps> RRGGBB RRGGBB - flash between colors
 *   sleep <ms>                   - delay
 *   reset                        - reset to default palette
 *
 * Reconstructed from binary (stripped ELF32/i386, GCC 2.96).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <errno.h>

#define MAX_PALETTE  16   /* Linux console has 16 colors */

typedef struct {
    unsigned char r, g, b;
} Color;

static Color palette[MAX_PALETTE];

/* Detect terminal type for escape sequence selection */
static int is_xterm(void)
{
    const char *term = getenv("TERM");
    return (term && strcmp(term, "xterm") == 0);
}

/* Set a single palette entry */
static void set_color(int idx, unsigned char r, unsigned char g, unsigned char b)
{
    if (idx < 0 || idx >= MAX_PALETTE) return;
    palette[idx].r = r;
    palette[idx].g = g;
    palette[idx].b = b;

    if (is_xterm()) {
        /* xterm uses OSC 4 sequence */
        printf("\033]4;%d;#%02x%02x%02x\007", idx, r, g, b);
    } else {
        /* Linux console uses ESC ] P sequence */
        printf("\033]P%1X%02X%02X%02X", idx, r, g, b);
    }
    fflush(stdout);
}

/* Parse a 6-digit hex color string RRGGBB into r,g,b */
static int parse_color(const char *s, unsigned char *r, unsigned char *g, unsigned char *b)
{
    unsigned int rv, gv, bv;
    if (sscanf(s, "%2x%2x%2x", &rv, &gv, &bv) != 3)
        return 0;
    *r = (unsigned char)rv;
    *g = (unsigned char)gv;
    *b = (unsigned char)bv;
    return 1;
}

/* Blend: set palette entry idx, stepping <steps> times from color1 to color2 */
static void blend_colors(int idx, int steps,
                          unsigned char r1, unsigned char g1, unsigned char b1,
                          unsigned char r2, unsigned char g2, unsigned char b2)
{
    int i;
    for (i = 0; i <= steps; i++) {
        unsigned char r = r1 + (int)(r2 - r1) * i / steps;
        unsigned char g = g1 + (int)(g2 - g1) * i / steps;
        unsigned char b = b1 + (int)(b2 - b1) * i / steps;
        set_color(idx, r, g, b);
        usleep(20000); /* 20ms between steps */
    }
}

/* Flash: alternates between two colors for <steps> cycles */
static void flash_colors(int idx, int steps,
                          unsigned char r1, unsigned char g1, unsigned char b1,
                          unsigned char r2, unsigned char g2, unsigned char b2)
{
    int i;
    for (i = 0; i < steps; i++) {
        set_color(idx, r1, g1, b1);
        usleep(50000);
        set_color(idx, r2, g2, b2);
        usleep(50000);
    }
}

/* Reset palette to Linux console defaults */
static void reset_palette(void)
{
    /* Standard Linux console palette */
    static const unsigned char defaults[16][3] = {
        {0x00,0x00,0x00}, /* 0: black */
        {0xAA,0x00,0x00}, /* 1: dark red */
        {0x00,0xAA,0x00}, /* 2: dark green */
        {0xAA,0x55,0x00}, /* 3: brown */
        {0x00,0x00,0xAA}, /* 4: dark blue */
        {0xAA,0x00,0xAA}, /* 5: dark magenta */
        {0x00,0xAA,0xAA}, /* 6: dark cyan */
        {0xAA,0xAA,0xAA}, /* 7: light gray */
        {0x55,0x55,0x55}, /* 8: dark gray */
        {0xFF,0x55,0x55}, /* 9: bright red */
        {0x55,0xFF,0x55}, /* 10: bright green */
        {0xFF,0xFF,0x55}, /* 11: yellow */
        {0x55,0x55,0xFF}, /* 12: bright blue */
        {0xFF,0x55,0xFF}, /* 13: bright magenta */
        {0x55,0xFF,0xFF}, /* 14: bright cyan */
        {0xFF,0xFF,0xFF}, /* 15: white */
    };
    int i;
    for (i = 0; i < 16; i++)
        set_color(i, defaults[i][0], defaults[i][1], defaults[i][2]);
}

/* Process a single theme script file (or stdin) */
static int process_theme(FILE *f, const char *filename)
{
    char line[256];
    long lineno = 0;

    while (fgets(line, sizeof(line), f)) {
        char *p = line;
        lineno++;

        /* Skip whitespace and comments */
        while (*p && isspace((unsigned char)*p)) p++;
        if (*p == '#' || *p == '\0') continue;

        if (strncmp(p, "set", 3) == 0 && isspace((unsigned char)p[3])) {
            /* set { <idx> RRGGBB ... } */
            p += 3;
            while (*p && *p != '{') p++;
            if (*p == '{') p++;
            while (*p) {
                unsigned int idx;
                unsigned int rv, gv, bv;
                while (*p && isspace((unsigned char)*p)) p++;
                if (*p == '}') break;
                if (sscanf(p, " %1x %2x%2x%2x", &idx, &rv, &gv, &bv) == 4) {
                    set_color((int)idx,
                              (unsigned char)rv,
                              (unsigned char)gv,
                              (unsigned char)bv);
                }
                /* advance past this entry */
                while (*p && !isspace((unsigned char)*p)) p++;
                while (*p && isspace((unsigned char)*p)) p++;
                while (*p && !isspace((unsigned char)*p)) p++;
            }

        } else if (strncmp(p, "blend", 5) == 0 && isspace((unsigned char)p[5])) {
            unsigned int idx, steps;
            unsigned int r1,g1,b1,r2,g2,b2;
            if (sscanf(p, " blend %1hx %i %2hx%2hx%2hx %2hx%2hx%2hx",
                       &idx, &steps, &r1,&g1,&b1, &r2,&g2,&b2) == 8) {
                blend_colors((int)idx, (int)steps,
                             r1,g1,b1, r2,g2,b2);
            } else {
                fprintf(stderr, "%s: error (%s: line %li) - blend: invalid palette index\n",
                        "ctheme", filename, lineno);
            }

        } else if (strncmp(p, "fblend", 6) == 0 && isspace((unsigned char)p[6])) {
            unsigned int idx, steps;
            unsigned int r1,g1,b1,r2,g2,b2;
            if (sscanf(p, " fblend %1hx %i %2hx%2hx%2hx %2hx%2hx%2hx",
                       &idx, &steps, &r1,&g1,&b1, &r2,&g2,&b2) == 8) {
                blend_colors((int)idx, (int)steps, r1,g1,b1, r2,g2,b2);
                blend_colors((int)idx, (int)steps, r2,g2,b2, r1,g1,b1);
            }

        } else if (strncmp(p, "flash", 5) == 0 && isspace((unsigned char)p[5])) {
            unsigned int idx, steps;
            unsigned int r1,g1,b1,r2,g2,b2;
            if (sscanf(p, " flash %1hx %i %2hx%2hx%2hx %2hx%2hx%2hx",
                       &idx, &steps, &r1,&g1,&b1, &r2,&g2,&b2) == 8) {
                flash_colors((int)idx, (int)steps, r1,g1,b1, r2,g2,b2);
            }

        } else if (strncmp(p, "fflash", 6) == 0 && isspace((unsigned char)p[6])) {
            unsigned int idx, steps;
            unsigned int r1,g1,b1,r2,g2,b2;
            if (sscanf(p, " fflash %1hx %i %2hx%2hx%2hx %2hx%2hx%2hx",
                       &idx, &steps, &r1,&g1,&b1, &r2,&g2,&b2) == 8) {
                flash_colors((int)idx, (int)steps, r1,g1,b1, r2,g2,b2);
                flash_colors((int)idx, (int)steps, r2,g2,b2, r1,g1,b1);
            }

        } else if (strncmp(p, "sleep", 5) == 0 && isspace((unsigned char)p[5])) {
            int ms = 0;
            sscanf(p + 5, " %i ", &ms);
            if (ms > 0)
                usleep((useconds_t)ms * 1000);

        } else if (strncmp(p, "reset", 5) == 0) {
            reset_palette();

        } else if (strncmp(p, "end", 3) == 0) {
            break;
        }
        /* else: unknown command, skip */
    }
    return 0;
}

static void usage(const char *prog)
{
    fprintf(stderr,
        "Usage: %s [FILE]...\n"
        "\tinput from standard in.\n"
        "\tConsole theme loader. Processes input files and sets up\n"
        "\tconsole palette. If no input files are specified, takes\n"
        "\tinput from standard in.\n",
        prog);
}

int main(int argc, char *argv[])
{
    int i;

    if (argc > 1 && (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-help") == 0)) {
        usage(argv[0]);
        return 0;
    }

    if (argc < 2) {
        /* Read from stdin */
        process_theme(stdin, "stdin");
    } else {
        for (i = 1; i < argc; i++) {
            FILE *f = fopen(argv[i], "r");
            if (!f) {
                fprintf(stderr, "%s: error - cannot open file %s\n",
                        argv[0], argv[i]);
                return 1;
            }
            process_theme(f, argv[i]);
            fclose(f);
        }
    }

    return 0;
}
