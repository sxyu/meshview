### Easy C++ mesh + point cloud visualizer
Basic OpenGL visualizer for meshes and pointclouds, supporting
- Drawing meshes (using phong lighting model)
    - Coloring mesh via texture mapping
        - Diffuse and specular textures
    - Alternatively, coloring mesh by interpolating vertex color
    - Automated vertex normal computation
- Drawing point clouds
- Drawing poly-lines (as point clouds with .line=true)
- Drawing several geometric objects (line, cube, square, [UV] sphere) directly
- Controlling camera and lighting
- RGB/XYZ axes

Note this project does not support importing/exporting models, and is
intended for visualizing specific models and programmatically generated objects,
e.g. in 3D vision. For model I/O please look into integrating assimp.

```cpp
#include "meshview/meshview.hpp"
...

meshview::Viewer viewer;
// do drawing
viewer.add_xyz(args);
// handle events
viewer.on_xyz = [&](args) {
}
viewer.show();
```

See `example.cpp` for usage examples.

## Installation
### External dependencies
- OpenGL 3+
- C++14
### Build
Use CMake in the usual way.
`mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release`

- Unix: `make -j<threads> && make install`
- Windows: `cmake --build . --target Release`

Options:
- `-DMESHVIEW_BUILD_IMGUI=OFF` to disable Dear ImGui GUI system
- `-DMESHVIEW_BUILD_EXAMPLE=OFF` to disable building the (very simple) example program
