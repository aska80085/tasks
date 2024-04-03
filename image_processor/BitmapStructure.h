#pragma once

#include <cstdint>
#include <vector>

#pragma pack(push, 1)
struct BitmapHeader {
    char signature[2];
    uint32_t file_size;
    uint32_t reserved;
    uint32_t data_offset;
};

struct DibHeader {
    uint32_t header_size;
    int32_t width;
    int32_t height;
    uint16_t color_plane_count;
    uint16_t bits_per_pixel;
    uint32_t compression_method;
    uint32_t data_size;
    int32_t horizontal_resolution;
    int32_t vertical_resolution;
    uint32_t color_count;
    uint32_t important_color_count;
};
#pragma pack(pop)

struct Pixel {
    uint8_t b;
    uint8_t g;
    uint8_t r;
};
using Image = std::vector<std::vector<Pixel>>;
