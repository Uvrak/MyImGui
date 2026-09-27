#include "Shader.h"
#include <glad/gl.h>
#include <glm/gtc/type_ptr.hpp>

namespace ow3d
{
    namespace
    {
        unsigned int compileShader(unsigned int type, const char* source)
        {
            const unsigned int shader = glCreateShader(type);

            glShaderSource(shader, 1, &source, nullptr);
            glCompileShader(shader);

            int success = 0;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

            if (!success)
            {
                glDeleteShader(shader);
                return 0;
            }

            return shader;
        }
    }

    Shader::Shader() = default;

    Shader::~Shader()
    {
        destroy();
    }

    bool Shader::create(
        const char* vertexSource,
        const char* fragmentSource)
    {
        destroy();
        const unsigned int vertexShader =
            compileShader(GL_VERTEX_SHADER, vertexSource);

        if (!vertexShader)
            return false;

        const unsigned int fragmentShader =
            compileShader(GL_FRAGMENT_SHADER, fragmentSource);

        if (!fragmentShader)
        {
            glDeleteShader(vertexShader);
            return false;
        }

        m_program = glCreateProgram();

        glAttachShader(m_program, vertexShader);
        glAttachShader(m_program, fragmentShader);

        glLinkProgram(m_program);

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        int success = 0;
        glGetProgramiv(m_program, GL_LINK_STATUS, &success);

        if (!success)
        {
            destroy();
            return false;
        }

        return true;
    }

    void Shader::destroy()
    {
        m_uniformLocations.clear();
        if (m_program)
        {
            glDeleteProgram(m_program);
            m_program = 0;
        }
    }

    void Shader::bind() const
    {
        glUseProgram(m_program);
    }
    
    void Shader::setInt(const char* name, int value) const
    {
        glUniform1i(uniformLocation(name), value);
    }

    void Shader::setFloat(const char* name, float value) const
    {
        glUniform1f(uniformLocation(name), value);
    }

    void Shader::setMat4(
        const char* name,
        const glm::mat4& matrix) const
    {
        const int location =
            uniformLocation(name);

        glUniformMatrix4fv(
            location,
            1,
            GL_FALSE,
            glm::value_ptr(matrix)
        );
    }

    void Shader::setVec3(
        const char* name,
        const glm::vec3& value) const
    {
        const int location =
            uniformLocation(name);

        glUniform3fv(location, 1, glm::value_ptr(value));
    }
    int Shader::uniformLocation(const char* name) const
    {
        const auto found=m_uniformLocations.find(std::string_view(name));
        if(found!=m_uniformLocations.end())return found->second;
        const int location=glGetUniformLocation(m_program,name);
        m_uniformLocations.emplace(name,location); // Missing/optimized-out uniforms (-1) are cached too.
        return location;
    }
}
