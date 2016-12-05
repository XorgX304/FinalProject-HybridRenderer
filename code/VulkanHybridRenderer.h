#pragma once

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include <vulkan/vulkan.h>

#include "Utilities.h"

#include "VulkanUtilities.h"
#include "VulkanRenderer.h"
#include "GfxScene.h"

#define VERTEX_BUFFER_BIND_ID 0
#define ENABLE_VALIDATION true

// Texture properties
#define TEX_DIM 2048
#define TEX_FILTER VK_FILTER_LINEAR

// Offscreen frame buffer properties
#define FB_DIM TEX_DIM

class VulkanHybridRenderer : public VulkanRenderer
{
public:

	VulkanHybridRenderer();
	~VulkanHybridRenderer();

	virtual void draw(SRendererContext& context);
	virtual void shutdownVulkan();

	////////////////////////////////////////////////////////////////////////////////////////////////
	////////					VulkanRenderer::VulkanInit() Interface						////////

	// Prepare a new framebuffer for offscreen rendering
	// The contents of this framebuffer are then
	// blitted to our render target
	virtual void setupFrameBuffer();

	virtual void setupUniformBuffers(SRendererContext& context);

	// set up the resources (eg: desriptorPool, descriptorSetLayout) that are going to be used by the descriptors.
	virtual void setupDescriptorFramework();

	// set up the scene shaders' descriptors.
	virtual void setupDescriptors();

	virtual void setupPipelines();

	virtual void buildCommandBuffers();

	void updateUniformBuffersScreen();

	void updateUniformBufferDeferredMatrices(SRendererContext& context);

	// Update fragment shader light position uniform block
	void updateUniformBufferDeferredLights(SRendererContext& context);

	/////////////////////////////////////////////////////////////////////////////////////////////////
	////////					Event-Handler Functions  								     ////////

	virtual void toggleDebugDisplay();

	// Called when view change occurs
	// Can be overriden in derived class to e.g. update uniform buffers 
	// Containing view dependant matrices
	virtual void viewChanged(SRendererContext& context);

private:

	struct SInputTextures
	{
		vkUtils::VulkanTexture m_colorMap;
		vkUtils::VulkanTexture m_normalMap;
	};

	struct SSceneMeshes
	{
		vkMeshLoader::MeshBuffer m_model;
		vkMeshLoader::MeshBuffer m_floor;
		vkMeshLoader::MeshBuffer m_quad;
	};

	struct SVkVertices
	{
		VkPipelineVertexInputStateCreateInfo			m_inputState;
		std::vector<VkVertexInputBindingDescription>	m_bindingDescriptions;
		std::vector<VkVertexInputAttributeDescription>	m_attributeDescriptions;
	};

	struct SVkPipelinesLayout
	{
		VkPipelineLayout m_onscreen;
		VkPipelineLayout m_offscreen;
		VkPipelineLayout m_raytrace;
	};

	struct SVkPipelines
	{
		VkPipeline m_onscreen;
		VkPipeline m_offscreen;
		VkPipeline m_debug;
		VkPipeline m_raytrace;
	};

	struct SVertexShaderUniforms
	{
		glm::mat4 m_projection;
		glm::mat4 m_model;
		glm::mat4 m_view;
		glm::vec4 m_instancePos[3];
	};

	struct SFragShaderUniforms
	{
		SSceneLight m_lights[6];
		glm::vec4	m_viewPos;
	};

	struct SVkUniformData
	{
		vkUtils::UniformData m_vsFullScreen;
		vkUtils::UniformData m_vsOffscreen;
		vkUtils::UniformData m_fsLights;
	};

	struct SVkDescriptorSets
	{
		VkDescriptorSet m_raytrace;
		VkDescriptorSet m_model;
		VkDescriptorSet m_floor;
		VkDescriptorSet m_quad;
		VkDescriptorSet m_onscreen;
	};

	struct SVkDescriptorSetLayouts
	{
		VkDescriptorSetLayout m_raytrace;
		VkDescriptorSetLayout m_raster;
	};

	// Framebuffer for offscreen rendering
	struct SFrameBufferAttachment
	{
		VkImage image;
		VkDeviceMemory mem;
		VkImageView view;
		VkFormat format;
	};

	struct SFrameBuffer
	{
		int32_t width, height;
		VkFramebuffer frameBuffer;
		SFrameBufferAttachment position, normal, albedo;
		SFrameBufferAttachment depth;
		VkRenderPass renderPass;
	};

private:

	// Create a frame buffer attachment
	void createAttachment(VkFormat format, VkImageUsageFlagBits usage, SFrameBufferAttachment *attachment, VkCommandBuffer layoutCmd);

	// Build command buffer for rendering the scene to the offscreen frame buffer attachments
	void buildDeferredCommandBuffer();
	void reBuildCommandBuffers();

	void prepareTextureTarget(vkUtils::VulkanTexture *tex, uint32_t width, uint32_t height, VkFormat format);
	void loadTextures();
	void loadMeshes();
	void generateQuads();

private:

	SInputTextures			m_floorTex;
	SInputTextures			m_modelTex;

	SSceneMeshes			m_sceneMeshes;

	SVkVertices				m_vertices;

	SVkPipelinesLayout		m_pipelineLayouts;

	SVkPipelines			m_pipelines;

	SVertexShaderUniforms	m_uboVS;
	SVertexShaderUniforms	m_uboOffscreenVS;

	SFragShaderUniforms		m_uboFragmentLights;

	SVkUniformData			m_uniformData;

	SVkDescriptorSetLayouts	m_descriptorSetLayouts;
	SVkDescriptorSets		m_descriptorSets;

	SFrameBuffer			m_offScreenFrameBuf;

	// One sampler for the frame buffer color attachments
	VkSampler				m_colorSampler;

	VkCommandBuffer			m_offScreenCmdBuffer;

	// Semaphore used to synchronize between offscreen and final scene rendering
	VkSemaphore				m_offscreenSemaphore;

	struct Compute {
		// -- Compute compatible queue
		VkQueue queue;
		VkFence fence;

		// -- Commands
		VkCommandBuffer commandBuffer;

		struct {
			// -- Uniform buffer
			vkUtils::UniformData ubo;
			vkUtils::UniformData materials;

			// -- Shapes buffers
			vk::Buffer indicesAndMaterialIDs;
			vk::Buffer positions;
			vk::Buffer normals;

		} buffers;

		// -- Output storage image
		vkUtils::VulkanTexture storageRaytraceImage;

		// -- Uniforms
		struct UBO { // Compute shader uniform block object
			glm::vec4 lightPosition[100];
			int lightCount;
			glm::ivec3 _pad;
		} ubo;

		VkSemaphore semaphore;

	} m_compute;
};
