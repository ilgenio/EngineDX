#include "Globals.h"

#include "MeshRenderPass.h"

#include "Application.h"
#include "ModuleRender.h"
#include "ModuleResources.h"
#include "ModuleLevel.h"
#include "ModuleCamera.h"

#include "Scene.h"
#include "Mesh.h"
#include "Material.h"

MeshRenderPass::MeshRenderPass(VkRenderPass renderPass)
{
    ModuleRender* render = App->getRender();
    ModuleResources* resources = App->getResources();

    // Shader 
    vertexPath = resources->normalizeUniquePath(path("Assets/Shaders/default.vert.spv"));
    vertex     = resources->createUniqueShader(vertexPath);

    fragmentPath = resources->normalizeUniquePath(path("Assets/Shaders/default.frag.spv"));
    fragment = resources->createUniqueShader(fragmentPath);

    // Layout
    VkPushConstantRange pushConstant = { VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants) };
    VkDescriptorSetLayout materialSetLayout = Material::getDescriptorSetLayout();
    VkDescriptorSetLayout globalSetLayout = render->getGlobalDescLayout();

    VkDescriptorSetLayout descSets[] = { globalSetLayout, materialSetLayout };

    layout = vulkan->createPipelineLayout(&pushConstant, 1, &descSets[0], uint32_t(std::size(descSets)));

    // Pipeline
    VkDynamicState dynamicStates[] = { VK_DYNAMIC_STATE_VIEWPORT , VK_DYNAMIC_STATE_SCISSOR };
    const VkVertexInputAttributeDescription *attribs = Mesh::getVertexAttribs();
    uint32_t attribCount = Mesh::getVertexAttribCount();

    opaquePipe = vulkan->createPipeline(attribs, attribCount, sizeof(Mesh::Vertex), true, vertex, fragment, false, VK_BLEND_FACTOR_ZERO, 
                                        VK_BLEND_FACTOR_ZERO, VK_BLEND_OP_ADD, dynamicStates, 2, layout, renderPass);

    blendPipe = vulkan->createPipeline(attribs, attribCount, sizeof(Mesh::Vertex), true, vertex, fragment, true, VK_BLEND_FACTOR_SRC_ALPHA,
        VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA, VK_BLEND_OP_ADD, dynamicStates, 2, layout, renderPass);
}

MeshRenderPass::~MeshRenderPass()
{
    ModuleVulkan* vulkan = App->getVulkan();
    ModuleResources* resources = App->getResources();

    vulkan->destroy(opaquePipe);
    vulkan->destroy(layout);
    resources->destroyUniqueShader(vertexPath);
    resources->destroyUniqueShader(fragmentPath);
}

void MeshRenderPass::record(VkCommandBuffer commandBuffer, const RenderList& renderList)
{
    ModuleCamera *camera = App->getCamera();
    ModuleVulkan *vulkan = App->getVulkan();

    uint32_t width = vulkan->getWidth();
    uint32_t height = vulkan->getHeight();
    glm::mat4x4 view = camera->getView();
    glm::mat4x4 proj = camera->getProj();

    vulkan->setBeginLabel(commandBuffer, "MeshRenderPass");

    VkViewport viewport = { 0.0f, 0.0f, float(width), float(height), 0.0f, 1.0f };
    VkRect2D scissor = { {0, 0}, {width, height} };

    // Opaque pipeline
    if (!renderList.getOpaqueList().empty())
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, opaquePipe);

        // Set viewport and scissor
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        recordList(commandBuffer, renderList.getOpaqueList());
    }

    // Transparent pipline
    if (!renderList.getBlendList().empty())
    {
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, blendPipe);

        // Set viewport and scissor
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        recordList(commandBuffer, renderList.getBlendList());
    }

    vulkan->setEndLabel(commandBuffer);
}

void MeshRenderPass::recordList(VkCommandBuffer commandBuffer, const RenderItemList& renderList)
{
    ModuleVulkan* vulkan = App->getVulkan();
    const Scene* scene = App->getLevel()->getScene();
    ModuleRender* render = App->getRender();

    for (const RenderItem& item : renderList)
    {
        const MeshInstance &instance = scene->getInstance(item.instance);
        const Mesh *mesh = scene->getMesh(instance.meshIndex);
        const Material *material = scene->getMaterial(mesh->getMaterialIndex());

        vulkan->setBeginLabel(commandBuffer, mesh->getName().c_str());

        VkBuffer vertexBuffer = mesh->getVertexBuffer();
        VkBuffer indexBuffer = mesh->getIndexBuffer();

        // upload the matrix to the GPU via push constants
        PushConstants constants = {instance.transformation, glm::inverseTranspose(glm::mat3(instance.transformation))};
        vkCmdPushConstants(commandBuffer, layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstants), &constants);

        // Bind vertex/index buffers
        VkDeviceSize offset = 0;
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffer, &offset);
        vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        // Bind DescriptorSets
        VkDescriptorSet materialDescSet = material->getDescriptorSet();
        VkDescriptorSet globalDescSet = render->getGlobalDescSet();
        VkDescriptorSet descSets[] = {globalDescSet, materialDescSet};
        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, layout, 0, uint32_t(std::size(descSets)), &descSets[0], 0, nullptr);

        // Draw Indexed command
        vkCmdDrawIndexed(commandBuffer, mesh->getNumIndices(), 1, 0, 0, 0);

        vulkan->setEndLabel(commandBuffer);
    }
}