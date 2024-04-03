#include "ImageChanger.h"
int main(int argc, char* argv[]) {
    setlocale(LC_ALL, "Russian");

    const uint8_t min_num_argc = 4;
    if (argc < min_num_argc)

    {
        std::cout << "Usage: " << argv[0]
                  << " input.bmp output.bmp [-filter1 [parameter1] [parameter2] ...] "
                     "[-filter2 [parameter1] [parameter2] ...] ..."
                  << std::endl;
        return 0;
    }

    std::string input_file_name = argv[1];

    std::string output_file_name = argv[2];

    ImageChanger image_changer(input_file_name);

    const uint8_t min_num_argc_for_filter = 3;
    for (int i = min_num_argc_for_filter; i < argc; ++i)

    {
        std::string filter_name = argv[i];

        if (filter_name == "-crop") {
            if (i + 2 >= argc) {
                std::cerr << "Not enough arguments for -crop filter" << std::endl;
                return 1;
            }

            int width = std::atoi(argv[++i]);

            int height = std::atoi(argv[++i]);

            image_changer.ApplyCrop(width, height);
        } else if (filter_name == "-gs") {
            image_changer.ApplyGrayscale();
        } else if (filter_name == "-neg") {
            image_changer.ApplyNegative();
        } else if (filter_name == "-sharp") {
            image_changer.ApplySharpening();
        } else if (filter_name == "-edge") {
            if (i + 1 >= argc) {
                std::cerr << "Not enough arguments for -edge filter" << std::endl;
                return 1;
            }

            double threshold = std::atof(argv[++i]);

            image_changer.ApplyEdgeDetection(threshold);
        } else if (filter_name == "-blur") {
            if (i + 1 >= argc) {
                std::cerr << "Not enough arguments for -gaussian filter" << std::endl;
                return 1;
            }

            double sigma = std::atof(argv[++i]);

            image_changer.ApplyGaussianBlur(sigma);
        } else if (filter_name == "-circular_blur") {
            if (i + 1 >= argc) {
                std::cerr << "Not enough arguments for -circular filter" << std::endl;
                return 1;
            }

            int radius = std::atoi(argv[++i]);

            image_changer.ApplyCircularBlur(radius);
        } else if (filter_name == "-image_split") {
            if (i + 1 >= argc) {
                std::cerr << "Not enough arguments for -image_split filter" << std::endl;
                return 1;
            }
            int block_size = std::atoi(argv[++i]);
            image_changer.ApplyImageSplit(block_size);
        } else {
            std::cerr << "Unknown filter: " << filter_name << std::endl;
        }
    }

    try {
        image_changer.WriteImage(output_file_name);

    } catch (const std::exception& e) {
        std::cerr << "Error writing output image: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
