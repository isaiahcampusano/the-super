# hydrogen atom visual sim

An interactive OpenGL sandbox for nuclear particle visualizations. This repository is
bootstrapped from the rendering infrastructure in
[`isaiahcampusano/atom`](https://github.com/isaiahcampusano/atom), without its orbital
math, quantum sampling, or background generation code.


<img width="691" height="391" alt="image" src="https://github.com/user-attachments/assets/ede37238-334f-43a1-8323-d11c06f2ac77" />


---
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

Use `--scenario-test` to verify stable-mode pause/reset, fusion triggering, fission
timing, and fragment separation without opening a window.

Use `--cinematic-test` to render the complete explosion lifecycle in a hidden window
and verify that the fireball and mushroom-cloud particle stages both spawn.

For an immediately running presentation, launch with `--demo-stable`, `--demo-fusion`,
or `--demo-fission`. Use `--demo-cinematic` to preview the four-second macro effect
immediately without waiting for a reaction trigger.

## Controls

- Left-drag to orbit.
- Right-drag to pan.
- Scroll to zoom.
- Press `R` to reset the camera.
- Press `Esc` to exit.

Choose a scenario in the control panel, then use **Start**, **Pause**, and **Reset**.
Fusion begins with two incoming seven-nucleon clusters; fission splits a thirty-nucleon
cluster after three simulated seconds. The progress bar and green event message show
when either reaction triggers.

Enable **Cinematic Explosion** before pressing **Start** to translate that microscopic
trigger into a scripted four-second flash, fireball, shockwave, and mushroom cloud.

## Next steps

Evolve the sphere loop into an instanced nucleon renderer, then add configurable
fission/fusion scenarios and collision initial conditions through the existing ImGui
panel.
