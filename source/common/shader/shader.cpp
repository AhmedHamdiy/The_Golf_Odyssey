#include "shader.hpp"

#include <cassert>
#include <iostream>
#include <filesystem>
#include <string>

#define STB_INCLUDE_LINE_GLSL
#define STB_INCLUDE_IMPLEMENTATION
#include <stb/stb_include.h>

//Forward definition for error checking functions
std::string checkForShaderCompilationErrors(GLuint shader);
std::string checkForLinkingErrors(GLuint program);

bool our::ShaderProgram::attach(const std::string &filename, GLenum type) const { 
    std::filesystem::path file_path = std::filesystem::path(filename);
    std::string file_path_string = file_path.string();
    std::string parent_path_string = file_path.parent_path().string();
    char *path_to_includes = &(parent_path_string[0]);
    char files_error[256];

    auto source = stb_include_file(&(file_path_string[0]), nullptr, path_to_includes, files_error);
    if (!source) {
        std::cerr << "ERROR: " << files_error << std::endl;
        return false;
    }

    //TODO: Complete this function
    //Note: The function "checkForShaderCompilationErrors" checks if there is
    // an error in the given shader. You should use it to check if there is a
    // compilation error and print it so that you can know what is wrong with
    // the shader. The returned string will be empty if there is no errors.
    std::string error;
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, nullptr);
    glCompileShader(shader);
    free(source);
    error = checkForShaderCompilationErrors(shader);
    if (!error.empty()) {
        std::cerr << "ERROR: Couldn't compile shader: " << filename << std::endl;
        std::cerr << error << std::endl;
        glDeleteShader(shader);
        return false;
    }
    glAttachShader(program, shader);
    glDeleteShader(shader);
    //We return true if the compilation succeeded
    return true;
}



bool our::ShaderProgram::link() const {
    //TODO: Complete this function
    //Note: The function "checkForLinkingErrors" checks if there is
    // an error in the given program. You should use it to check if there is a
    // linking error and print it so that you can know what is wrong with the
    // program. The returned string will be empty if there is no errors.
    std::string error;
    glLinkProgram(program);
    error = checkForLinkingErrors(program);
    if (!error.empty()) {
        std::cerr << "ERROR: Couldn't link shader program" << std::endl;
        std::cerr << error << std::endl;
        return false;
    }

    return true;
}

////////////////////////////////////////////////////////////////////
// Function to check for compilation and linking error in shaders //
////////////////////////////////////////////////////////////////////

std::string checkForShaderCompilationErrors(GLuint shader){
     //Check and return any error in the compilation process
    GLint status;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &status);
    if (!status) {
        GLint length;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &length);
        char *logStr = new char[length];
        glGetShaderInfoLog(shader, length, nullptr, logStr);
        std::string errorLog(logStr);
        delete[] logStr;
        return errorLog;
    }
    return std::string();
}

std::string checkForLinkingErrors(GLuint program){
     //Check and return any error in the linking process
    GLint status;
    glGetProgramiv(program, GL_LINK_STATUS, &status);
    if (!status) {
        GLint length;
        glGetProgramiv(program, GL_INFO_LOG_LENGTH, &length);
        char *logStr = new char[length];
        glGetProgramInfoLog(program, length, nullptr, logStr);
        std::string error(logStr);
        delete[] logStr;
        return error;
    }
    return std::string();
}