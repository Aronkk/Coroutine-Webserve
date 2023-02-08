cmake_minimum_required(VERSION 3.0)
project(sylar)

# 将文件的绝对路径修改成相对路径
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include (cmake/utils.cmake)

# # 编译后需要显示的详细信息
# set(CMAKE_VERBOSE_MAKEFILE ON)
set(CMAKE_CXX_FLAGS "$ENV{CXXFLAGS} -rdynamic -O0 -fPIC -ggdb -std=c++11 -Wall -Wno-deprecated -Werror -Wno-unused-function -Wno-builtin-macro-redefined -Wno-deprecated-declarations")

include_directories(.)
include_directories(/home/lwk/apps/yaml-cpp-master/include)

link_directories(/home/lwk/Sylar/lib)
# find_library(YAMLCPP libyaml-cpp.a)

# 将下面的 .cc 文件，统称为 LIB_SRC 变量
set(LIB_SRC
    webserve/config.cc
    webserve/fiber.cc
    webserve/iomanager.cc
    webserve/log.cc
    webserve/scheduler.cc
    webserve/thread.cc
    webserve/util.cc
    )

# 通过变量 LIB_SRC 生成 libsylar.so 共享库（shared--动态，static--静态）
add_library(my_sylar SHARED ${LIB_SRC})
force_redefine_file_macro_for_sources(my_sylar) #__FILE__
#add_library(sylar_static STATIC ${LIB_SRC})
#SET_TARGET_PROPERTIES (sylar_static PROPERTIES OUTPUT_NAME "sylar")


set(LIB_LIB
    my_sylar
    pthread
    yaml-cpp
)


# 测试文件# add_definitions(test_log my_sylar)
add_executable(test_log tests/test_log.cc)
force_redefine_file_macro_for_sources(test_log) #__FILE__
target_link_libraries(test_log ${LIB_LIB})


add_executable(test_config tests/test_config.cc)
force_redefine_file_macro_for_sources(test_config) #__FILE__
target_link_libraries(test_config ${LIB_LIB})


add_executable(test_thread tests/test_thread.cc)
force_redefine_file_macro_for_sources(test_thread) #__FILE__
target_link_libraries(test_thread ${LIB_LIB})


add_executable(test_util tests/test_util.cc)
force_redefine_file_macro_for_sources(test_util) #__FILE__
target_link_libraries(test_util ${LIB_LIB})


add_executable(test_fiber tests/test_fiber.cc)
force_redefine_file_macro_for_sources(test_fiber) #__FILE__
target_link_libraries(test_fiber ${LIB_LIB})


add_executable(test_scheduler tests/test_scheduler.cc)
force_redefine_file_macro_for_sources(test_scheduler) #__FILE__
target_link_libraries(test_scheduler ${LIB_LIB})


add_executable(test_iomanager tests/test_iomanager.cc)
force_redefine_file_macro_for_sources(test_iomanager) #__FILE__
target_link_libraries(test_iomanager ${LIB_LIB})


SET(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)
SET(LIBRARY_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/lib)
