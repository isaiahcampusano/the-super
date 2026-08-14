# The Super

An interactive OpenGL sandbox for nuclear particle visualizations. This repository is
bootstrapped from the rendering infrastructure in
[`isaiahcampusano/atom`](https://github.com/isaiahcampusano/atom), without its orbital
math, quantum sampling, or background generation code.

The starter scene includes:

- an OpenGL 3.3 window powered by GLFW and GLAD;
- an orbit, pan, and zoom camera;
- Dear ImGui controls;
- reusable shader and generic point-cloud wrappers;
- a small animated proton/neutron placeholder nucleus.

## Build

The first configure downloads GLFW, GLM, and Dear ImGui.

```sh
cmake -S . -B build
cmake --build build --config Release
```

Run `build/the-super` on single-configuration generators, or
`build/Release/the-super.exe` with Visual Studio generators.

Use `--smoke-test` to open a hidden window, render three frames, and exit. This still
requires a working graphical environment.

## Controls

- Left-drag to orbit.
- Right-drag to pan.
- Scroll to zoom.
- Press `R` to reset the camera.
- Press `Esc` to exit.

## Next steps

Replace the placeholder scene with simulation state, evolve `PointCloudRenderer` into
an instanced nucleon renderer, and add fission/fusion controls through the existing
ImGui panel.
