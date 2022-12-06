find_package(Vulkan REQUIRED)

add_subdirectory(glm)
add_subdirectory(glfw)

target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} glfw/include fidelityfx-parallelsort/ffx-parallelsort spirv-reflect vma/include)
target_link_libraries(${PROJECT_NAME} glfw glm)
