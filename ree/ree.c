/*
 * ree.c - ROM/BIOS memory reader for Tinfoil Hat Linux v2
 * Compiled with GCC 3.3, statically linked against uClibc
 *
 * Scans /dev/mem for ROM regions (Option ROMs, System BIOS) and dumps
 * them to files named <hex_address>.rom. THL uses this to detect BIOS
 * tampering by MD5-hashing all ROMs at boot and comparing against a
 * stored hash on the floppy.
 *
 * Usage: ree
 *   Must be run as root (needs /dev/mem access).
 *   Output files: <address>.rom (e.g. f0000.rom for system BIOS)
 *
 * ROM regions scanned:
 *   0xC0000 - 0xFFFFF (option ROMs and system BIOS area)
 *   System BIOS always at 0xF0000 (65536 bytes)
 *
 * Reconstructed from binary (stripped ELF32/i386, GCC 3.3).
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/mman.h>

#define MEM_DEV         "/dev/mem"
#define ROM_START       0xC0000     /* Start of ROM/option ROM space */
#define ROM_END         0xFFFFF     /* End of ROM space (1MB boundary) */
#define BIOS_ADDR       0xF0000     /* System BIOS always here */
#define BIOS_SIZE       65536       /* System BIOS is 64KB */
#define ROM_ALIGN       0x200       /* ROMs are aligned on 512-byte boundaries */
#define ROM_HEADER_MAGIC 0xAA55     /* PCI option ROM magic (little-endian) */
#define SCAN_STEP       0x200       /* Scan every 512 bytes */

/* Compute a simple 8-bit checksum of a ROM */
static int rom_checksum(const unsigned char *buf, int len)
{
    int sum = 0;
    int i;
    for (i = 0; i < len; i++)
        sum += buf[i];
    return sum & 0xFF;
}

/* Write a ROM region to a .rom file */
static int dump_rom(const unsigned char *mem_base, unsigned int addr, int size,
                    int checksum)
{
    char filename[32];
    FILE *f;
    snprintf(filename, sizeof(filename), "%x.rom", addr);
    f = fopen(filename, "wb");
    if (!f) {
        fprintf(stderr, "could not write %s\n", filename);
        return -1;
    }
    fwrite(mem_base + addr, 1, size, f);
    fclose(f);
    fprintf(stdout, "Found something at %x (%d bytes) (checksum %d)\n",
            addr, size, checksum);
    return 0;
}

int main(int argc, char *argv[])
{
    int mem_fd;
    unsigned char *mem;
    const size_t map_size = 0x100000; /* Map first 1MB */
    unsigned int addr;
    (void)argc; (void)argv;

    mem_fd = open(MEM_DEV, O_RDONLY);
    if (mem_fd < 0) {
        fprintf(stderr, "could not open /dev/mem\n");
        fprintf(stderr, "(are you root?)\n");
        return 1;
    }

    mem = mmap(NULL, map_size, PROT_READ, MAP_SHARED, mem_fd, 0);
    if (mem == MAP_FAILED) {
        fprintf(stderr, "could not mmap /dev/mem\n");
        close(mem_fd);
        return 1;
    }

    /* Scan for option ROMs (magic 0x55AA at start) and system BIOS */
    fprintf(stdout, "\033[1GPlease wait, scanning... %x", ROM_START);
    fflush(stdout);

    for (addr = ROM_START; addr < ROM_END; addr += SCAN_STEP) {
        const unsigned char *p = mem + addr;
        int size, checksum;

        fprintf(stdout, "\033[1GPlease wait, scanning... %x", addr);
        fflush(stdout);

        /* Check for option ROM magic (0x55, 0xAA) */
        if (p[0] == 0x55 && p[1] == 0xAA) {
            /* Size is in 512-byte units at offset 2 */
            size = p[2] * 512;
            if (size == 0) size = SCAN_STEP;
            if (addr + size > (unsigned)map_size) size = map_size - addr;

            checksum = rom_checksum(p, size);
            dump_rom(mem, addr, size, checksum);
        }
    }

    /* Always dump the system BIOS at F0000 */
    {
        int checksum = rom_checksum(mem + BIOS_ADDR, BIOS_SIZE);
        char filename[32];
        FILE *f;

        snprintf(filename, sizeof(filename), "%x.rom", (unsigned)BIOS_ADDR);
        f = fopen(filename, "wb");
        if (f) {
            fwrite(mem + BIOS_ADDR, 1, BIOS_SIZE, f);
            fclose(f);
        }
        fprintf(stdout, "\nFound system bios at F0000 (65536) (checksum %d)\n",
                checksum);
    }

    munmap(mem, map_size);
    close(mem_fd);
    return 0;
}
