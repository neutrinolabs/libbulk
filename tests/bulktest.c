
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
    int lcdata_bytes;
    char *ldata;
    int ldata_bytes;
    int flags;
    int error;
    int rv;
    unsigned long index;

    (void)argc;
    (void)argv;

    unsigned char data[][128] =
    {
        "\x01\x02\xFF\x65\x65\x65\x65\x65",
        "The quick brown fox jumps over the lazy dog",
        "ABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABC",
        "The quick brown fox jumps over the lazy dogThe quick brown fox jumps over the lazy dog"
    };
    int data_bytes [] =
    {
        8,
        43,
        60,
        86
    };
    unsigned char cdata[][128] =
    {
        "\xCE\x9B\x19\x62\x18\x00",
        "NA",
        "\x20\x90\x88\x71\x1F\xB2\x01",
        "NA"
    };
    /* -1 in this list means don't check */
    int cdata_bytes [] =
    {
        6,
        0,
        7,
        -1
    };
    int expected_error[] =
    {
        RDP8_ERROR_NONE,
        RDP8_ERROR_NO_COMPRESS,
        RDP8_ERROR_NONE,
        RDP8_ERROR_NONE
    };

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

    for (index = 0; index < sizeof(data) / sizeof(data[0]); index++)
    {
        printf("main: ----------------------------------------------------\n");
        printf("main: ----------------------------------------------------\n");
        printf("main: performing test %ld\n", index);
        rv = 1;
        flags = BULK_PACKET_COMPR_TYPE_RDP8 | BULK_PACKET_FLUSHED;
        error = rdp8_compress(comp_han, &lcdata, &lcdata_bytes, &flags,
                              (const char *) (data[index]), data_bytes[index]);
        printf("main: rdp8_compress rv %d\n", error);
        if ((error != RDP8_ERROR_NONE) && (error == expected_error[index]))
        {
            /* this is ok */
            printf("main: compress failed but that was expected\n");
            rv = 0;
        }
        if ((error == RDP8_ERROR_NONE) && (expected_error[index] != RDP8_ERROR_NONE))
        {
            /* this is not ok */
            printf("main: compress succeeded but that was not expected\n");
            error = 1;
        }
        /* check against expected cdata_bytes */
        if ((error == 0) && (cdata_bytes[index] != -1) &&
            (lcdata_bytes != cdata_bytes[index]))
        {
            printf("main: compress succeeded but does not match expected\n");
            error = 1;
        }
        /* check against expected cdata */
        if ((error == 0) && (cdata_bytes[index] != -1) &&
            (memcmp(lcdata, cdata[index], lcdata_bytes) != 0))
        {
            printf("main: compress succeeded but does not match expected\n");
            error = 1;
        }
        if (error == 0)
        {
            printf("main: cdata_bytes %d\n", lcdata_bytes);
            HEXDUMP(lcdata, lcdata_bytes);
            /* now decompress */
            error = rdp8_decompress(decomp_han, lcdata, lcdata_bytes, flags,
                                    &ldata, &ldata_bytes);
            printf("main: rdp8_decompress 0 rv %d\n", error);
            if (error == 0)
            {
                printf("main: ldata_bytes %d\n", ldata_bytes);
                HEXDUMP(ldata, ldata_bytes);
                if (data_bytes[index] == ldata_bytes)
                {
                    if (memcmp(ldata, data[index], ldata_bytes) == 0)
                    {
                        printf("main: compare decompress to original, match\n");
                        rv = 0;
                    }
                }
            }
        }
        /* second run to make sure history buffer is used and cdata is smaller */
        if (error == 0)
        {
            lcdata_bytes = 0;
            error = rdp8_compress(comp_han, &lcdata, &lcdata_bytes, &flags,
                                  (const char *) (data[index]),
                                  data_bytes[index]);
            printf("main: second run, rdp8_compress rv %d\n", error);
            if (error == 0)
            {
                printf("main: second run, cdata_bytes %d\n", lcdata_bytes);
                HEXDUMP(lcdata, lcdata_bytes);
                /* now decompress */
                error = rdp8_decompress(decomp_han, lcdata, lcdata_bytes, flags,
                                        &ldata, &ldata_bytes);
                printf("main: second run, rdp8_decompress 0 rv %d\n", error);
                if (error == 0)
                {
                    printf("main: second run, ldata_bytes %d\n", ldata_bytes);
                    HEXDUMP(ldata, ldata_bytes);
                    if (data_bytes[index] == ldata_bytes)
                    {
                        if (memcmp(ldata, data[index], ldata_bytes) == 0)
                        {
                            printf("main: second run, compare second decompress to original, match\n");
                            rv = 0;
                        }
                    }
                }
            }
        }
        if (rv != 0)
        {
            rdp8_decompress_destroy(decomp_han);
            rdp8_compress_destroy(comp_han);
            return rv;
        }
    }
    rdp8_decompress_destroy(decomp_han);
    rdp8_compress_destroy(comp_han);
    return rv;
}
