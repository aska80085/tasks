cmake_minimum_required(VERSION 3.0)  # Здесь указывается минимальная версия CMake
project(image_processor)                   # Затем вызов функции project() с указанием имени проекта

add_executable(
        image_processor
        image_processor.cpp
        "../image_processor/image_processor.cpp"
        "../image_processor/BitmapStructure.cpp"
        ImageChanger.cpp
        BitmapStructure.cpp
)