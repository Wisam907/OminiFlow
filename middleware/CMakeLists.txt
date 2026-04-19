# 限制CMake最低版本
cmake_minimum_required(VERSION 3.15)

# 定义项目名称，并指定字符集为Utf-8
project(OminiFlowMiddleware LANGUAGES CXX)

# 1. 强制C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# 要求编译器必须支持C++20标准，无法满足则编译失败
set(CMAKE_CXX_EXTENSIONS OFF)

# 2. 编译选项：高性能优化 + Utf-8源文件支持
if (MSVC)
    # 若编译器为MSVC，则添加如下编译选项：
    # 开启警告等级4（较严格的编译告警）；使__cplusplus宏报告正确的C++标准版本；源文件使用Utf-8编码
    add_compile_options(/W4 /Zc:__cplusplus /utf-8)
else()
    # 其他编译器，-Wall和-Wextra开启常用警告；
    # -O3启用最高级别优化（不包含体积优化）；
    # -march=native针对当前CPU架构优化（使用该CPU支持的全部指令集）；
    # -pthread启用pthread多线程支持（同时影响编译和链接）
    # 指定输入字符集和执行字符集均为Utf-8
    add_compile_options(-Wall -Wextra -O3 -march=native -pthread -finput-charset=UTF-8 -fexec-charset=UTF-8)
endif()

# 3. 查找依赖
# 查找Google Protobuf库，REQUIRED表示若未找到则失败。
# 该命令会设置Protobuf_VERSION等变量，并导入protobuf::libprotobuf等目标
find_package(Protobuf REQUIRED)
# 打印一条状态选项，显示找到的Protobuf版本号
message(STATUS "Using Protobuf ${Protobuf_VERSION}")

# 4. 处理Protobbuf
# 定义变量PROTO_FILE，其值为common.proto/messages.proto文件的绝对路径
set(PROTO_FILE ${CMAKE_CURRENT_SOURCE_DIR}/../common/proto/messages.proto)
# 调用Protobuf提供的函数，根据指定的.proto自动生成.pb.h和.pb.cc并分别保存到两个变量中
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

# 5. 核心可执行文件
# 定义一个可执行文件目标ominiflow_node，并添加源文件main.cpp以及Protobuf生成的.pb.cc和.pb.h等源文件
add_executable(ominiflow_node
    src/main.cpp
    ${PROTO_SRCS}
    ${PROTO_HDRS}
)

# 6. 包含目录
# 添加头文件搜索路径
target_include_directories(ominiflow_node PRIVATE
    include
    ${CMAKE_CURRENT_SOURCE_DIR}/../common/include
    ${CMAKE_CURRENT_BINARY_DIR} # 存放生成的Proto头文件
)

# 7. 添加生成目标文件所需要的链接库，PRIVATE属性表明这些库仅被ominiflow_node自身使用，不会传递给依赖它的其他目标
find_package(Threads REQUIRED)
target_link_libraries(ominiflow_node PRIVATE
    protobuf::libprotobuf
    Threads::Threads
)

# 8. 性能优化标志（LTO）
# 包含CMake模块CheckIPOSupported，用于检查编译器是否支持IPO（跨过程优化）
include(CheckIPOSupported)
# 检查编译器是否支持IPO（即LTO），结果存入变量ipo_supported，如有错误信息则存入变量error
check_ipo_supported(RESULT ipo_supported OUTPUT error)
if(ipo_supported)
    set_target_properties(ominiflow_node PROPERTIES INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
