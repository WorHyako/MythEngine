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
    unsigned int loadTexture(char const* path, const Texture2DParam& TexParam)
    {
        unsigned int textureID;
        glGenTextures(1, &textureID);

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
                    format = GL_RED;
                else if (nrChannels == 3)
                    format = GL_RGB;
                else if (nrChannels == 4)
                    format = GL_RGBA;
            }
            if (TexParam.InternalFormat == GL_NONE) internalFormat = static_cast<GLint>(format);

            glBindTexture(GL_TEXTURE_2D, textureID);
            glTexImage2D(GL_TEXTURE_2D, 0,
                internalFormat,
                width, height, 0, format, TexParam.TypeData, data);
            glGenerateMipmap(GL_TEXTURE_2D);

            // set the texture wrapping/filtering options (on the currently bound texture object)
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, TexParam.WrapS);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, TexParam.WrapT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, TexParam.MinFilter);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, TexParam.MagFilter);

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
        glGenTextures(1, &textureID);
        glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

        stbi_set_flip_vertically_on_load(false); // tell stb_image.h to flip loaded texture's on the y-axis.
        int width, height, nrChannels;
        unsigned char* data;
        for (unsigned int i = 0; i < faces.size(); i++)
        {
            data = stbi_load(faces[i].c_str(), &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB,
                    width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
                stbi_image_free(data);
            }
            else
            {
                std::cout << "Cubemap tex failed to load at path: " << faces[i] << std::endl;
                stbi_image_free(data);
            }
        }
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

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

    void createBuffer(
        unsigned int& VAO,
        unsigned int& VBO,
        unsigned int& EBO,
        typeOfVertex typeVertex,
        std::vector<float> vertices,
        unsigned int sizeOfVertex,
        std::vector<unsigned int> indices)
    {
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
        // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
        glBindVertexArray(VAO);

        glBindBuffer(GL_ARRAY_BUFFER, VBO);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), &vertices[0], GL_STATIC_DRAW);

        if (indices.size() > 0)
        {
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(int), &indices[0], GL_STATIC_DRAW);
        }

        // position attribute
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        if (typeVertex == typeOfVertex::PosNormal || typeVertex == typeOfVertex::PosNormalUV)
        {
            // normal attribute
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
        }
        if (typeVertex == typeOfVertex::PosNormalUV)
        {
            // texture coord attribute
            glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)(6 * sizeof(float)));
            glEnableVertexAttribArray(2);
        }
        else if (typeVertex == typeOfVertex::PosUV)
        {
            // texture coord attribute
            glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, (int)sizeOfVertex * sizeof(float), (void*)(3 * sizeof(float)));
            glEnableVertexAttribArray(1);
        }

        // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
        glBindBuffer(GL_ARRAY_BUFFER, 0);

        // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
        //if (sizeof(indices) != 0)
            //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

        // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
        // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
        //glBindVertexArray(0);*/
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
        glBindBuffer(GL_COPY_READ_BUFFER, vbo1);
        // another approach
        //glBindBuffer(GL_ARRAY_BUFFER, vbo1);
        glBindBuffer(GL_COPY_WRITE_BUFFER, vbo2);
        glCopyBufferSubData(GL_COPY_READ_BUFFER, GL_COPY_WRITE_BUFFER, 0, 0, 8 * sizeof(float));
    }
}