#include "SceneBuilder.h"
#include "Scene.h"
#include "GameObject.h"
#include "Behaviors.h"
#include "Mesh.h"
#include "Material.h"
#include "Primitive.h"
#include "VulkanShader.h"
#include "VulkanDevice.h"
#include "VulkanRenderPass.h"

SceneBuilder::~SceneBuilder() = default;

SceneBuilder::SceneBuilder(const VulkanDevice &device, const VulkanRenderPass &renderPass)
    : deviceRef(device), renderPassRef(renderPass)
{
}

void SceneBuilder::createDemoScene(Scene &scene)
{
    createMeshes();
    createMaterials();
    createRotatingCubes(scene);
    createBouncingTriangle(scene);
}

void SceneBuilder::createMeshes()
{
    // Create cube mesh
    auto cubeVerts = Primitives::createCube();
    meshes.push_back(std::make_unique<Mesh>(deviceRef, cubeVerts));

    // Create triangle mesh
    auto triangleVerts = Primitives::createTriangle();
    meshes.push_back(std::make_unique<Mesh>(deviceRef, triangleVerts));
}

void SceneBuilder::createMaterials()
{
    // Default (back-face culled) material
    auto cubeShader = std::make_unique<VulkanShader>(
        deviceRef, "shaders/cube.vert.spv", "shaders/cube.frag.spv");

    materials.push_back(std::make_unique<Material>(
        deviceRef, renderPassRef, std::move(cubeShader),
        vk::CullModeFlagBits::eBack));

    // Double-sided material
    auto doubleSidedShader = std::make_unique<VulkanShader>(
        deviceRef, "shaders/cube.vert.spv", "shaders/cube.frag.spv");

    materials.push_back(std::make_unique<Material>(
        deviceRef, renderPassRef, std::move(doubleSidedShader),
        vk::CullModeFlagBits::eNone));
}

void SceneBuilder::createRotatingCubes(Scene &scene)
{
    Mesh *cubeMesh = meshes[0].get();
    Material *defaultMaterial = materials[0].get();

    // Cube 1 - center with continuous rotation
    {
        Transform t;
        t.position = glm::vec3(0.0f, -2.0f, 0.0f);
        t.rotation = glm::vec3(-25.0f, 45.0f, 0.0f);
        t.scale = glm::vec3(1.0f);

        auto cube = std::make_unique<GameObject>(cubeMesh, defaultMaterial, t);
        cube->updateFunc = Behaviors::rotate(glm::vec3(0.0f, 45.0f, 0.0f));
        scene.addGameObject(std::move(cube));
    }

    // Cube 2 - orbits and spins
    {
        Transform t;
        t.position = glm::vec3(2.0f, 0.0f, 0.0f);
        t.scale = glm::vec3(0.5f);

        auto cube = std::make_unique<GameObject>(cubeMesh, defaultMaterial, t);
        cube->updateFunc = Behaviors::combine({Behaviors::orbit(glm::vec3(0.0f), 2.0f, 30.0f),
                                               Behaviors::rotate(glm::vec3(90.0f, 0.0f, 0.0f))});
        scene.addGameObject(std::move(cube));
    }

    // Cube 3 - spins in place
    {
        Transform t;
        t.position = glm::vec3(-2.0f, 0.0f, 0.0f);
        t.scale = glm::vec3(0.75f);

        auto cube = std::make_unique<GameObject>(cubeMesh, defaultMaterial, t);
        cube->updateFunc = Behaviors::rotate(glm::vec3(0.0f, 0.0f, 60.0f));
        scene.addGameObject(std::move(cube));
    }
}

void SceneBuilder::createBouncingTriangle(Scene &scene)
{
    Mesh *triangleMesh = meshes[1].get();
    Material *doubleSidedMaterial = materials[1].get();

    Transform t;
    t.position = glm::vec3(0.0f, 1.5f, 0.0f);
    t.scale = glm::vec3(1.5f);

    auto triangle = std::make_unique<GameObject>(triangleMesh, doubleSidedMaterial, t);
    triangle->updateFunc = Behaviors::combine({Behaviors::bounce(0.5f, 2.0f),
                                               Behaviors::rotate(glm::vec3(0.0f, 90.0f, 0.0f))});

    scene.addGameObject(std::move(triangle));
}