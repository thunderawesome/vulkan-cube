#include "Scene.h"
#include "GameObject.h"

Scene::Scene() = default;

Scene::~Scene() = default;

void Scene::addGameObject(std::unique_ptr<GameObject> obj)
{
    if (obj)
        gameObjects.push_back(std::move(obj));
}

void Scene::clearGameObjects()
{
    gameObjects.clear();
}

void Scene::update(float deltaTime)
{
    for (auto &obj : gameObjects)
    {
        if (obj && obj->enabled)
            obj->update(deltaTime);
    }
}

std::vector<GameObject *> Scene::getActiveGameObjects() const
{
    std::vector<GameObject *> active;
    active.reserve(gameObjects.size());
    for (const auto &obj : gameObjects)
    {
        if (obj && obj->enabled)
            active.push_back(obj.get());
    }
    return active;
}