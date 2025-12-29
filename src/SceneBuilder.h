#pragma once

#include <memory>
#include <vector>

class VulkanDevice;
class VulkanRenderPass;
class Scene;
class Mesh; // Forward declarations only
class Material;

class SceneBuilder
{
public:
    SceneBuilder(const VulkanDevice &device, const VulkanRenderPass &renderPass);
    ~SceneBuilder();

    void createDemoScene(Scene &scene);

    const std::vector<std::unique_ptr<Mesh>> &getMeshes() const { return meshes; }
    const std::vector<std::unique_ptr<Material>> &getMaterials() const { return materials; }

private:
    const VulkanDevice &deviceRef;
    const VulkanRenderPass &renderPassRef;

    std::vector<std::unique_ptr<Mesh>> meshes;
    std::vector<std::unique_ptr<Material>> materials;

    void createMeshes();
    void createMaterials();
    void createRotatingCubes(Scene &scene);
    void createBouncingTriangle(Scene &scene);
};