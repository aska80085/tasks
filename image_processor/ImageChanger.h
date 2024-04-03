#pragma once

#include <iostream>
#include <string>

#include "BitmapStructure.h"

constexpr int MaxColorValue = 255;
class ImageChanger {
public:
    ImageChanger() = delete;
    explicit ImageChanger(const std::string& filename);

    void ApplyCrop(int width, int height);
    void ApplyGrayscale();
    void ApplyNegative();
    void ApplySharpening();
    void ApplyEdgeDetection(double threshold);
    void ApplyGaussianBlur(double sigma);
    void ApplyCircularBlur(int radius);
    void WriteImage(const std::string& filename);
    void ApplyImageSplit(int block_size);

    ~ImageChanger() = default;

private:
    void ReadImage(const std::string& filename);
    void ApplyMatrix(const std::vector<std::vector<int>>& kernel);
    int CeilDiv(int numerator, int denominator) {
        return (numerator + denominator - 1) / denominator;
    };
    Image image_;
};