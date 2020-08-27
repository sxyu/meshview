#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/eigen.h>
#include <pybind11/stl.h>
#include <pybind11/functional.h>
#include <iostream>
#include <string>
#include <vector>
#include <map>

#include <meshview/meshview.hpp>
using namespace meshview;

namespace py = pybind11;

namespace {
std::map<bool, bool*> bools;
std::map<bool, float*> floats;
std::map<bool, int*> ints;
std::map<bool, std::string*> strs;
}  // namespace

PYBIND11_MODULE(meshview, m) {
    m.doc() = R"pbdoc(meshview: mesh and point cloud viewer)pbdoc";

    auto input = m.def_submodule("input");
    py::enum_<input::Action>(input, "Action")
        .value("press", input::Action::press)
        .value("release", input::Action::release)
        .value("repat", input::Action::repeat);

    py::enum_<Mesh::ShadingType>(m, "ShadingType")
        .value("texture", Mesh::ShadingType::texture)
        .value("vertex", Mesh::ShadingType::vertex);

    py::class_<Mesh>(m, "Mesh")
        .def("update", &Mesh::update, py::arg("force_init") = false)
        .def("save_basic_obj", &Mesh::save_basic_obj)
        .def("load_basic_obj", &Mesh::load_basic_obj)
        .def("resize", &Mesh::resize, py::arg("num_verts"),
             py::arg("num_triangles") = 0)
        .def_property_readonly("n_verts",
                               [](Mesh& self) { return self.data.rows(); })
        .def_property_readonly("n_faces",
                               [](Mesh& self) { return self.faces.rows(); })
        .def("set_tex_coords", &Mesh::set_tex_coords, py::arg("coords"),
             py::arg("tri_faces"), py::return_value_policy::reference_internal)
        .def("unset_tex_coords", &Mesh::unset_tex_coords,
             py::return_value_policy::reference_internal)
        .def(
            "add_texture",
            [](Mesh& self, const std::string& path, bool flip_y) {
                self.add_texture(path, flip_y);
            },
            py::arg("image_path"), py::arg("flip_y") = true)
        .def(
            "add_texture",
            [](Mesh& self, float r, float g, float b) {
                self.add_texture(r, g, b);
            },
            py::arg("r"), py::arg("g"), py::arg("b"))
        .def(
            "add_texture",
            [](Mesh& self, const Eigen::Ref<const Image>& image,
               int n_channels) { self.add_texture(image, n_channels); },
            py::arg("image"), py::arg("n_channels") = 1)
        .def(
            "add_texture_specular",
            [](Mesh& self, const std::string& path, bool flip_y) {
                self.add_texture<Texture::TYPE_SPECULAR>(path, flip_y);
            },
            py::arg("image_path"), py::arg("flip_y") = true)
        .def(
            "add_texture_specular",
            [](Mesh& self, float r, float g, float b) {
                self.add_texture<Texture::TYPE_SPECULAR>(r, g, b);
            },
            py::arg("r"), py::arg("g"), py::arg("b"))
        .def(
            "add_texture_specular",
            [](Mesh& self, const Eigen::Ref<const Image>& image,
               int n_channels) {
                self.add_texture<Texture::TYPE_SPECULAR>(image, n_channels);
            },
            py::arg("image"), py::arg("n_channels") = 1)
        .def("clear_textures",
             [](Mesh& self) { self.textures[Texture::TYPE_DIFFUSE].clear(); })
        .def("clear_textures_specular",
             [](Mesh& self) { self.textures[Texture::TYPE_SPECULAR].clear(); })
        .def("translate", &Mesh::translate,
             py::return_value_policy::reference_internal)
        .def("set_translation", &Mesh::set_translation,
             py::return_value_policy::reference_internal)
        .def(
            "scale", [](Mesh& self, float scale) { return self.scale(scale); },
            py::return_value_policy::reference_internal)
        .def(
            "scale",
            [](Mesh& self, Eigen::Ref<const Vector3f> scale) {
                return self.scale(scale);
            },
            py::return_value_policy::reference_internal)
        .def("rotate", &Mesh::rotate,
             py::return_value_policy::reference_internal)
        .def("apply_transform", &Mesh::apply_transform,
             py::return_value_policy::reference_internal)
        .def("set_transform", &Mesh::set_transform)
        .def("set_shininess", &Mesh::set_shininess,
             py::return_value_policy::reference_internal)
        .def_readwrite("data", &Mesh::data)
        .def_property(
            "verts_pos",
            [](Mesh& self) -> Eigen::Ref<Points> { return self.verts_pos(); },
            [](Mesh& self, Eigen::Ref<const Points> val) {
                self.verts_pos() = val;
            })
        .def_property(
            "verts_rgb",
            [](Mesh& self) -> Eigen::Ref<Points> { return self.verts_rgb(); },
            [](Mesh& self, Eigen::Ref<const Points> val) {
                self.verts_rgb() = val;
            })
        .def_property(
            "verts_norm",
            [](Mesh& self) -> Eigen::Ref<Points> { return self.verts_norm(); },
            [](Mesh& self, Eigen::Ref<const Points> val) {
                self.verts_norm() = val;
            })
        .def_readwrite("faces", &Mesh::faces)
        .def_readwrite("enabled", &Mesh::enabled)
        .def_readwrite("shininess", &Mesh::shininess)
        .def_readwrite("shading_type", &Mesh::shading_type)
        .def_readwrite("transform", &Mesh::transform);

    py::class_<PointCloud>(m, "PointCloud")
        .def("update", &PointCloud::update, py::arg("force_init") = false)
        .def("resize", &PointCloud::resize, py::arg("num_verts"))
        .def_property_readonly(
            "n_verts", [](PointCloud& self) { return self.data.rows(); })
        .def("translate", &PointCloud::translate,
             py::return_value_policy::reference_internal)
        .def("set_translation", &PointCloud::set_translation,
             py::return_value_policy::reference_internal)
        .def(
            "scale",
            [](PointCloud& self, float scale) { return self.scale(scale); },
            py::return_value_policy::reference_internal)
        .def(
            "scale",
            [](PointCloud& self, Eigen::Ref<const Vector3f> scale) {
                return self.scale(scale);
            },
            py::return_value_policy::reference_internal)
        .def("rotate", &PointCloud::rotate,
             py::return_value_policy::reference_internal)
        .def("apply_transform", &PointCloud::apply_transform,
             py::return_value_policy::reference_internal)
        .def("set_transform", &PointCloud::set_transform,
             py::return_value_policy::reference_internal)
        .def("set_point_size", &PointCloud::set_point_size,
             py::return_value_policy::reference_internal)
        .def("draw_lines", &PointCloud::draw_lines,
             py::return_value_policy::reference_internal)
        .def_readwrite("data", &PointCloud::data)
        .def_property(
            "verts_pos",
            [](PointCloud& self) -> Eigen::Ref<Points> {
                return self.verts_pos();
            },
            [](PointCloud& self, Eigen::Ref<const Points> val) {
                self.verts_pos() = val;
            })
        .def_property(
            "verts_rgb",
            [](PointCloud& self) -> Eigen::Ref<Points> {
                return self.verts_rgb();
            },
            [](PointCloud& self, Eigen::Ref<const Points> val) {
                self.verts_rgb() = val;
            })
        .def_readwrite("enabled", &PointCloud::enabled)
        .def_readwrite("transform", &PointCloud::transform)
        .def_readwrite("point_size", &PointCloud::point_size)
        .def_readwrite("lines", &PointCloud::lines,
                       "If true, draws polylines instead of points");

    py::class_<Camera>(m, "Camera")
        .def("update_view", &Camera::update_view)
        .def("update_proj", &Camera::update_proj)
        .def("reset_view", &Camera::reset_view)
        .def("reset_proj", &Camera::reset_proj)

        // * View parameters
        // Center of rotation
        .def_readwrite("center_of_rot", &Camera::center_of_rot)
        // Distance to cor
        .def_readwrite("dist_to_center", &Camera::dist_to_center)
        // Euler angles
        .def_readwrite("yaw", &Camera::yaw)
        .def_readwrite("pitch", &Camera::pitch)
        .def_readwrite("roll", &Camera::roll)

        // * Projection parameters
        // Orthographic?
        .def_readwrite("ortho", &Camera::ortho)
        // Field of view, aspect ratio
        .def_readwrite("fovy", &Camera::fovy)
        .def_readwrite("aspect", &Camera::aspect)
        // Clip distances
        .def_readwrite("z_close", &Camera::z_close)
        .def_readwrite("z_far", &Camera::z_far)

        // Computed matrices
        .def_readonly("view", &Camera::view)
        .def_readonly("proj", &Camera::proj)

        // Computed vectors
        .def_readonly("front", &Camera::front)
        .def_readonly("up", &Camera::up)
        .def_readonly("world_up", &Camera::world_up);

    py::class_<Viewer>(m, "Viewer")
        .def(py::init<>())
        .def(
            "add_mesh",
            [](Viewer& self, const std::string& path) -> Mesh& {
                return self.add_mesh(path);
            },
            py::arg("path"), py::return_value_policy::reference_internal)
        .def(
            "add_mesh",
            [](Viewer& self, int num_verts, int num_faces) -> Mesh& {
                return self.add_mesh(num_verts, num_faces);
            },
            py::arg("num_verts"), py::arg("num_faces") = 0,
            py::return_value_policy::reference_internal)
        .def(
            "add_mesh",
            [](Viewer& self, const Eigen::Ref<const Points>& verts,
               const Eigen::Ref<const Triangles>& tri_faces,
               const Eigen::Ref<const Points>& rgb,
               const Eigen::Ref<const Points>& normals) -> Mesh& {
                return self.add_mesh(verts, tri_faces, rgb, normals);
            },
            py::arg("verts"), py::arg("tri_faces"), py::arg("rgb"),
            py::arg("normals") = Points(),
            py::return_value_policy::reference_internal)
        .def(
            "add_mesh",
            [](Viewer& self, const Eigen::Ref<const Points>& verts,
               const Eigen::Ref<const Triangles>& tri_faces, float r, float g,
               float b, const Eigen::Ref<const Points>& normals) -> Mesh& {
                return self.add_mesh(verts, tri_faces, r, g, b, normals);
            },
            py::arg("verts"), py::arg("tri_faces") = Triangles(),
            py::arg("r") = 1.f, py::arg("g") = 1.f, py::arg("b") = 1.f,
            py::arg("normals") = Points(),
            py::return_value_policy::reference_internal)
        .def(
            "add_point_cloud",
            [](Viewer& self, int num_verts) -> PointCloud& {
                return self.add_point_cloud(num_verts);
            },
            py::arg("num_verts"), py::return_value_policy::reference_internal)
        .def(
            "add_point_cloud",
            [](Viewer& self, const Eigen::Ref<const Points>& verts,
               const Eigen::Ref<const Points>& rgb) -> PointCloud& {
                return self.add_point_cloud(verts, rgb);
            },
            py::arg("verts"), py::arg("rgb"),
            py::return_value_policy::reference_internal)
        .def(
            "add_point_cloud",
            [](Viewer& self, const Eigen::Ref<const Points>& verts, float r,
               float g, float b) -> PointCloud& {
                return self.add_point_cloud(verts, r, g, b);
            },
            py::arg("verts"), py::arg("r") = 1.f, py::arg("g") = 1.f,
            py::arg("b") = 1.f, py::return_value_policy::reference_internal)
        .def("add_line", &Viewer::add_line, py::arg("a"), py::arg("b"),
             py::arg("color") = Eigen::Vector3f(1.f, 1.f, 1.f),
             py::return_value_policy::reference_internal)
        .def("add_cube", &Viewer::add_cube,
             py::arg("cen") = Eigen::Vector3f(0.f, 0.f, 0.f),
             py::arg("side_len") = 1.0f,
             py::arg("color") = Eigen::Vector3f(1.f, 0.5f, 0.f),
             py::return_value_policy::reference_internal)
        .def("add_square", &Viewer::add_square,
             py::arg("cen") = Eigen::Vector3f(0.f, 0.f, 0.f),
             py::arg("side_len") = 1.0f,
             py::arg("color") = Eigen::Vector3f(1.f, 0.5f, 0.f),
             py::return_value_policy::reference_internal)
        .def("add_sphere", &Viewer::add_sphere,
             py::arg("cen") = Eigen::Vector3f(0.f, 0.f, 0.f),
             py::arg("radius") = 1.0f,
             py::arg("color") = Eigen::Vector3f(1.f, 0.5f, 0.f),
             py::arg("rings") = 30, py::arg("sections") = 30,
             py::return_value_policy::reference_internal)
        .def_readwrite("camera", &Viewer::camera)
        .def_readwrite("title", &Viewer::title)
        .def_readwrite("cull_face", &Viewer::cull_face)
        .def_readwrite("wireframe", &Viewer::wireframe)
        .def_readwrite("draw_axes", &Viewer::draw_axes)
        .def_readwrite("background", &Viewer::background)
        .def_readwrite("light_pos", &Viewer::light_pos)
        .def_readwrite("light_color_ambient", &Viewer::light_color_ambient)
        .def_readwrite("light_color_diffuse", &Viewer::light_color_diffuse)
        .def_readwrite("light_color_specular", &Viewer::light_color_specular)
        .def_readwrite("on_key", &Viewer::on_key)
        .def_readwrite("on_loop", &Viewer::on_loop)
        .def_readwrite("on_open", &Viewer::on_open)
        .def_readwrite("on_close", &Viewer::on_close)
        .def_readwrite("on_scroll", &Viewer::on_scroll)
        .def_readwrite("on_mouse_move", &Viewer::on_mouse_move)
        .def_readwrite("on_mouse_button", &Viewer::on_mouse_button)
        .def_property_readonly("n_meshes",
                               [](Viewer& self) { return self.meshes.size(); })
        .def_property_readonly(
            "n_point_clouds",
            [](Viewer& self) { return self.point_clouds.size(); })
        .def(
            "get_mesh",
            [](Viewer& self, int idx) -> Mesh& { return *self.meshes[idx]; },
            py::return_value_policy::reference_internal)
        .def(
            "get_point_cloud",
            [](Viewer& self, int idx) -> PointCloud& {
                return *self.point_clouds[idx];
            },
            py::return_value_policy::reference_internal)
        .def("remove_mesh",
             [](Viewer& self, int idx) {
                 self.meshes.erase(self.meshes.begin() + idx);
             })
        .def("remove_point_cloud",
             [](Viewer& self, int idx) {
                 self.point_clouds.erase(self.point_clouds.begin() + idx);
             })
        .def("clear_meshes", [](Viewer& self) { self.meshes.clear(); })
        .def("clear_point_clouds",
             [](Viewer& self) { self.point_clouds.clear(); })
        .def("show", &Viewer::show);
}
