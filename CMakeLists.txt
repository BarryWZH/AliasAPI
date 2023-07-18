cmake_minimum_required(VERSION 3.2)
project(SneakerAPI LANGUAGES CXX C)

# set(CMAKE_C_COMPILER x86_64-w64-mingw32-gcc)
# set(CMAKE_CXX_COMPILER x86_64-w64-mingw32-g++)
# set(CMAKE_EXECUTABLE_SUFFIX ".exe")
# set(CMAKE_SYSTEM_NAME Windows)
# set(CMAKE_CROSSCOMPILING TRUE)
# set(CMAKE_SYSTEM_PROCESSOR x86_64)

find_package(OpenCV REQUIRED)
find_package(Xlnt REQUIRED)
find_package(CURL REQUIRED)

include_directories(
    /usr/local/include/nlohmann
    ${OpenCV_INCLUDE_DIRS}
    ${Xlnt_INCLUDE_DIRS}
    ${CURL_INCLUDE_DIRS}
)

add_subdirectory(src)
add_subdirectory(libs)
