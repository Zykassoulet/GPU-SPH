cmake_minimum_required(VERSION 3.20)
project(project)
set(CMAKE_CXX_STANDARD 20)

add_compile_definitions(VULKAN_HPP_NO_SPACESHIP_OPERATOR)

add_executable(project main.cpp
        VulkanContext.cpp
        PhysicsEngine.cpp
        GPURadixSorter.cpp
        VulkanHelpers.cpp   
        ZIndexer.cpp
        SimulatorComputeStage.cpp
        App.cpp
        VulkanCleaner.cpp
        DensityCompute.cpp
        ForceCompute.cpp
        Blocker.cpp
        Compactor.cpp
        InstanceRenderer.cpp
        Mesh.cpp)

find_package(Vulkan REQUIRED)
add_subdirectory(vendor)


set(SHADER_BUILD_DIR shaders/build)

function(compile_hlsl_shader SHADER_FILE SHADER_ENTRY_POINT)
    get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/${SHADER_BUILD_DIR}/${SHADER_FILE_NAME}_${SHADER_ENTRY_POINT}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND dxc -spirv -fspv-target-env=vulkan1.1 -T cs_6_0 -E ${SHADER_ENTRY_POINT} ${PROJECT_SOURCE_DIR}/${SHADER_FILE} -Fo ${SHADER_OUTPUT_FILE})
endfunction()

function(compile_glsl_shader_comp SHADER_FILE)
    get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/${SHADER_BUILD_DIR}/${SHADER_FILE_NAME}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND glslc -g -c -o ${SHADER_OUTPUT_FILE} ${PROJECT_SOURCE_DIR}/${SHADER_FILE})
endfunction()

function(compile_glsl_shader_frag SHADER_FILE)
    get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/${SHADER_BUILD_DIR}/${SHADER_FILE_NAME}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND glslc -g -c -fshader-stage=fragment -o ${SHADER_OUTPUT_FILE} ${PROJECT_SOURCE_DIR}/${SHADER_FILE})
endfunction()

function(compile_glsl_shader_vert SHADER_FILE)
    get_filename_component(SHADER_FILE_NAME ${SHADER_FILE} NAME_WE)
    set(SHADER_OUTPUT_FILE "${PROJECT_SOURCE_DIR}/${SHADER_BUILD_DIR}/${SHADER_FILE_NAME}.spv")
    add_custom_command(TARGET ${PROJECT_NAME} PRE_BUILD COMMAND glslc -g -c -fshader-stage=vertex -o ${SHADER_OUTPUT_FILE} ${PROJECT_SOURCE_DIR}/${SHADER_FILE})
endfunction()

compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Count)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_CountReduce)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Scan)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_ScanAdd)
compile_hlsl_shader(shaders/radixsort/ParallelSortCS.hlsl FPS_Scatter)
compile_glsl_shader_comp(shaders/z_indexer/z_indexer.glsl)
compile_glsl_shader_comp(shaders/physics_engine/density.glsl)
compile_glsl_shader_comp(shaders/physics_engine/forces.glsl)
compile_glsl_shader_comp(shaders/blocker/blocker.glsl)
compile_glsl_shader_comp(shaders/blocker/block_buffer_clear.glsl)
compile_glsl_shader_comp(shaders/blocker/compactor.glsl)
compile_glsl_shader_frag(shaders/render/instance_renderer_frag.glsl)
compile_glsl_shader_vert(shaders/render/instance_renderer_vert.glsl)
