cmake_minimum_required(VERSION 3.10.0)
project(adsbSerializer VERSION 0.1.0 LANGUAGES C CXX)


file(GLOB_RECURSE filesource CONFIGURE_DEPENDS "src/*.cpp" "src/*.h")

add_executable(adsbSerializer ${filesource})
target_compile_features(adsbSerializer PRIVATE cxx_std_20)
