cmake_minimum_required(VERSION 3.22)
project(project)

set(CMAKE_CXX_STANDARD 20)

add_executable(project main.cpp
        VulkanContext.cpp
        PhysicsEngine.cpp
        GPURadixSorter.cpp
        VulkanHelpers.cpp
        vendor/spirv-reflect/spirv_reflect.cpp ZIndexer.cpp SimulatorComputeStage.cpp App.cpp App.h)

find_package(Vulkan REQUIRED)
add_subdirectory(vendor)


set(SHADER_BUILD_DIR shaders/build)

function(compile_hlsl_shader SHADER_FILE SHADER_ENTRY_POINT)
    get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/${SHADER_BUILD_DIR}/${SHADER_FILE_NAME}_${SHADER_ENTRY_POINT}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND dxc -spirv -fspv-target-env=vulkan1.1 -T cs_6_0 -E ${SHADER_ENTRY_POINT} ${PROJECT_SOURCE_DIR}/${SHADER_FILE} -Fo ${SHADER_OUTPUT_FILE})
endfunction()

function(compile_glsl_shader SHADER_FILE)
    get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/${SHADER_BUILD_DIR}/${SHADER_FILE_NAME}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND glslc -c -o ${SHADER_OUTPUT_FILE} ${PROJECT_SOURCE_DIR}/${SHADER_FILE})
endfunction()

compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Count)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_CountReduce)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Scan)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_ScanAdd)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Scatter)
compile_glsl_shader(shaders/z_indexer/z_indexer.glsl)


find_program(GLSL_VALIDATOR glslangValidator HINTS /usr/bin /usr/local/bin $ENV{VULKAN_SDK}/Bin/ $ENV{VULKAN_SDK}/Bin32/)

## find all the shader files under the shaders folder
file(GLOB_RECURSE GLSL_SOURCE_FILES
    "${PROJECT_SOURCE_DIR}/shaders/physics_engine/*.comp"
    )

## iterate each shader
foreach(GLSL ${GLSL_SOURCE_FILES})
  message(STATUS "BUILDING SHADER")
  get_filename_component(FILE_NAME ${GLSL} NAME)
  set(SPIRV "${PROJECT_SOURCE_DIR}/shaders/physics_engine/${FILE_NAME}.spv")
  message(STATUS ${GLSL})
  ##execute glslang command to compile that specific shader
  add_custom_command(
    OUTPUT ${SPIRV}
    COMMAND ${GLSL_VALIDATOR} -V ${GLSL} -o ${SPIRV}
    DEPENDS ${GLSL})
  list(APPEND SPIRV_BINARY_FILES ${SPIRV})
endforeach(GLSL)
