
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <bulk_rdp8_compress.h>
#include <bulk_rdp8_decompress.h>

#define CHUNK_SIZE 65000
#define HIST_BUF_LEN 2500000
#define NUM_CHUNKS 60

int main(int argc, char **argv)
{
    void *comp_han;
    void *decomp_han;
    char *cdata;
    int cdata_bytes;
    char *ddata;
    int ddata_bytes;
    int flags;
    int error;
    int i;
    int j;
    int total_bytes;
    unsigned char chunk[CHUNK_SIZE];

    (void)argc;
    (void)argv;

    comp_han = rdp8_compress_create(BULK_PACKET_COMPR_TYPE_RDP8);
    if (comp_han == NULL)
    {
        printf("rdp8_compress_create failed\n");
        return 1;
    }

    decomp_han = rdp8_decompress_create(BULK_PACKET_COMPR_TYPE_RDP8);
    if (decomp_han == NULL)
    {
        printf("rdp8_decompress_create failed\n");
        rdp8_compress_destroy(comp_han);
        return 1;
    }

    total_bytes = 0;
    for (i = 0; i < NUM_CHUNKS; i++)
    {
        /* fill chunk with a repeating pattern that varies per chunk */
        for (j = 0; j < CHUNK_SIZE; j++)
        {
            chunk[j] = (unsigned char)((j * 7 + i * 13) & 0xFF);
        }

        flags = BULK_PACKET_COMPR_TYPE_RDP8 | BULK_PACKET_COMPRESSED;
        if (i == 0)
        {
            flags |= BULK_PACKET_FLUSHED;
        }
        error = rdp8_compress(comp_han, &cdata, &cdata_bytes, &flags,
                              (const char *)chunk, CHUNK_SIZE);
        total_bytes += CHUNK_SIZE;

        if (error == RDP8_ERROR_NO_COMPRESS)
        {
            printf("chunk %d: no compress (%d total bytes)\n",
                   i, total_bytes);
            continue;
        }
        if (error != RDP8_ERROR_NONE)
        {
            printf("chunk %d: compress error %d\n", i, error);
            rdp8_decompress_destroy(decomp_han);
            rdp8_compress_destroy(comp_han);
            return 1;
        }

        printf("chunk %d: %d -> %d bytes (%d total, %s wrap)\n",
               i, CHUNK_SIZE, cdata_bytes, total_bytes,
               total_bytes > HIST_BUF_LEN ? "past" : "before");

        error = rdp8_decompress(decomp_han, cdata, cdata_bytes, flags,
                                &ddata, &ddata_bytes);
        if (error != 0)
        {
            printf("chunk %d: decompress error %d\n", i, error);
            rdp8_decompress_destroy(decomp_han);
            rdp8_compress_destroy(comp_han);
            return 1;
        }

        if (ddata_bytes != CHUNK_SIZE)
        {
            printf("chunk %d: decompress size mismatch %d != %d\n",
                   i, ddata_bytes, CHUNK_SIZE);
            rdp8_decompress_destroy(decomp_han);
            rdp8_compress_destroy(comp_han);
            return 1;
        }

        if (memcmp(ddata, chunk, CHUNK_SIZE) != 0)
        {
            printf("chunk %d: decompress data mismatch\n", i);
            rdp8_decompress_destroy(decomp_han);
            rdp8_compress_destroy(comp_han);
            return 1;
        }
    }

    printf("all %d chunks passed round-trip (%d total bytes, "
           "wrapped %d times)\n",
           NUM_CHUNKS, total_bytes, total_bytes / HIST_BUF_LEN);

    rdp8_decompress_destroy(decomp_han);
    rdp8_compress_destroy(comp_han);
    return 0;
}
