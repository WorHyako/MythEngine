#include <RHI/Vulkan/VulkanBaseModel.hpp>
#include <Mesh/BaseMesh.hpp>

VulkanBaseModel::VulkanBaseModel(std::string const& path, bool gamma)
	:BaseModel(path, gamma)
{
	loadModel(path);
}