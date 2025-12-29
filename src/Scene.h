#pragma once

#include <vector>
#include <memory>

struct GameObject;

class Scene
{
public:
    Scene();
    ~Scene();

    void addGameObject(std::unique_ptr<GameObject> obj);
    void clearGameObjects();
    void update(float deltaTime);
    std::vector<GameObject *> getActiveGameObjects() const;

    const std::vector<std::unique_ptr<GameObject>> &getGameObjects() const { return gameObjects; }

private:
    std::vector<std::unique_ptr<GameObject>> gameObjects;
};