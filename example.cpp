#include "meshview/meshview.hpp"
#include "meshview/meshview_imgui.hpp"
#include "Eigen/Core"
#include "Eigen/Geometry"
#include <iostream>

using namespace meshview;

int main(int argc, char** argv) {
    Viewer viewer;
    viewer.draw_axes = true;
    // Adjust camera
    viewer.camera.dist_to_center = 5.f;
    // viewer.camera.center_of_rot = Vector3f();

    // Here's how to adjust lighting
    // viewer.light_pos = Vector3f()
    // viewer.light_color_ambient = Vector3f();
    // viewer.light_color_diffuse = Vector3f();
    // viewer.light_color_specular = Vector3f();

    // ** Some primitives
    // * Draw line
    auto& line = viewer.add_line(Vector3f(-1.f, 1.f, 1.f), Vector3f::Ones(),
                                 /*color*/ Vector3f(1.f, 1.f, 0.f));

    // * Textured cube
    Image mat(256, 256 * 3 /*channels*/);
    Image mat_spec(256, 256 * 3 /*channels*/);
    for (int i = 0; i < 256; ++i) {
        for (int j = 0; j < 256; ++j) {
            // Manually generate texture image
            mat.block<1, 3>(i, j * 3) << i / 255.f,  // r
                1.f - i / 255.f,                     // g
                mat(i, j * 3) = 0.5f;                // b
            mat_spec.block<1, 3>(i, j * 3).setConstant(1.f -
                                                       (j - 128.f) / 128.f);
        }
    }
    auto& cube =
        viewer.add_cube()
            .translate(Vector3f(-2.f, 0.f, 0.f))
            // Diffuse texture (optional)
            .add_texture(mat, /* channels */ 3)
            // Specular texture (optional)
            .add_texture<Texture::TYPE_SPECULAR>(mat_spec, /* channels */ 3);

    // * Basic sphere
    auto& uv_sph =
        viewer
            .add_sphere(Vector3f::Zero(),
                        /* radius */ 0.5f)  // use defalt color orange
            .translate(Vector3f(2.f, 0.f, 0.f))
            .set_shininess(32.f);

    // * Point Cloud: random
    Points random_pts(150, 3);
    random_pts.setRandom();
    viewer.add_point_cloud(random_pts, /*r,g,b*/ 0.f, 1.0f, 1.0f)
        .rotate(Eigen::AngleAxisf(-M_PI / 2.f, Eigen::Vector3f(1.f, 0.f, 0.f))
                    .toRotationMatrix())
        .scale(1.5f);

    // * Triangle mesh: single color
    Points pyra_vert(3 * 6, 3);
    pyra_vert << -1.f, -1.f, -1.f, -1.f, 1.f, -1.f, 1.f, -1.f, -1.f,

        -1.f, 1.f, -1.f, 1.f, 1.f, -1.f, 1.f, -1.f, -1.f,

        -1.f, -1.f, -1.f, 0.f, 0.f, 1.f, -1.f, 1.f, -1.f,

        -1.f, 1.f, -1.f, 0.f, 0.f, 1.f, 1.f, 1.f, -1.f,

        1.f, 1.f, -1.f, 0.f, 0.f, 1.f, 1.f, -1.f, -1.f,

        1.f, -1.f, -1.f, 0.f, 0.f, 1.f, -1.f, -1.f, -1.f;
    auto& pyra = viewer
                     .add_mesh(pyra_vert,
                               /* Triangle indices (unsigned fx3) here; pass
                                  empty to use: 0 1 2, 3 4 5 etc */
                               Triangles(),
                               /*r,g,b*/ 0.f, 1.0f, 1.0f)
                     .translate(Vector3f(0.f, 0.f, 3.f))
                     .set_shininess(32.f);

    // * Triangle mesh: vertex-interpolated colors
    Points pyra_vert_colors(3 * 6, 3);
    pyra_vert_colors << 1.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f, 0.f,

        0.f, 1.f, 0.f, 1.f, 1.f, 0.f, 1.f, 0.f, 0.f,

        1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f, 1.f, 0.f,

        0.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 1.f, 0.f,

        1.f, 1.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f,

        1.f, 0.f, 0.f, 0.f, 0.f, 1.f, 1.f, 0.f, 0.f;
    auto& pyra2 = viewer
                      .add_mesh(pyra_vert,
                                /* Triangle indices (unsigned fx3) here; pass
                                   empty to use: 0 1 2, 3 4 5 etc */
                                Triangles(),
                                /* vx3 color matrix*/ pyra_vert_colors)
                      .set_shininess(32.f)
                      .translate(Vector3f(3.f, 3.f, 0.f));

    // * Triangle mesh: textured (tex coords are for demonstration only, not a
    // real uv unwrap)
    Points2D pyra_vert_texcoord(3 * 6, 2);
    pyra_vert_texcoord << 0.f, 0.f, 0.f, 1.f, 1.f, 0.f,

        0.f, 1.f, 1.f, 1.f, 1.f, 0.f,

        0.f, 0.f, 0.5f, 0.5f, 0.f, 1.f,

        0.f, 1.f, 0.5f, 0.5f, 1.f, 1.f,

        1.f, 1.f, 0.5f, 0.5f, 1.f, 0.f,

        1.f, 0.f, 0.5f, 0.5f, 0.f, 0.f;
    Triangles pyra_vert_tex_triangles(6, 3);
    // these indices are tex coord indices corresponding to the vertex
    // indices of each face
    for (size_t i = 0; i < pyra_vert_tex_triangles.size(); ++i)
        pyra_vert_tex_triangles.data()[i] = i;

    auto& pyra3 =
        viewer
            .add_mesh(pyra_vert,
                      /* Triangle indices (unsigned fx3) here; pass empty to
                         use: 0 1 2, 3 4 5 etc */
                      Triangles())
            .set_tex_coords(pyra_vert_texcoord, pyra_vert_tex_triangles)
            .translate(Vector3f(-3.f, 3.f, 0.f))
            // Diffuse texture (reusing from above)
            .add_texture(mat, /* channels */ 3)
            // Specular texture (reusing from above)
            .add_texture<Texture::TYPE_SPECULAR>(mat_spec, /* channels */ 3);

    // * Events: key handler
    viewer.on_key = [&](int button, input::Action action, int mods) -> bool {
        if (action != input::Action::release) {
            if (button == 'D') {
                // Press D to make textured pyramid go right
                pyra3.translate(Vector3f(0.05f, 0.f, 0.f));
            } else if (button == 'E') {
                // Press E to make textured pyramid's top vertex become larger
                pyra3.verts_pos()(4, 2) += 0.1f;
                // In order to update mesh, we need to use:
                pyra3.update();
                // OR if we are in on_loop or on_gui event we can return true
                // to update all
            }
        }
        return true;  // Don't prevent default
    };
    // * Events: loop callback
    viewer.on_loop = [&]() -> bool {
        return false;  // True to update all meshes and camera
    };
#ifdef MESHVIEW_IMGUI
    viewer.on_gui = [&]() -> bool {
        ImGui::SetNextWindowSize(ImVec2(200, 100));
        ImGui::Begin("Hello");
        ImGui::Text("Hello world");
        ImGui::Button("Panic button");
        ImGui::End();
        return false;  // True to update all meshes and camera
    };
#else
    std::cout
        << "meshview was built without Dear ImGUI, no GUI will be available\n";
#endif
    viewer.show();
}
