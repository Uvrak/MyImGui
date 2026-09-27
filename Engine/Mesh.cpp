#include "Mesh.h"
#include <glad/gl.h>
#include <utility>

namespace ow3d
{
    Mesh::Mesh() = default;

    Mesh::~Mesh()
    {
        destroy();
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : m_vao(std::exchange(other.m_vao, 0)),
          m_vbo(std::exchange(other.m_vbo, 0)),
          m_vertexCount(std::exchange(other.m_vertexCount, 0))
    {
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept
    {
        if (this != &other)
        {
            destroy();
            m_vao = std::exchange(other.m_vao, 0);
            m_vbo = std::exchange(other.m_vbo, 0);
            m_vertexCount = std::exchange(other.m_vertexCount, 0);
        }
        return *this;
    }

    bool Mesh::create(
        const float* vertices,
        unsigned int vertexCount,
        unsigned int components)
    {
        if (components != 3 && components != 6 && components != 9 && components != 17)
            return false;

        destroy();

        m_vertexCount = vertexCount;

        glGenVertexArrays(1, &m_vao);
        glGenBuffers(1, &m_vbo);

        if (!m_vao || !m_vbo)
            return false;

        glBindVertexArray(m_vao);

        glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

        glBufferData(
            GL_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(
                vertexCount * components * sizeof(float)
                ),
            vertices,
            GL_STATIC_DRAW
        );

        glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            components * sizeof(float),
            nullptr
        );

        glEnableVertexAttribArray(0);
        if (components == 6)
        {
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, components * sizeof(float),
                reinterpret_cast<const void*>(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, components * sizeof(float),
                reinterpret_cast<const void*>(5 * sizeof(float)));
            glEnableVertexAttribArray(2);
        }
        else if (components == 9 || components == 17)
        {
            // Static scene geometry: position, normal, UV, atlas material.
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, components * sizeof(float),
                reinterpret_cast<const void*>(3 * sizeof(float)));
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, components * sizeof(float),
                reinterpret_cast<const void*>(6 * sizeof(float)));
            glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, components * sizeof(float),
                reinterpret_cast<const void*>(8 * sizeof(float)));
            glEnableVertexAttribArray(1);
            glEnableVertexAttribArray(2);
            glEnableVertexAttribArray(3);
            if(components==17){
                glVertexAttribPointer(4,4,GL_FLOAT,GL_FALSE,components*sizeof(float),reinterpret_cast<const void*>(9*sizeof(float)));
                glVertexAttribPointer(5,4,GL_FLOAT,GL_FALSE,components*sizeof(float),reinterpret_cast<const void*>(13*sizeof(float)));
                glEnableVertexAttribArray(4);glEnableVertexAttribArray(5);
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        glBindVertexArray(0);

        return true;
    }

    void Mesh::destroy()
    {
        if (m_vbo)
        {
            glDeleteBuffers(1, &m_vbo);
            m_vbo = 0;
        }

        if (m_vao)
        {
            glDeleteVertexArrays(1, &m_vao);
            m_vao = 0;
        }

        m_vertexCount = 0;
    }

    void Mesh::bind() const
    {
        glBindVertexArray(m_vao);
    }
}
