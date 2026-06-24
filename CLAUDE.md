# CLAUDE.md — Kerr Black Hole Simulator (kerr-blackhole-sim)

> This file is auto-loaded by Claude Code on every session. Read it fully before touching any file.

---

## 0. What this project is and why it exists

This is a ground-up rewrite of a previous project ("Relativistic Sandbox Engine", `black hole NDA/`).
The old project was abandoned because it had deep architectural problems that made further
development inefficient. **Do not copy code from the old project.** Do not reference it for
physics formulas. Its geodesic math is wrong (see §2 below).

The goal: a physically accurate, real-time Kerr (spinning) black hole ray tracer calibrated to
Sgr A*, running on the GPU via Vulkan 1.3 compute shaders, with a clean pure-physics CPU library
that is fully unit-tested before any GPU work begins.

---

## 1. Non-negotiable stack decisions

| Layer | Choice | Reason / What NOT to do |
|---|---|---|
| Language | **C++20** | Not C++17. Use concepts, `std::span`, ranges where appropriate. |
| Build | **CMake + Ninja + CMakePresets.json** | No global `include_directories`. All targets via `target_*`. |
| Packages | **vcpkg manifest mode** (`vcpkg.json`) | Not FetchContent. Declare deps in manifest; toolchain picks them up. |
| GPU API | **Vulkan 1.3** with dynamic rendering | **NOT OpenGL**. No render passes, no framebuffers for the ray tracer. Use `VK_KHR_dynamic_rendering`. |
| Vulkan bootstrap | **vk-bootstrap** | Handles instance/device/swapchain tedium. |
| Vulkan loader | **volk** | Meta-loader. Do not link against the platform Vulkan loader directly. |
| GPU memory | **VMA** (Vulkan Memory Allocator) | Do not hand-roll allocation. |
| Resource binding | **Bindless** (descriptor indexing / buffer device addresses) | No per-draw descriptor set churn. One global resource array. Use `VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT`. |
| Synchronization | **Synchronization2** (`vkCmdPipelineBarrier2`, `VkSubmitInfo2`) | Not the old sync1 model. |
| Math | **GLM** | GLSL-mirrored types make CPU↔shader porting direct. |
| Windowing | **GLFW** | Straightforward Vulkan companion. |
| GUI | **Dear ImGui** (docking branch) + its Vulkan backend | Spin/inclination/mass/exposure sliders live here. |
| Shaders | **GLSL compute shaders** → SPIR-V via `shaderc` / `glslang` | Start with GLSL. Consider Slang later. |
| Unit tests | **Catch2 v3** | Mandatory. ALL physics tests green before any GPU work. |
| Benchmarks | **Google Benchmark** | CPU baseline for the geodesic integrator before GPU port. |
| GPU debug | **RenderDoc** + **Vulkan Validation Layers** | Validation layers ON in every Debug build. Non-negotiable. |
| CPU profiling | **Tracy Profiler** | Frame timing. |
| GPU profiling | **NVIDIA Nsight** (or vendor equivalent) | Shader occupancy. |
| CI | **GitHub Actions** | Matrix: Linux + Windows. cmake --preset, build, ctest, clang-tidy. |

**vcpkg.json must declare:** `glm`, `catch2`, `benchmark`, `glfw3`, `imgui`, `vulkan-headers`,
`vk-bootstrap`, `vulkan-memory-allocator`, `shaderc`.

---

## 2. What was wrong with the old project's physics (critical — read carefully)

The old `geodesic.comp` shader uses this acceleration formula:
```
vec3 acc = -1.5 * u_rs * h2 * rayPos / pow(r, 5.0);
```
This is a **Cartesian angular-momentum approximation** of Schwarzschild geodesics — a numerical
trick, not the actual geodesic equation. It works visually for a non-spinning hole but:

1. It cannot correctly model Kerr (spinning) spacetime.
2. The spin (`spinParam`) is bolted on as a crude Lense-Thirring frame drag:
   ```
   vec3 frameDrag = (spinParam * u_rs * u_rs * 2.5) / pow(r, 4.0) * cross(spinVec, rayPos);
   ```
   This is physically wrong for anything other than weak-field far-field approximations.
3. There are no conserved quantities being tracked. The photon's energy E, angular momentum L_z,
   and Carter constant Q are never computed. Without them there is no way to validate the integrator.

**The correct approach is Carter's separated equations (1968):**
```
Σ (dr/dλ)  = ± √R(r)
Σ (dθ/dλ) = ± √Θ(θ)
Σ (dφ/dλ) = −(aE − L_z/sin²θ) + a/Δ · P(r)
Σ (dt/dλ) = −a(aE sin²θ − L_z) + (r²+a²)/Δ · P(r)
```
where:
```
Σ = r² + a²cos²θ
Δ = r² − 2Mr + a²
P(r) = E(r²+a²) − aL_z
R(r) = P(r)² − Δ[μ²r² + (L_z − aE)² + Q]
Θ(θ) = Q − cos²θ[a²(μ²−E²) + L_z²/sin²θ]
```
These are four first-order ODEs. For null geodesics (photons), μ=0. The ± signs flip at turning
points where R(r)=0 or Θ(θ)=0. Reference: Bardeen, Press & Teukolsky (1972).

The **ahiser.comp** shader in the old project uses Rodrigues' rotation / RK4 on angular deflection
— a different approximation, also not proper Carter equations.

---

## 3. Kerr metric essentials

Boyer-Lindquist coordinates (G=c=1 geometric units, M=mass, a=J/M spin per unit mass):

```
Σ  = r² + a²cos²θ
Δ  = r² − 2Mr + a²
A  = (r²+a²)² − a²Δsin²θ

ds² = −(1 − 2Mr/Σ) dt²
      − (4Mar sin²θ/Σ) dt dφ
      + (Σ/Δ) dr²
      + Σ dθ²
      + (A sin²θ/Σ) dφ²
```

Dimensionless spin: `a* = a/M ∈ [0,1]` (extremal at 1, never physically reached).

### Key radii — VALIDATE THESE FIRST, before any ray tracing

| Quantity | Formula | Schwarzschild check (a*=0) |
|---|---|---|
| Outer horizon | `r₊ = M + √(M²−a²)` | 2M ✓ |
| Ergosphere | `r_E(θ) = M + √(M²−a²cos²θ)` | 2M at equator, 2M at poles ✓ |
| Photon sphere prograde | `r_ph = 2M{1+cos[(2/3)cos⁻¹(−a/M)]}` | 3M ✓ |
| Photon sphere retrograde | `r_ph = 2M{1+cos[(2/3)cos⁻¹(+a/M)]}` | 3M ✓ |
| ISCO prograde | See Z₁, Z₂ formula (BPT72 or §2.2 of blueprint) | 6M ✓ |

**At a*=0, ALL Kerr formulas must reduce exactly to Schwarzschild values.**
**At a*>0, prograde photon sphere ≠ retrograde photon sphere — shadow is lopsided.**
If your shadow is a perfect circle for a*>0, the spin terms are not working. Do not proceed to Phase 3 until this is fixed.

---

## 4. Sgr A* calibration targets

| Parameter | Value |
|---|---|
| Mass | 4.297×10⁶ M☉ (8.54×10³⁶ kg) |
| Distance | 8,277 ± 9 pc |
| Best-bet spin | a* ≈ 0.94 (prograde; build as slider, not constant) |
| Observer inclination | ~30° from spin axis |
| Shadow angular diameter | ~48.7 ± 7 μas (EHT 2022) |
| Photon ring diameter | ~51.8 ± 2.3 μas |

Final validation: render at these parameters and confirm photon ring angular size at correct distance.

---

## 5. Conserved quantities — the integrator health check

Along any correctly integrated geodesic, these must be constant to within numerical tolerance:
- **E** = −p_t (energy, from t-independence of metric)
- **L_z** = p_φ (angular momentum, from φ-independence)
- **Q** = Carter's constant (from hidden Killing tensor symmetry)

Add a `test_conserved_quantities.cpp` Catch2 test that integrates 100 steps and asserts:
```
|E(λ) − E(0)| / |E(0)| < 1e-6   (for RK45 with appropriate step size)
|L_z(λ) − L_z(0)| / |L_z(0)| < 1e-6
|Q(λ) − Q(0)| / |Q(0)| < 1e-6
```
If these drift, your integrator step size is too coarse OR you have a bug in the equations.
This test must pass before Phase 2 is considered complete.

---

## 6. Git versioning — mandatory at every gate

This project uses **git tags** to preserve a runnable snapshot of the codebase at the end of every iteration. This allows any previous iteration to be checked out and run at any time, without maintaining separate copies of the code.

### Rules — follow these exactly, every time

1. **Initialise git on the first commit** (if not already done):
   ```bash
   git init
   git add .
   git commit -m "chore: initial project scaffold"
   ```

2. **Commit frequently** during development — at minimum once per file created and once per gate passed. Commit messages must follow this format:
   ```
   feat: implement KerrMetric class (Phase 0)
   fix: correct Carter Q derivative sign error
   test: Gate 0 — all Catch2 tests passing
   ```

3. **Tag every gate** immediately after `ctest` goes green (or the equivalent gate check passes). Use these exact tag names:

   | Gate | Tag name | When to apply |
   |---|---|---|
   | Gate 0 | `iteration-1-gate-0` | Catch2: Schwarzschild + key radii + conserved quantities green |
   | Gate 1 | `iteration-1-complete` | CPU Schwarzschild shadow PNG saved |
   | Gate 2 | `iteration-2-gate-2` | Kerr θ-oscillation confirmed, Q conserved |
   | Gate 3 | `iteration-2-complete` | Lopsided Kerr shadow PNG saved |
   | Gate 4 | `iteration-3-complete` | Disk beaming, photon ring, ISCO inner edge correct |
   | Gate 5 | `iteration-4-gate-5` | Vulkan engine running, zero validation errors |
   | Gate 6 | `iteration-4-complete` | CPU vs GPU L2 diff < 0.01 at 64×64 |
   | Gate 8 | `iteration-5-complete` | Sgr A* photon ring 41.7–55.7 μas |
   | Presets | `iteration-6-complete` | All four visual presets working, git diff confirms geodesic.comp unchanged |

4. **Tag command:**
   ```bash
   git tag -a iteration-1-complete -m "Iteration 1 complete: CPU Schwarzschild shadow, Gate 0 + Gate 1 passing"
   git push origin --tags   # if a remote is configured
   ```

5. **Never tag a gate that is failing.** If tests are red, fix them first, then tag.

6. **Screenshot the tag** — after running `git tag`, run `git log --oneline --decorate -5` and take a terminal screenshot. This is evidence for the NEA write-up showing version control was used throughout development.

### Why tags, not separate folders

Separate iteration folders would mean duplicating the entire physics library in every iteration folder. A bug fixed in Iteration 3's `geodesics.cpp` would need to be manually patched in Iterations 1 and 2 as well. Git tags give a clean, single-copy history where any iteration is reachable via `git checkout iteration-2-complete`.

---

## 6b. Screenshot policy — mandatory throughout development

Screenshots are required for the NEA write-up. Take a screenshot at every one of these moments and save it to `writeup/screenshots/iteration_N/` (where N is the current iteration number):

- After `cmake --preset debug` completes successfully (configuration output)
- After each new class header is written (before the .cpp exists — shows the interface design)
- **Any compilation or test error** — screenshot BEFORE fixing it (the error message is evidence)
- After fixing the error — screenshot confirming it is resolved
- After every `ctest` run — pass or fail
- After every rendered output (open the PNG and screenshot it)
- After running `git tag` — screenshot of `git log --oneline --decorate -5`
- End of each iteration — full-screen screenshot of the running application

Name screenshots descriptively: `iter1_cmake_config.png`, `iter1_kerr_metric_error.png`, `iter1_kerr_metric_fixed.png`, `iter1_ctest_gate0_pass.png`.

---

## 7. Build order — phases and gates

### Phase 0 — Math warm-up (CPU only, ~Schwarzschild)
- Implement `physics/src/metric.cpp`: Kerr/Schwarzschild metric tensor, Christoffel symbols
- Implement `physics/src/geodesics.cpp`: Carter equations, hand-rolled RK4/RK45 integrator
- Implement `physics/include/kerr_physics/constants.hpp`: Sgr A* params, G, c, M
- **Gate**: `ctest` green on:
  - `test_schwarzschild_limit.cpp` — a*=0 reduces correctly (r₊=2M, r_ph=3M, r_ISCO=6M)
  - `test_key_radii.cpp` — prograde/retrograde photon sphere asymmetry for a*=0.5, 0.94
  - `test_conserved_quantities.cpp` — E, L_z, Q constant along integrated geodesic

### Phase 1 — CPU reference ray tracer (Schwarzschild)
- `physics/src/raytrace_cpu.cpp`: backward ray trace, Schwarzschild, simple background + disk
- Validate: shadow is a **perfect circle**, angular size matches analytic formula
- Save reference PNG. This is your ground truth for all future GPU work.
- **Gate**: CPU image produced and stored in `validation/reference_images/`

### Phase 2 — Kerr geodesics
- Extend geodesic integrator to full Kerr (a*≠0)
- Re-run all `test_key_radii.cpp` tests for multiple a* values
- **Gate**: Conserved quantities still constant in Kerr. Prograde ≠ retrograde photon sphere.

### Phase 3 — Kerr CPU ray tracer
- Full backward ray tracing through Kerr spacetime
- **Gate**: Shadow is **lopsided** (flattened on prograde side). Perfect circle = failure, debug spin terms.

### Phase 4 — Disk emission physics
- Redshift factor `g = ν_obs/ν_emit` (gravitational + kinematic Doppler)
- Relativistic beaming: intensity ∝ g⁴
- Novikov-Thorne or T(r)∝r^(−3/4) temperature profile
- Blackbody color table
- **Gate**: Disk brightness is asymmetric (approaching side brighter). Lensed "second image" of far disk visible above the shadow.

### Phase 5 — Vulkan engine stand-up (NO physics yet)
- Stand up `engine/` with vk-bootstrap + volk + dynamic rendering
- Compute shader writes a solid color to a storage image and presents it
- **Absolutely solve all Vulkan plumbing before introducing physics bugs on top of it**
- Validation layers ON from the first line of Vulkan code
- **Gate**: Application runs, solid color renders, no validation layer errors

### Phase 6 — GPU geodesic port
- Port `physics/src/geodesics.cpp` → `engine/src/shaders/geodesic.comp.glsl`
- Use bindless resources for all per-frame parameters (camera, spin, mass, inclination)
- **Gate**: Run `validation/compare_shadow_size.cpp` — pixel diff between CPU reference (Phase 1/3) and GPU output must be below threshold at 64×64 resolution

### Phase 7 — Disk shading + ImGui
- Port disk emission to `engine/src/shaders/disk_shading.comp.glsl`
- Add ImGui sliders: a* (spin), inclination, mass, exposure, disk inner/outer radius
- **Gate**: Interactive render, disk beaming visible, no validation errors

### Phase 8 — Sgr A* calibration
- Plug in real Sgr A* numbers from §4
- Render at 30° inclination
- Convert simulation units → angular size at 8,277 pc
- **Gate**: Photon ring diameter falls within ~48–52 μas

### Phase 9 — Performance
- Tracy CPU profiling, Nsight GPU profiling
- Compare against `physics/benches/geodesic_bench.cpp` (Google Benchmark baseline)
- Optimize shader occupancy, memory access patterns

---

## 7. Project file structure

```
kerr-blackhole-sim/
├── CMakeLists.txt                        # add_subdirectory(physics), add_subdirectory(engine)
├── CMakePresets.json                     # debug/release × linux/windows
├── vcpkg.json                            # glm, catch2, benchmark, glfw3, imgui,
│                                         # vulkan-headers, vk-bootstrap, vma, shaderc
├── docs/
│   ├── physics-notes.md                  # derivations, Carter's constant, key radii
│   └── references.bib                    # BPT72, Luminet79, James2015, EHT papers
│
├── physics/                              # pure CPU static library, GLM only
│   ├── CMakeLists.txt
│   ├── include/kerr_physics/
│   │   ├── metric.hpp
│   │   ├── geodesics.hpp
│   │   ├── constants.hpp
│   │   └── raytrace_cpu.hpp
│   ├── src/
│   │   ├── metric.cpp
│   │   ├── geodesics.cpp                 # Carter-separated RK4/RK45 integrator
│   │   ├── constants.cpp
│   │   └── raytrace_cpu.cpp              # slow CPU reference renderer
│   ├── tests/
│   │   ├── test_schwarzschild_limit.cpp  # a*=0 reduces to Schwarzschild
│   │   ├── test_key_radii.cpp            # r₊, r_ph (prograde/retrograde), r_ISCO
│   │   └── test_conserved_quantities.cpp # E, L_z, Q constant along geodesic
│   └── benches/
│       └── geodesic_bench.cpp            # Google Benchmark — CPU baseline
│
├── engine/                               # Vulkan renderer
│   ├── CMakeLists.txt
│   ├── src/
│   │   ├── main.cpp
│   │   ├── app.hpp / app.cpp
│   │   ├── camera.hpp / camera.cpp       # observer → per-pixel impact params
│   │   ├── vk/
│   │   │   ├── context.hpp / context.cpp  # vk-bootstrap, volk init
│   │   │   ├── allocator.hpp              # VMA wrapper
│   │   │   ├── pipeline.hpp               # compute pipeline, dynamic rendering
│   │   │   └── descriptors.hpp            # bindless descriptor set
│   │   ├── shaders/
│   │   │   ├── geodesic.comp.glsl         # PORT of physics/src/geodesics.cpp
│   │   │   ├── disk_shading.comp.glsl     # redshift/beaming → color
│   │   │   └── common.glsl                # shared metric math, mirrors metric.cpp
│   │   ├── ui.hpp / ui.cpp               # ImGui overlay
│   │   └── io/
│   │       ├── starfield.hpp              # HDR env map loader
│   │       └── export.hpp                 # frame → PNG/EXR
│   └── assets/
│       ├── starfield_8k.hdr
│       └── disk_colormap.png
│
├── validation/
│   ├── compare_shadow_size.cpp           # CPU vs GPU pixel diff
│   └── reference_images/
│       └── sgrA_eht_2022.png
│
├── scripts/
│   ├── build.sh
│   ├── run_tests.sh
│   └── render_sgrA_sweep.sh
│
└── .github/workflows/
    └── cpp-ci.yml
```

---

## 8. What NOT to do (common failure modes from the old project)

1. **Do not start with the GPU.** The Vulkan engine is Phase 5. Phases 0–4 are pure CPU.
   The most common mistake is writing the shader before validating the equations on CPU.

2. **Do not copy the old shader's geodesic math.** The formula `-1.5 * rs * h2 * pos / r^5` is
   wrong for Kerr. The Lense-Thirring frame drag bolted onto it is wrong for close-in geodesics.

3. **Do not use FetchContent.** Use vcpkg manifest mode. FetchContent causes SSL/version chaos
   on Windows (as demonstrated in the old CMakeLists.txt with `CMAKE_TLS_VERIFY OFF`).

4. **Do not use OpenGL.** Even as "just a test." The whole point of the rewrite is Vulkan.
   If you need a quick visual check on CPU output, write it to a PNG with stb_image_write.

5. **Do not bind a new descriptor set every frame.** Use bindless from day one. Retrofitting
   bindless onto per-draw descriptor architecture is painful.

6. **Do not leave Vulkan validation layers OFF.** If they are off in Debug builds, turn them on
   immediately. 90% of "why is this black/crashing" answers come from validation output.

7. **Do not proceed to Phase N+1 if Phase N's gate tests are failing.** The phase gates exist
   because errors in geodesic equations are invisible visually until Phase 3 or later, but
   their root cause is in Phase 0. Find bugs early, in unit tests, not in rendered images.

8. **Do not hardcode spin.** a* is a slider from day one. Architecture that assumes a*=0 will
   require full rewrites when spin is introduced.

9. **Do not skip the CPU reference renderer.** `raytrace_cpu.cpp` is the ground truth that
   `compare_shadow_size.cpp` diffs against. Without it, you have no way to know if your GPU
   output is correct or just accidentally pretty.

---

## 9. Geodesic integrator implementation notes

Use **RK45 (Dormand-Prince adaptive)** as the primary integrator (`scipy.integrate.solve_ivp`
style, but in C++). RK4 is acceptable for prototyping in Phase 0 but switch to adaptive before
Phase 1. Near the photon sphere, the equations become stiff and fixed-step RK4 will either miss
turning points or require impractically small steps.

Handle turning points explicitly:
- When `R(r) < 0` or `Θ(θ) < 0`, you have overstepped a turning point. Bisect to find it, then
  flip the ± sign. Do not allow `sqrt` of a negative number to NaN the integrator.

Boyer-Lindquist has coordinate singularities at θ=0 and θ=π (the spin axis). These are
coordinate artifacts, not physical singularities. Handle them by adding a small regularization
epsilon (e.g., clamp θ away from 0 and π by 1e-8) rather than trying to remove them analytically.

Terminate ray integration when:
- `r < r₊ + ε` → photon captured, render black
- `r > r_escape` (e.g., 10⁴ M) AND `dr/dλ > 0` → photon escaped, sample starfield
- `θ = π/2` within disk radial bounds → photon hit disk, compute emission

---

## 10. Accretion disk — Phase 4 physics

For Phase 4, use the **Novikov-Thorne thin disk model** (not volumetric noise). Geometrically
thin, optically thick disk in the equatorial plane (θ=π/2) from r_ISCO to r_outer.

Per disk intersection:
1. Compute redshift factor: `g = ν_obs/ν_emit = 1 / (u^t(1 + L_z Ω / E))`
   where Ω = dφ/dt = orbital angular velocity of the disk matter.
2. Disk specific flux: Novikov-Thorne F(r) profile (analytically known, tabulate it)
3. Observed intensity: `I_obs = g⁴ · I_emit(T_eff(r))`
4. Map T_eff to blackbody color via pre-computed table in `assets/disk_colormap.png`

The **Doppler beaming asymmetry** — approaching side brighter/bluer — must be visible and
physically correct before Phase 5 begins. If the disk looks symmetric, g⁴ beaming is not applied.

**Do NOT use procedural FBM noise for the disk.** The old project used noise for a cinematic
look. This project uses analytic physics. Noise can be added as an optional visual layer later,
but the physics-based intensity profile must be the base.

---

## 11. Reading list (refer to these, do not guess)

1. **Bardeen, Press & Teukolsky (1972)** — Carter-separated Kerr geodesics. THE reference for §3.
2. **James, von Tunzelmann, Franklin & Thorne (2015)** — "Gravitational Lensing by Spinning Black
   Holes in Astrophysics, and in the Movie Interstellar." The single most practically useful paper
   for the rendering pipeline.
3. **Luminet (1979)** — First computed black hole accretion disk image. Good for Phase 1 validation.
4. **EHT Collaboration, Sgr A* Papers I–VIII (2022–2024)** — Observational targets for Phase 8.
5. **EinsteinPy source** — Runnable Python Kerr geodesics for cross-checking your own integrator.

---

## 12. Self-check before calling anything "done"

Physics library:
- [ ] At a*=0, all Kerr formulas reduce to Schwarzschild (r₊=2M, r_ph=3M, r_ISCO=6M)
- [ ] Prograde photon sphere ≠ retrograde for a*>0
- [ ] ISCO moves toward r₊ as a*→1 prograde
- [ ] Ergosphere touches horizon at poles, bulges at equator
- [ ] E, L_z, Q constant along each integrated geodesic to within 1e-6 relative

CPU render:
- [ ] Schwarzschild shadow is a perfect circle
- [ ] Kerr shadow is lopsided (flattened on prograde side)
- [ ] Disk is asymmetrically bright (approaching side louder)
- [ ] Photon ring visible as second lensed image of disk

GPU render:
- [ ] `compare_shadow_size.cpp` pixel diff CPU vs GPU < threshold
- [ ] No Vulkan validation layer errors in Debug build
- [ ] Sgr A* photon ring diameter 48–52 μas at correct distance
