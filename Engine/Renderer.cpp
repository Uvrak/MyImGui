#include "Renderer.h"
#include "WorldSettings.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glad/gl.h>

#include <MyImGui.h>

#include <vector>
#include <cmath>
#include <SDL3/SDL.h>
#include <fstream>
#include <sstream>
#include <limits>
#include <algorithm>

static glm::vec3 projectToSphere(
    const glm::vec3& cubePosition,
    float radius)
{
    return glm::normalize(cubePosition) * radius;
}

namespace ow3d
{
    Renderer::~Renderer()
    {
        shutdown();
    }

    bool Renderer::loadTileShader(const char* vertexPath, const char* fragmentPath)
    {
        std::ifstream vertexFile(vertexPath), fragmentFile(fragmentPath);
        if (!vertexFile || !fragmentFile)
            return false;
        std::ostringstream vertex, fragment;
        vertex << vertexFile.rdbuf();
        fragment << fragmentFile.rdbuf();
        m_tileShader.destroy();
        m_hasTileShader = m_tileShader.create(vertex.str().c_str(), fragment.str().c_str());
        return m_hasTileShader;
    }

    bool Renderer::setTileMaterialTexture(TileMaterialId id, const char* bmpPath, bool smooth, bool quadrantArray)
    {
        SDL_Surface* source = SDL_LoadBMP(bmpPath);
        if (!source)
            return false;
        SDL_Surface* rgba = SDL_ConvertSurface(source, SDL_PIXELFORMAT_RGBA32);
        SDL_DestroySurface(source);
        if (!rgba)
            return false;
        if (quadrantArray && (rgba->w % 2 != 0 || rgba->h % 2 != 0))
        {
            SDL_DestroySurface(rgba);
            return false;
        }

        GLuint texture = 0;
        glGenTextures(1, &texture);
        if (!texture)
        {
            SDL_DestroySurface(rgba);
            return false;
        }
        const GLenum target = quadrantArray ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D;
        glBindTexture(target, texture);
        GLint oldRowLength = 0;
        glGetIntegerv(GL_UNPACK_ROW_LENGTH, &oldRowLength);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, rgba->pitch / 4);
        if (quadrantArray)
        {
            // Independent layers prevent mipmaps from mixing neighbouring atlas materials.
            const int width = rgba->w / 2, height = rgba->h / 2;
            glTexImage3D(target, 0, GL_RGBA8, width, height, 4, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
            for (int layer = 0; layer < 4; ++layer)
            {
                const auto* pixels = static_cast<const unsigned char*>(rgba->pixels)
                    + (layer / 2) * height * rgba->pitch + (layer % 2) * width * 4;
                glTexSubImage3D(target, 0, 0, 0, layer, width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
            }
        }
        else
            glTexImage2D(target, 0, GL_RGBA8, rgba->w, rgba->h,
                0, GL_RGBA, GL_UNSIGNED_BYTE, rgba->pixels);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, oldRowLength);
        if (smooth)
        {
            glGenerateMipmap(target);
        }
        glTexParameteri(target, GL_TEXTURE_MIN_FILTER, smooth ? GL_LINEAR_MIPMAP_LINEAR : GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_MAG_FILTER, smooth ? GL_LINEAR : GL_NEAREST);
        glTexParameteri(target, GL_TEXTURE_WRAP_S, quadrantArray ? GL_MIRRORED_REPEAT : GL_REPEAT);
        glTexParameteri(target, GL_TEXTURE_WRAP_T, quadrantArray ? GL_MIRRORED_REPEAT : GL_REPEAT);
        SDL_DestroySurface(rgba);
        auto& previous = m_tileTextures[static_cast<int>(id)];
        if (previous)
            glDeleteTextures(1, &previous);
        previous = texture;
        m_tileTextureArrays[static_cast<int>(id)] = quadrantArray;
        return true;
    }

    bool Renderer::initialize()
    {
        glEnable(GL_DEPTH_TEST);

        const char* vertexShaderSource = R"(
           #version 460 core

layout(location = 0) in vec3 aPosition;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

void main()
{
    gl_Position =
        projection *
        view *
        model *
        vec4(aPosition, 1.0);
}
        )";

        const char* fragmentShaderSource = R"(
            #version 460 core

            out vec4 FragColor;

            uniform vec3 color;

            void main()
            {
                FragColor = vec4(color, 1.0);
            }
        )";

        if (!m_shader.create(vertexShaderSource, fragmentShaderSource))
            return false;

        const float vertices[] =
        {
            // RÃƒÂ¼ckseite
            -0.5f, -0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,
             0.5f,  0.5f, -0.5f,

             0.5f,  0.5f, -0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            // Vorderseite
            -0.5f, -0.5f,  0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

             0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,
            -0.5f, -0.5f,  0.5f,

            // Links
            -0.5f,  0.5f,  0.5f,
            -0.5f,  0.5f, -0.5f,
            -0.5f, -0.5f, -0.5f,

            -0.5f, -0.5f, -0.5f,
            -0.5f, -0.5f,  0.5f,
            -0.5f,  0.5f,  0.5f,

            // Rechts
             0.5f,  0.5f,  0.5f,
             0.5f,  0.5f, -0.5f,
             0.5f, -0.5f, -0.5f,

             0.5f, -0.5f, -0.5f,
             0.5f, -0.5f,  0.5f,
             0.5f,  0.5f,  0.5f,

             // Unten
             -0.5f, -0.5f, -0.5f,
              0.5f, -0.5f, -0.5f,
              0.5f, -0.5f,  0.5f,

              0.5f, -0.5f,  0.5f,
             -0.5f, -0.5f,  0.5f,
             -0.5f, -0.5f, -0.5f,

             // Oben
             -0.5f,  0.5f, -0.5f,
              0.5f,  0.5f, -0.5f,
              0.5f,  0.5f,  0.5f,

              0.5f,  0.5f,  0.5f,
             -0.5f,  0.5f,  0.5f,
             -0.5f,  0.5f, -0.5f
        };

        if (!m_mesh.create(vertices, 36))
            return false;

        constexpr int gridResolution = 10;

        std::vector<float> gridVertices;

        for (int z = 0; z <= gridResolution; ++z)
        {
            const float v =
                -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

            for (int x = 0; x < gridResolution; ++x)
            {
                const float u0 =
                    -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

                const float u1 =
                    -1.0f + 2.0f * static_cast<float>(x + 1) / gridResolution;

                const glm::vec3 p0 =
                    projectToSphere(
                        glm::vec3(u0, 1.0f, v),
                        PlanetRadius
                    );

                const glm::vec3 p1 =
                    projectToSphere(
                        glm::vec3(u1, 1.0f, v),
                        PlanetRadius
                    );

                gridVertices.insert(
                    gridVertices.end(),
                    {
                        p0.x, p0.y, p0.z,
                        p1.x, p1.y, p1.z
                    }
                );
            }
        }

        for (int x = 0; x <= gridResolution; ++x)
        {
            const float u =
                -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

            for (int z = 0; z < gridResolution; ++z)
            {
                const float v0 =
                    -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

                const float v1 =
                    -1.0f + 2.0f * static_cast<float>(z + 1) / gridResolution;

                const glm::vec3 p0 =
                    projectToSphere(
                        glm::vec3(u, 1.0f, v0),
                        PlanetRadius
                    );

                const glm::vec3 p1 =
                    projectToSphere(
                        glm::vec3(u, 1.0f, v1),
                        PlanetRadius
                    );

                gridVertices.insert(
                    gridVertices.end(),
                    {
                        p0.x, p0.y, p0.z,
                        p1.x, p1.y, p1.z
                    }
                );
            }

            for (int z = 0; z <= gridResolution; ++z)
            {
                const float v =
                    -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

                for (int x = 0; x < gridResolution; ++x)
                {
                    const float u0 =
                        -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

                    const float u1 =
                        -1.0f + 2.0f * static_cast<float>(x + 1) / gridResolution;

                    const glm::vec3 p0 =
                        projectToSphere(
                            glm::vec3(u0, -1.0f, v),
                            PlanetRadius
                        );

                    const glm::vec3 p1 =
                        projectToSphere(
                            glm::vec3(u1, -1.0f, v),
                            PlanetRadius
                        );

                    gridVertices.insert(
                        gridVertices.end(),
                        {
                            p0.x, p0.y, p0.z,
                            p1.x, p1.y, p1.z
                        }
                    );
                }
            }

            for (int x = 0; x <= gridResolution; ++x)
            {
                const float u =
                    -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

                for (int z = 0; z < gridResolution; ++z)
                {
                    const float v0 =
                        -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

                    const float v1 =
                        -1.0f + 2.0f * static_cast<float>(z + 1) / gridResolution;

                    const glm::vec3 p0 =
                        projectToSphere(
                            glm::vec3(u, -1.0f, v0),
                            PlanetRadius
                        );

                    const glm::vec3 p1 =
                        projectToSphere(
                            glm::vec3(u, -1.0f, v1),
                            PlanetRadius
                        );

                    gridVertices.insert(
                        gridVertices.end(),
                        {
                            p0.x, p0.y, p0.z,
                            p1.x, p1.y, p1.z
                        }
                    );
                }

                // Vorderseite +Z
                for (int y = 0; y <= gridResolution; ++y)
                {
                    const float v =
                        -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                    for (int x = 0; x < gridResolution; ++x)
                    {
                        const float u0 =
                            -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

                        const float u1 =
                            -1.0f + 2.0f * static_cast<float>(x + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(u0, v, 1.0f),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(u1, v, 1.0f),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }

                for (int x = 0; x <= gridResolution; ++x)
                {
                    const float u =
                        -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

                    for (int y = 0; y < gridResolution; ++y)
                    {
                        const float v0 =
                            -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                        const float v1 =
                            -1.0f + 2.0f * static_cast<float>(y + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(u, v0, 1.0f),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(u, v1, 1.0f),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }

                // RÃƒÂ¼ckseite -Z
                for (int y = 0; y <= gridResolution; ++y)
                {
                    const float v =
                        -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                    for (int x = 0; x < gridResolution; ++x)
                    {
                        const float u0 =
                            -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

                        const float u1 =
                            -1.0f + 2.0f * static_cast<float>(x + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(u0, v, -1.0f),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(u1, v, -1.0f),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }

                for (int x = 0; x <= gridResolution; ++x)
                {
                    const float u =
                        -1.0f + 2.0f * static_cast<float>(x) / gridResolution;

                    for (int y = 0; y < gridResolution; ++y)
                    {
                        const float v0 =
                            -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                        const float v1 =
                            -1.0f + 2.0f * static_cast<float>(y + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(u, v0, -1.0f),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(u, v1, -1.0f),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }

                // Rechte Seite +X
                for (int z = 0; z <= gridResolution; ++z)
                {
                    const float v =
                        -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

                    for (int y = 0; y < gridResolution; ++y)
                    {
                        const float u0 =
                            -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                        const float u1 =
                            -1.0f + 2.0f * static_cast<float>(y + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(1.0f, u0, v),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(1.0f, u1, v),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }

                for (int y = 0; y <= gridResolution; ++y)
                {
                    const float u =
                        -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                    for (int z = 0; z < gridResolution; ++z)
                    {
                        const float v0 =
                            -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

                        const float v1 =
                            -1.0f + 2.0f * static_cast<float>(z + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(1.0f, u, v0),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(1.0f, u, v1),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }

                // Linke Seite -X
                for (int z = 0; z <= gridResolution; ++z)
                {
                    const float v =
                        -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

                    for (int y = 0; y < gridResolution; ++y)
                    {
                        const float u0 =
                            -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                        const float u1 =
                            -1.0f + 2.0f * static_cast<float>(y + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(-1.0f, u0, v),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(-1.0f, u1, v),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }

                for (int y = 0; y <= gridResolution; ++y)
                {
                    const float u =
                        -1.0f + 2.0f * static_cast<float>(y) / gridResolution;

                    for (int z = 0; z < gridResolution; ++z)
                    {
                        const float v0 =
                            -1.0f + 2.0f * static_cast<float>(z) / gridResolution;

                        const float v1 =
                            -1.0f + 2.0f * static_cast<float>(z + 1) / gridResolution;

                        const glm::vec3 p0 =
                            projectToSphere(
                                glm::vec3(-1.0f, u, v0),
                                PlanetRadius
                            );

                        const glm::vec3 p1 =
                            projectToSphere(
                                glm::vec3(-1.0f, u, v1),
                                PlanetRadius
                            );

                        gridVertices.insert(
                            gridVertices.end(),
                            {
                                p0.x, p0.y, p0.z,
                                p1.x, p1.y, p1.z
                            }
                        );
                    }
                }
            }


        }

        if (!m_gridMesh.create(
            gridVertices.data(),
            static_cast<unsigned int>(gridVertices.size() / 3)))
        {
            return false;
        }

        m_camera.setPerspective(
            60.0f,
            1280.0f / 720.0f,
            0.005f,
            1000.0f
        );

        const float cubeAngle =
            glm::radians(5.0f);

        const glm::vec3 cubeDirection{
            std::sin(cubeAngle),
            0.0f,
            std::cos(cubeAngle)
        };

        const glm::vec3 cubePosition =
            cubeDirection * (PlanetRadius + 0.5f);

        m_camera.lookAt(cubePosition);


        glGenFramebuffers(1, &m_framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

        glGenTextures(1, &m_colorTexture);
        glBindTexture(GL_TEXTURE_2D, m_colorTexture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            m_viewportWidth,
            m_viewportHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            m_colorTexture,
            0
        );

        glGenRenderbuffers(1, &m_depthRenderbuffer);
        glBindRenderbuffer(GL_RENDERBUFFER, m_depthRenderbuffer);

        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            m_viewportWidth,
            m_viewportHeight
        );

        glFramebufferRenderbuffer(
            GL_FRAMEBUFFER,
            GL_DEPTH_STENCIL_ATTACHMENT,
            GL_RENDERBUFFER,
            m_depthRenderbuffer
        );

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            return false;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        return true;
    }

    bool Renderer::loadSceneObjects(const char* meshPath, const char* bmpPath,
        const char* vertexPath, const char* fragmentPath)
    {
        m_hasSceneObjects = false;
        m_sceneRoof.destroy();
        m_sceneRoofHidden = false;
        m_scenePickVertices.clear();
        std::ifstream input(meshPath);
        std::string magic;
        unsigned int version = 0, count = 0;
        if (!(input >> magic >> version >> count) || magic != "OW3D_MESH" ||
            version != 1 || count == 0 || count > 3000000 || count % 3 != 0)
            return false;
        std::vector<float> vertices(static_cast<size_t>(count) * 9);
        for (auto& value : vertices)
            if (!(input >> value) || !std::isfinite(value)) return false;
        m_sceneMin = glm::vec3(std::numeric_limits<float>::max());
        m_sceneMax = glm::vec3(std::numeric_limits<float>::lowest());
        for (size_t i = 0; i < vertices.size(); i += 9)
        {
            const glm::vec3 p(vertices[i], vertices[i + 1], vertices[i + 2]);
            m_scenePickVertices.push_back(p);
            m_sceneMin = glm::min(m_sceneMin, p);
            m_sceneMax = glm::max(m_sceneMax, p);
        }
        std::ifstream vs(vertexPath), fs(fragmentPath);
        if (!vs || !fs) return false;
        std::ostringstream vertex, fragment;
        vertex << vs.rdbuf(); fragment << fs.rdbuf();
        m_sceneShader.destroy();
        if (!m_sceneShader.create(vertex.str().c_str(), fragment.str().c_str()) ||
            !m_sceneMesh.create(vertices.data(), count, 9) ||
            !setTileMaterialTexture(TileMaterialId::Material2, bmpPath, true, true))
            return false;
        m_hasSceneObjects = true;
        m_sceneBodyVertexCount=m_scenePickVertices.size();
        return true;
    }

    void Renderer::drawSceneObjects()
    {
        if (!m_hasSceneObjects) return;
        m_sceneShader.bind();
        m_sceneShader.setMat4("view", m_camera.viewMatrix());
        m_sceneShader.setMat4("projection", m_camera.projectionMatrix());
        m_sceneShader.setInt("houseAtlas", 0);
        m_sceneShader.setFloat("time", m_frameTime);
        m_sceneShader.setInt("meadowInstances", 0);
        m_sceneShader.setMat4("objectTransform",glm::mat4(1));
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D_ARRAY, m_tileTextures.at(static_cast<int>(TileMaterialId::Material2)));
        m_sceneMesh.bind();
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_sceneMesh.vertexCount()));
        m_openings.draw(m_sceneShader);
        if (m_meadow.vertexCount())
        {
            m_sceneShader.setInt("meadowInstances", 1);
            m_meadow.bind();
            glDrawArraysInstanced(GL_TRIANGLES, 0, static_cast<GLsizei>(m_meadow.vertexCount()), m_meadowInstanceCount);
            m_sceneShader.setInt("meadowInstances", 0);
        m_sceneShader.setMat4("objectTransform",glm::mat4(1));
        }
        if (!sceneRoofHidden() && m_sceneRoof.vertexCount())
        {
            m_sceneRoof.bind();
            glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(m_sceneRoof.vertexCount()));
        }
    }

    void Renderer::drawSceneGlass() {
        if(!m_hasSceneObjects)return;
        m_sceneShader.bind();
        m_sceneShader.setMat4("view",m_camera.viewMatrix());
        m_sceneShader.setMat4("projection",m_camera.projectionMatrix());
        m_sceneShader.setInt("meadowInstances",0);
        const bool blend=glIsEnabled(GL_BLEND),cull=glIsEnabled(GL_CULL_FACE);
        GLboolean depthWrite;glGetBooleanv(GL_DEPTH_WRITEMASK,&depthWrite);
        GLint srcRGB,dstRGB,srcAlpha,dstAlpha;
        glGetIntegerv(GL_BLEND_SRC_RGB,&srcRGB);glGetIntegerv(GL_BLEND_DST_RGB,&dstRGB);
        glGetIntegerv(GL_BLEND_SRC_ALPHA,&srcAlpha);glGetIntegerv(GL_BLEND_DST_ALPHA,&dstAlpha);
        glEnable(GL_BLEND);glBlendFuncSeparate(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA,GL_ONE,GL_ONE_MINUS_SRC_ALPHA);
        glDisable(GL_CULL_FACE);glDepthMask(GL_FALSE);
        m_openings.drawGlass(m_sceneShader,m_camera.viewMatrix());
        glDepthMask(depthWrite);glBlendFuncSeparate(srcRGB,dstRGB,srcAlpha,dstAlpha);
        if(!blend)glDisable(GL_BLEND);if(cull)glEnable(GL_CULL_FACE);
    }

    bool Renderer::loadMeadow(const char* meshPath)
    {
        std::ifstream input(meshPath);
        std::string magic;
        unsigned int version = 0, count = 0, instances = 0;
        if (!(input >> magic >> version >> count >> instances) || magic != "OW3D_MEADOW" ||
            version != 1 || count == 0 || count > 3000 || count % 3 ||
            instances == 0 || instances > 1000000) return false;
        std::vector<float> vertices(static_cast<size_t>(count) * 9);
        for (auto& value : vertices)
            if (!(input >> value) || !std::isfinite(value)) return false;
        std::vector<float> transforms(static_cast<size_t>(instances) * 8);
        for (auto& value : transforms)
            if (!(input >> value) || !std::isfinite(value)) return false;
        if (!m_meadow.create(vertices.data(), count, 9)) return false;
        if (m_meadowInstances) glDeleteBuffers(1, &m_meadowInstances);
        glGenBuffers(1, &m_meadowInstances);
        m_meadow.bind();
        glBindBuffer(GL_ARRAY_BUFFER, m_meadowInstances);
        glBufferData(GL_ARRAY_BUFFER, transforms.size() * sizeof(float), transforms.data(), GL_STATIC_DRAW);
        for (unsigned int attribute = 4; attribute <= 5; ++attribute)
        {
            glVertexAttribPointer(attribute, 4, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                reinterpret_cast<const void*>((attribute - 4) * 4 * sizeof(float)));
            glEnableVertexAttribArray(attribute);
            glVertexAttribDivisor(attribute, 1);
        }
        glBindVertexArray(0);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
        m_meadowInstanceCount = instances;
        // All tufts share one mesh and one draw; none participates in roof picking.
        return true;
    }

    bool Renderer::loadSceneRoof(const char* meshPath)
    {
        if (!m_hasSceneObjects) return false;
        std::ifstream input(meshPath);
        std::string magic;
        unsigned int version = 0, count = 0;
        if (!(input >> magic >> version >> count) || magic != "OW3D_MESH" || version != 1 ||
            count == 0 || count > 3000000 || count % 3) return false;
        std::vector<float> vertices(static_cast<size_t>(count) * 9);
        for (auto& value : vertices)
            if (!(input >> value) || !std::isfinite(value)) return false;
        if (!m_sceneRoof.create(vertices.data(), count, 9)) return false;
        for (size_t i = 0; i < vertices.size(); i += 9)
        {
            const glm::vec3 p(vertices[i], vertices[i + 1], vertices[i + 2]);
            m_scenePickVertices.push_back(p);
            m_sceneMin = glm::min(m_sceneMin, p);
            m_sceneMax = glm::max(m_sceneMax, p);
        }
        return true;
    }

    void Renderer::showSceneTopView()
    {
        if (m_character.loaded() && m_character.follow) { m_character.setTopDownView(!m_character.topDown,m_camera); return; }
        float roofRadius = PlanetRadius;
        for (const auto& p : m_scenePickVertices)
            roofRadius = std::max(roofRadius, glm::length(p));
        // One metre of clearance above roof height, centered on the character.
        m_camera.showCharacterTopView(roofRadius - PlanetRadius + 1.0f);
    }

    void Renderer::updateSceneHover(float ndcX, float ndcY, bool active)
    {
        m_sceneRoofHidden = false;
        if (!active || !m_hasSceneObjects || !m_sceneRoof.vertexCount() ||
            !std::isfinite(ndcX) || !std::isfinite(ndcY) ||
            std::abs(ndcX) > 1.0f || std::abs(ndcY) > 1.0f) return;
        const glm::mat4 inverse = glm::inverse(m_camera.projectionMatrix() * m_camera.viewMatrix());
        const glm::vec4 farPoint = inverse * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        const glm::vec3 origin = m_camera.position();
        const glm::vec3 direction = glm::normalize(glm::vec3(farPoint) / farPoint.w - origin);
        float nearHit = 0.0f, farHit = std::numeric_limits<float>::max();
        for (int axis = 0; axis < 3; ++axis)
        {
            if (std::abs(direction[axis]) < 1e-7f)
            {
                if (origin[axis] < m_sceneMin[axis] || origin[axis] > m_sceneMax[axis]) return;
                continue;
            }
            const float a = (m_sceneMin[axis] - origin[axis]) / direction[axis];
            const float b = (m_sceneMax[axis] - origin[axis]) / direction[axis];
            nearHit = std::max(nearHit, std::min(a, b));
            farHit = std::min(farHit, std::max(a, b));
            if (nearHit > farHit) return;
        }
        // Ignore the house when it is occluded by the planet on its far side.
        const float projection = glm::dot(origin, direction);
        const float discriminant = projection * projection - glm::dot(origin, origin) + PlanetRadius * PlanetRadius;
        if (discriminant >= 0.0f)
        {
            const float ground = -projection - std::sqrt(discriminant);
            if (ground > 0.0f) farHit = std::min(farHit, ground + 0.015f);
        }
        // Always pick the complete house, including its hidden roof, to avoid flicker.
        for (size_t i = 0; i + 2 < m_scenePickVertices.size(); i += 3)
        {
            const glm::vec3 a = m_scenePickVertices[i];
            const glm::vec3 e1 = m_scenePickVertices[i + 1] - a;
            const glm::vec3 e2 = m_scenePickVertices[i + 2] - a;
            const glm::vec3 p = glm::cross(direction, e2);
            const float determinant = glm::dot(e1, p);
            if (std::abs(determinant) < 1e-8f) continue;
            const float inverseDet = 1.0f / determinant;
            const glm::vec3 t = origin - a;
            const float u = glm::dot(t, p) * inverseDet;
            if (u < 0.0f || u > 1.0f) continue;
            const glm::vec3 q = glm::cross(t, e1);
            const float v = glm::dot(direction, q) * inverseDet;
            if (v < 0.0f || u + v > 1.0f) continue;
            const float distance = glm::dot(e2, q) * inverseDet;
            if (distance >= 0.0f && distance <= farHit)
            {
                m_sceneRoofHidden = true;
                return;
            }
        }
    }

    int Renderer::pickOpening(float x,float y) const {
        const auto inverse=glm::inverse(m_camera.projectionMatrix()*m_camera.viewMatrix());
        auto point=inverse*glm::vec4(x,y,1,1);auto origin=m_camera.position();
        auto direction=glm::normalize(glm::vec3(point)/point.w-origin);
        float limit=HouseOpenings::hit(origin,direction,m_scenePickVertices,sceneRoofHidden()?m_sceneBodyVertexCount:m_scenePickVertices.size());
        float along=glm::dot(origin,direction);float disc=along*along-glm::dot(origin,origin)+PlanetRadius*PlanetRadius;
        if(disc>=0){float t=-along-std::sqrt(disc);if(t>0)limit=std::min(limit,t);}
        return m_openings.pick(origin,direction,limit);
    }

    bool Renderer::pickGround(float x,float y,glm::vec3& ground) const {
        auto inverse=glm::inverse(glm::dmat4(m_camera.projectionMatrix())*glm::dmat4(m_camera.viewMatrix()));
        auto p=inverse*glm::dvec4(x,y,1,1);auto origin=m_camera.position();
        auto direction=glm::vec3(glm::normalize(glm::dvec3(p)/p.w-glm::dvec3(origin)));
        float along=glm::dot(origin,direction),disc=along*along-glm::dot(origin,origin)+PlanetRadius*PlanetRadius;
        float distance=HouseOpenings::hit(origin,direction,m_scenePickVertices,sceneRoofHidden()?m_sceneBodyVertexCount:m_scenePickVertices.size());
        if(disc>=0){float t=-along-std::sqrt(disc);if(t>0)distance=std::min(distance,t);}
        if(!std::isfinite(distance)||distance>5.f||distance<=0||m_openings.pick(origin,direction,distance)>=0)return false;
        auto hit=origin+direction*distance;
        ground=m_character.collision.groundPosition(hit);
        return glm::distance(hit,ground)<.008f;
    }

    bool Renderer::saveScreenshot(const char* bmpPath) const
    {
        std::vector<unsigned char> pixels(static_cast<size_t>(m_viewportWidth) * m_viewportHeight * 4);
        GLint previous = 0;
        glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previous);
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_framebuffer);
        glReadPixels(0, 0, m_viewportWidth, m_viewportHeight, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());
        glBindFramebuffer(GL_READ_FRAMEBUFFER, previous);
        const size_t stride = static_cast<size_t>(m_viewportWidth) * 4;
        for (int y = 0; y < m_viewportHeight / 2; ++y)
            for (size_t x = 0; x < stride; ++x)
                std::swap(pixels[y * stride + x], pixels[(m_viewportHeight - 1 - y) * stride + x]);
        SDL_Surface* surface = SDL_CreateSurfaceFrom(m_viewportWidth, m_viewportHeight,
            SDL_PIXELFORMAT_RGBA32, pixels.data(), static_cast<int>(stride));
        if (!surface) return false;
        const bool saved = SDL_SaveBMP(surface, bmpPath);
        SDL_DestroySurface(surface);
        return saved;
    }

    void Renderer::shutdown()
    {
        m_openings.clear();
        m_character.destroy();
        m_sceneRoof.destroy();
        m_scenePickVertices.clear();
        m_sceneMesh.destroy();
        m_meadow.destroy();
        if (m_meadowInstances) glDeleteBuffers(1, &m_meadowInstances);
        m_meadowInstances = 0;
        m_meadowInstanceCount = 0;
        m_sceneShader.destroy();
        m_hasSceneObjects = false;
        for (auto& [id, texture] : m_tileTextures)
            glDeleteTextures(1, &texture);
        m_tileTextures.clear();
        m_tileTextureArrays.clear();
        m_tileShader.destroy();
        m_hasTileShader = false;
        if (m_depthRenderbuffer) glDeleteRenderbuffers(1, &m_depthRenderbuffer);
        if (m_colorTexture) glDeleteTextures(1, &m_colorTexture);
        if (m_framebuffer) glDeleteFramebuffers(1, &m_framebuffer);
        m_depthRenderbuffer = m_colorTexture = m_framebuffer = 0;
        m_gridMesh.destroy();
        m_mesh.destroy();
        m_shader.destroy();
    }

    

    void Renderer::beginFrame()
    {
        m_frameTime = m_renderTime >= 0.0f ? m_renderTime : static_cast<float>(SDL_GetTicks() / 1000.0);
        glBindFramebuffer(GL_FRAMEBUFFER, m_framebuffer);

        glViewport(
            0,
            0,
            m_viewportWidth,
            m_viewportHeight
        );

        glClearColor(0.08f, 0.10f, 0.14f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        m_shader.bind();

        const float cubeAngle =
            glm::radians(5.0f);

        const glm::vec3 cubeDirection{
            std::sin(cubeAngle),
            0.0f,
            std::cos(cubeAngle)
        };

        const glm::vec3 cubePosition =
            cubeDirection * (PlanetRadius + 0.5f);

        const glm::mat4 model =
            glm::translate(
                glm::mat4(1.0f),
                cubePosition
            );

        m_shader.setMat4("model", model);
        m_shader.setMat4("view", m_camera.viewMatrix());

        m_shader.setMat4(
            "projection",
            m_camera.projectionMatrix()
        );

        if (!m_debugGeometryVisible)
            return;

        // TestwÃƒÂ¼rfel
        m_shader.setVec3(
            "color",
            glm::vec3(1.0f, 0.6f, 0.2f)
        );

        m_mesh.bind();

        glDrawArrays(
            GL_TRIANGLES,
            0,
            static_cast<GLsizei>(m_mesh.vertexCount())
        );

        const glm::mat4 planetModel(1.0f);

        m_shader.setMat4("model", planetModel);

        // Kugel-Grid
        m_shader.setVec3(
            "color",
            glm::vec3(0.35f, 0.40f, 0.48f)
        );

        m_gridMesh.bind();

        glDrawArrays(
            GL_LINES,
            0,
            static_cast<GLsizei>(m_gridMesh.vertexCount())
        );
    }

    unsigned int Renderer::colorTexture() const
    {
        return m_colorTexture;
    }

    void Renderer::endFrame()
    {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    int Renderer::viewportWidth() const
    {
        return m_viewportWidth;
    }

    int Renderer::viewportHeight() const
    {
        return m_viewportHeight;
    }

    void Renderer::resizeViewport(int width, int height)
    {
        if (width <= 0 || height <= 0)
            return;

        if (width == m_viewportWidth &&
            height == m_viewportHeight)
        {
            return;
        }

        m_viewportWidth = width;
        m_viewportHeight = height;

        m_camera.setPerspective(
            m_camera.fieldOfView(),
            static_cast<float>(m_viewportWidth) /
            static_cast<float>(m_viewportHeight),
            0.005f,
            1000.0f
        );

        glBindTexture(GL_TEXTURE_2D, m_colorTexture);

        glTexImage2D(
            GL_TEXTURE_2D,
            0,
            GL_RGBA8,
            m_viewportWidth,
            m_viewportHeight,
            0,
            GL_RGBA,
            GL_UNSIGNED_BYTE,
            nullptr
        );

        glBindRenderbuffer(
            GL_RENDERBUFFER,
            m_depthRenderbuffer
        );

        glRenderbufferStorage(
            GL_RENDERBUFFER,
            GL_DEPTH24_STENCIL8,
            m_viewportWidth,
            m_viewportHeight
        );
    }
   
    void Renderer::setTileMaterialColor(TileMaterialId id, const glm::vec3& color)
    {
        m_tileColors[static_cast<int>(id)] = color;
    }

    glm::vec3 Renderer::tileMaterialColor(TileMaterialId id) const
    {
        const auto found = m_tileColors.find(static_cast<int>(id));
        return found != m_tileColors.end() ? found->second : glm::vec3(1.0f);
    }

    void Renderer::drawTile(
        const glm::vec3& topLeft,
        const glm::vec3& topRight,
        const glm::vec3& bottomLeft,
        const glm::vec3& bottomRight,
        TileMaterialId materialId)
    {
        const float vertices[] =
        {
            topLeft.x,     topLeft.y,     topLeft.z,
            bottomLeft.x,  bottomLeft.y,  bottomLeft.z,
            bottomRight.x, bottomRight.y, bottomRight.z,

            topLeft.x,     topLeft.y,     topLeft.z,
            bottomRight.x, bottomRight.y, bottomRight.z,
            topRight.x,    topRight.y,    topRight.z
        };

        Mesh tileMesh;

        if (!tileMesh.create(vertices, 6))
            return;

        m_shader.setMat4(
            "model",
            glm::mat4(1.0f)
        );

        m_shader.setVec3("color", tileMaterialColor(materialId));

        tileMesh.bind();

        glDrawArrays(
            GL_TRIANGLES,
            0,
            6
        );

        tileMesh.destroy();
    }

    void Renderer::drawTileMesh(
        const Mesh& mesh,
        TileMaterialId materialId)
    {
        Shader& shader = m_hasTileShader ? m_tileShader : m_shader;
        shader.bind();
        shader.setMat4("model", glm::mat4(1.0f));
        shader.setMat4("view", m_camera.viewMatrix());
        shader.setMat4("projection", m_camera.projectionMatrix());
        shader.setVec3("color", tileMaterialColor(materialId));
        shader.setFloat("time", m_frameTime);
        shader.setInt("tileAtlas", 0);
        const auto texture = m_tileTextures.find(static_cast<int>(materialId));
        shader.setInt("hasTexture", texture != m_tileTextures.end());
        glActiveTexture(GL_TEXTURE0);
        const bool array = m_tileTextureArrays[static_cast<int>(materialId)];
        glBindTexture(array ? GL_TEXTURE_2D_ARRAY : GL_TEXTURE_2D,
            texture != m_tileTextures.end() ? texture->second : 0);

        mesh.bind();

        glDrawArrays(
            GL_TRIANGLES,
            0,
            static_cast<GLsizei>(
                mesh.vertexCount()
                )
        );
    }
}
