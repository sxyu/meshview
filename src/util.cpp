#include "meshview/util.hpp"

#include <fstream>
#include <iostream>
#include <Eigen/Geometry>

#include "meshview/common.hpp"
#include "meshview/internal/assert.hpp"

namespace meshview {
namespace util {

Matrix4f persp(float xscale, float yscale, float z_near, float z_far) {
    Matrix4f m;
    m << xscale, 0.f, 0.f, 0.f, 0.f, yscale, 0.f, 0.f, 0.f, 0.f,
        -(z_far + z_near) / (z_far - z_near),
        -2.f * z_near * z_far / (z_far - z_near), 0.f, 0.f, -1.f, 0.f;
    // 0.f, 0.f, z_far / (z_near - z_far), -(z_far * z_near) / (z_far - z_near),
    // 0.f, 0.f, -1.f, 0.f;
    return m;
}

Matrix4f look_at(const Eigen::Ref<const Vector3f>& pos,
                 const Eigen::Ref<const Vector3f>& fw,
                 const Eigen::Ref<const Vector3f>& up) {
    Matrix4f m;
    m.topLeftCorner<1, 3>() = fw.cross(up).transpose();
    m.block<1, 3>(1, 0) = up.transpose();
    m.block<1, 3>(2, 0) = -fw.transpose();
    m.bottomLeftCorner<1, 3>().setZero();
    m(3, 3) = 1.f;
    m.topRightCorner<3, 1>() = -m.topLeftCorner<3, 3>() * pos;
    return m;
}

void estimate_normals(const Eigen::Ref<const Points>& verts,
                      const Eigen::Ref<const Triangles>& faces,
                      Eigen::Ref<Points> out) {
    if (faces.rows() == 0) {
        // Defer to 'no-element-buffer' version
        estimate_normals(verts, out);
        return;
    }
    Vector face_cnt(verts.rows());
    face_cnt.setZero();

    out.setZero();

    Eigen::RowVector3f face_normal;
    for (int i = 0; i < faces.rows(); ++i) {
        face_normal =
            (verts.row(faces(i, 1)) - verts.row(faces(i, 0)))
                .cross(verts.row(faces(i, 2)) - verts.row(faces(i, 1)))
                .normalized();
        for (int j = 0; j < 3; ++j) {
            out.row(faces(i, j)) += face_normal;
            face_cnt[faces(i, j)] += 1.f;
        }
    }
    out.array().colwise() /= face_cnt.array();
}

void estimate_normals(const Eigen::Ref<const Points>& verts,
                      Eigen::Ref<Points> out) {
    Vector face_cnt(verts.rows());
    face_cnt.setZero();
    out.setZero();
    Vector3f face_normal;
    for (int i = 0; i < verts.rows(); i += 3) {
        face_cnt.segment<3>(i).array() += 1.f;
        out.middleRows<3>(i).rowwise() +=
            (verts.row(i + 1) - verts.row(i))
                .cross(verts.row(i + 2) - verts.row(i + 1))
                .normalized();
    }
    out.array().colwise() /= face_cnt.array();
}

Eigen::Matrix<Index, Eigen::Dynamic, 1> make_uv_to_vert_map(
    size_t num_uv_verts, const Eigen::Ref<const Triangles>& tri_faces,
    const Eigen::Ref<const Triangles>& uv_tri_faces) {
    _MESHVIEW_ASSERT_EQ(tri_faces.rows(), uv_tri_faces.rows());
    Eigen::Matrix<Index, Eigen::Dynamic, 1> result(num_uv_verts);
    result.setConstant(-1);
    for (size_t i = 0; i < uv_tri_faces.rows(); ++i) {
        for (size_t j = 0; j < 3; ++j) {
            result[uv_tri_faces(i, j)] = tri_faces(i, j);
        }
    }
    // Each uv vertex must be matched to some vertex
    for (size_t i = 0; i < num_uv_verts; ++i) {
        _MESHVIEW_ASSERT_NE(result[i], (Index)-1);
    }
    return result;
}

}  // namespace util
}  // namespace meshview
