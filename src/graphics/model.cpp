#include "graphics/model.h"

Vector<vec3> GetModelVertexPositions(Model *model, vec3 position, vec3 scale)
{
    if (!model)
        return {};

    mat3 transform = glm::translate(position) * glm::scale(scale);

    Vector<vec3> positions;
    for (auto &mesh : model->meshes) {
        for (auto &prim : mesh.primitives) {
            for (auto &vertex : prim.vertices) {
                positions.push_back(transform * vertex.position);
            }
        }
    }

    return positions;
}