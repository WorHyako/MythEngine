#pragma once

#include <Mesh/BaseModel.hpp>

class VulkanBaseModel : public BaseModel
{
public:
	VulkanBaseModel(std::string const& path, bool gamma = false);
	~VulkanBaseModel() override = default;
};
