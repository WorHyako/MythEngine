#include <OpenGLUtils.hpp>

#ifndef __gl_h_
#include <glad/gl.h>
#endif
#include <stb_image.h>
#include <iostream>

Texture2DParameter::Texture2DParameter()
: bLoadHDR(false)
, bFlipVertical(true)
, Format(GL_NONE)
, InternalFormat(GL_NONE)
, TypeData(GL_UNSIGNED_BYTE)
, WrapS(GL_CLAMP_TO_EDGE)
, WrapT(GL_CLAMP_TO_EDGE)
, MinFilter(GL_LINEAR)
, MagFilter(GL_LINEAR)
{
	
}

namespace OpenGLUtils
{
    // utility function for loading a 2D texture from file
    // ---------------------------------------------------
    unsigned int loadTexture2D(char const* path, const Texture2DParam& TexParam)
    {
        unsigned int textureID;
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
        glCreateTextures(GL_TEXTURE_2D, 1, &textureID);

        int width, height, nrChannels;
        // load image, create texture and generate the mipmaps
        stbi_set_flip_vertically_on_load(TexParam.bFlipVertical); // tell stb_image.h to flip loaded texture's on the y-axis.
        void* data = nullptr;
        if (TexParam.bLoadHDR)
        {
            data = stbi_loadf(path, &width, &height, &nrChannels, 0);
        }
        else
        {
            data = stbi_load(path, &width, &height, &nrChannels, 0);
        }
        if (data)
        {
            GLenum format = TexParam.Format;
            GLint internalFormat = TexParam.InternalFormat;
            if (TexParam.Format == GL_NONE)
            {
                if (nrChannels == 1)
                {
                    format = GL_RED;
                    internalFormat = TexParam.bLoadHDR ? GL_R16F : GL_R8;
                }
                else if (nrChannels == 3)
                {
                    format = GL_RGB;
                    internalFormat = TexParam.bLoadHDR ? GL_RGB16F : GL_RGB8;
                }
                else if (nrChannels == 4)
                {
                    format = GL_RGBA;
                    internalFormat = TexParam.bLoadHDR ? GL_RGBA16F : GL_RGBA8;
                }
            }
            if (TexParam.InternalFormat != GL_NONE)
            {
                internalFormat = TexParam.InternalFormat;
            }

            // set the texture wrapping/filtering options (on the currently bound texture object)
            glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, TexParam.WrapS);
            glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, TexParam.WrapT);
            glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, TexParam.MinFilter);
            glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, TexParam.MagFilter);

            glTextureStorage2D(textureID, 1, internalFormat, width, height);
            glTextureSubImage2D(textureID, 0, 0, 0, width, height, format, TexParam.TypeData, data);
            glGenerateTextureMipmap(textureID);

            stbi_image_free(data);
        }
        else
        {
            std::cout << "Texture failed to load at path: " << path << std::endl;
            stbi_image_free(data);
        }

        return textureID;
    }

    unsigned int loadCubemap(std::vector<std::string> faces)
    {
        unsigned int textureID;
        glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &textureID);
        glTextureParameteri(textureID, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTextureParameteri(textureID, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTextureParameteri(textureID, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

        stbi_set_flip_vertically_on_load(false); // tell stb_image.h to flip loaded texture's on the y-axis.
        int width, height, nrChannels;
        unsigned char* data;
        stbi_info(faces[0].c_str(), &width, &height, &nrChannels);
        glTextureStorage2D(textureID, 1, GL_RGB8, width, height);
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTextureSubImage3D(textureID, 0, 0, 0, i, width, height, 1, GL_RGB, GL_UNSIGNED_BYTE, data);
                /*glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                    width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);*/
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }

        return textureID;
    }

    void createTextureAttachment(unsigned int& texture, const unsigned int width, const unsigned int height)
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texture, 0);

        glBindTexture(GL_TEXTURE_2D, 0);

        //glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8, 800, 600, 0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL);

        //glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, texture, 0);
    }

    void createTextureMultisampleAttachment(unsigned int& texture, const float sample, const unsigned int width, const unsigned int height)
    {
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, texture);

        glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, sample, GL_RGB, width, height, GL_TRUE);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, texture, 0);

        glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, 0);
    }

    void createRenderBufferObject(unsigned int& rbo, const unsigned int width, const unsigned int height)
    {
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);

        // use a single renderbuffer object for both a depth AND stencil buffer
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        // now actually attach it
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void createRenderBufferMultisampleObject(unsigned int& rbo, const unsigned int width, const unsigned int height)
    {
        glGenRenderbuffers(1, &rbo);
        glBindRenderbuffer(GL_RENDERBUFFER, rbo);

        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH24_STENCIL8, width, height);

        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    void createVAO(
        unsigned int& VAO,
        unsigned int& VBO,
        unsigned int& EBO,
        typeOfVertex typeVertex,
        std::vector<float> vertices,
        unsigned int sizeOfVertex,
        std::vector<unsigned int> indices)
    {
        glCreateVertexArrays(1, &VAO);
        glCreateBuffers(1, &VBO);

        glNamedBufferData(VBO, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        if (indices.size() > 0)
        {
            glCreateBuffers(1, &EBO);
            glNamedBufferData(EBO, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
        }

        // position attribute
        glEnableVertexArrayAttrib(VAO, 0);
        glVertexArrayAttribFormat(VAO, 0, 3, GL_FLOAT, GL_FALSE, 0);
        glVertexArrayAttribBinding(VAO, 0, 0);
        if (typeVertex == typeOfVertex::PosNormal || typeVertex == typeOfVertex::PosNormalUV)
        {
            // normal attribute
            glEnableVertexArrayAttrib(VAO, 1);
            glVertexArrayAttribFormat(VAO, 1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT));
            glVertexArrayAttribBinding(VAO, 1, 0);
        }
        if (typeVertex == typeOfVertex::PosNormalUV)
        {
            // texture coord attribute
            glEnableVertexArrayAttrib(VAO, 2);
            glVertexArrayAttribFormat(VAO, 2, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GL_FLOAT));
            glVertexArrayAttribBinding(VAO, 2, 0);
        }
        else if (typeVertex == typeOfVertex::PosUV)
        {
            // texture coord attribute
            glEnableVertexArrayAttrib(VAO, 1);
            glVertexArrayAttribFormat(VAO, 1, 2, GL_FLOAT, GL_FALSE, 3 * sizeof(GL_FLOAT));
            glVertexArrayAttribBinding(VAO, 1, 0);
        }

        glVertexArrayVertexBuffer(VAO, 0, VBO, 0, (int)sizeOfVertex * sizeof(GL_FLOAT));
        glVertexArrayElementBuffer(VAO, EBO);
    }

    void bindBuffer(unsigned int& buffer)
    {
        std::vector<unsigned int> data = {};
        glBufferData(GL_ARRAY_BUFFER, data.size() * sizeof(unsigned int), NULL, GL_STATIC_DRAW);

        glBufferSubData(GL_ARRAY_BUFFER, 10, data.size() * sizeof(unsigned int), data.data());

        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        // get pointer
        void* ptr = glMapBuffer(GL_ARRAY_BUFFER, GL_WRITE_ONLY);
        // now copy data into memory
        memcpy(ptr, data.data(), sizeof(data));
        // make sure to tell OpenGL we're done the pointer
        glUnmapBuffer(GL_ARRAY_BUFFER);

        float positions[] = { 1.0f };
        float normals[] = { 1.0f };
        float texs[] = { 1.0f };
        // fill buffer
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(positions), &positions);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions), sizeof(normals), &normals);
        glBufferSubData(GL_ARRAY_BUFFER, sizeof(positions) + sizeof(normals), sizeof(texs), &texs);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), 0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(sizeof(positions)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)(sizeof(positions) + sizeof(normals)));

        unsigned int vbo1, vbo2;
        glGenBuffers(1, &vbo1);
        glGenBuffers(2, &vbo2);
        glCopyNamedBufferSubData(vbo1, vbo2, 0, 0, 8 * sizeof(float));
    }
}