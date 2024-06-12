#include <RHI/OpenGL/Framework/GLShader.hpp>
#include <Utils/Utils.hpp>

#include <glad/gl.h>
#include <assert.h>
#include <stdio.h>
#include <string>


GLShader::GLShader(GLenum type, const char* text)
	: type_(type)
	, handle_(glCreateShader(type))
{
	glShaderSource(handle_, 1, &text, nullptr);
	glCompileShader(handle_);

	char buffer[8192];
	GLsizei length = 0;
	glGetShaderInfoLog(handle_, sizeof(buffer), &length, buffer);
	if (length)
	{
		printf("%s\n", buffer);
		printShaderSource(text);
		assert(false);
	}
}

GLShader::GLShader(const char* fileName)
	:GLShader(GLShaderTypeFromFileName(fileName), readShaderFile(fileName).c_str())
{

}

GLShader::~GLShader()
{
	glDeleteShader(handle_);
}

void printProgramInfoLog(GLuint handle)
{
	char buffer[8192];
	GLsizei length = 0;
	glGetProgramInfoLog(handle, sizeof(buffer), &length, buffer);
	if (length)
	{
		printf("%s\n", buffer);
		assert(false);
	}
}

GLProgram::GLProgram(const GLShader& a)
	: handle_(glCreateProgram())
{
	glAttachShader(handle_, a.getHandle());
	glLinkProgram(handle_);
	printProgramInfoLog(handle_);
}

GLProgram::GLProgram(const GLShader& a, const GLShader& b)
	: handle_(glCreateProgram())
{
	glAttachShader(handle_, a.getHandle());
	glAttachShader(handle_, b.getHandle());
	glLinkProgram(handle_);
	printProgramInfoLog(handle_);
}

GLProgram::GLProgram(const GLShader& a, const GLShader& b, const GLShader& c)
	: handle_(glCreateProgram())
{
	glAttachShader(handle_, a.getHandle());
	glAttachShader(handle_, b.getHandle());
	glAttachShader(handle_, c.getHandle());
	glLinkProgram(handle_);
	printProgramInfoLog(handle_);
}

GLProgram::GLProgram(const GLShader& a, const GLShader& b, const GLShader& c, const GLShader& d, const GLShader& e)
	: handle_(glCreateProgram())
{
	glAttachShader(handle_, a.getHandle());
	glAttachShader(handle_, b.getHandle());
	glAttachShader(handle_, c.getHandle());
	glAttachShader(handle_, d.getHandle());
	glAttachShader(handle_, e.getHandle());
	glLinkProgram(handle_);
	printProgramInfoLog(handle_);
}

GLProgram::~GLProgram()
{
	glDeleteProgram(handle_);
}

void GLProgram::useProgram() const
{
	glUseProgram(handle_);
}

GLBuffer::GLBuffer(GLsizeiptr size, const void* data, GLbitfield flags)
{
	glCreateBuffers(1, &handle_);
	glNamedBufferStorage(handle_, size, data, flags);
}

GLBuffer::~GLBuffer()
{
	glDeleteBuffers(1, &handle_);
}