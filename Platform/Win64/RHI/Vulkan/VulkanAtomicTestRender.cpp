#include <RHI/Vulkan/VulkanAtomicTestRender.hpp>

AtomicTestApp::AtomicTestApp()
	: CameraApp(-80, -80, { false, true, false, true, true})
	, sizeBuffer(ctx_.resources.addUniformBuffer(8))
	, atom(ctx_, sizeBuffer)
	, anim(ctx_, atom.getOutputs(), sizeBuffer)
	, imgui(ctx_, std::vector<VulkanTexture>{})
{
	onScreenRenderers_.emplace_back(atom, false);
	onScreenRenderers_.emplace_back(anim, false);
	onScreenRenderers_.emplace_back(imgui, false);

	struct WH {
		float w, h;
	} wh{
		(float)ctx_.vkDev.framebufferWidth,
		(float)ctx_.vkDev.framebufferHeight
	};

	uploadBufferData(ctx_.vkDev, sizeBuffer.memory, 0, &wh, sizeof(wh));
}

void AtomicTestApp::drawUI()
{
	ImGui::Begin("Settings", nullptr);
	ImGui::SliderFloat("Percentage", &anim.percentage, 0.0f, 1.0f);
	ImGui::End();
}
