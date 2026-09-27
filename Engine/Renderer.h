#pragma once

#include "Shader.h"
#include "Mesh.h"
#include "Camera.h"
#include "AnimatedCharacter.h"
#include "TileMaterial.h"
#include "HouseOpenings.h"

#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>

namespace ow3d
{
    class Renderer
    {
    public:
        ~Renderer();
        AnimatedCharacter& character() { return m_character; }
        void drawCharacter() { m_character.draw(m_camera, m_viewportHeight); }
        bool loadTileShader(const char* vertexPath, const char* fragmentPath);
        bool setTileMaterialTexture(TileMaterialId id, const char* bmpPath,
            bool smooth = false, bool quadrantArray = false);
        bool loadSceneObjects(const char* meshPath, const char* bmpPath,
            const char* vertexPath, const char* fragmentPath);
        void drawSceneObjects();
        void drawSceneGlass();
        HouseOpenings& openings() { return m_openings; }
        void loadSceneOpenings(const std::filesystem::path& path) { m_openings.load(path); }
        void updateOpenings(float dt) { auto pos=m_character.position();m_openings.update(dt,m_character.collision,m_character.loaded()?&pos:nullptr); }
        int pickOpening(float ndcX,float ndcY) const;
        bool pickGround(float x,float y,glm::vec3& ground) const;
        bool loadSceneRoof(const char* meshPath);
        bool loadMeadow(const char* meshPath);
        void updateSceneHover(float ndcX, float ndcY, bool active);
        bool sceneRoofHidden() const { return m_sceneRoofHidden || (m_character.loaded() && m_character.collision.contains(m_character.position())); }
        glm::vec3 sceneCenter() const { return (m_sceneMin + m_sceneMax) * 0.5f; }
        void showSceneTopView();
        bool saveScreenshot(const char* bmpPath) const;
        void setRenderTime(float seconds) { m_renderTime = seconds; }
        float frameTime() const { return m_frameTime; }
        void setDebugGeometryVisible(bool visible) { m_debugGeometryVisible = visible; }
        void setTileMaterialColor(TileMaterialId id, const glm::vec3& color);

        bool initialize();
        void shutdown();

        void beginFrame();
        void endFrame();

        int viewportWidth() const;
        int viewportHeight() const;

        Camera& camera()
        {
            return m_camera;
        }

        unsigned int colorTexture() const;

        void resizeViewport(int width, int height);

        void drawTile(
            const glm::vec3& topLeft,
            const glm::vec3& topRight,
            const glm::vec3& bottomLeft,
            const glm::vec3& bottomRight,
            TileMaterialId materialId
        );

        void drawTileMesh(
            const Mesh& mesh,
            TileMaterialId materialId
        );

    private:
        glm::vec3 tileMaterialColor(TileMaterialId id) const;
        std::unordered_map<int, glm::vec3> m_tileColors;

        Shader m_tileShader;
        Shader m_sceneShader;
        Mesh m_sceneMesh;
        HouseOpenings m_openings;
        size_t m_sceneBodyVertexCount=0;
        Mesh m_sceneRoof;
        Mesh m_meadow;
        unsigned int m_meadowInstances = 0;
        unsigned int m_meadowInstanceCount = 0;
        std::vector<glm::vec3> m_scenePickVertices;
        glm::vec3 m_sceneMin{0.0f}, m_sceneMax{0.0f};
        bool m_sceneRoofHidden = false;
        bool m_hasSceneObjects = false;
        float m_renderTime = -1.0f;
        float m_frameTime = 0.0f;
        bool m_hasTileShader = false;
        bool m_debugGeometryVisible = true;
        std::unordered_map<int, unsigned int> m_tileTextures;
        std::unordered_map<int, bool> m_tileTextureArrays;
        Shader m_shader;
        Mesh m_mesh;
        Mesh m_gridMesh;
        Camera m_camera;
        AnimatedCharacter m_character;

        unsigned int m_framebuffer = 0;
        unsigned int m_colorTexture = 0;
        unsigned int m_depthRenderbuffer = 0;

        int m_viewportWidth = 1280;
        int m_viewportHeight = 720;
    };
}
