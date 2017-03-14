#pragma once

#include <map>
#include <vector>
#include <string>

// usually I avoid including this because it is so large, but the interested party must have 
// access to GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
#include "ThirdParty/glload/include/glload/gl_4_4.h"

/*------------------------------------------------------------------------------------------------
Description:
    Handles the assembly, storage, and retrieval of different shader programs.  The term 
    "storage" does not imply assembly, but the main functionality of this class after startup 
    will be storage and retrieval of shader program ID, so "storage" seemed to be an appropriate 
    description.
Creator:    John Cox (7-4-2016)
------------------------------------------------------------------------------------------------*/
class ShaderStorage
{
public:
    static ShaderStorage &GetInstance();

    ~ShaderStorage();
    void NewShader(const std::string &programKey);
    void NewCompositeShader(const std::string &programKey);
    void DeleteShader(const std::string &programKey);

    void AddAndCompileShaderFile(const std::string &programKey, const std::string &filePath, const GLenum shaderType);
    void AddPartialShaderFile(const std::string &programKey, const std::string &filePath);
    void CompileCompositeShader(const std::string &programKey, const GLenum shaderType);
    
    GLuint LinkShader(const std::string &programKey);
    GLuint GetShaderProgram(const std::string &programKey) const;
    GLint GetUniformLocation(const std::string &programKey,
        const std::string &uniformName) const;
    GLint GetAttributeLocation(const std::string &programKey,
        const std::string &attributeName) const;

private:
    // defined privately to enforce singleton-ness
    ShaderStorage();
    ShaderStorage(const ShaderStorage&) {}
    ShaderStorage &operator=(const ShaderStorage&) {}

    GLuint CompileShader(const std::string &shaderAsString, const GLenum shaderType);

    typedef std::map<std::string, GLuint> _PROGRAM_MAP;
    _PROGRAM_MAP _compiledPrograms;

    // before a shader program is compiled, it is a collection of binaries, so each shader 
    // program can have multiple binaries
    // Note: The typedefs make typing easier when checking for iterator 
    typedef std::map<std::string, std::vector<GLuint>> _BINARY_MAP;
    _BINARY_MAP _shaderBinaries;

    // used to piece together shader files before compiling
    // Note: This item allows me to keep a bunch of separate files for individual structures 
    // instead of repeating them, and it allows me to use C-style header blocks with #define 
    // declarations that both the shader and C++ can read, which means that I can declare buffer 
    // and uniform binding locations with constants instead of having to look them up at program 
    // start.
    //typedef std::map<std::string, std::map<GLuint, std::string>> _COMPOSITE_SHADER_MAP;
    typedef std::map<std::string, std::string> _COMPOSITE_SHADER_MAP;
    _COMPOSITE_SHADER_MAP _partialShaderContents;
};