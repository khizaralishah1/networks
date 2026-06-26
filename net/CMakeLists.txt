find_package(OpenSSL REQUIRED)

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(net_lib ${SOURCES})
target_link_libraries(net_lib PRIVATE OpenSSL::SSL OpenSSL::Crypto)
target_include_directories(net_lib PUBLIC include)