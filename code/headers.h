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

#define PNG_COLOR_TYPE_GREYSCALE       0
#define PNG_COLOR_TYPE_TRUECOLOR       2
#define PNG_COLOR_TYPE_INDEXED         3
#define PNG_COLOR_TYPE_GREYSCALE_ALPHA 4
#define PNG_COLOR_TYPE_TRUECOLOR_ALPHA 6

#define PNG_CHUNK_TYPE_IHDR 'IHDR'  //{ 73, 72, 68,  82}
#define PNG_CHUNK_TYPE_PLTE 'PLTE'  //{ 80, 76, 84,  69}
#define PNG_CHUNK_TYPE_IDAT 'IDAT'  //{ 73, 68, 65,  84}
#define PNG_CHUNK_TYPE_IEND 'IEND'  //{ 73, 69, 78,  68}
#define PNG_CHUNK_TYPE_cHRM 'cHRM'  //{ 99, 72, 82,  77}
#define PNG_CHUNK_TYPE_gAMA 'gAMA'  //{103, 65, 77,  65}
#define PNG_CHUNK_TYPE_iCCP 'iCCP'  //{105, 67, 67,  80}
#define PNG_CHUNK_TYPE_sBIT 'sBIT'  //{115, 66, 73,  84}
#define PNG_CHUNK_TYPE_sRGB 'sRGB'  //{115, 82, 71,  66}
#define PNG_CHUNK_TYPE_bKGD 'bKGD'  //{ 98, 75, 71,  68}
#define PNG_CHUNK_TYPE_hIST 'hIST'  //{104, 73, 83,  84}
#define PNG_CHUNK_TYPE_tRNS 'tRNS'  //{116, 82, 78,  83}
#define PNG_CHUNK_TYPE_pHYs 'pHYs'  //{112, 72, 89, 115}
#define PNG_CHUNK_TYPE_sPLT 'sPLT'  //{115, 80, 76,  84}
#define PNG_CHUNK_TYPE_tIME 'tIME'  //{116, 73, 77,  69}
#define PNG_CHUNK_TYPE_iTXt 'iTXt'  //{105, 84, 88, 116}
#define PNG_CHUNK_TYPE_tEXt 'tEXt'  //{116, 69, 88, 116}
#define PNG_CHUNK_TYPE_zTXt 'zTXt'  //{122, 84, 88, 116}

struct PNG_Header
{
    u64 signature;           // must be 0x0A1A0A0D474E5089
    u32 header_size;         // must be 0x0D
    u32 header_id;           // must be 0x52444849
    u32 width;
    u32 height;
    u8  bit_depth;
    u8  color_type;
    u8  compression_method;  // must be 0
    u8  filter_method;       // must be 0
    u8  interlace_method;
    u32 header_crc;
    u8  chunks_start;
};
#pragma pack(pop)

#define HEADERS_H
#endif
