#include "meshview/meshview.hpp"

#include <iostream>
#include <GL/glew.h>
#include "stb_image.h"
#include "meshview/util.hpp"
#include "meshview/internal/assert.hpp"

namespace meshview {

// *** Texture ***
Texture::Texture(const std::string& path, bool flip)
    : path(path), fallback_color(/*pink*/ 1.f, 0.75f, 0.8f), flip(flip) {
}

Texture::Texture(float r, float g, float b) : fallback_color(r, g, b) { }
Texture::Texture(const Eigen::Ref<const Image>& im, int n_channels) : im_data(im),
     n_channels(n_channels), fallback_color(/*pink*/ 1.f, 0.75f, 0.8f), flip(false) {
    _MESHVIEW_ASSERT(n_channels == 1 || n_channels == 3 || n_channels == 4);
    _MESHVIEW_ASSERT_EQ(im.cols() % n_channels, 0);
}

Texture::~Texture() {
    free_bufs();
}

void Texture::free_bufs() {
    if (~id) glDeleteTextures(1, &id);
    id = -1;
}

void Texture::load() {
    if (!~id)
        glGenTextures(1, &id);
    glBindTexture(GL_TEXTURE_2D, id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    auto gl_load_mipmap = [](int width, int height, int chnls, void* data,
                             int dtype) {
        _MESHVIEW_ASSERT(chnls == 1 || chnls == 3 || chnls == 4);
        GLenum format;
        if (chnls == 1)
            format = GL_RED;
        else if (chnls == 3)
            format = GL_RGB;
        else //if (chnls == 4)
            format = GL_RGBA;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height,
                0, format, dtype, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    };

    bool success = false;
    if (im_data.rows()) {
        // From memory
        gl_load_mipmap(im_data.cols() / n_channels, im_data.rows(), n_channels,
                (void*) im_data.data(), GL_FLOAT);
        success = true;
    } else if (path.size()) {
        // From file
        stbi_set_flip_vertically_on_load(flip);
        int width, height, chnls;
        uint8_t * data = stbi_load(path.c_str(), &width, &height, &chnls, 0);
        if (data) {
            im_data.resize(height, width * chnls);
            im_data.noalias() = Eigen::Map<ImageU>(data, height, width * chnls)
                .cast<float>() / 255.f;
            n_channels = chnls;
            gl_load_mipmap(width, height, chnls, (void*) data, GL_UNSIGNED_BYTE);
            stbi_image_free(data);
            success = true;
        } else {
            std::cerr << "Failed to load texture " << path << ", using fallback color\n";
        }
    }
    if (!success) {
        gl_load_mipmap(1, 1, 3, fallback_color.data(), GL_FLOAT);
    }
}

}  // namespace
