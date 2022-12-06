#include "ComputePipeline.h"

void ComputePipeline::init(App* app_ptr, PhysicsEngine* phy_eng_ptr) {
	m_phy_eng_ptr = phy_eng_ptr;
	m_app_ptr = app_ptr;


	vk::BufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	//this is the total size, in bytes, of the buffer we are allocating
	bufferInfo.size = m_phy_eng_ptr->m_number_particles * sizeof(glm::vec3);
	//this buffer is going to be used as a Vertex Buffer
	bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;


	//let the VMA library know that this data should be writeable by CPU, but also readable by GPU
	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = VMA_MEMORY_USAGE_CPU_TO_GPU;

	m_pos_buf = st



	//TODO :other thing 

	//
//    u32 buffer_size = 1024;
//    vk::Buffer in_buffer = createBuffer(buffer_size, m_queue_family_indices.compute_family.value());
//    vk::Buffer out_buffer = createBuffer(buffer_size, m_queue_family_indices.compute_family.value());
//
//    vk::MemoryRequirements in_buffer_memory_requirements = m_device.getBufferMemoryRequirements(in_buffer);
//    vk::MemoryRequirements out_buffer_memory_requirements = m_device.getBufferMemoryRequirements(out_buffer);
//
//    //get correct memory property
//
//    vk::PhysicalDeviceMemoryProperties memory_properties = m_physical_device.getMemoryProperties();
//    u32 memory_type_index = 0;
//    vk::DeviceSize memory_size = 0;
//    for (u32 CurrentMemoryTypeIndex = 0; CurrentMemoryTypeIndex < memory_properties.memoryTypeCount; ++CurrentMemoryTypeIndex) {
//
//        vk::MemoryType memory_type = memory_properties.memoryTypes[CurrentMemoryTypeIndex];
//        if ((vk::MemoryPropertyFlagBits::eHostVisible & memory_type.propertyFlags) &&
//            (vk::MemoryPropertyFlagBits::eHostCoherent & memory_type.propertyFlags))
//        {
//            memory_size = memory_properties.memoryHeaps[memory_type.heapIndex].size;
//            memory_type_index = CurrentMemoryTypeIndex;
//            break;
//        }
//    }
//
//    vk::MemoryAllocateInfo in_buffer_memory_allocate_info(in_buffer_memory_requirements.size, memory_type_index);
//    vk::MemoryAllocateInfo out_buffer_memory_allocate_info(out_buffer_memory_requirements.size, memory_type_index);
//    vk::DeviceMemory in_buffer_memory = m_device.allocateMemory(in_buffer_memory_allocate_info);
//    vk::DeviceMemory out_buffer_memory = m_device.allocateMemory(out_buffer_memory_allocate_info);
//
//    compute_in_buffer_ptr = static_cast<i32*>(m_device.mapMemory(in_buffer_memory, 0, buffer_size));
//    m_device.bindBufferMemory(in_buffer, in_buffer_memory, 0);
//    m_device.bindBufferMemory(out_buffer, out_buffer_memory, 0);
}
