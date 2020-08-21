// base.cpp
#include "base.h"

inline u16
safe_truncate(u32 x)
{
    Assert(x <= 0xFFFF);
    return (u16)x;
}

// compares strings and returns 1 if equal, 0 if different
b32 same_string(char *s1, char *s2)
{
    b32 result = 1;

    if (!s1 || !s2)
        result = 0;
    else {
        while (!((*s1 == '\0') && (*s2 == '\0')) && (result))
        {
            result = (*(s1++) == *(s2++));
        }
    }

    return result;
}

inline char *
get_extension(char *path)
{
    char *result = path;
    for (char *c = path; *c != '\0'; ++c)
    {
        if (*c == '.')
            result = c;
    }
    return result;
}

inline u8
png_paeth_(u32 a, u32 b, u32 c)
{
    u32 p = a + b - c;
    u32 pa = Abs((i32)p - (i32)a);
    u32 pb = Abs((i32)p - (i32)b);
    u32 pc = Abs((i32)p - (i32)c);

    u8 result = (u8)c;
    if ((pa <= pb) && (pa <= pc))
        result = (u8)a;
    else if (pb <= pc)
        result = (u8)b;
    return result;
}

internal Image
load_png(char *path)
{
    Image image = {};
    PNG_Header *png = (PNG_Header *)platform_read_file(path).data;
    if (png->signature != 0x0A1A0A0D474E5089) {
        // @todo: log
        Assert(0);
    }
    else {
        byte_swap(&png->width);
        byte_swap(&png->height);
        byte_swap(&png->header_size);
        byte_swap(&png->header_id);

        image.width = png->width;
        image.height = png->height;
        image.depth = png->bit_depth;
        // @todo: maybe use 3 channels for greyscale too? 
        switch(png->color_type) {
            case PNG_COLOR_TYPE_GREYSCALE:       image.n_channels = 1; break;
            case PNG_COLOR_TYPE_TRUECOLOR:       image.n_channels = 3; break;
            case PNG_COLOR_TYPE_INDEXED:         image.n_channels = 0; break;
            case PNG_COLOR_TYPE_GREYSCALE_ALPHA: image.n_channels = 2; break;
            case PNG_COLOR_TYPE_TRUECOLOR_ALPHA: image.n_channels = 4; break;
            default: Assert(0); break;
        }
        Assert(!png->compression_method);  // @todo: guard for theese in release build
        Assert(!png->filter_method);
        Assert(png->header_size == 0x0D);
        Assert(png->header_id == PNG_CHUNK_TYPE_IHDR);
        image.bytes = push_array(image.width*image.height*image.n_channels, u8); // @todo: adapt this

        u8  compression_method = 0;
        u8  compression_info   = 0;
        b32 dict_flag          = 0;
        u8  compression_level  = 0;
        u16 compression_check  = 0;

        b32 end_reached = 0;
        void *chunk = &png->chunks_start;
        u8 *decompressed_start = 0;
        while (!end_reached) {
            u32 *chunk_size = (u32 *)chunk;
            byte_swap(chunk_size);
            u32 *chunk_id   = (u32 *)byte_offset(chunk, 4);
            byte_swap(chunk_id);
            u8 *chunk_data  = byte_offset(chunk, 8);
            u32 *chunk_crc  = (u32 *)byte_offset(chunk_data, *chunk_size);
            byte_swap(chunk_crc);

            chunk = byte_offset(chunk, 12+(*chunk_size));
            switch(*chunk_id) {
                case PNG_CHUNK_TYPE_IDAT:
                {
                    print("IDAT\n");
                    u8 *compressed = &chunk_data[0];
                    u8 compression_method = chunk_data[0] & 0x0F;
                    u8 compression_info   = chunk_data[0] >> 4;
                    b32 dict_flag = (chunk_data[1] & 0x20) >> 5;
                    u8 compression_level = chunk_data[1] >> 6;
                    u16 compression_check = offset_read(chunk_data, 0, u16);
                    byte_swap(&compression_check);

                    Assert(compression_method == 8); // @todo: log theese in release build
                    Assert(compression_info <= 7);
                    Assert((compression_check % 31) == 0);
                    Assert(!dict_flag);

                    compressed = &chunk_data[2];
                    *chunk_size -= 2;

                    u8 *decompressed = 0;
                    // compressed data starts here
                    b32 deflate_bfinal = 0;
                    do {
                        // @note: DEFLATE IS LITTLE ENDIAN
                        deflate_bfinal = *compressed & 0x1;
                        b32 deflate_btype  = (*compressed >> 1) & 0x3;

                        if (deflate_btype == 0) { // no compression
                            compressed++;
                            u16 *len  = (u16 *)(compressed);
                            compressed += 2;
                            u16 *nlen = (u16 *)(compressed);
                            compressed += 2;
                            //                            byte_swap(len);
                            //                            byte_swap(nlen);
                            if (!decompressed_start) {
                                decompressed = push_array(*len, u8);
                                decompressed_start = decompressed;
                            }
                            else {
                                AssertF(extend_array(decompressed_start, *len));
                            }
                            *chunk_size -= 5;

                            while (*len > 0) {
                                if (*len < *chunk_size) {
                                    memcpy(decompressed, compressed, *len);
                                    compressed   += *len;
                                    decompressed += *len;
                                    *chunk_size -= *len;
                                    *len = 0;
                                }
                                else {
                                    memcpy(decompressed, compressed, *chunk_size);
                                    decompressed += *chunk_size;
                                    *len -= safe_truncate(*chunk_size);

                                    chunk_size = (u32 *)chunk;
                                    byte_swap(chunk_size);
                                    chunk_id   = (u32 *)byte_offset(chunk, 4);
                                    byte_swap(chunk_id);
                                    chunk_data  = byte_offset(chunk, 8);

                                    Assert(*chunk_id == PNG_CHUNK_TYPE_IDAT);

                                    compressed = chunk_data;
                                    chunk = byte_offset(chunk, 12+(*chunk_size));
                                }
                            }
                        }
                        else if (deflate_btype != 3) {
                        }
                        else {
                            throw_error("'%s' is not a valid PNG file\n", path);
                            Assert(0);
                            // @todo: handle incorrect png
                        }
                    } while(!deflate_bfinal);
                } break;
                case PNG_CHUNK_TYPE_PLTE: {print("PLTE\n");} break;
                case PNG_CHUNK_TYPE_cHRM: {print("cHRM\n");} break;
                case PNG_CHUNK_TYPE_gAMA: {print("gAMA\n");} break;
                case PNG_CHUNK_TYPE_iCCP: {print("iCCP\n");} break;
                case PNG_CHUNK_TYPE_sBIT: {print("sBIT\n");} break;
                case PNG_CHUNK_TYPE_sRGB: {print("sRGB\n");} break;
                case PNG_CHUNK_TYPE_bKGD: {print("bKGD\n");} break;
                case PNG_CHUNK_TYPE_hIST: {print("hIST\n");} break;
                case PNG_CHUNK_TYPE_tRNS: {print("tRNS\n");} break;
                case PNG_CHUNK_TYPE_pHYs: {print("pHYs\n");} break;
                case PNG_CHUNK_TYPE_sPLT: {print("sPLT\n");} break;
                case PNG_CHUNK_TYPE_tIME: {print("tIME\n");} break;
                case PNG_CHUNK_TYPE_iTXt: {print("iTXt\n");} break;
                case PNG_CHUNK_TYPE_tEXt: {print("tEXt\n");} break;
                case PNG_CHUNK_TYPE_zTXt: {print("zTXt\n");} break;
                case PNG_CHUNK_TYPE_IEND: {
                    end_reached = 1;
                } break;
                default: Assert(0); break;
            }
        }

        u8 *   source_row = decompressed_start;
        u8 *prev_dest_row = 0;
        u8 *     dest_row = image.bytes;
        u32 pixel_index = 0;
        for(u32 scanline = 0; scanline < image.height; ++scanline)
        {
            // @todo: adapt this (bit_depth)
//            row = byte_offset(decompressed_start, y*(image.width*image.n_channels + 1));
            u8 filter_type = source_row[0];
            for(u32 pixel = 0; pixel < image.width; ++pixel)
            {
                u8  zero[4] = {};

                u8 *x = byte_offset(source_row, pixel*image.n_channels + 1);
                u8 *a = pixel                    ? byte_offset(     dest_row, (pixel-1)*image.n_channels) : zero;  // @todo: (or the byte immediately before x, when the bit depth is less than 8)
                u8 *b = prev_dest_row            ? byte_offset(prev_dest_row,     pixel*image.n_channels) : zero;
                u8 *c = (pixel && prev_dest_row) ? byte_offset(prev_dest_row, (pixel-1)*image.n_channels) : zero;
                u8 *dest = byte_offset(dest_row, pixel*image.n_channels);
                switch(filter_type)
                {
                    case 0:
                    {
                        dest[0] = x[0];
                        if (image.n_channels >= 2)
                            dest[1] = x[1];
                        if (image.n_channels >= 3)
                            dest[2] = x[2];
                        if (image.n_channels >= 4)
                            dest[3] = x[3];
                    } break;
                    // @todo: case 1 and case 2 can be merged
                    case 1:  // SUB
                    {
                        dest[0] = x[0] + a[0];
                        if (image.n_channels >= 2)
                            dest[1] = x[1] + a[1];
                        if (image.n_channels >= 3)
                            dest[2] = x[2] + a[2];
                        if (image.n_channels >= 4)
                            dest[3] = x[3] + a[3];
                    } break;
                    case 2:  // UP
                    {
                        dest[0] = x[0] + b[0];
                        if (image.n_channels >= 2)
                            dest[1] = x[1] + b[1];
                        if (image.n_channels >= 3)
                            dest[2] = x[2] + b[2];
                        if (image.n_channels >= 4)
                            dest[3] = x[3] + b[3];
                    } break;
                    case 3:  // AVERAGE
                    {
                        dest[0] = x[0] + (u8)(((u32)(a[0]) + (u32)(b[0])) / 2.f);
                        if (image.n_channels >= 2)
                            dest[1] = x[1] + (u8)(((u32)(a[1]) + (u32)(b[1])) / 2.f);
                        if (image.n_channels >= 3)
                            dest[2] = x[2] + (u8)(((u32)(a[2]) + (u32)(b[2])) / 2.f);
                        if (image.n_channels >= 4)
                            dest[3] = x[3] + (u8)(((u32)(a[3]) + (u32)(b[3])) / 2.f);
                    } break;
                    case 4:  // PAETH
                    {
                        dest[0] = x[0] + png_paeth_(a[0], b[0], c[0]);
                        if (image.n_channels >= 2)
                            dest[1] = x[1] + png_paeth_(a[1], b[1], c[1]);
                        if (image.n_channels >= 3)
                            dest[2] = x[2] + png_paeth_(a[2], b[2], c[2]);
                        if (image.n_channels >= 4)
                            dest[3] = x[3] + png_paeth_(a[3], b[3], c[3]);
                    } break;
                    default: Assert(0); break;
                }
            }
            prev_dest_row = dest_row;
            source_row   += image.width*image.n_channels + 1;
            dest_row     += image.width*image.n_channels;
        }
    }

    pop_last; // decompressed_start
    return image;
}

// @note: this function is only capable of reading Bitmaps with 3 bytes per pixel
// @todo: add support for all possible cases
internal Image
load_bitmap(char *path)
{
    Image image = {};
    Bitmap_Header *bmp = (Bitmap_Header *)platform_read_file(path).data;
    image.width  = bmp->width;
    image.height = bmp->height;
    image.n_channels = bmp->bits_per_pixel/8;
    image.bytes = push_array(image.width*image.height*image.n_channels, u8);

    Assert(bmp->bits_per_pixel == 24);
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

