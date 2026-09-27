#pragma once

#include <glm/glm.hpp>
#include <string>
#include <string_view>
#include <unordered_map>

namespace ow3d
{
    class Shader
    {
    public:
        Shader();
        ~Shader();

        bool create(const char* vertexSource, const char* fragmentSource);
        void destroy();

        void bind() const;
        void setInt(const char* name, int value) const;
        void setFloat(const char* name, float value) const;
        void setMat4(const char* name, const glm::mat4& matrix) const;
        void setVec3(const char* name, const glm::vec3& value) const;

    private:
        unsigned int m_program = 0;
        struct NameHash {
            using is_transparent = void;
            size_t operator()(std::string_view name) const noexcept { return std::hash<std::string_view>{}(name); }
        };
        mutable std::unordered_map<std::string,int,NameHash,std::equal_to<>> m_uniformLocations;
        int uniformLocation(const char* name) const;
    };
}
