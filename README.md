# hydrogen atom visual sim

An interactive OpenGL sandbox for nuclear particle visualizations. This repository is
bootstrapped from the rendering infrastructure in
[`isaiahcampusano/atom`](https://github.com/isaiahcampusano/atom), without its orbital
math, quantum sampling, or background generation code.

The starter scene includes:

- an OpenGL 3.3 window powered by GLFW and GLAD;
- an orbit, pan, and zoom camera;
- Dear ImGui controls;
- reusable shader and generic point-cloud wrappers;
- a dynamic proton/neutron nucleus driven by Yukawa attraction and softened Coulomb
  repulsion;
- real-time controls for force strength, damping, and simulation speed.

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

Use `--physics-test` to advance the default nucleus for 600 steps without opening a
window and verify that its positions remain finite, bounded, and dynamic.

## Controls

- Left-drag to orbit.
- Right-drag to pan.
- Scroll to zoom.
- Press `R` to reset the camera.
- Press `Esc` to exit.

## Next steps

Evolve the sphere loop into an instanced nucleon renderer, then add configurable
fission/fusion scenarios and collision initial conditions through the existing ImGui
panel.
