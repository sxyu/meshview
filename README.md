### C++ mesh + point cloud visualizer
A basic OpenGL visualizer for meshes and pointclouds.
Uses Eigen matrices to store data, which is convenient for projects already using Eigen.

Example of visualizer created using meshview:
![Screenshot of smplx-viewer](https://github.com/sxyu/meshview/blob/master/readme-img/smpl-visual.png?raw=true)
Code: <https://github.com/sxyu/smplxpp/blob/master/main_viewer.cpp>

Azure Kinect camera point cloud visualization:
![Screenshot of pcview (depth map viewer)](https://github.com/sxyu/meshview/blob/master/readme-img/depth-cam.png?raw=true)

Scene from `example.cpp`:
![Screenshot of example](https://github.com/sxyu/meshview/blob/master/readme-img/example.png?raw=true)

### Features
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
- Optionally includes Dear ImGUI which can be used without any additional setup, by
    writing ImGui calls (like ImGui::Begin()) in the `viewer.on_gui` event handler  

Note this project does not support importing/exporting models, and is
mostly intended for visualizing programmatically generated objects.
For model I/O please look into integrating assimp.

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
- Eigen 3.3 or 3.4 beta

### Build
Use CMake in the usual way.
`mkdir build && cd build && cmake .. -DCMAKE_BUILD_TYPE=Release`

- Unix: `make -j<threads> && make install`
- Windows: `cmake --build . --target Release`

Options:
- `-DMESHVIEW_BUILD_IMGUI=OFF` to disable Dear ImGui GUI system
- `-DMESHVIEW_BUILD_EXAMPLE=OFF` to disable building the (very simple) example program
