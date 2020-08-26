#pragma once
#ifndef MESHVIEW_B1FE2D07_A12E_4C8B_A673_D9AC48841D24
#define MESHVIEW_B1FE2D07_A12E_4C8B_A673_D9AC48841D24

#include "meshview/common.hpp"
#include <vector>
#include <array>
#include <string>
#include <cstdint>
#include <cstddef>
#include <cmath>
#include <memory>

namespace meshview {

// Represents a texture/material
struct Texture {
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
        switch (type) {
            case TYPE_SPECULAR:
                return "specular";
            // case TYPE_NORMAL: return "normal";
            // case TYPE_HEIGHT: return "height";
            default:
                return "diffuse";
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

class Camera {
   public:
    // Construct camera with given params
    explicit Camera(const Vector3f& center_of_rot = Eigen::Vector3f(0.f, 0.f,
                                                                    0.f),
                    const Vector3f& world_up = Eigen::Vector3f(0.f, 1.f, 0.f),
                    float dist_to_center = 3.f, float yaw = -M_PI / 2,
                    float pitch = 0.0f, float roll = 0.0f, bool ortho = false,
                    float fovy = M_PI / 4.f, float aspect = 5.f / 3.f,
                    float z_close = 0.01f, float z_far = 1e3f);

    // Get camera position
    inline Vector3f get_pos() const { return pos; }

    // Update view matrix, call after changing any view parameter
    void update_view();

    // Update proj matrix, call after changing any projection parameter
    void update_proj();

    // Handlers
    void rotate_with_mouse(float xoffset, float yoffset);
    void roll_with_mouse(float xoffset, float yoffset);
    void pan_with_mouse(float xoffset, float yoffset);
    void zoom_with_mouse(float amount);

    // Reset the view
    void reset_view();
    // Reset the projection
    void reset_proj();

    // * Camera matrices
    // View matrix: global -> view coords
    Matrix4f view;
    // Projection matrix: view -> clip coords (perspective)
    Matrix4f proj;

    // Camera mouse control options
    float pan_speed = .0015f, rotate_speed = .008f, scroll_factor = 1.1f;

    // * Projection parameters
    // Orthographic proj?
    bool ortho;
    // Field of view aspect ratio
    float fovy, aspect;
    // Clip distances
    float z_close, z_far;

    // * View parameters
    // Center of rotation
    Vector3f center_of_rot;
    // Normalized direction vectors
    Vector3f front, up, world_up;
    // Distance to cor
    float dist_to_center;

    // Euler angles, for mouse control
    float yaw, pitch, roll;

   private:
    // For caching only; right = front cross up; pos = cor - d2c * front
    Vector3f pos, right;
};

// Represents a triangle mesh with vertices (including uv, normals),
// triangular faces, and textures
class Mesh {
   public:
    enum class ShadingType {
        // Color by linear interpolation from vertices, use vertex color as rgb
        vertex,
        // Color by texture, use color info as texture uv coordinates
        texture
    };

    // Construct uninitialized mesh with given number of vertices, triangles
    explicit Mesh(size_t num_verts = 0, size_t num_triangles = 0);
    // Construct from basic OBJ (see load_basic_obj)
    explicit Mesh(const std::string& path);
    // Construct with given points, faces, and (optionally) color data
    // faces: triangles. if not specified, assumes they are 0 1 2, 3 4 5 etc
    // rgb: optional per-vertex color (ignored if you call set_tex_coords later)
    // normals: normal vectors per point.
    // if not specified, will compute automatically on update
    explicit Mesh(const Eigen::Ref<const Points>& pos,
                  const Eigen::Ref<const Triangles>& tri_faces,
                  const Eigen::Ref<const Points>& rgb,
                  const Eigen::Ref<const Points>& normals = Points());
    // Construct with given points, faces, and (optionally) rgb color for all
    // vertices r,g,b: the color given to all vertices (ignored if you call
    // set_tex_coords later) normals: normal vectors per point. if not
    // specified, will compute automatically on update
    explicit Mesh(const Eigen::Ref<const Points>& pos,
                  const Eigen::Ref<const Triangles>& tri_faces = Triangles(),
                  float r = 1.f, float g = 1.f, float b = 1.f,
                  const Eigen::Ref<const Points>& normals = Points());
    ~Mesh();

    // Resize mesh (destroys current data)
    void resize(size_t num_verts, size_t num_triangles = 0);

    // Draw mesh to shader wrt camera
    void draw(Index shader_id, const Camera& camera);

    // Set the texture coordinates with given texture triangles
    // (shading mode will be set to texture)
    Mesh& set_tex_coords(const Eigen::Ref<const Points2D>& coords,
                         const Eigen::Ref<const Triangles>& tri_faces);

    // Remove texture coordinates data
    // (shading mode will be set to vertex)
    Mesh& unset_tex_coords();

    // Add a texture. Arguments are passed to Texture constructor
    // returns self (for chaining)
    template <int Type = Texture::TYPE_DIFFUSE, typename... Args>
    Mesh& add_texture(Args&&... args) {
        textures[Type].emplace_back(std::forward<Args>(args)...);
        return *this;
    }

    // Set specular shininess parameter
    Mesh& set_shininess(float val);

    // * Transformations
    // Apply translation
    Mesh& translate(const Eigen::Ref<const Vector3f>& vec);
    // Set translation
    Mesh& set_translation(const Eigen::Ref<const Vector3f>& vec);

    // Apply rotation (3x3 rotation matrix)
    Mesh& rotate(const Eigen::Ref<const Matrix3f>& mat);

    // Apply scaling
    Mesh& scale(const Eigen::Ref<const Vector3f>& vec);
    // Apply uniform scaling
    Mesh& scale(float val);

    // Apply transform (4x4 affine transform matrix)
    Mesh& apply_transform(const Eigen::Ref<const Matrix4f>& mat);
    // Set transform (4x4 affine transform matrix)
    Mesh& set_transform(const Eigen::Ref<const Matrix4f>& mat);

    // Init or update VAO/VBO/EBO buffers from current vertex and triangle data
    // Must called before first draw for each GLFW context to ensure
    // textures are reconstructed.
    void update(bool force_init = false);

    // ADVANCED: Free buffers. Used automatically in destructor.
    void free_bufs();

    // * Accessors
    // Position data of vertices (#verts, 3).
    inline Eigen::Ref<Points> verts_pos() { return data.leftCols<3>(); }

    // The optional RGB data of each vertex (#verts, 3).
    inline Eigen::Ref<Points> verts_rgb() { return data.middleCols<3>(3); }

    // ADVANCED: Normal vectors data (#verts, 3).
    // If called, this disables automatic normal computation.
    inline Eigen::Ref<Points> verts_norm() {
        _auto_normals = false;
        return data.rightCols<3>();
    }

    // Enable/disable object
    Mesh& enable(bool val = true);

    // * VERY basic OBJ I/O
    // - Only supports v and f types
    // - Each f row must consist of three integers (triangle indices)
    // - Each v row may consist of 3 OR 6 floats (position, or position+rgb
    // color)
    void save_basic_obj(const std::string& path) const;
    void load_basic_obj(const std::string& path);

    // * Example meshes
    // Triangle
    static Mesh Triangle(const Eigen::Ref<const Vector3f>& a,
                         const Eigen::Ref<const Vector3f>& b,
                         const Eigen::Ref<const Vector3f>& c);
    // Square centered at 0,0,0 with normal in z direction and side length 1
    static Mesh Square();

    // Cube centered at 0,0,0 with side length 1
    static Mesh Cube();

    // UV sphere centered at 0,0,0 with radius 1
    static Mesh Sphere(int rings = 30, int sectors = 30);

    // Shape (num_verts, 9)
    // 3 x vertex position
    // 3 x vertex color data (either uv coords, or rgb color, depending on
    // shading_type) 3 x normal
    // TODO: make some of the columns optional/only transfer partly
    // to save data transfer time and memory
    PointsRGBNormal data;

    // Shape (num_triangles, 3)
    // Triangle indices, empty if num_triangles = -1 (not using EBO)
    Triangles faces;

    // Whether this mesh is enabled; if false, does not draw anything
    bool enabled = true;

    // Textures
    std::array<std::vector<Texture>, Texture::__TYPE_COUNT> textures;

    // Shininess
    float shininess = 10.f;

    // Model local transfom
    Matrix4f transform;

    // Shading type : interpretation of middle 3 columns in verts
    // the right shader will be chosen by viewer (mesh does not have control)
    ShadingType shading_type = ShadingType::vertex;

   private:
    // Generate a white 1x1 texture to blank_tex_id
    // used to fill maps if no texture provided
    void gen_blank_texture();

    // Vertex Array Object index
    Index VAO = -1;

    Index VBO = -1, EBO = -1;
    Index blank_tex_id = -1;

    // Texture data
    Points2D _tex_coords;
    Triangles _tex_faces;

    // Data, arranged in texture coord indexing
    PointsRGBNormal _data_tex;
    // Map from texture coord index -> vertex index
    Eigen::Matrix<Index, Eigen::Dynamic, 1> _tex_to_vert;

    // Whether to use automatic normal estimation (get normals automatically on
    // update)
    bool _auto_normals = true;
};

// Represents a 3D point cloud with vertices (including uv, normals)
// where each vertex has a color.
// Also supports drawing the points as a polyline (call draw_lines()).
class PointCloud {
   public:
    explicit PointCloud(size_t num_verts = 0);
    // Set vertices with positions pos with colors rgb
    explicit PointCloud(const Eigen::Ref<const Points>& pos,
                        const Eigen::Ref<const Points>& rgb);
    // Set all points to same color
    // (can't put Eigen::Vector3f due 'ambiguity' with above)
    explicit PointCloud(const Eigen::Ref<const Points>& pos, float r = 1.f,
                        float g = 1.f, float b = 1.f);

    ~PointCloud();

    // Resize point cloud (destroys current data)
    void resize(size_t num_verts);

    // Draw mesh to shader wrt camera
    void draw(Index shader_id, const Camera& camera);

    // Position part of verts
    inline Eigen::Ref<Points> verts_pos() { return data.leftCols<3>(); }
    // RGB part of verts
    inline Eigen::Ref<Points> verts_rgb() { return data.rightCols<3>(); }

    // Enable/disable object
    PointCloud& enable(bool val = true);
    // Set the point size for drawing
    inline PointCloud& set_point_size(float val) {
        point_size = val;
        return *this;
    }
    // Draw polylines between consecutive points
    inline PointCloud& draw_lines() {
        lines = true;
        return *this;
    }

    // Apply translation
    PointCloud& translate(const Eigen::Ref<const Vector3f>& vec);
    // Set translation
    PointCloud& set_translation(const Eigen::Ref<const Vector3f>& vec);

    // Apply rotation (3x3 rotation matrix)
    PointCloud& rotate(const Eigen::Ref<const Matrix3f>& mat);
    // Apply scaling
    PointCloud& scale(const Eigen::Ref<const Vector3f>& vec);
    // Apply uniform scaling in each dimension
    PointCloud& scale(float val);

    // Apply transform (4x4 affine transform matrix)
    PointCloud& apply_transform(const Eigen::Ref<const Matrix4f>& mat);
    // Set transform (4x4 affine transform matrix)
    PointCloud& set_transform(const Eigen::Ref<const Matrix4f>& mat);

    // Init or update VAO/VBO buffers from current vertex data
    // Must called before first draw for each GLFW context to ensure
    // textures are reconstructed.
    // force_init: INTERNAL, whether to force recreating buffers, DO NOT use
    // this
    void update(bool force_init = false);

    // ADVANCED: Free buffers. Used automatically in destructor.
    void free_bufs();

    // * Example point clouds/lines
    static PointCloud Line(
        const Eigen::Ref<const Vector3f>& a,
        const Eigen::Ref<const Vector3f>& b,
        const Eigen::Ref<const Vector3f>& color = Vector3f(1.f, 1.f, 1.f));

    // Data store
    PointsRGB data;

    // Whether this point cloud is enabled; if false, does not draw anything
    bool enabled = true;

    // If true, draws polylines between vertices
    // If false (default), draws points only
    bool lines = false;

    // Point size (if lines = false)
    float point_size = 1.f;

    // Model local transfom
    Matrix4f transform;

   private:
    // Buffer indices
    Index VAO = -1, VBO = -1;
};

// MeshView OpenGL 3D viewer
class Viewer {
   public:
    Viewer();
    ~Viewer();

    // Show window and start render loop; blocks execution
    // press q/ESC to close window and exit loop
    void show();

    // Add mesh (to Viewer::meshes), arguments are forwarded to Mesh constructor
    template <typename... Args>
    Mesh& add_mesh(Args&&... args) {
        meshes.push_back(std::make_unique<Mesh>(std::forward<Args>(args)...));
        if (_looping) meshes.back()->update();
        return *meshes.back();
    }
    // Add point_cloud (to Viewer::point_clouds)
    // arguments are forwarded to PointCloud constructor
    template <typename... Args>
    PointCloud& add_point_cloud(Args&&... args) {
        point_clouds.push_back(
            std::make_unique<PointCloud>(std::forward<Args>(args)...));
        if (_looping) point_clouds.back()->update();
        return *point_clouds.back();
    }
    // Add a cube centered at cen with given side length.
    // Mesh will have identity transform (points are moved physically in the
    // mesh)
    Mesh& add_cube(const Eigen::Ref<const Vector3f>& cen = Vector3f(0.f, 0.f,
                                                                    0.f),
                   float side_len = 1.0f,
                   const Eigen::Ref<const Vector3f>& color = Vector3f(1.f, 0.5f,
                                                                      0.f));

    // Add a square centered at cen with given side length, normal to the
    // +z-axis. Mesh will have identity transform (points are moved physically
    // in the mesh)
    Mesh& add_square(
        const Eigen::Ref<const Vector3f>& cen = Vector3f(0.f, 0.f, 0.f),
        float side_len = 1.0f,
        const Eigen::Ref<const Vector3f>& color = Vector3f(1.f, 0.5f, 0.f));

    // Add a UV sphere centered at cen with given radius.
    // Mesh will have identity transform (points are moved physically in the
    // mesh)
    Mesh& add_sphere(
        const Eigen::Ref<const Vector3f>& cen = Vector3f(0.f, 0.f, 0.f),
        float radius = 1.0f,
        const Eigen::Ref<const Vector3f>& color = Vector3f(1.f, 0.5f, 0.f),
        int rings = 30, int sectors = 30);

    // Add a line (PointCloud with line rendering).
    // Point cloud will have identity transform (not translated or scaled)
    PointCloud& add_line(
        const Eigen::Ref<const Vector3f>& a,
        const Eigen::Ref<const Vector3f>& b,
        const Eigen::Ref<const Vector3f>& color = Vector3f(1.f, 1.f, 1.f));

    // * The meshes
    std::vector<std::unique_ptr<Mesh>> meshes;
    // * The point clouds
    std::vector<std::unique_ptr<PointCloud>> point_clouds;

    // * Lighting
    // Ambient light color, default 0.2 0.2 0.2
    Vector3f light_color_ambient;

    // Point light position (in VIEW space, so that light follows camera)
    Vector3f light_pos;
    // Light color diffuse/specular, default white
    Vector3f light_color_diffuse;
    Vector3f light_color_specular;

    // * Camera
    Camera camera;

    // * Render params
    // Axes? (a)
    bool draw_axes = true;
    // Wireframe mode? (w)
    bool wireframe = false;
    // Backface culling? (c)
    bool cull_face = true;
    // Whether to wait for event on loop
    // true: loops on user input (glfwWaitEvents), saves power and computation
    // false: loops continuously (glfwPollEvents), useful for e.g. animation
    bool loop_wait_events = true;

    // * Aesthetics
    // Window title, updated on show() calls only (i.e. please set before
    // show())
    std::string title = "meshview";

    // Background color
    Vector3f background;

    // * Event callbacks
    // Called after GL cnotext init
    std::function<void()> on_open;
    // Called when window is about to close
    std::function<void()> on_close;
    // Called per iter of render loop, before on_gui
    // return true if mesh/point cloud/camera data has been updated, false
    // otherwise
    std::function<bool()> on_loop;
#ifdef MESHVIEW_IMGUI
    // Called per iter of render loop, after on_loop
    // only avaiable if MESHVIEW_IMGUI defined.
    // Within the function, Dear ImGui is already set up,
    // ie. ready to use ImGui::Begin etc
    std::function<bool()> on_gui;
#endif
    // Called on key up/down/repeat: args (key, action, mods), return false to
    // prevent default see https://www.glfw.org/docs/3.3/group__mods.html on
    // mods
    std::function<bool(int, input::Action, int)> on_key;
    // Called on mouse up/down/repeat: args (button, action, mods), return false
    // to prevent default see https://www.glfw.org/docs/3.3/group__mods.html on
    // mods
    std::function<bool(int, input::Action, int)> on_mouse_button;
    // Called on mouse move: args(x, y) return false to prevent default
    std::function<bool(double, double)> on_mouse_move;
    // Called on mouse scroll: args(xoffset, yoffset) return false to prevent
    // default
    std::function<bool(double, double)> on_scroll;

    // * Dynamic data (advanced, for use in callbacks)
    // Window width/height, as set in system
    // (don't modify after show() and before window close)
    int _width = 1000, _height = 600;

    // Mouse position, if available (don't modify)
    double _mouse_x, _mouse_y;

    // Mouse button and modifier keys; mouse_button is -1 if nothing down (don't
    // modify)
    int _mouse_button = -1, _mouse_mods;

    // Window pos/size prior to full screen
    int _fullscreen_backup[4];

    // Is window in fullscreen? (do not modify)
    bool _fullscreen;

    // ADNANCED: Pointer to GLFW window object
    void* _window = nullptr;

   private:
    // True only during the render loop (show())
    bool _looping = false;
};

}  // namespace meshview

#endif  // ifndef MESHVIEW_B1FE2D07_A12E_4C8B_A673_D9AC48841D24
