file(GLOB srcfile *.cpp)
# file(GLOB includefile *.h *.hpp)

add_library(AliasAPI ${srcfile})
# target_include_directories(AliasAPI PRIVATE ${includefile})
target_include_directories(AliasAPI
PRIVATE
${CURL_INCLUDE_DIRS}
)
target_link_libraries(AliasAPI 
${OpenCV_LIBS}
${CURL_LIBRARIES}
pthread
xlnt
)