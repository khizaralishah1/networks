cmake_minimum_required(VERSION 3.22)
project(networks)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(net)

add_executable(browser Browser.cpp)
target_link_libraries(browser PRIVATE net_lib)

# git clone https://github.com/openssl/openssl.git
# cd openssl
# ./Configure
# make -j$(sysctl -n hw.ncpu)
# make test
# sudo make install_sw