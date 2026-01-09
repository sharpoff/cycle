#include "cycle/level.h"
#include "cycle/filesystem.h"
#include "cycle/logger.h"
#include "cycle/managers/entity_manager.h"
#include "cycle/managers/model_manager.h"

#define JSON_NOEXCEPTION 1 // NOTE: comment this to debug json issues
#include <nlohmann/json.hpp>

extern EntityManager *g_entityManager;
extern ModelManager *g_modelManager;

void Level::loadPrefab(std::filesystem::path path, String prefabName, vec3 prefabPosition)
{
    if (!std::filesystem::exists(path)) {
        LOGE("Failed to load prefab from path '%s'", path.c_str());
        return;
    }

    Vector<char> fileDataBuf = filesystem::readFile(path);
    if (fileDataBuf.empty()) {
        LOGE("Failed to load prefab from path '%s'", path.c_str());
        return;
    }

    const EntityID entityID = g_entityManager->createEntity();

    String jsonStr = String(fileDataBuf.begin(), fileDataBuf.end());
    auto j = nlohmann::json::parse(jsonStr, nullptr, false);
    if (j.is_discarded()) {
        LOGE("Failed to parse prefab json from path '%s'", path.c_str());
        return;
    }

    if (j.contains("name")) { // Name component
        auto jsonNameComponent = j.at("name");
        NameComponent &nameComponent = g_entityManager->names.addComponent(entityID);

        if (prefabName.empty()) {
            String name = jsonNameComponent.value("name", "");
            if (!name.empty()) {
                nameComponent.name = name;
            }
        } else {
            nameComponent.name = prefabName;
        }
    }

    if (j.contains("transform")) { // Transform component
        auto jsonTransformComponent = j.at("transform");
        TransformComponent &transformComponent = g_entityManager->transforms.addComponent(entityID);

        Vector<float> matrixMat4 = jsonTransformComponent.value("matrix", Vector<float>());
        if (!matrixMat4.empty() && matrixMat4.size() == 16) {
            transformComponent.transform = glm::make_mat4(matrixMat4.data());
        }

        if (matrixMat4.empty()) { // try to compose matrix
            vec3 position = vec3(0.0f);
            quat rotation = glm::identity<quat>();
            vec3 scale = vec3(1.0f);

            Vector<float> positionVec3 = jsonTransformComponent.value("position", Vector<float>());
            if (!positionVec3.empty() && positionVec3.size() == 3) {
                position = glm::make_vec3(positionVec3.data());
            } else {
                position = prefabPosition;
            }

            Vector<float> rotationQuat = jsonTransformComponent.value("rotation", Vector<float>());
            if (!rotationQuat.empty() && rotationQuat.size() == 4) {
                rotation = glm::make_quat(rotationQuat.data());
            }

            Vector<float> scaleVec3 = jsonTransformComponent.value("scale", Vector<float>());
            if (!scaleVec3.empty() && scaleVec3.size() == 3) {
                scale = glm::make_vec3(scaleVec3.data());
            }

            transformComponent.transform = glm::translate(position) * glm::toMat4(rotation) * glm::scale(scale);
        }
    }

    if (j.contains("light")) { // Light component
        auto jsonLightComponent = j.at("light");
        LightComponent &lightComponent = g_entityManager->lights.addComponent(entityID);

        String type = jsonLightComponent.value("type", "directional");
        if (!type.empty()) {
            if (type == "directional")
                lightComponent.lightType = LIGHT_TYPE_DIRECTIONAL;
            else if (type == "point")
                lightComponent.lightType = LIGHT_TYPE_POINT;
            else if (type == "spot")
                lightComponent.lightType = LIGHT_TYPE_POINT;
        }

        Vector<float> colorVec3 = jsonLightComponent.value("color", Vector<float>());
        if (!colorVec3.empty()) {
            lightComponent.color = glm::make_vec3(colorVec3.data());
        }

        Vector<float> directionVec3 = jsonLightComponent.value("direction", Vector<float>());
        if (!directionVec3.empty()) {
            lightComponent.direction = glm::make_vec3(directionVec3.data());
        }
    }

    if (j.contains("model")) { // Model component
        auto jsonModelComponent = j.at("model");
        ModelComponent &modelComponent = g_entityManager->models.addComponent(entityID);

        String file = jsonModelComponent.value("file", "");
        if (!file.empty()) {
            modelComponent.modelID = g_modelManager->loadModel(file);
        }
    }

    if (j.contains("rigid_body")) { // Rigid body component
        auto jsonRigidBodyComponent = j.at("rigid_body");
        RigidBodyComponent &rigidBodyComponent = g_entityManager->rigidBodies.addComponent(entityID);

        rigidBodyComponent.isDynamic = jsonRigidBodyComponent.value("is_dynamic", false);
        String type = jsonRigidBodyComponent.value("type", "from_model");
        if (type == "box") {
            rigidBodyComponent.type = RigidBodyType::Box;
            Vector<float> halfExtentVec3 = jsonRigidBodyComponent.value("half_extent", Vector<float>());
            if (!halfExtentVec3.empty() && halfExtentVec3.size() == 3) {
                rigidBodyComponent.halfExtent = glm::make_vec3(halfExtentVec3.data());
            }
        } else if (type == "sphere") {
            rigidBodyComponent.type = RigidBodyType::Sphere;
            float radius = jsonRigidBodyComponent.value("radius", 0.0f);
            rigidBodyComponent.radius = radius;
        } else if (type == "from_model") {
            rigidBodyComponent.type = RigidBodyType::FromModel;
        }
    }
}