#pragma once
#ifndef MESHVIEW_TEXTURE_4C9E53AB_10FA_4057_8EDB_1E117271A1C3
#define MESHVIEW_TEXTURE_4C9E53AB_10FA_4057_8EDB_1E117271A1C3

#include "meshview/common.hpp"
#include <string>

namespace meshview {

// Represents a texture/material
struct Texture {
    using Image = Eigen::Matrix<float, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    using ImageU = Eigen::Matrix<uint8_t, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>;
    // Texture types
    enum {
        TYPE_DIFFUSE,
        TYPE_SPECULAR,
        // TYPE_NORMAL,
        // TYPE_HEIGHT,
        __TYPE_COUNT
    };
    // Texture type names (TYPE_DIFFUSE -> "diffuse" etc)
    static constexpr inline const char* type_to_name(int type) {
        switch(type) {
            case TYPE_SPECULAR: return "specular";
            // case TYPE_NORMAL: return "normal";
            // case TYPE_HEIGHT: return "height";
            default: return "diffuse";
        }
    }

    // Texture from path
    Texture(const std::string& path, bool flip = true);
    // Texture from solid color (1x1)
    Texture(float r, float g, float b);
    // Texture from an (rows, cols*3) row-major
    // red(1-channel)/RGB(3-channel)/RGBA(4-channel) f32 matrix
    Texture(const Eigen::Ref<const Image>& im, int n_channels = 1);

    ~Texture();

    // Load texture; need to be called before first use in each context
    // (called by Mesh::reset())
    void load();

    // ADVANCED: free buffers, called by destructor
    void free_bufs();

    // GL texture id; -1 if unavailable
    Index id = -1;

private:
    // File path (optional)
    std::string path;

    // Image data (optional)
    Image im_data;
    // Channels in above
    int n_channels;

    // Color to use if path empty OR failed to load the texture image
    Vector3f fallback_color;

    // Vertical flip on load?
    bool flip;
};

}  // namespace meshview

#endif  // ifndef MESHVIEW_TEXTURE_4C9E53AB_10FA_4057_8EDB_1E117271A1C3
