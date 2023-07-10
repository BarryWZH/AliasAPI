cmake_minimum_required(VERSION 3.2)
project(SneakerAPI LANGUAGES CXX C)

find_package(OpenCV REQUIRED)
find_package(Xlnt REQUIRED)

include_directories(
    /usr/local/include/nlohmann
    ${OpenCV_INCLUDE_DIRS}
    ${Xlnt_INCLUDE_DIRS}
)
find_package(CURL REQUIRED)

add_subdirectory(src)
add_subdirectory(libs)
