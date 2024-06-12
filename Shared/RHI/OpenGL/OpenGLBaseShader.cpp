#include <RHI/OpenGL/OpenGLBaseShader.hpp>

#include <Filesystem/FilesystemUtilities.hpp>
#include <Utils/Utils.hpp>

Shader::Shader() : ShaderInterface()
{

}

Shader::Shader(const char* vertexPath, const char* fragmentPath, const char* geometryPath) :
	ShaderInterface()
{
	// 1. retrieve the vertex/fragment source code from filepath
	std::string vertexCode;
	std::string fragmentCode;
	std::string geometryCode;
	std::ifstream vShaderFile;
	std::ifstream fShaderFile;
	std::ifstream gShaderFile;
	// ensure ifstream object can throw exceptions
	vShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	fShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);
	gShaderFile.exceptions(std::ifstream::failbit | std::ifstream::badbit);

	bool gShaderValid = geometryPath != nullptr;

	try
	{
		// open files
		vShaderFile.open(FilesystemUtilities::GetShadersDir() + "OpenGL/" + vertexPath);
		fShaderFile.open(FilesystemUtilities::GetShadersDir() + "OpenGL/" + fragmentPath);
		std::stringstream vShaderStream, fShaderStream, gShaderStream;
		// read file's buffer contents into streams
		vShaderStream << vShaderFile.rdbuf();
		fShaderStream << fShaderFile.rdbuf();
		// close file handlers
		vShaderFile.close();
		fShaderFile.close();
		// convert stream into string
		vertexCode = vShaderStream.str();
		fragmentCode = fShaderStream.str();
		if (gShaderValid)
		{
			gShaderFile.open(FilesystemUtilities::GetShadersDir() + "OpenGL/" + geometryPath);
			gShaderStream << gShaderFile.rdbuf();
			gShaderFile.close();
			geometryCode = gShaderStream.str();
		}
	}
	catch (std::ifstream::failure& e)
	{
		std::cout << "ERROR::SHADER::FILE_NOT_SUCCESSFULLY_READ: " << e.what() << std::endl;
	}
	const char* vShaderCode = vertexCode.c_str();
	const char* fShaderCode = fragmentCode.c_str();
	const char* gShaderCode = geometryCode.c_str();
	// 2. compile shaders
	unsigned int vertex, fragment, geometry;
	vertex = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertex, 1, &vShaderCode, NULL);
	glCompileShader(vertex);
	checkCompileErrors(vertex, "VERTEX");
	// fragment shader
	fragment = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragment, 1, &fShaderCode, NULL);
	glCompileShader(fragment);
	checkCompileErrors(fragment, "FRAGMENT");
	// geometry shader
	if (gShaderValid)
	{
		geometry = glCreateShader(GL_GEOMETRY_SHADER);
		glShaderSource(geometry, 1, &gShaderCode, NULL);
		glCompileShader(geometry);
		checkCompileErrors(geometry, "GEOMETRY");
	}
	// shader Program
	ID = glCreateProgram();
	glAttachShader(ID, vertex);
	glAttachShader(ID, fragment);
	if (gShaderValid)
		glAttachShader(ID, geometry);
	glLinkProgram(ID);
	checkCompileErrors(ID, "PROGRAM");
	// delete the shaders as they're linkend into our program now and no longer necessary
	glDeleteShader(vertex);
	glDeleteShader(fragment);
	if (gShaderValid)
		glDeleteShader(geometry);
}

GLenum GLShaderTypeFromFileName(const char* fileName)
{
	if (endsWith(fileName, ".vert")) return GL_VERTEX_SHADER;
	if (endsWith(fileName, ".frag")) return GL_FRAGMENT_SHADER;
	if (endsWith(fileName, ".geom")) return GL_GEOMETRY_SHADER;
	if (endsWith(fileName, ".tesc")) return GL_TESS_CONTROL_SHADER;
	if (endsWith(fileName, ".tese")) return GL_TESS_EVALUATION_SHADER;
	if (endsWith(fileName, ".comp")) return GL_COMPUTE_SHADER;
	assert(false);
	return 0;
}
