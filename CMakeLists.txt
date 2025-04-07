cmake_minimum_required(VERSION 3.27)
project(VirtualCamera)

add_subdirectory(vendor/SDL)
add_subdirectory(vendor/glm)

add_executable(VirtualCamera)
target_compile_features(VirtualCamera PRIVATE cxx_std_20)
target_sources(VirtualCamera PRIVATE src/main.cc)
target_link_libraries(VirtualCamera PRIVATE SDL3::SDL3 glm::glm)

# Copy the SDL3 DLL to the same directory as the VirtualCamera executable.
if(WIN32)
    add_custom_command(
        TARGET VirtualCamera POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy $<TARGET_FILE:SDL3::SDL3-shared> $<TARGET_FILE_DIR:VirtualCamera>
        VERBATIM
    )
endif()
