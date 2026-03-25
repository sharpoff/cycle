#include "graphics/model.h"
#include "graphics/cache/mesh_cache.h"
#include "physics/physics_shape.h"

Vector<Vertex> Model::getVertices(MeshCache &meshCache)
{
    Vector<Vertex> vertices;
    for (const MeshID &id : meshIDs) {
        Mesh *mesh = meshCache.getMeshByID(id);
        if (!mesh)
            continue;

        for (auto &prim : mesh->primitives) {
            vertices.insert(vertices.end(), prim.vertices.begin(), prim.vertices.end());
        }
    }

    return vertices;
}

ConvexHullShape Model::createConvexHullShape(MeshCache &meshCache, float scale)
{
    mat3 scaleM = mat3(glm::scale(vec3(scale)));

    ConvexHullShape convexHull{};
    Vector<Vertex> vertices = getVertices(meshCache);
    for (auto &vert : vertices) {
        convexHull.points.push_back(vert.position * scaleM);
    }

    return convexHull;
}