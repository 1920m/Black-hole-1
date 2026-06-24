# Existing Project Audit — "black hole NDA" / RSE

> This is an analysis-only document. It describes what the old project contains and
> why it is being replaced. Do NOT use any of these files as a starting point.

---

## What exists

**Project name:** Relativistic Sandbox Engine (RSE)
**Binary:** `RSE.exe`
**Build:** CMake + MSVC, OpenGL 4.3, C++17
**Dependency strategy:** FetchContent (GLFW 3.4, GLM 1.0.1, nlohmann-json 3.11.3, GLEW cmake 2.2.0)
  — Note: `vcpkg.json` exists but the CMakeLists.txt does NOT use vcpkg; it uses FetchContent.

### Source modules

| File | What it does |
|---|---|
| `main.cpp` | State machine: MENU → SIM_BLACKHOLE or SIM_GRAVITY |
| `black_hole_renderer.h/cpp` | OpenGL compute pipeline wrapper; dispatches geodesic.comp |
| `renderer.h/cpp` | OpenGL sphere mesh, fullscreen quad, shader compilation utils |
| `camera.h/cpp` | Orbit / free-fly / target-lock camera |
| `nbody_simulator.h/cpp` | Velocity Verlet N-body with Roche limit and inelastic merging |
| `planet_spawner.h/cpp` | Click-to-spawn bodies |
| `trajectory_predictor.h/cpp` | 10,000-step lookahead for trajectory lines |
| `spacetime_grid.h/cpp` | Warped grid for visual gravity well effect |
| `serializer.h/cpp` | nlohmann/json save/load of `std::vector<Body>` |
| `imgui_manager.h/cpp` | Dear ImGui menus for both modes |
| `types.h` | All core structs: Body, BlackHoleParams, CameraUBO, GPUObjectData |

### Shaders

| File | What it does |
|---|---|
| `geodesic.comp` | Volumetric raymarcher. Schwarzschild angular-momentum approximation with bolted-on Lense-Thirring spin. FBM noise disk. |
| `ahiser.comp` | Rodrigues rotation / RK4 angular deflection raymarcher. Different approximation, also not Carter equations. |
| `planet.vert/frag` | Phong sphere rendering for sandbox mode |
| `grid.vert/frag` | Spacetime grid rendering |
| `skybox.vert/frag` | Equirectangular background |

---

## What is good (salvageable ideas, not code)

1. **N-body physics architecture** — The `NBodySimulator` using Velocity Verlet is structurally
   sound. The new project's CPU physics library should implement equivalent functionality,
   but the code should be rewritten cleanly for C++20.

2. **ImGui overlay pattern** — The idea of using ImGui for live parameter tweaking (spin,
   inclination, mass, exposure) is correct and should be carried forward.

3. **State machine design** — MENU → SIM_BLACKHOLE → SIM_GRAVITY as app states is a good
   pattern. The new project can keep this structure.

4. **Conserved quantities documentation** — The types.h and projectPRD.md show the author
   understands the physics at a high level. The problem is implementation, not understanding.

5. **`types.h` std140 alignment** — The GPU struct alignment thinking (GPUObjectData, CameraUBO)
   is correct and should inform the new Vulkan UBO/push-constant design.

---

## What is wrong (why this is being rebuilt)

### Physics errors (critical)

**Problem 1: geodesic.comp uses the wrong equation**
```glsl
// FROM geodesic.comp (OLD - DO NOT USE)
vec3 acc = -1.5 * u_rs * h2 * rayPos / pow(r, 5.0);
```
This is a Newtonian-style approximation derived from the angular momentum vector h = r × v.
It is a Cartesian embedding trick that happens to reproduce qualitatively correct Schwarzschild
lensing for distant observers, but:
- It is not a solution to the geodesic equation
- It does not have proper conserved quantities
- It cannot be extended to Kerr without fundamental restructuring

**Problem 2: Kerr spin is faked**
```glsl
// FROM geodesic.comp (OLD - DO NOT USE)
if (spinParam > 0.001) {
    vec3 spinVec = vec3(0.0, 1.0, 0.0);
    vec3 frameDrag = (spinParam * u_rs * u_rs * 2.5) / pow(r, 4.0) * cross(spinVec, rayPos);
    acc += frameDrag;
}
```
This is a Lense-Thirring approximation valid only in the weak-field, far-from-horizon regime.
Near the ergosphere and photon sphere (where the interesting effects are), this formula is
quantitatively wrong by large factors. The Kerr shadow asymmetry it produces is not physically
accurate.

**Problem 3: ahiser.comp uses a different approximation**
The Rodrigues rotation formula in ahiser.comp computes angular deflection per step using the
impact parameter, not Carter's constants. This is also not the geodesic equation. The two
shaders produce different (wrong) results and cannot both be "correct."

**Problem 4: No conserved quantities**
Neither shader tracks E, L_z, or Q. There is no way to validate that the integrator is
physically correct. The renders look good but the physics is unverifiable.

**Problem 5: FBM noise disk**
The accretion disk uses 3D FBM noise for a cinematic look. While visually impressive, it is
not physically motivated. The Novikov-Thorne temperature profile is not implemented.
The Doppler beaming formula is:
```glsl
float doppler = 1.0 / (gamma * (1.0 - beta * cosTheta));
float beaming = pow(doppler, 4.2);
```
The D^4.2 exponent is empirically chosen, not derived. The correct exponent is D^3 or D^4
depending on convention (specific intensity vs flux). This is a small error but indicative
of the overall lack of rigour.

### Stack errors

**Problem 6: OpenGL 4.3, not Vulkan**
The entire rendering pipeline is OpenGL. This means no modern bindless resources, no
Synchronization2, no dynamic rendering, no proper GPU memory management.

**Problem 7: FetchContent with SSL disabled**
```cmake
set(CMAKE_TLS_VERIFY OFF)
```
This is a workaround for a dependency management problem, not a solution.

**Problem 8: No CMakePresets, no Ninja**
The build system uses MSVC project generation. No reproducible cross-platform presets.

**Problem 9: No unit tests**
There is not a single test file in the old project. No Catch2, no Google Test, nothing.
The physics has never been validated programmatically.

**Problem 10: C++17, not C++20**
Missing concepts, std::span, and other modern features that would improve the buffer
and resource handling code.

**Problem 11: No separation of physics and rendering**
All physics is entangled with OpenGL calls in main.cpp and its subsystems. There is no
pure physics library that can be tested independently of the GPU.

---

## Conclusion

The old project is a working visual demo of an approximate black hole renderer. It is not
a physically accurate simulation. The decision to rebuild from scratch is correct. The new
architecture (physics library → CPU reference tracer → Vulkan engine) is the right approach
and the conserved-quantity unit tests are the key mechanism that will make the physics
verifiable and trustworthy.
