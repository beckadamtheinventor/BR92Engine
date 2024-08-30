#pragma once
#include "DynamicArray.hpp"
#include "external/glad.h"
#include <cstdint>
template<unsigned int STRIDE>
class IntMesh {
    DynamicArray<uint32_t> _vertices;
    DynamicArray<unsigned short> _indices;
    public:
    unsigned short vertexCount=0;
    unsigned short triangleCount=0;
    unsigned int vbo=0;
    unsigned int ebo=0;
    unsigned int vao=0;
    uint32_t* vertices=nullptr;
    unsigned short* indices=nullptr;
    // Returns true if the mesh is ready for upload.
    bool IsReady() {
        return !(vertices == nullptr || indices == nullptr || vertexCount == 0 || triangleCount == 0);
    }
    void Clear() {
        _vertices.clear();
        _indices.clear();
    }
    // Upload the mesh to the GPU. Returns false if failed.
    bool Upload() {
        if (!IsReady()) {
            return false;
        }
        if (vao == 0) {
            glGenVertexArrays(1, &vao);
        }
        if (vbo == 0) {
            glGenBuffers(1, &vbo);
        }
        if (ebo == 0) {
            glGenBuffers(1, &ebo);
        }
        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertexCount*STRIDE*sizeof(uint32_t), vertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, triangleCount*3*sizeof(unsigned short), indices, GL_STATIC_DRAW);
        for (unsigned int i=0; i<STRIDE; i++) {
            glVertexAttribIPointer(i, 1, GL_UNSIGNED_INT, sizeof(uint32_t)*STRIDE, (void*)(sizeof(uint32_t)*i));
            glEnableVertexAttribArray(i);
        }
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
        return true;
    }
    // Update mesh edits. Returns true if mesh is ready for upload.
    bool Flush() {
        if (vertices != nullptr) {
            delete vertices;
        }
        if (indices != nullptr) {
            delete indices;
        }
        vertices = _vertices.collapse();
        vertexCount = _vertices.length() / STRIDE;
        indices = _indices.collapse();
        triangleCount = _indices.length() / 3;
        return IsReady();
    }
    // Add a vertex to the mesh. Note that data must point to at least STRIDE values.
    void addVertex(uint32_t* data) {
        for (unsigned int i=0; i<STRIDE; i++) {
            _vertices.append(data[i]);
        }
    }
    // Add a triangle to the mesh given vertex indices
    void addTriangle(unsigned short a, unsigned short b, unsigned short c) {
        _indices.append(a);
        _indices.append(b);
        _indices.append(c);
    }
    // Draw the mesh.
    void Draw() {
        glBindVertexArray(vao);
        glDrawElements(GL_TRIANGLES, triangleCount*3, GL_UNSIGNED_SHORT, 0);
    }
};