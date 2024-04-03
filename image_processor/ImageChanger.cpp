#include <cmath>
#include <fstream>
#include "ImageChanger.h"

constexpr uint16_t Pixels = 24;

ImageChanger::ImageChanger(const std::string& filename) {
    try {
        ReadImage(filename);
    } catch (const std::exception& e) {
        std::cerr << "Error reading input image: " << e.what() << std::endl;
        std::exit(1);
    }
}

void ImageChanger::ReadImage(const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to read file / invalid file path");
    }

    BitmapHeader bitmap_header;
    DibHeader dib_header;

    file.read(reinterpret_cast<char*>(&bitmap_header), sizeof(BitmapHeader));
    file.read(reinterpret_cast<char*>(&dib_header), sizeof(DibHeader));

    if (bitmap_header.signature[0] != 'B' || bitmap_header.signature[1] != 'M') {
        throw std::runtime_error("Invalid file format");
    }

    if (dib_header.bits_per_pixel != Pixels || dib_header.compression_method != 0) {
        throw std::runtime_error("Invalid image format");
    }

    image_.resize(dib_header.height, std::vector<Pixel>(dib_header.width));

    const std::streamsize row_size = dib_header.width * 3;
    const std::streamsize padding_size = (4 - row_size % 4) % 4;

    for (int y = dib_header.height - 1; y >= 0; --y) {
        for (int x = 0; x < dib_header.width; ++x) {
            Pixel& pixel = image_[y][x];
            file.read(reinterpret_cast<char*>(&pixel), sizeof(Pixel));
        }
        file.ignore(padding_size);
    }
}

void ImageChanger::WriteImage(const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file) {
        throw std::runtime_error("Unable to open file");
    }

    const size_t width = image_[0].size();
    const size_t height = image_.size();
    const size_t row_size = width * 3;
    const size_t padding_size = (4 - row_size % 4) % 4;
    const size_t data_size = (row_size + padding_size) * height;

    BitmapHeader bitmap_header{{'B', 'M'},
                               static_cast<uint32_t>(sizeof(BitmapHeader) + sizeof(DibHeader) + data_size),
                               0,
                               static_cast<uint32_t>(sizeof(BitmapHeader) + sizeof(DibHeader))};

    const uint16_t x_p = 2835;
    const uint16_t y_p = 2835;
    DibHeader dib_header{sizeof(DibHeader),
                         static_cast<int32_t>(width),
                         static_cast<int32_t>(height),
                         1,
                         Pixels,
                         0,
                         static_cast<uint32_t>(data_size),
                         x_p,
                         y_p,
                         0,
                         0};

    file.write(reinterpret_cast<const char*>(&bitmap_header), sizeof(BitmapHeader));
    file.write(reinterpret_cast<const char*>(&dib_header), sizeof(DibHeader));

    for (int32_t y = static_cast<int32_t>(height) - 1; y >= 0; --y) {
        for (int x = 0; x < width; ++x) {
            const Pixel& pixel = image_[y][x];
            file.write(reinterpret_cast<const char*>(&pixel), sizeof(Pixel));
        }
        for (size_t i = 0; i < padding_size; ++i) {
            file.put(0);
        }
    }
}

void ImageChanger::ApplyCrop(int width, int height) {
    const size_t original_height = image_.size();
    const size_t original_width = image_[0].size();

    const int crop_width = std::min(static_cast<int>(width), static_cast<int>(original_width));
    const int crop_height = std::min(static_cast<int>(height), static_cast<int>(original_height));

    image_.resize(crop_height);
    for (auto& row : image_) {
        row.resize(crop_width);
    }
}

void ImageChanger::ApplyGrayscale() {
    for (auto& row : image_) {
        for (auto& pixel : row) {
            const uint8_t gray_value = static_cast<uint8_t>(0.299 * pixel.r + 0.587 * pixel.g + 0.114 * pixel.b);
            pixel.r = gray_value;
            pixel.g = gray_value;
            pixel.b = gray_value;
        }
    }
}

void ImageChanger::ApplyNegative() {
    for (auto& row : image_) {
        for (auto& pixel : row) {
            pixel.r = MaxColorValue - pixel.r;
            pixel.g = MaxColorValue - pixel.g;
            pixel.b = MaxColorValue - pixel.b;
        }
    }
}

void ImageChanger::ApplySharpening() {
    const std::vector<std::vector<int>> kernel = {{0, -1, 0}, {-1, 5, -1}, {0, -1, 0}};
    ApplyMatrix(kernel);
}

void ImageChanger::ApplyEdgeDetection(double threshold) {
    ApplyGrayscale();

    const std::vector<std::vector<int>> kernel = {{0, -1, 0}, {-1, 4, -1}, {0, -1, 0}};

    ApplyMatrix(kernel);

    const size_t height = image_.size();
    const size_t width = image_[0].size();

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            if (static_cast<double>(image_[y][x].r / static_cast<double>(MaxColorValue)) < threshold) {
                image_[y][x].r = 0;
                image_[y][x].b = 0;
                image_[y][x].g = 0;
            } else {
                image_[y][x].r = MaxColorValue;
                image_[y][x].b = MaxColorValue;
                image_[y][x].g = MaxColorValue;
            }
        }
    }
}

void ImageChanger::ApplyGaussianBlur(double sigma) {
    int radius = static_cast<int>(std::ceil(3 * sigma));
    int kernel_size = 2 * radius + 1;
    std::vector<float> kernel(kernel_size);
    float sum = 0.0f;

    auto gaussian = [sigma](int x) {
        const double numerator = std::exp(-(x * x) / (2 * sigma * sigma));
        const double denominator = std::sqrt(2 * M_PI * sigma * sigma);
        return numerator / denominator;
    };

    for (int i = -radius; i <= radius; i++) {
        kernel[i + radius] = static_cast<float>(gaussian(i));
        sum += kernel[i + radius];
    }

    for (auto& value : kernel) {
        value /= sum;
    }

    auto horizontal_pass = [&kernel, radius](const Image& source) {
        Image temp(source.size(), std::vector<Pixel>(source[0].size()));
        for (int y = 0; y < source.size(); y++) {
            for (int x = 0; x < source[0].size(); x++) {
                float sum_r = 0.0f;
                float sum_g = 0.0f;
                float sum_b = 0.0f;
                for (int k = -radius; k <= radius; k++) {
                    int sample_x = std::min(std::max(x + k, 0), static_cast<int>(source[0].size()) - 1);
                    const Pixel& sample = source[y][sample_x];
                    float weight = kernel[k + radius];
                    sum_r += static_cast<float>(sample.r) * weight;
                    sum_g += static_cast<float>(sample.g) * weight;
                    sum_b += static_cast<float>(sample.b) * weight;
                }
                temp[y][x].r = static_cast<uint8_t>(std::max(0.0f, std::min(static_cast<float>(MaxColorValue), sum_r)));
                temp[y][x].g = static_cast<uint8_t>(std::max(0.0f, std::min(static_cast<float>(MaxColorValue), sum_g)));
                temp[y][x].b = static_cast<uint8_t>(std::max(0.0f, std::min(static_cast<float>(MaxColorValue), sum_b)));
            }
        }
        return temp;
    };

    auto vertical_pass = [&kernel, radius](Image& temp) {
        Image output(temp.size(), std::vector<Pixel>(temp[0].size()));
        for (int x = 0; x < temp[0].size(); x++) {
            for (int y = 0; y < temp.size(); y++) {
                float sum_r = 0.0f;
                float sum_g = 0.0f;
                float sum_b = 0.0f;
                for (int k = -radius; k <= radius; k++) {
                    int sample_y = std::min(std::max(y + k, 0), static_cast<int>(temp.size()) - 1);
                    const Pixel& sample = temp[sample_y][x];
                    float weight = kernel[k + radius];
                    sum_r += static_cast<float>(sample.r) * weight;
                    sum_g += static_cast<float>(sample.g) * weight;
                    sum_b += static_cast<float>(sample.b) * weight;
                }
                output[y][x].r =
                    static_cast<uint8_t>(std::max(0.0f, std::min(static_cast<float>(MaxColorValue), sum_r)));
                output[y][x].g =
                    static_cast<uint8_t>(std::max(0.0f, std::min(static_cast<float>(MaxColorValue), sum_g)));
                output[y][x].b =
                    static_cast<uint8_t>(std::max(0.0f, std::min(static_cast<float>(MaxColorValue), sum_b)));
            }
        }
        return output;
    };

    Image temp = horizontal_pass(image_);

    Image result = vertical_pass(temp);

    image_ = result;
}

void ImageChanger::ApplyCircularBlur(int radius) {
    const int kernel_size = (2 * radius) + 1;
    const size_t height = image_.size();
    const size_t width = image_[0].size();
    Image result = image_;
    std::vector<std::vector<float>> kernel(kernel_size, std::vector<float>(kernel_size, 0.0f));
    float kernel_sum = 0.0f;

    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radius * radius) {
                kernel[y + radius][x + radius] = 1.0f;
                kernel_sum += 1.0f;
            }
        }
    }

    for (auto& row : kernel) {
        for (auto& value : row) {
            value /= kernel_sum;
        }
    }

    for (int y = radius; y < static_cast<int>(height) - radius; ++y) {
        for (int x = radius; x < static_cast<int>(width) - radius; ++x) {
            float sum_r = 0.0f;
            float sum_g = 0.0f;
            float sum_b = 0.0f;
            for (int ky = 0; ky < kernel_size; ++ky) {
                for (int kx = 0; kx < kernel_size; ++kx) {
                    const Pixel& pixel = image_[y - radius + ky][x - radius + kx];
                    const float kernel_value = kernel[ky][kx];
                    sum_r += static_cast<float>(pixel.r) * kernel_value;
                    sum_g += static_cast<float>(pixel.g) * kernel_value;
                    sum_b += static_cast<float>(pixel.b) * kernel_value;
                }
            }
            Pixel& pixel = result[y][x];
            pixel.r = std::min(MaxColorValue, std::max(0, static_cast<int>(sum_r)));
            pixel.g = std::min(MaxColorValue, std::max(0, static_cast<int>(sum_g)));
            pixel.b = std::min(MaxColorValue, std::max(0, static_cast<int>(sum_b)));
        }
    }
    image_ = std::move(result);
}

void ImageChanger::ApplyImageSplit(int block_size) {
    const int original_height = static_cast<int>(image_.size());
    const int original_width = static_cast<int>(image_[0].size());

    const int num_blocks_width = CeilDiv(original_width, block_size);
    const int num_blocks_height = CeilDiv(original_height, block_size);

    const int new_width = num_blocks_width * block_size;
    const int new_height = num_blocks_height * block_size;

    image_.resize(new_height);
    for (auto& row : image_) {
        row.resize(new_width);
    }

    for (int i = 0; i < num_blocks_height; ++i) {
        for (int j = 0; j < num_blocks_width; ++j) {
            const int block_start_x = j * block_size;
            const int block_start_y = i * block_size;

            const int block_end_x = block_start_x + block_size;
            const int block_end_y = block_start_y + block_size;

            int sum_r = 0;
            int sum_g = 0;
            int sum_b = 0;
            int num_pixels = 0;

            for (int y = block_start_y; y < block_end_y; ++y) {
                for (int x = block_start_x; x < block_end_x; ++x) {
                    if (y < original_height && x < original_width) {
                        const Pixel& pixel = image_[y][x];

                        sum_r += pixel.r;
                        sum_g += pixel.g;
                        sum_b += pixel.b;

                        num_pixels++;
                    }
                }
            }

            const uint8_t avg_r = static_cast<uint8_t>(sum_r / num_pixels);
            const uint8_t avg_g = static_cast<uint8_t>(sum_g / num_pixels);
            const uint8_t avg_b = static_cast<uint8_t>(sum_b / num_pixels);

            for (int y = block_start_y; y < block_end_y; ++y) {
                for (int x = block_start_x; x < block_end_x; ++x) {
                    if (y < original_height && x < original_width) {
                        Pixel& pixel = image_[y][x];
                        pixel.r = avg_r;
                        pixel.g = avg_g;
                        pixel.b = avg_b;
                    }
                }
            }
        }
    }
}

void ImageChanger::ApplyMatrix(const std::vector<std::vector<int>>& kernel) {

    const size_t kernel_size = kernel.size();
    const size_t kernel_half_size = kernel_size / 2;

    const size_t height = image_.size();
    const size_t width = image_[0].size();

    Image result = image_;

    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            int sum_r = 0;
            int sum_g = 0;
            int sum_b = 0;
            for (int ky = 0; ky < kernel_size; ++ky) {
                for (int kx = 0; kx < kernel_size; ++kx) {
                    int image_x = x - static_cast<int>(kernel_half_size) + kx;
                    int image_y = y - static_cast<int>(kernel_half_size) + ky;

                    if (image_x < 0) {
                        image_x = 0;
                    } else if (image_x >= static_cast<int>(width)) {
                        image_x = static_cast<int>(width) - 1;
                    }
                    if (image_y < 0) {
                        image_y = 0;
                    } else if (image_y >= height) {
                        image_y = static_cast<int>(height) - 1;
                    }
                    const Pixel& pixel = image_[image_y][image_x];
                    const int kernel_value = kernel[ky][kx];

                    sum_r += pixel.r * kernel_value;
                    sum_g += pixel.g * kernel_value;
                    sum_b += pixel.b * kernel_value;
                }
            }

            Pixel& pixel = result[y][x];
            pixel.r = std::min(MaxColorValue, std::max(0, sum_r));
            pixel.g = std::min(MaxColorValue, std::max(0, sum_g));
            pixel.b = std::min(MaxColorValue, std::max(0, sum_b));
        }
    }

    image_ = result;
}