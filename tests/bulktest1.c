
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <bulk_rdp8_compress.h>
#include <bulk_rdp8_decompress.h>

#define DO_HEXDUMP 0

#if DO_HEXDUMP
#define HEXDUMP(_p, _len) g_hexdump(_p, _len)
#else
#define HEXDUMP(_p, _len)
#endif

/*****************************************************************************/
/* print a hex dump to stdout*/
void
g_hexdump(const void *p, int len)
{
    unsigned char *line;
    int i;
    int thisline;
    int offset;

    line = (unsigned char *)p;
    offset = 0;

    while (offset < len)
    {
        printf("%04x ", offset);
        thisline = len - offset;

        if (thisline > 16)
        {
            thisline = 16;
        }

        for (i = 0; i < thisline; i++)
        {
            printf("%02x ", line[i]);
        }

        for (; i < 16; i++)
        {
            printf("   ");
        }

        for (i = 0; i < thisline; i++)
        {
            printf("%c", (line[i] >= 0x20 && line[i] < 0x7f) ? line[i] : '.');
        }

        printf("%s", "\n");
        offset += thisline;
        line += thisline;
    }
}

int main(int argc, char **argv)
{
    void *comp_han;
    void *decomp_han;
    char *lcdata;
    int cdata_bytes;
    char *data;
    int data_bytes;
    int cflags;
    int error;
    int rv;
    int fd;
    int udata_bytes;
    unsigned long index;
    char filename[256];
    char *udata = malloc(1024 * 1024);
    char *cdata = malloc(1024 * 1024);

    char *udata1;
    int udata_bytes1;

    (void)argc;
    (void)argv;

    comp_han = rdp8_compress_create(BULK_PACKET_COMPR_TYPE_RDP8);
    if (comp_han == NULL)
    {
        printf("main: rdp8_compress_create failed\n");
    }

    decomp_han = rdp8_decompress_create(BULK_PACKET_COMPR_TYPE_RDP8);
    if (decomp_han == NULL)
    {
        printf("main: rdp8_decompress_create failed\n");
    }

    for (index = 0; index < 6; index++)
    {
        snprintf(filename, 256, "udata%4.4X.bin", index);
        printf("main: filename %s\n", filename);
        fd = open(filename, O_RDWR);
        if (fd == -1)
        {
            continue;
        }
        udata_bytes = read(fd, udata, 1024 * 1024);
        close(fd);
        printf("main: udata_bytes %d\n", udata_bytes);
        cflags = BULK_PACKET_COMPR_TYPE_RDP8 | BULK_PACKET_COMPRESSED;
        error = rdp8_compress(comp_han, &cdata, &cdata_bytes, &cflags, udata, udata_bytes);
        printf("main: compress error %d cdata_bytes %d cflags 0x%2.2X\n", error, cdata_bytes, cflags);
        error = rdp8_decompress(decomp_han, cdata, cdata_bytes, cflags, &udata1, &udata_bytes1);
        printf("main: decompress error %d udata_bytes1 %d\n", error, udata_bytes1);
        if (udata_bytes != udata_bytes1)
        {
            printf("main: udata_bytes != udata_bytes1\n");
            continue;
        }
        if (memcmp(udata, udata1, udata_bytes) != 0)
        {
            printf("memcmp(udata, udata1, udata_bytes) != 0\n");
            continue;
        }
        printf("main: match\n\n\n");

    }

    rdp8_decompress_destroy(decomp_han);
    rdp8_compress_destroy(comp_han);
    return rv;
}
