#pragma once

namespace ow3d
{
    class Mesh
    {
    public:
        Mesh();
        ~Mesh();
        Mesh(const Mesh&) = delete;
        Mesh& operator=(const Mesh&) = delete;
        Mesh(Mesh&& other) noexcept;
        Mesh& operator=(Mesh&& other) noexcept;

        bool create(const float* vertices, unsigned int vertexCount, unsigned int components = 3);
        void destroy();

        void bind() const;

        unsigned int vertexCount() const
        {
            return m_vertexCount;
        }

    private:
        unsigned int m_vao = 0;
        unsigned int m_vbo = 0;
        unsigned int m_vertexCount = 0;
    };
}