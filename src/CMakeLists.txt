# add_executable(test test.cpp)
# target_link_libraries(test ${CURL_LIBRARIES})

# add_executable(multithread multithread.cpp)
# target_link_libraries(multithread pthread)

add_executable(testAPI testAPI.cpp)
target_link_libraries(testAPI
    AliasAPI
)