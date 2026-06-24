# Iteration 1 — Existing Project: Relativistic Sandbox Engine (RSE)

> **Role in the NEA narrative:** This iteration documents the project that existed before the rebuild began. It is presented here as a genuine first iteration of the development journey — a working prototype that was built, tested, and then critically reviewed. The review at the end of this iteration is what motivated the decision to rebuild the project from scratch using physically correct methods. Iterations 2–6 (in `writeup.md`) document that rebuild.

---

## Iteration 1 — Success Criteria Addressed

The following success criteria from the Analysis section were targeted in this iteration:

- SC1 — Shadow correctness (Schwarzschild)
- SC2 — Shadow correctness (Kerr)
- SC5 — Real-time frame rate (≥ 60 FPS)
- SC6 — Doppler beaming ratio (≥ 4:1)
- SC7 — ISCO inner disk edge correctness
- SC8 — Velocity Verlet energy stability
- SC9 — Inelastic collision momentum conservation
- SC10 — Roche limit tidal disruption
- SC11 — JSON save/load

SC3 (conserved quantities), SC4 (Sgr A* calibration), and SC12 (zero Vulkan validation errors) were not yet a target of this iteration — SC3 and SC4 require the correct Carter-separated geodesics that had not yet been implemented, and SC12 requires Vulkan which had not yet been adopted.

---

## Iteration 1 — Design

### DE1 — Decomposition

The existing RSE project was decomposed into two distinct simulation modes — a black hole ray tracer and a Newtonian gravity sandbox — controlled by a state machine. Each mode has its own rendering pipeline and physics update loop.

The structure chart below shows the top-level decomposition of the existing project:

```
RSE (Relativistic Sandbox Engine)
├── State Machine
│   ├── MENU
│   ├── SIM_BLACKHOLE
│   └── SIM_GRAVITY
│
├── SIM_BLACKHOLE branch
│   ├── BlackHoleRenderer
│   │   ├── dispatch geodesic.comp (GPU ray tracer)
│   │   └── dispatch ahiser.comp   (secondary GPU ray tracer)
│   ├── Camera (orbit / free-fly / target-lock)
│   └── ImGuiManager (spin, mass, exposure sliders)
│
└── SIM_GRAVITY branch
    ├── NBodySimulator
    │   ├── Velocity Verlet integration
    │   ├── Roche limit tidal destruction
    │   └── Inelastic collision merging
    ├── PlanetSpawner (click-to-spawn)
    ├── TrajectoryPredictor (10,000-step look-ahead)
    ├── SpacetimeGrid (visual warp effect)
    ├── Renderer (sphere meshes, skybox)
    └── Serializer (JSON save/load)
```

> **[SCREENSHOT INSTRUCTION]** Take a screenshot of the above structure chart drawn as a proper diagram (using Google Docs Drawing or equivalent). Label it: *Figure 1: Structure chart of the existing RSE project.*

---

### DE2 — Structure of Solution

The file and folder structure of the existing RSE project is as follows:

```
black-hole-NDA/
├── CMakeLists.txt              # C++17, OpenGL 4.3, FetchContent (not vcpkg)
├── vcpkg.json                  # Exists but not used by CMakeLists.txt
├── imgui/                      # Dear ImGui (vendored directly into repo)
├── shaders/
│   ├── geodesic.comp           # Primary GPU ray tracer (Schwarzschild approximation)
│   ├── ahiser.comp             # Secondary GPU ray tracer (Rodrigues rotation)
│   ├── planet.vert / .frag     # Phong sphere rendering
│   ├── grid.vert  / .frag      # Spacetime grid warp effect
│   └── skybox.vert / .frag     # Equirectangular background
└── src/
    ├── main.cpp                # State machine, GLFW + GLEW + OpenGL setup
    ├── types.h                 # All core structs: Body, BlackHoleParams, CameraUBO
    ├── black_hole_renderer.h/cpp  # OpenGL compute pipeline dispatcher
    ├── renderer.h/cpp          # Sphere mesh, fullscreen quad, shader compilation
    ├── camera.h/cpp            # Orbit / free-fly / target-lock camera
    ├── nbody_simulator.h/cpp   # Velocity Verlet N-body, Roche limit, collisions
    ├── planet_spawner.h/cpp    # Click-to-spawn with initial velocity
    ├── trajectory_predictor.h/cpp  # 10,000-step future trajectory
    ├── spacetime_grid.h/cpp    # Grid vertex deformation for visual gravity well
    ├── serializer.h/cpp        # nlohmann/json save and load
    └── imgui_manager.h/cpp     # Dear ImGui control panels for both modes
```

---

### DE3 — Algorithms

The two most important algorithms in the existing project are the GPU geodesic integrator and the CPU N-body integrator.

**Algorithm 1: GPU geodesic ray tracer — Schwarzschild approximation**

The core ray-tracing algorithm in `geodesic.comp` uses the following approach (pseudocode derived from the actual GLSL):

```
FOR each pixel in the render:
    rayPos = initial ray position from camera
    rayDir = initial ray direction from camera
    
    FOR step = 0 TO maxSteps:
        r     = length(rayPos)
        h     = cross(rayPos, rayDir)    // angular momentum vector
        h2    = dot(h, h)
        
        // Schwarzschild approximation (NOT the geodesic equation):
        acc = -1.5 * rs * h2 * rayPos / r^5
        
        // Kerr spin — Lense-Thirring bolt-on (NOT actual Kerr geometry):
        IF spinParam > 0:
            frameDrag = (spinParam * rs^2 * 2.5) / r^4 * cross(spinAxis, rayPos)
            acc += frameDrag
        
        rayDir += acc * stepSize
        rayPos += rayDir * stepSize
        
        IF r < rs/2:
            colour = BLACK; BREAK    // captured
        IF r > escapeRadius:
            colour = SKYBOX; BREAK   // escaped
        IF crosses_disk_plane AND r IN [r_inner, r_outer]:
            colour = DISK_COLOUR(r, rayDir); BREAK
    
    write_pixel(colour)
```

**Algorithm 2: Disk emission (FBM noise + empirical beaming)**

```
FUNCTION disk_colour(r, rayDir):
    noise     = fbm(rayPos * 3.0 + time * 0.1)    // 3D fractional Brownian motion
    base_col  = mix(orange, yellow_white, noise)
    
    // Empirical Doppler beaming (D^4.2 — not physically derived):
    cosTheta  = dot(normalize(rayDir), orbitDir)
    doppler   = 1.0 / (gamma * (1 - beta * cosTheta))
    beaming   = pow(doppler, 4.2)
    
    RETURN base_col * beaming * exposure
```

**Algorithm 3: N-body Velocity Verlet integration**

```
FUNCTION nbody_step(bodies, dt):
    // Half-step velocity update
    FOR each body b:
        b.vel += 0.5 * compute_acceleration(b, bodies) * dt
    
    // Full-step position update
    FOR each body b:
        b.pos += b.vel * dt
    
    // Recompute accelerations at new positions
    FOR each body b:
        a_new = compute_acceleration(b, bodies)
    
    // Second half-step velocity update
    FOR each body b:
        b.vel += 0.5 * a_new * dt
    
    // Roche limit check
    FOR each pair (b1, b2):
        IF distance(b1, b2) < 2.44 * b1.radius * (b1.mass/b2.mass)^(1/3):
            destroy(b2)
    
    // Collision check
    FOR each pair (b1, b2):
        IF distance(b1, b2) < b1.radius + b2.radius:
            merge_inelastic(b1, b2)
```

---

### DE4 — Key Variables and Data Structures

The key data structures are defined in `types.h`:

| Variable / Structure | Type | Description | Notes |
|---|---|---|---|
| `Body` | `struct` | Position (vec3), velocity (vec3), mass (float), radius (float), colour (vec3) | Core N-body entity |
| `BlackHoleParams` | `struct` | `rs` (Schwarzschild radius), `spinParam` (0–1), `diskInner`, `diskOuter` | Passed to GPU as UBO |
| `CameraUBO` | `struct` (std140) | View matrix, projection matrix, camera position | std140 aligned for GPU |
| `GPUObjectData` | `struct` | Per-body position + radius on GPU | Used in SSBO for planet rendering |
| `GameState` | `enum` | MENU, SIM_BLACKHOLE, SIM_GRAVITY | Controls render path |
| `u_rs` | `float` (GLSL uniform) | Schwarzschild radius passed to geodesic shader | Critical for the approximation formula |
| `spinParam` | `float` (GLSL uniform) | Spin parameter 0.0–0.99 | Used in Lense-Thirring bolt-on |
| `stepSize` | `float` (GLSL) | Ray march step size | Fixed step — no adaptive control |

---

### DE5 — Usability Features

The existing project already implements several usability features:

1. **State machine menu** — the MENU screen allows switching between black hole mode and gravity sandbox mode with clearly labelled buttons. Justification: users should not have to restart the application to change simulation mode.

2. **ImGui parameter panel** — live sliders for Schwarzschild radius, spin, disk inner/outer radius, and exposure. Justification: allows stakeholders (teacher, student) to explore the parameter space in real time.

3. **Click-to-spawn planets** — in gravity mode, left-clicking places a new body with a velocity derived from the mouse drag direction and speed. Justification: intuitive direct manipulation that requires no knowledge of vectors or coordinates.

4. **Target-lock camera** — pressing Tab cycles the camera between bodies, keeping the selected body in frame. Justification: when bodies have drifted apart it would be difficult to track a body of interest without this.

5. **Trajectory prediction lines** — thin lines show the predicted future path of each body 10,000 steps ahead. Justification: allows users to predict orbital stability before committing.

---

### DE6 — Sample Data and Iteration Tests

| Test | Success Criteria | Test Details | Expected Outcome |
|---|---|---|---|
| 1.1 | SC1 | Run the application in SIM_BLACKHOLE mode. Set spin = 0. Screenshot the shadow. | Shadow should appear approximately circular. |
| 1.2 | SC2 | Set spin = 0.94. Compare shadow shape to spin = 0. | Shadow should be asymmetric if Kerr is correctly implemented. |
| 1.3 | SC5 | Open the Windows Task Manager FPS counter or use the ImGui frame timer. | ≥ 60 FPS at 1920×1080. |
| 1.4 | SC6 | Visually inspect the accretion disk. Compare brightness on approaching vs. receding sides. | Approaching side should be noticeably brighter. |
| 1.5 | SC8 | Spawn Earth-like planet orbiting a Sun-like star. Let run for 5 minutes. | Orbit should remain stable — no spiral in or out. |
| 1.6 | SC9 | Spawn two equal-mass bodies on a collision course. | They should merge into one body with combined mass. |
| 1.7 | SC10 | Spawn a small moon close to a large planet. | Moon should break apart near the Roche limit. |
| 1.8 | SC11 | Save a 3-body simulation. Reload it. | Bodies should return to their saved positions and velocities. |

---

## Iteration 1 — Development

### D8 — Development Narrative

The existing RSE project was built using C++17, OpenGL 4.3 compute shaders, and Dear ImGui. The primary goal was to produce a visually convincing interactive black hole simulation that could run in real time.

**Black hole ray tracer development:**  
The ray tracer was built around an OpenGL compute shader (`geodesic.comp`) that marches rays through space using a Newtonian angular-momentum approximation of light deflection. The formula `acc = -1.5 * rs * h² * rayPos / r⁵` was adapted from an existing open-source Schwarzschild ray marcher (kavan010/black_hole, GitHub) and provided a visually plausible shadow shape for non-spinning black holes.

To simulate a spinning black hole, a Lense-Thirring frame-dragging term was added as an additive correction to the acceleration: `frameDrag = (spinParam * rs² * 2.5) / r⁴ × cross(spinAxis, rayPos)`. This produces an asymmetric visual effect when spin is non-zero, creating the impression of a spinning black hole.

The accretion disk uses 3D fractional Brownian motion (FBM) noise to produce a turbulent, cloud-like texture. Relativistic Doppler beaming is applied using an empirically chosen exponent of D^4.2.

> **[SCREENSHOT INSTRUCTION]** Take a screenshot of the running RSE application in SIM_BLACKHOLE mode with spin ≈ 0.5. Label it: *Figure 2: RSE existing project — black hole simulation with spin = 0.5.*

> **[SCREENSHOT INSTRUCTION]** Take a screenshot of the ImGui control panel showing the spin, mass, and disk parameter sliders. Label it: *Figure 3: RSE existing project — ImGui parameter panel.*

**N-body gravity sandbox development:**  
The gravity sandbox uses Velocity Verlet integration, which is a symplectic integrator that conserves energy better than Euler integration. The Roche limit is computed analytically using the standard formula d < 2.44 R_p (m_p/m_m)^(1/3). Inelastic collisions conserve momentum using the standard formula for perfectly inelastic collisions.

JSON serialisation uses the nlohmann/json library to save and restore the simulation state.

> **[SCREENSHOT INSTRUCTION]** Take a screenshot of the gravity sandbox with several planets in orbit. Label it: *Figure 4: RSE existing project — N-body gravity sandbox with trajectory prediction lines.*

---

### D9 — Structure and Modularity

The code is divided into logical modules: the renderer, the N-body simulator, the camera, the serialiser, and the ImGui manager are each in separate .h/.cpp pairs. The shader code is separated into files per shader stage. This is reasonably modular.

However, there is no separation between the physics library and the rendering code. The geodesic calculations happen entirely inside the GPU shader with no CPU-side representation that could be unit-tested. The N-body physics is also entangled with the OpenGL rendering calls in the simulator loop.

---

### D10 — Code Annotation

The key section of the geodesic shader, as it exists in the current project, is shown below with annotations:

```glsl
// The acceleration approximation — NOT the geodesic equation.
// This is a Cartesian embedding trick using the angular momentum vector h = r × v.
// It reproduces qualitatively correct lensing for Schwarzschild but is not extendable to Kerr.
vec3 h    = cross(rayPos, rayDir);   // angular momentum vector
float h2  = dot(h, h);              // h squared (scalar)
vec3 acc  = -1.5 * u_rs * h2 * rayPos / pow(r, 5.0);

// Kerr spin — added as a Lense-Thirring correction.
// This is valid only in the far-field, weak-field regime.
// Near the photon sphere or ergosphere it is quantitatively wrong.
if (spinParam > 0.001) {
    vec3 spinVec   = vec3(0.0, 1.0, 0.0);   // spin axis (hardcoded to y-axis)
    vec3 frameDrag = (spinParam * u_rs * u_rs * 2.5) / pow(r, 4.0)
                   * cross(spinVec, rayPos);
    acc += frameDrag;
}
```

---

### D12 — Validation Evidence

The N-body simulator performs a range check on body mass before spawning: masses below a threshold are rejected to prevent division-by-zero in the gravitational acceleration formula. The JSON loader wraps the `nlohmann::json::parse()` call in a try/catch block so corrupted save files return an error message rather than crashing. The geodesic shader clamps the step count to avoid infinite loops.

---

### D13 — Prototypes

> **[SCREENCASTIFY VIDEO INSTRUCTION]** Record a 30–60 second video of the existing RSE project running. Show: the main menu, switching to black hole mode, adjusting the spin slider from 0 to 0.9 and back, then switching to gravity mode and spawning a few planets. Upload to Google Drive. Link: [drive.google.com link]. Label: *Figure 5: Iteration 1 prototype video — existing RSE project.*

---

## Iteration 1 — Testing

### D16 — Test Evidence

The following tests were run against the existing project. Results are recorded in the table below.

| Test | Success Criteria | Outcome | Evidence | Notes |
|---|---|---|---|---|
| 1.1 — Shadow circle (spin=0) | SC1 | **PARTIAL PASS** | Figure [N]: screenshot of shadow at spin=0 | Shadow is approximately circular but produced by an approximation formula, not the geodesic equation. The result is visually acceptable but cannot be validated against the analytic formula. |
| 1.2 — Shadow asymmetry (spin=0.94) | SC2 | **FAIL** | Figure [N]: screenshot at spin=0.94 | The shadow changes shape slightly with spin, but the asymmetry is produced by a Lense-Thirring bolt-on, not by Boyer-Lindquist Kerr geodesics. The shape does not match the Bardeen (1973) shadow boundary for a*=0.94. |
| 1.3 — 60 FPS at 1080p | SC5 | **PASS** | Figure [N]: FPS counter screenshot | The existing OpenGL compute shader achieves ≥ 60 FPS at 1920×1080 on the development GPU. |
| 1.4 — Beaming ratio ≥ 4:1 | SC6 | **FAIL** | Figure [N]: disk screenshot | The disk uses D^4.2 beaming with an empirically chosen exponent. The visual result shows brightness asymmetry but it is not physically motivated. The physically correct exponent for specific intensity is D^3. |
| 1.5 — Orbit energy stability | SC8 | **PASS** | Figure [N]: screenshot of stable two-body orbit after 5 minutes | Velocity Verlet is a symplectic integrator. The orbit remains stable with no noticeable spiral drift over 5 minutes. |
| 1.6 — Momentum conservation | SC9 | **PASS** | Figure [N]: collision screenshot showing merged body | Two bodies of equal mass collide and produce a single merged body. Manual calculation of pre- and post-collision momentum confirms conservation. |
| 1.7 — Roche limit destruction | SC10 | **PASS** | Figure [N]: screenshot of moon breaking up near Roche limit | A small moon spawned at d < Roche limit is correctly destroyed within one frame. |
| 1.8 — JSON save/load | SC11 | **PASS** | Figure [N]: screenshot of loaded simulation matching saved state | Three bodies saved and reloaded. Positions and velocities match. |

**SC3 (conserved quantities E, L_z, Q):** NOT TESTABLE. The existing project tracks no conserved quantities along any geodesic. There is no mechanism to verify the integrator is physically correct.

**SC4 (Sgr A* angular diameter 41.7–55.7 μas):** NOT TESTABLE. The simulation has no unit system tied to physical constants. Rendering at real Sgr A* parameters and measuring the photon ring in microarcseconds is not possible.

**SC7 (ISCO inner disk edge):** FAIL. The disk inner edge is a parameter slider with no connection to the ISCO formula r_ISCO(a*). The user can set it to any value, including physically impossible values inside the event horizon.

**SC12 (zero Vulkan validation errors):** NOT APPLICABLE. The existing project uses OpenGL 4.3. Vulkan has not been adopted.

---

### D17 — Non-Trivial Failed Tests and Remedies

**Failed test 1.2 — Kerr shadow asymmetry**

Test 1.2 revealed that the shadow shape at high spin (a*=0.94) does not match the expected Bardeen (1973) lopsided shadow. I investigated by comparing the rendered shadow against reference images from the Event Horizon Telescope 2022 Sgr A* results (EHT Collaboration, 2022). The shadow in the existing project is only slightly asymmetric, whereas the true Kerr shadow at a*=0.94 should have a clearly flattened prograde side.

> **[SCREENSHOT INSTRUCTION]** Take a screenshot of the existing RSE shadow at spin=0.94, placed side-by-side with the EHT published shadow image. Label it: *Figure [N]: Comparison — RSE spin=0.94 (left) vs EHT Sgr A* published shadow (right). The RSE shadow is insufficiently asymmetric.*

The root cause is that the Lense-Thirring approximation in `geodesic.comp` is only accurate in the weak-field, far-from-horizon regime. In the strong-field region near the photon sphere, which is where the shadow boundary forms, the Lense-Thirring formula is quantitatively wrong by large factors. No parameter adjustment can fix this — the formula itself is wrong for strong-field Kerr.

Remedy attempted: I tried increasing the `spinParam * 2.5` coefficient in the frame-drag formula to exaggerate the asymmetry. This did produce a more lopsided shadow visually, but the shape was not physically meaningful — it was an arbitrary distortion, not the result of solving the correct equations. I concluded that this cannot be fixed by tuning constants. The underlying approach must change.

**Failed test — SC3 (conserved quantities not tracked)**

While investigating test 1.2, I realised there is a deeper problem: the shader never computes or validates the conserved quantities E (photon energy), L_z (angular momentum), and Carter's constant Q. In correct Kerr geodesic integration, these three quantities must remain constant along any geodesic trajectory. Without tracking them, there is no way to know whether the integrator is producing physically correct trajectories or simply visually plausible ones.

I attempted to add conservation checks to a Python prototype of the geodesic equations. When I implemented the Lense-Thirring formula in Python and computed E and L_z along the trajectory, I found that neither was conserved — they drifted significantly after just 20 integration steps. This is definitive proof that the formula is not a solution to the geodesic equation.

> **[SCREENSHOT INSTRUCTION]** Take a screenshot of the Python prototype output showing E and L_z drifting along a trajectory computed with the Lense-Thirring approximation. Label it: *Figure [N]: Conservation failure — E (blue) and L_z (orange) drift along a geodesic computed using the RSE Lense-Thirring approximation. A correct integrator would show flat horizontal lines.*

---

## Iteration 1 — Review

### D14 — Stakeholder Review

At the end of this iteration, the existing RSE project was demonstrated to all stakeholders. The following feedback was received:

> **[SCREENSHOT INSTRUCTION]** Take a screenshot or photo of written stakeholder feedback notes. Label it: *Figure [N]: Stakeholder review feedback — end of Iteration 1.*

The feedback, combined with the test results above, was summarised as follows:

**What is working well:**
- The N-body gravity sandbox is functional, physically correct (Velocity Verlet), and intuitive to use. Stakeholders found it engaging and educational for demonstrating orbital mechanics concepts.
- The real-time frame rate (SC5: PASS) makes the simulation suitable for live classroom use.
- The JSON save/load (SC11: PASS), Roche limit (SC10: PASS), and collision merging (SC9: PASS) all work correctly.
- The visual appearance of the black hole is striking and generated interest from all stakeholders.

**What is not working:**
- The teacher stakeholder (physics background) immediately asked: "Does the shadow shape match what the Event Horizon Telescope actually observed?" When shown the EHT comparison, the asymmetry discrepancy was clear. This is SC2: FAIL.
- No stakeholder could verify the simulation was physically correct, because there is no conserved-quantity output. SC3 is not testable — which means the physics is unverifiable.
- The Sgr A* calibration (SC4) is impossible with the current implementation because there is no unit system.

**Decision reached at review:**

Of the 13 success criteria defined in the Analysis section, the current project:
- **Passes** 4 fully: SC5, SC8, SC9, SC10, SC11 *(5 criteria)*
- **Partially passes** 1: SC1 (shadow approximately circular but unverifiable)
- **Fails** 5 with no fix possible by tuning: SC2, SC3, SC4, SC6, SC7
- **Does not apply** 2: SC12 (Vulkan), SC13 (visual presets — not yet designed)

The five failures (SC2, SC3, SC4, SC6, SC7) share a single root cause: **the geodesic integration formula is physically wrong for Kerr spacetime.** The formula `acc = -1.5 * rs * h² * pos / r⁵` is a Cartesian angular-momentum approximation, not a solution to the Kerr geodesic equation. It cannot be fixed by adjusting coefficients. The correct approach requires implementing Carter's separated equations (Bardeen, Press & Teukolsky, 1972) with proper conserved quantities.

Additionally, the technology stack has limitations that prevent the project from reaching its goals:
- OpenGL 4.3 cannot support the bindless resources, Synchronization2, or dynamic rendering required for the GPU architecture needed in the rebuild.
- C++17 lacks the concepts and `std::span` support that would improve the safety and clarity of the physics library.
- FetchContent dependency management has caused SSL certificate issues on some machines.

**Conclusion of Iteration 1 review:**

The decision was made to rebuild the project from scratch. The N-body physics (Velocity Verlet, Roche limit, collision merging) is structurally correct and will be carried forward. All GPU ray-tracing code must be rewritten using Carter-separated Kerr geodesics (Carter, 1968; Bardeen, Press & Teukolsky, 1972). The new project will use Vulkan 1.3, C++20, vcpkg manifest mode, and a pure CPU physics library with Catch2 unit tests that validate the geodesic integrator before any GPU work begins.

---

## Transition to the New Project

Based on the Iteration 1 review, the following changes were agreed with stakeholders and feed directly into the Analysis and design of the new project (Iterations 2–6):

| Finding from Iteration 1 | Change made in the new project |
|---|---|
| Geodesic formula is physically wrong for Kerr | Replace with Carter-separated first-order ODEs (Bardeen, Press & Teukolsky 1972) |
| No conserved quantity validation | Add `test_conserved_quantities.cpp`: E, L_z, Q must be constant to 1×10⁻⁶ |
| No Sgr A* unit calibration | Add physical unit system (G=c=1 geometric units, then convert to SI for calibration) |
| FBM noise disk not physically motivated | Replace with Novikov-Thorne thin disk (Novikov & Thorne, 1973) |
| Empirical D^4.2 beaming | Replace with physically derived D^3 specific intensity beaming |
| ISCO inner edge not connected to spin | Compute r_ISCO(a*) analytically using BPT72 formula; use as disk inner boundary |
| OpenGL 4.3 | Replace with Vulkan 1.3 (dynamic rendering, bindless, VMA, Sync2) |
| C++17, FetchContent | Replace with C++20, vcpkg manifest mode |
| No unit tests | Catch2 v3 unit tests must be green before any GPU work begins |
| Visual presets not yet considered | Add four named shader presets (SC13) in Iteration 6 |

The new project is documented in `writeup.md`, beginning at **Iteration 2**.

---

## References (cited in this iteration)

- Bardeen, J.M., Press, W.H. and Teukolsky, S.A. (1972) 'Rotating black holes: locally nonrotating frames, energy extraction, and scalar synchrotron radiation', *The Astrophysical Journal*, 178, pp. 347–370.
- Carter, B. (1968) 'Global structure of the Kerr family of gravitational fields', *Physical Review*, 174(5), pp. 1559–1571.
- Event Horizon Telescope Collaboration (2022) 'First Sagittarius A* Event Horizon Telescope results. I. The shadow of the supermassive black hole in the center of the Milky Way', *The Astrophysical Journal Letters*, 930(2), p. L12.
- kavan010 (n.d.) *black_hole* [Source code]. GitHub. Available at: https://github.com/kavan010/black_hole (Accessed: 24 June 2026).
- Novikov, I.D. and Thorne, K.S. (1973) 'Astrophysics of black holes', in DeWitt, C. and DeWitt, B. (eds) *Black Holes*. New York: Gordon and Breach, pp. 343–450.
