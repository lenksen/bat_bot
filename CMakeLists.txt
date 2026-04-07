cmake_minimum_required(VERSION 3.20)

project(TfBotInjector LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(MSVC)
    add_compile_options(/utf-8)
endif()

add_executable(TfBotInjector
    src/main.cpp
    src/app/application.cpp
    src/io/logger.cpp
    src/io/config_store.cpp
    src/io/text_io.cpp
    src/ui/console_ui.cpp
    src/ui/dialogs.cpp
    src/fs/path_utils.cpp
    src/fs/target_resolver.cpp
    src/archive/archive_reader.cpp
    src/archive/archive_extractor.cpp
    src/postprocess/post_processor.cpp
    src/win/encoding.cpp
    src/win/console.cpp
)

set_target_properties(TfBotInjector PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/app")

target_include_directories(TfBotInjector PRIVATE src)

find_package(LibArchive REQUIRED)

target_link_libraries(TfBotInjector PRIVATE LibArchive::LibArchive)
target_link_libraries(TfBotInjector PRIVATE ole32 shell32 comdlg32)

target_compile_definitions(TfBotInjector PRIVATE UNICODE _UNICODE NOMINMAX WIN32_LEAN_AND_MEAN)
