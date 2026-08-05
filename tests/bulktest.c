
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bulk_rdp8_compress.h>
#include <bulk_rdp8_decompress.h>

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
    char *cdata;
    int cdata_bytes;
    int flags;
    int error;
    int rv;
    int index;

    (void)argc;
    (void)argv;

    char data[][128] =
        {
            "\x01\x02\xFF\x65\x65\x65\x65\x65",
            "The quick brown fox jumps over the lazy dog",
            "ABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABCABC"
        };
    int expected_error[] = {0, 1, 0};
    int data_bytes;
    char *decomp_data;
    int decomp_data_bytes;

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

    for (index = 0; index < 3; index++)
    {
        rv = 1;
        data_bytes = strlen(data[index]);
        error = rdp8_compress(comp_han, &cdata, &cdata_bytes, &flags, data[index], data_bytes);
        printf("main: rdp8_compress 0 rv %d\n", error);
        if ((error != 0) && (expected_error[index] != 0))
        {
            /* this is ok too */
            printf("main: compress failed but that was expected\n");
            rv = 0;
        }
        if ((error == 0) && (expected_error[index] != 0))
        {
            /* this is ok too */
            printf("main: compress succeded but that was not expected\n");
            error = 1;
        }
        if (error == 0)
        {
            printf("main: cdata_bytes %d\n", cdata_bytes);
            g_hexdump(cdata, cdata_bytes);
            /* now decompress */
            error = rdp8_decompress(decomp_han, cdata, cdata_bytes, flags,
                                    &decomp_data, &decomp_data_bytes);
            printf("main: rdp8_decompress 0 rv %d\n", error);
            if (error == 0)
            {
                printf("main: decomp_data_bytes %d\n", decomp_data_bytes);
                g_hexdump(decomp_data, decomp_data_bytes);
                if (data_bytes == decomp_data_bytes)
                {
                    if (memcmp(decomp_data, data[index], decomp_data_bytes) == 0)
                    {
                        printf("main: match\n");
                        rv = 0;
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

