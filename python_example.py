import meshview

v = meshview.Viewer()
v.draw_axes = False

sph = v.add_sphere([0, 0, 0], 0.5)
sph.shading_type = meshview.ShadingType.vertex
sph.verts_rgb[:] = [1, 0, 0]  # red

cube = v.add_cube([2, 0, 0], 0.25)

v.show()
