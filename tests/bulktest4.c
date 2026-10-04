
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>

#include <bulk_rdp8_compress.h>
#include <bulk_rdp8_decompress.h>

#define DO_HEXDUMP 1

#if DO_HEXDUMP
#define HEXDUMP(_p, _len) g_hexdump(_p, _len)
#else
#define HEXDUMP(_p, _len)
#endif

#if DO_HEXDUMP

/*****************************************************************************/
/* print a hex dump to stdout*/
static void
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

#endif

#define DATA_DIR "/home/jay/rdp8_data/test3/tmp/rdp8"

int main(int argc, char **argv)
{
    void *comp_han;
    void *decomp_han;
    int error;
    int rv;
    int fd;
    unsigned long index;
    char filename[256];

    char *udata = (char *) malloc(1024 * 1024);
    int udata_bytes;

    char *cdata = (char *) malloc(1024 * 1024);
    int cdata_bytes;

    char *cdata1;
    int cdata_bytes1;
    int cflags1;

    char *udata1;
    int udata_bytes1;

    (void)argc;
    (void)argv;

    rv = 0;
    comp_han = rdp8_compress_create(BULK_PACKET_COMPR_TYPE_RDP8);
    if (comp_han == NULL)
    {
        printf("main: rdp8_compress_create failed\n");
        return 1;
    }

    decomp_han = rdp8_decompress_create(BULK_PACKET_COMPR_TYPE_RDP8);
    if (decomp_han == NULL)
    {
        printf("main: rdp8_decompress_create failed\n");
        return 1;
    }

    for (index = 0; index < 1024 * 1024; index++)
    {
        snprintf(filename, 256, "%s/udata%4.4X.bin", DATA_DIR, (int)index);
        //printf("main: filename %s\n", filename);
        fd = open(filename, O_RDWR);
        if (fd == -1)
        {
            printf("main: error open\n");
            break;;
        }
        udata_bytes = read(fd, udata, 1024 * 1024);
        close(fd);
        //printf("main: udata_bytes %d\n", udata_bytes);

        snprintf(filename, 256, "%s/cdata%4.4X.bin", DATA_DIR, (int)index);
        //printf("main: filename %s\n", filename);
        fd = open(filename, O_RDWR);
        if (fd == -1)
        {
            printf("main: error open\n");
            break;;
        }
        cdata_bytes = read(fd, cdata, 1024 * 1024);
        close(fd);
        //printf("main: cdata_bytes %d\n", cdata_bytes);

        cflags1 = BULK_PACKET_COMPR_TYPE_RDP8 | BULK_PACKET_COMPRESSED;
        error = rdp8_compress(comp_han, &cdata1, &cdata_bytes1, &cflags1, udata, udata_bytes);
        //printf("main: compress error %d cdata_bytes1 %d cflags1 0x%2.2X\n", error, cdata_bytes1, cflags1);

        if (error == RDP8_ERROR_NONE)
        {
            error = rdp8_decompress(decomp_han, cdata1, cdata_bytes1, cflags1, &udata1, &udata_bytes1);
            if (error == 0)
            {
                printf("main: ok udata_bytes %d udata_bytes1 %d\n", udata_bytes, udata_bytes1);
            }
            else
            {
                printf("main: rdp8_decompress failed error %d\n", error);
                return 1;
            }
            if (udata_bytes != udata_bytes1)
            {
                printf("main: udata_bytes missmatch %d %d\n", udata_bytes, udata_bytes1);
                return 1;
            }
            else if (memcmp(udata, udata1, udata_bytes) != 0)
            {
                int index1;
                for (index1 = 0; index1 < udata_bytes; index1++)
                {
                    if (udata[index1] != udata1[index1])
                    {
                        break;
                    }
                }
                printf("main: udata missmatch udata_bytes %d index %d index1 %d\n", udata_bytes, index, index1);
                index1 = udata_bytes;
                if (index1 > 64) index1 = 64;
                HEXDUMP(udata, index1);
                HEXDUMP(udata1, index1);
                return 1;
            }
        }
        else if (error == RDP8_ERROR_NO_COMPRESS)
        {
            printf("main: no compress filename %s cdata_bytes %d\n", filename, cdata_bytes);
        }
        else
        {
            printf("main: error\n");
            return 1;
        }
    }

    rdp8_decompress_destroy(decomp_han);
    rdp8_compress_destroy(comp_han);
    return rv;
}
