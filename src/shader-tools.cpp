// BSD 3 - Clause License
//
// Copyright(c) 2020, Aaron Hornby
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met :
//
// 1. Redistributions of source code must retain the above copyright notice, this
// list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
// this list of conditions and the following disclaimer in the documentation
// and/or other materials provided with the distribution.
//
// 3. Neither the name of the copyright holder nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED.IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
// FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
//     SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
//     CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
//     OR TORT(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
//     OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#include "shader-tools.h"

namespace wave_tool
{
    GLuint ShaderTools::compileShaders(char const *vertexFilename, char const *fragmentFilename,
                                       char const *tessControlFilename, char const *tessEvalFilename)
    {
        GLuint vertex_shader;
        GLuint fragment_shader;
        GLuint tess_control_shader = 0; // Optional
        GLuint tess_eval_shader = 0;    // Optional
        GLuint program;

        GLchar const *vertex_shader_source[] = {loadshader(vertexFilename)};
        GLchar const *fragment_shader_source[] = {loadshader(fragmentFilename)};
        GLchar const *tess_control_shader_source[] = {tessControlFilename ? loadshader(tessControlFilename) : nullptr};
        GLchar const *tess_eval_shader_source[] = {tessEvalFilename ? loadshader(tessEvalFilename) : nullptr};

        // Create and compile vertex shader
        vertex_shader = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex_shader, 1, vertex_shader_source, nullptr);
        glCompileShader(vertex_shader);
        checkShaderCompilation(vertex_shader, "vertex_shader");

        // Create and compile fragment shader
        fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment_shader, 1, fragment_shader_source, nullptr);
        glCompileShader(fragment_shader);
        checkShaderCompilation(fragment_shader, "fragment_shader");

        // Optional: Create and compile tessellation control shader
        if (tess_control_shader_source[0])
        {
            tess_control_shader = glCreateShader(GL_TESS_CONTROL_SHADER);
            glShaderSource(tess_control_shader, 1, tess_control_shader_source, nullptr);
            glCompileShader(tess_control_shader);
            checkShaderCompilation(tess_control_shader, "tess_control_shader");
        }

        // Optional: Create and compile tessellation evaluation shader
        if (tess_eval_shader_source[0])
        {
            tess_eval_shader = glCreateShader(GL_TESS_EVALUATION_SHADER);
            glShaderSource(tess_eval_shader, 1, tess_eval_shader_source, nullptr);
            glCompileShader(tess_eval_shader);
            checkShaderCompilation(tess_eval_shader, "tess_eval_shader");
        }

        // Create program, attach shaders to it, and link it
        program = glCreateProgram();
        glAttachShader(program, vertex_shader);
        if (tess_control_shader)
            glAttachShader(program, tess_control_shader);
        if (tess_eval_shader)
            glAttachShader(program, tess_eval_shader);
        glAttachShader(program, fragment_shader);

        glLinkProgram(program);
        checkProgramLink(program);

        // Delete the shaders as the program has them now
        glDeleteShader(vertex_shader);
        glDeleteShader(fragment_shader);
        if (tess_control_shader)
            glDeleteShader(tess_control_shader);
        if (tess_eval_shader)
            glDeleteShader(tess_eval_shader);

        unloadshader((GLchar **)vertex_shader_source);
        unloadshader((GLchar **)fragment_shader_source);
        if (tess_control_shader_source[0])
            unloadshader((GLchar **)tess_control_shader_source);
        if (tess_eval_shader_source[0])
            unloadshader((GLchar **)tess_eval_shader_source);

        return program;
    }

    unsigned long ShaderTools::getFileLength(std::ifstream &file)
    {
        if (!file.good())
            return 0;

        file.seekg(0, std::ios::end);
        unsigned long len = file.tellg();
        file.seekg(std::ios::beg);

        return len;
    }

    GLchar *ShaderTools::loadshader(std::string filename)
    {
        std::ifstream file;
        file.open(filename.c_str(), std::ios::in); // opens as ASCII!
        if (!file)
            return nullptr;

        unsigned long len = getFileLength(file);

        if (0 == len)
            return nullptr; // Error: Empty File

        GLchar *ShaderSource = nullptr;
        ShaderSource = new char[len + 1];
        if (nullptr == ShaderSource)
            return nullptr; // can't reserve memoryf

        // len isn't always strlen cause some characters are stripped in ascii read...
        // it is important to 0-terminate the real length later, len is just max possible value...
        ShaderSource[len] = 0;

        unsigned int i = 0;
        while (file.good())
        {
            ShaderSource[i] = file.get(); // get character from file.
            if (!file.eof())
                ++i;
        }

        ShaderSource[i] = 0; // 0-terminate it at the correct position

        file.close();

        return ShaderSource; // No Error
    }

    void ShaderTools::unloadshader(GLchar **ShaderSource)
    {
        if (nullptr != *ShaderSource)
        {
            delete[] *ShaderSource;
        }
        *ShaderSource = nullptr;
    }

    void ShaderTools::checkShaderCompilation(GLuint shader, const char *shaderName)
    {
        GLint status;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

        if (GL_FALSE == status)
        {
            GLint infoLogLength;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLogLength);

            GLchar *strInfoLog = new GLchar[infoLogLength + 1];
            glGetShaderInfoLog(shader, infoLogLength, nullptr, strInfoLog);

            fprintf(stderr, "Compilation error in %s: %s\n", shaderName, strInfoLog);
            delete[] strInfoLog;
        }
    }

    void ShaderTools::checkProgramLink(GLuint program)
    {
        GLint status;
        glGetProgramiv(program, GL_LINK_STATUS, &status);

        if (GL_FALSE == status)
        {
            GLint infoLogLength;
            glGetProgramiv(program, GL_INFO_LOG_LENGTH, &infoLogLength);

            GLchar *strInfoLog = new GLchar[infoLogLength + 1];
            glGetProgramInfoLog(program, infoLogLength, nullptr, strInfoLog);

            fprintf(stderr, "Linking error in program: %s\n", strInfoLog);
            delete[] strInfoLog;
        }
    }

}
