#include "meshview/meshview.hpp"

#include <cmath>
#include <iostream>
#include <fstream>
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <Eigen/Geometry>

#include "meshview/util.hpp"
#include "meshview/internal/shader.hpp"
#include "meshview/internal/assert.hpp"

namespace meshview {
namespace {

void shader_set_transform_matrices(const internal::Shader& shader,
                                   const Camera& camera,
                                   const Matrix4f& transform) {
    shader.set_mat4("M", transform);
    shader.set_mat4("MVP", camera.proj * camera.view * transform);

    auto normal_matrix = transform.topLeftCorner<3, 3>().inverse().transpose();
    shader.set_mat3("NormalMatrix", normal_matrix);
}

}  // namespace

// *** Mesh ***
Mesh::Mesh(size_t num_verts, size_t num_triangles) : VAO((Index)-1) {
    resize(num_verts, num_triangles);
}

Mesh::Mesh(const std::string& path) { load_basic_obj(path); }

Mesh::Mesh(const Eigen::Ref<const Points>& pos,
           const Eigen::Ref<const Triangles>& tri_faces,
           const Eigen::Ref<const Points>& rgb,
           const Eigen::Ref<const Points>& normals)
    : Mesh(pos.rows(), tri_faces.rows()) {
    if (tri_faces.rows()) faces.noalias() = tri_faces;
    verts_pos().noalias() = pos;
    if (rgb.rows()) {
        _MESHVIEW_ASSERT_EQ(rgb.rows(), pos.rows());
        verts_rgb().noalias() = rgb;
    }
    _auto_normals = !normals.rows();
    if (!_auto_normals) {
        _MESHVIEW_ASSERT_EQ(normals.rows(), pos.rows());
        verts_norm().noalias() = normals;
    }
}

Mesh::Mesh(const Eigen::Ref<const Points>& pos,
           const Eigen::Ref<const Triangles>& tri_faces, float r, float g,
           float b, const Eigen::Ref<const Points>& normals)
    : Mesh(pos.rows(), tri_faces.rows()) {
    _MESHVIEW_ASSERT_LT(0, pos.rows());
    if (tri_faces.rows()) faces.noalias() = tri_faces;
    verts_pos().noalias() = pos;
    verts_rgb().rowwise() = Eigen::RowVector3f(r, g, b);
    _auto_normals = !normals.rows();
    if (!_auto_normals) {
        _MESHVIEW_ASSERT_EQ(normals.rows(), pos.rows());
        verts_norm().noalias() = normals;
    }
}

Mesh::~Mesh() { free_bufs(); }

void Mesh::resize(size_t num_verts, size_t num_triangles) {
    data.resize(num_verts, data.ColsAtCompileTime);
    if (num_triangles == 0) {
        _MESHVIEW_ASSERT_EQ(num_verts % 3, 0);
        faces.resize(num_verts / 3, 3);
        for (Index i = 0; i < (Index)num_verts; ++i) {
            faces.data()[i] = i;
        }
    } else {
        faces.resize(num_triangles, faces.ColsAtCompileTime);
    }
    transform.setIdentity();
}

void Mesh::draw(Index shader_id, const Camera& camera) {
    if (!enabled || data.rows() == 0) return;
    if (!~VAO) {
        std::cerr << "ERROR: Please call meshview::Mesh::update() before "
                     "Mesh::draw()\n";
        return;
    }
    internal::Shader shader(shader_id);

    if (shading_type == ShadingType::texture) {
        // Bind appropriate textures
        for (int ttype = 0; ttype < Texture::__TYPE_COUNT; ++ttype) {
            const char* ttype_name = Texture::type_to_name(ttype);
            if (textures[ttype].empty()) {
                // No texture, create default (grey)
                gen_blank_texture();
                shader.set_int("material." + std::string(ttype_name), 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, blank_tex_id);
            }
        }
        Index tex_id = 1;
        for (int ttype = 0; ttype < Texture::__TYPE_COUNT; ++ttype) {
            const char* ttype_name = Texture::type_to_name(ttype);
            auto& tex_vec = textures[ttype];
            Index cnt = 0;
            for (size_t i = tex_vec.size() - 1; ~i; --i, ++tex_id) {
                glActiveTexture(
                    GL_TEXTURE0 +
                    tex_id);  // Active proper texture unit before binding
                // Now set the sampler to the correct texture unit
                shader.set_int("material." + std::string(ttype_name) +
                                   (cnt ? std::to_string(cnt) : ""),
                               tex_id);
                ++cnt;
                // And finally bind the texture
                glBindTexture(GL_TEXTURE_2D, tex_vec[i].id);
            }
        }
    }
    shader.set_float("material.shininess", shininess);

    // Set space transform matrices
    shader_set_transform_matrices(shader, camera, transform);

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawElements(GL_TRIANGLES, (GLsizei)faces.size(), GL_UNSIGNED_INT, 0);
    glBindVertexArray(0);

    // Always good practice to set everything back to defaults once configured.
    glActiveTexture(GL_TEXTURE0);
}

Mesh& Mesh::set_tex_coords(const Eigen::Ref<const Points2D>& coords,
                           const Eigen::Ref<const Triangles>& tri_faces) {
    // Each tex coord can be matched to at most one vertex,
    // so the number of vertices <= tex coords
    _MESHVIEW_ASSERT_LE(data.rows(), coords.rows());
    _tex_coords.noalias() = coords;
    _tex_faces.noalias() = tri_faces;
    _tex_to_vert = util::make_uv_to_vert_map(coords.rows(), faces, tri_faces);
    shading_type = ShadingType::texture;
    return *this;
}

Mesh& Mesh::unset_tex_coords() {
    _tex_coords.resize(0, 0);
    _tex_faces.resize(0, 0);
    shading_type = ShadingType::vertex;
    return *this;
}

Mesh& Mesh::set_shininess(float val) {
    shininess = val;
    return *this;
}

void Mesh::update(bool force_init) {
    static const size_t SCALAR_SZ = sizeof(float);
    static const size_t POS_OFFSET = 0;
    static const size_t COLOR_OFFSET = 3;
    static const size_t NORMALS_OFFSET = 6;
    static const size_t VERT_INDICES = data.ColsAtCompileTime;
    static const size_t VERT_SZ = VERT_INDICES * SCALAR_SZ;

    if (glfwGetCurrentContext() == nullptr) {
        // No OpenGL context is created, exit
        return;
    }

    // Auto normals
    if (_auto_normals) {
        util::estimate_normals(verts_pos(), faces, data.rightCols<3>());
    }

    if (!force_init && ~VAO) {
        // Already initialized, load any new textures
        for (auto& tex_vec : textures) {
            for (auto& tex : tex_vec) {
                if (!~tex.id) tex.load();
            }
        }
    } else {
        // Re-generate textures and buffers
        for (auto& tex_vec : textures) {
            for (auto& tex : tex_vec) {
                tex.id = -1;
                tex.load();
            }
        }
        blank_tex_id = -1;

        // create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
        glGenBuffers(1, &EBO);
    }

    // Figure out buffer sizes
    Eigen::Index n_verts, n_faces;
    float* vert_data_ptr;
    Index* face_data_ptr;
    if (_tex_coords.rows()) {
        n_verts = _tex_coords.rows();
        n_faces = _tex_faces.rows();
        _data_tex.resize(n_verts, data.ColsAtCompileTime);
        // Convert to texture indexing data -> data_tex
        for (size_t i = 0; i < n_verts; ++i) {
            _data_tex.block<1, 3>(i, POS_OFFSET).noalias() =
                data.block<1, 3>(_tex_to_vert[i], POS_OFFSET);
            _data_tex.block<1, 3>(i, NORMALS_OFFSET).noalias() =
                data.block<1, 3>(_tex_to_vert[i], NORMALS_OFFSET);
        }
        _data_tex.middleCols<2>(COLOR_OFFSET).noalias() = _tex_coords;
        vert_data_ptr = _data_tex.data();
        face_data_ptr = _tex_faces.data();
    } else {
        n_verts = data.rows();
        n_faces = faces.rows();
        vert_data_ptr = data.data();
        face_data_ptr = faces.data();
    }
    const size_t BUF_SZ = n_verts * data.ColsAtCompileTime * SCALAR_SZ;
    const size_t INDEX_SZ = n_faces * faces.ColsAtCompileTime * SCALAR_SZ;

    glBindVertexArray(VAO);
    // load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, BUF_SZ, (GLvoid*)vert_data_ptr,
                 GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, INDEX_SZ, (GLvoid*)face_data_ptr,
                 GL_STATIC_DRAW);

    // set the vertex attribute pointers
    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERT_SZ,
                          (GLvoid*)(POS_OFFSET * SCALAR_SZ));
    // vertex texture coords
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERT_SZ,
                          (GLvoid*)(COLOR_OFFSET * SCALAR_SZ));
    // vertex normals
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, VERT_SZ,
                          (GLvoid*)(NORMALS_OFFSET * SCALAR_SZ));
    glBindVertexArray(0);
}

void Mesh::free_bufs() {
    if (~VAO) glDeleteVertexArrays(1, &VAO);
    if (~VBO) glDeleteBuffers(1, &VBO);
    if (~EBO) glDeleteBuffers(1, &EBO);
    if (~blank_tex_id) glDeleteTextures(1, &blank_tex_id);
}

void Mesh::gen_blank_texture() {
    if (~blank_tex_id) return;
    glGenTextures(1, &blank_tex_id);
    glBindTexture(GL_TEXTURE_2D, blank_tex_id);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    Vector3f grey(0.7f, 0.7f, 0.7f);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 1, 1, 0, GL_RGB, GL_FLOAT,
                 grey.data());
    glGenerateMipmap(GL_TEXTURE_2D);
}

Mesh Mesh::Triangle(const Eigen::Ref<const Vector3f>& a,
                    const Eigen::Ref<const Vector3f>& b,
                    const Eigen::Ref<const Vector3f>& c) {
    Vector3f n = (b - a).cross(c - b);
    Mesh m(3);
    m.data << a[0], a[1], a[2], 0.f, 0.f, 0.f, n[0], n[1], n[2], b[0], b[1],
        b[2], 0.f, 1.f, 0.f, n[0], n[1], n[2], c[0], c[1], c[2], 1.f, 1.f, 0.f,
        n[0], n[1], n[2];
    m.shading_type = ShadingType::texture;
    return m;
}

Mesh Mesh::Square() {
    Mesh m(4, 2);
    m.faces << 0, 3, 1, 1, 3, 2;
    m.data << 0.5, 0.5, 0.f, 1.0f, 1.0f, 0.f, 0.0f, 0.0f, 1.0f, 0.5, -0.5, 0.f,
        1.0f, 0.0f, 0.f, 0.0f, 0.0f, 1.0f, -0.5, -0.5, 0.f, 0.0f, 0.0f, 0.f,
        0.0f, 0.0f, 1.0f, -0.5, 0.5, 0.f, 0.0f, 1.0f, 0.f, 0.0f, 0.0f, 1.0f;
    m.shading_type = ShadingType::texture;
    return m;
}

Mesh Mesh::Cube() {
    Mesh m(36);
    m.data <<
        // positions         // uv coords         // normals
        // back
        -0.5,
        -0.5, -0.5, 0.0f, 0.0f, 0.f, 0.0f, 0.0f, -1.0f, 0.5, 0.5, -0.5, 1.0f,
        1.0f, 0.f, 0.0f, 0.0f, -1.0f, 0.5, -0.5, -0.5, 1.0f, 0.0f, 0.f, 0.0f,
        0.0f, -1.0f, 0.5, 0.5, -0.5, 1.0f, 1.0f, 0.f, 0.0f, 0.0f, -1.0f, -0.5,
        -0.5, -0.5, 0.0f, 0.0f, 0.f, 0.0f, 0.0f, -1.0f, -0.5, 0.5, -0.5, 0.0f,
        1.0f, 0.f, 0.0f, 0.0f, -1.0f,

        // front
        -0.5, -0.5, 0.5, 0.0f, 0.0f, 0.f, 0.0f, 0.0f, 1.0f, 0.5, -0.5, 0.5,
        1.0f, 0.0f, 0.f, 0.0f, 0.0f, 1.0f, 0.5, 0.5, 0.5, 1.0f, 1.0f, 0.f, 0.0f,
        0.0f, 1.0f, 0.5, 0.5, 0.5, 1.0f, 1.0f, 0.f, 0.0f, 0.0f, 1.0f, -0.5, 0.5,
        0.5, 0.0f, 1.0f, 0.f, 0.0f, 0.0f, 1.0f, -0.5, -0.5, 0.5, 0.0f, 0.0f,
        0.f, 0.0f, 0.0f, 1.0f,

        // left
        -0.5, 0.5, 0.5, 1.0f, 0.0f, 0.f, -1.0f, 0.0f, 0.0f, -0.5, 0.5, -0.5,
        1.0f, 1.0f, 0.f, -1.0f, 0.0f, 0.0f, -0.5, -0.5, -0.5, 0.0f, 1.0f, 0.f,
        -1.0f, 0.0f, 0.0f, -0.5, -0.5, -0.5, 0.0f, 1.0f, 0.f, -1.0f, 0.0f, 0.0f,
        -0.5, -0.5, 0.5, 0.0f, 0.0f, 0.f, -1.0f, 0.0f, 0.0f, -0.5, 0.5, 0.5,
        1.0f, 0.0f, 0.f, -1.0f, 0.0f, 0.0f,

        // right
        0.5, 0.5, 0.5, 1.0f, 0.0f, 0.f, 1.0f, 0.0f, 0.0f, 0.5, -0.5, -0.5, 0.0f,
        1.0f, 0.f, 1.0f, 0.0f, 0.0f, 0.5, 0.5, -0.5, 1.0f, 1.0f, 0.f, 1.0f,
        0.0f, 0.0f, 0.5, -0.5, -0.5, 0.0f, 1.0f, 0.f, 1.0f, 0.0f, 0.0f, 0.5,
        0.5, 0.5, 1.0f, 0.0f, 0.f, 1.0f, 0.0f, 0.0f, 0.5, -0.5, 0.5, 0.0f, 0.0f,
        0.f, 1.0f, 0.0f, 0.0f,

        // bottom
        -0.5, -0.5, -0.5, 0.0f, 1.0f, 0.f, 0.0f, -1.0f, 0.0f, 0.5, -0.5, -0.5,
        1.0f, 1.0f, 0.f, 0.0f, -1.0f, 0.0f, 0.5, -0.5, 0.5, 1.0f, 0.0f, 0.f,
        0.0f, -1.0f, 0.0f, 0.5, -0.5, 0.5, 1.0f, 0.0f, 0.f, 0.0f, -1.0f, 0.0f,
        -0.5, -0.5, 0.5, 0.0f, 0.0f, 0.f, 0.0f, -1.0f, 0.0f, -0.5, -0.5, -0.5,
        0.0f, 1.0f, 0.f, 0.0f, -1.0f, 0.0f,

        // top
        -0.5, 0.5, -0.5, 0.0f, 1.0f, 0.f, 0.0f, 1.0f, 0.0f, 0.5, 0.5, 0.5, 1.0f,
        0.0f, 0.f, 0.0f, 1.0f, 0.0f, 0.5, 0.5, -0.5, 1.0f, 1.0f, 0.f, 0.0f,
        1.0f, 0.0f, 0.5, 0.5, 0.5, 1.0f, 0.0f, 0.f, 0.0f, 1.0f, 0.0f, -0.5, 0.5,
        -0.5, 0.0f, 1.0f, 0.f, 0.0f, 1.0f, 0.0f, -0.5, 0.5, 0.5, 0.0f, 0.0f,
        0.f, 0.0f, 1.0f, 0.0f;
    m.shading_type = ShadingType::texture;
    return m;
}

Mesh Mesh::Sphere(int rings, int sectors) {
    Mesh m(rings * sectors, (rings - 1) * sectors * 2);
    const float R = M_PI / (float)(rings - 1);
    const float S = 2 * M_PI / (float)sectors;
    size_t vid = 0;
    for (int r = 0; r < rings; r++) {
        for (int s = 0; s < sectors; s++) {
            const float y = sin(-0.5f * M_PI + r * R);
            const float x = cos(s * S) * sin(r * R);
            const float z = sin(s * S) * sin(r * R);
            m.data.row(vid++) << x, y, z, s * S, r * R, 0.f, x, y, z;
        }
    }
    size_t fid = 0;
    for (int r = 0; r < rings - 1; r++) {
        const int nx_r = r + 1;
        for (int s = 0; s < sectors; s++) {
            const int nx_s = (s == sectors - 1) ? 0 : s + 1;
            m.faces.row(fid++) << r * sectors + nx_s, r * sectors + s,
                nx_r * sectors + s;
            m.faces.row(fid++) << nx_r * sectors + s, nx_r * sectors + nx_s,
                r * sectors + nx_s;
        }
    }
    _MESHVIEW_ASSERT_EQ(vid, m.data.rows());
    _MESHVIEW_ASSERT_EQ(fid, m.faces.rows());
    m.shading_type = ShadingType::texture;
    return m;
}

void Mesh::save_basic_obj(const std::string& path) const {
    std::ofstream ofs(path);
    for (int i = 0; i < data.rows(); ++i) {
        ofs << "v";
        // Transform point
        Vector4f v =
            transform * data.block<1, 3>(i, 0).transpose().homogeneous();
        for (int j = 0; j < 3; ++j) {
            ofs << " " << v(j);
        }
        if (shading_type == ShadingType::vertex) {
            for (int j = 3; j < 6; ++j) {
                ofs << " " << data(i, j);
            }
        }
        ofs << "\n";
    }
    for (int i = 0; i < faces.rows(); ++i) {
        ofs << "f";
        for (int j = 0; j < 3; ++j) {
            ofs << " " << faces(i, j) + 1;
        }
        ofs << "\n";
    }
}

void Mesh::load_basic_obj(const std::string& path) {
    std::ifstream ifs(path);
    std::string line;
    std::vector<float> tmp_pos, tmp_rgb;
    std::vector<Index> tmp_faces;
    size_t attrs_per_vert = 0;
    while (std::getline(ifs, line)) {
        if (line.size() < 2 || line[1] != ' ') continue;
        if (line[0] == 'v') {
            std::stringstream ss(line.substr(2));
            double tmp;
            size_t cnt = 0;
            while (ss >> tmp) {
                if (cnt >= 3) {
                    tmp_rgb.push_back(tmp);
                } else {
                    tmp_pos.push_back(tmp);
                }
                ++cnt;
            }
            if (attrs_per_vert) {
                _MESHVIEW_ASSERT_EQ(cnt, attrs_per_vert);
            } else {
                _MESHVIEW_ASSERT(cnt == 3 || cnt == 6);
                attrs_per_vert = cnt;
            }
        } else if (line[0] == 'f') {
            size_t i = 2, j = 0;
            while (i < line.size() && std::isspace(i)) ++i;
            std::string num;
            for (; i < line.size(); ++i) {
                if (line[i] == '/') {
                    ++j;
                } else if (std::isspace(line[i])) {
                    tmp_faces.push_back((Index)std::atoll(num.c_str()) - 1);
                    j = 0;
                    num.clear();
                } else if (j == 0) {
                    num.push_back(line[i]);
                }
            }
            if (num.size()) {
                tmp_faces.push_back((Index)std::atoll(num.c_str()) - 1);
            }
        }
    }
    if (attrs_per_vert == 6) {
        shading_type = ShadingType::vertex;
        _MESHVIEW_ASSERT_EQ(tmp_rgb.size(), tmp_pos.size());
        _MESHVIEW_ASSERT_EQ(tmp_rgb.size() % 3, 0);
    } else {
        shading_type = ShadingType::texture;
        _MESHVIEW_ASSERT_EQ(tmp_rgb.size(), 0);
    }
    _MESHVIEW_ASSERT_EQ(tmp_faces.size() % 3, 0);
    data.resize(tmp_pos.size() / 3, data.ColsAtCompileTime);

    data.leftCols<3>().noalias() =
        Eigen::Map<Points>(tmp_pos.data(), data.rows(), 3);
    if (tmp_rgb.size()) {
        data.middleCols<3>(3).noalias() =
            Eigen::Map<Points>(tmp_rgb.data(), data.rows(), 3);
    }
    if (tmp_faces.size()) {
        faces.resize(tmp_faces.size() / 3, faces.ColsAtCompileTime);
        faces.noalias() =
            Eigen::Map<Triangles>(tmp_faces.data(), faces.rows(), 3);
    } else {
        faces.resize(data.rows() / 3, faces.ColsAtCompileTime);
        for (Index i = 0; i < (Index)data.rows(); ++i) {
            faces.data()[i] = i;
        }
    }
    transform.setIdentity();
}

// *** PointCloud ***
PointCloud::PointCloud(size_t num_verts) : VAO((Index)-1) { resize(num_verts); }
PointCloud::PointCloud(const Eigen::Ref<const Points>& pos,
                       const Eigen::Ref<const Points>& rgb)
    : PointCloud(pos.rows()) {
    if (!pos.rows() || (rgb.rows() && pos.rows() != rgb.rows())) {
        std::cerr << "Invalid meshview::PointCloud construction: "
                     "pos cannot be empty and pos, rgb should have identical # "
                     "rows\n";
        return;
    }
    verts_pos().noalias() = pos;
    if (rgb.rows()) {
        verts_rgb().noalias() = rgb;
    }
}
PointCloud::PointCloud(const Eigen::Ref<const Points>& pos, float r, float g,
                       float b)
    : PointCloud(pos.rows()) {
    verts_pos().noalias() = pos;
    verts_rgb().rowwise() = Eigen::RowVector3f(r, g, b);
}
PointCloud::~PointCloud() { free_bufs(); }

void PointCloud::resize(size_t num_verts) {
    data.resize(num_verts, data.ColsAtCompileTime);
    transform.setIdentity();
}

void PointCloud::update(bool force_init) {
    static const size_t SCALAR_SZ = sizeof(float);
    static const size_t POS_OFFSET = 0;
    static const size_t RGB_OFFSET = 3 * SCALAR_SZ;
    static const size_t VERT_INDICES = data.ColsAtCompileTime;
    static const size_t VERT_SZ = VERT_INDICES * SCALAR_SZ;

    if (glfwGetCurrentContext() == nullptr) {
        // No OpenGL context is created, exit
        return;
    }

    const size_t BUF_SZ = data.size() * SCALAR_SZ;

    // Already initialized
    if (force_init || !~VAO) {
        // Create buffers/arrays
        glGenVertexArrays(1, &VAO);
        glGenBuffers(1, &VBO);
    }
    glBindVertexArray(VAO);
    // load data into vertex buffers
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    // A great thing about structs is that their memory layout is sequential
    // for all its items. The effect is that we can simply pass a pointer to
    // the struct and it translates perfectly to a glm::vec3/2 array which
    // again translates to 3/2 floats which translates to a byte array.
    glBufferData(GL_ARRAY_BUFFER, BUF_SZ, (GLvoid*)data.data(), GL_STATIC_DRAW);

    // set the vertex attribute pointers
    // vertex positions
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, VERT_SZ,
                          (GLvoid*)POS_OFFSET);
    // vertex color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, VERT_SZ,
                          (GLvoid*)RGB_OFFSET);
    glBindVertexArray(0);
}

void PointCloud::draw(Index shader_id, const Camera& camera) {
    if (!enabled) return;
    if (!~VAO) {
        std::cerr << "ERROR: Please call meshview::PointCloud::update() before "
                     "PointCloud::draw()\n";
        return;
    }
    internal::Shader shader(shader_id);

    // Set point size
    glPointSize(point_size);

    // Set space transform matrices
    shader_set_transform_matrices(shader, camera, transform);

    // Draw mesh
    glBindVertexArray(VAO);
    glDrawArrays(lines ? GL_LINES : GL_POINTS, 0, (GLsizei)data.rows());
    glBindVertexArray(0);

    // Always good practice to set everything back to defaults once
    // configured.
    glActiveTexture(GL_TEXTURE0);
}

void PointCloud::free_bufs() {
    if (~VAO) glDeleteVertexArrays(1, &VAO);
    if (~VBO) glDeleteBuffers(1, &VBO);
}

PointCloud PointCloud::Line(const Eigen::Ref<const Vector3f>& a,
                            const Eigen::Ref<const Vector3f>& b,
                            const Eigen::Ref<const Vector3f>& color) {
    PointCloud tmp(2);
    tmp.verts_pos().topRows<1>().noalias() = a;
    tmp.verts_pos().bottomRows<1>().noalias() = b;
    tmp.verts_rgb().rowwise() = color.transpose();
    tmp.draw_lines();
    return tmp;
}

// *** Shared ***
// Define identical function for both mesh, pointcloud classes
#define BOTH_MESH_AND_POINTCLOUD(fbody) \
    Mesh& Mesh::fbody PointCloud& PointCloud::fbody

BOTH_MESH_AND_POINTCLOUD(translate(const Eigen::Ref<const Vector3f>& vec) {
    (transform.topRightCorner<3, 1>() += vec);
    return *this;
})

BOTH_MESH_AND_POINTCLOUD(
    set_translation(const Eigen::Ref<const Vector3f>& vec) {
        (transform.topRightCorner<3, 1>() = vec);
        return *this;
    })

BOTH_MESH_AND_POINTCLOUD(rotate(const Eigen::Ref<const Matrix3f>& mat) {
    (transform.topLeftCorner<3, 3>() = mat * transform.topLeftCorner<3, 3>());
    return *this;
})

BOTH_MESH_AND_POINTCLOUD(scale(const Eigen::Ref<const Vector3f>& vec) {
    (transform.topLeftCorner<3, 3>().array().colwise() *= vec.array());
    return *this;
})

BOTH_MESH_AND_POINTCLOUD(scale(float val) {
    (transform.topLeftCorner<3, 3>().array() *= val);
    return *this;
})

BOTH_MESH_AND_POINTCLOUD(
    apply_transform(const Eigen::Ref<const Matrix4f>& mat) {
        transform = mat * transform;
        return *this;
    })

BOTH_MESH_AND_POINTCLOUD(set_transform(const Eigen::Ref<const Matrix4f>& mat) {
    transform = mat;
    return *this;
})

BOTH_MESH_AND_POINTCLOUD(enable(bool val) {
    enabled = val;
    return *this;
})

}  // namespace meshview
