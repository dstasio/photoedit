// base.cpp
#include "base.h"

internal Image
load_bitmap(char *path)
{
    Image image = {};
    Bitmap_Header *bmp = (Bitmap_Header *)platform_read_file(path).data;
    image.width  = bmp->width;
    image.height = bmp->height;
    image.n_channels = bmp->bits_per_pixel/8;
    image.bytes = push_array(image.width*image.height*image.n_channels, u8);

    Assert((bmp->bits_per_pixel == 24) || (bmp->bits_per_pixel == 32));
    Assert(bmp->compression == 0);

    u32 scanline_byte_count = image.width*3;
    scanline_byte_count    += (4 - (scanline_byte_count & 0x3)) & 0x3;
    u32 r_mask = 0x000000FF;
    u32 g_mask = 0x0000FF00;
    u32 b_mask = 0x00FF0000;
    u32 a_mask = 0xFF000000;
    for(u32 y = 0; y < image.height; ++y)
    {
        for(u32 x = 0; x < image.width; ++x)
        {
            u32 src_offset  = (image.height - y - 1)*scanline_byte_count + x*3;
            u32 dest_offset = y*image.width*4 + x*4;
            u32 *src_pixel  = (u32 *)byte_offset(bmp, bmp->data_offset + src_offset);
            u32 *dest_pixel = (u32 *)byte_offset(image.bytes, dest_offset);
            *dest_pixel = (r_mask & (*src_pixel >> 16)) | (g_mask & (*src_pixel)) | (b_mask & (*src_pixel >> 8)) | a_mask;
        }
    }

    image.n_channels = 4;
    return image;
}

