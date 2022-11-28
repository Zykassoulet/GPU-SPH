
find_package(Vulkan REQUIRED)

find_package(glm REQUIRED)
target_link_libraries(${PROJECT_NAME} glm)



target_include_directories(${PROJECT_NAME} PUBLIC ${Vulkan_INCLUDE_DIRS} glfw/include)
target_link_libraries(${PROJECT_NAME} glfw)
