cmake_minimum_required(VERSION 3.22)
project(project)

set(CMAKE_CXX_STANDARD 20)

add_executable(project main.cpp
        App.cpp
        PhysicsEngine.cpp
        GPURadixSorter.cpp
        VulkanHelpers.cpp
        ComputePipeline.cpp
        vendor/spirv-reflect/spirv_reflect.cpp)

find_package(Vulkan REQUIRED)
add_subdirectory(vendor)


set(SHADER_BUILD_DIR shaders/build)

function(compile_hlsl_shader SHADER_FILE SHADER_ENTRY_POINT)
    get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/${SHADER_BUILD_DIR}/${SHADER_FILE_NAME}_${SHADER_ENTRY_POINT}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND dxc -spirv -fspv-target-env=vulkan1.1 -T cs_6_0 -E ${SHADER_ENTRY_POINT} ${PROJECT_SOURCE_DIR}/${SHADER_FILE} -Fo ${SHADER_OUTPUT_FILE})
endfunction()

compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Count)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_CountReduce)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Scan)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_ScanAdd)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Scatter)