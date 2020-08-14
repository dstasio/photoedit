#if !defined(HEADERS_H)

#pragma pack(push, 1)
struct Bitmap_Header
{
    u16 signature;
    u32 file_size;
    u32 reserved;
    u32 data_offset;
    u32 info_header_size;
    u32 width;
    u32 height;
    u16 planes;
    u16 bits_per_pixel;
    u32 compression;
    u32 image_size;
    u32 x_pixels_per_meter;
    u32 y_pixels_per_meter;
    u32 colors_used;
    u32 important_colors;
};
#pragma pack(pop)

#define HEADERS_H
#endif
