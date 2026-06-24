# Phase Gate Specifications — kerr-blackhole-sim

> This document defines the exact pass/fail criteria for each phase gate.
> The coding agent must run all relevant tests and confirm they pass BEFORE
> starting the next phase. No exceptions.

---

## How to use this document

At the end of each phase, the agent should:
1. Run the relevant commands listed in the gate block
2. Paste or confirm the output
3. Only then begin the next phase

If any gate fails, diagnose in the current phase. Do not paper over failures
with tweaked constants or visual approximations.

---

## Gate 0 — Schwarzschild math baseline

**Triggered after:** Phase 0 (metric.cpp + geodesics.cpp complete)

### What to run
```bash
cmake --preset debug && cmake --build --preset debug
ctest --preset debug --output-on-failure
```

### Tests that must pass

**`test_schwarzschild_limit`**
For a* = 0.0, M = 1.0 (geometric units), verify numerically:
```
r_plus      == 2.0      ± 1e-10
r_ph_pro    == 3.0      ± 1e-10
r_ph_retro  == 3.0      ± 1e-10   (must equal prograde at a*=0)
r_ISCO_pro  == 6.0      ± 1e-10
r_ISCO_retro== 6.0      ± 1e-10
```

**`test_key_radii`**
For a* = 0.5, M = 1.0, verify against known analytic values:
```
r_plus      == 1 + sqrt(0.75)        ≈ 1.8660254...  ± 1e-8
r_ph_pro    <  3.0   (prograde photon sphere shrinks inward as spin increases)
r_ph_retro  >  3.0   (retrograde photon sphere moves outward)
r_ph_pro    !=  r_ph_retro           (MUST differ for a*>0)
r_ISCO_pro  <  6.0
r_ISCO_retro > 6.0
```
Repeat for a* = 0.94 (Sgr A* best-bet).

**`test_conserved_quantities`**
Integrate a null geodesic (μ=0) for 100 steps from r=20M, θ=π/3, with
E=1, L_z=2, Q=3 (arbitrary but fixed). Check after each step:
```
|E(λ_i)   − E(0)|   / |E(0)|   < 1e-6
|L_z(λ_i) − L_z(0)| / |L_z(0)| < 1e-6
|Q(λ_i)   − Q(0)|   / |Q(0)|   < 1e-6
```
If drift > 1e-6: step size is too large, or equations have a bug.
Acceptable integrators: RK4 with step ≤ 0.01M, or any adaptive (RK45/DOP853).

### Gate verdict
ALL three test suites must show 0 failures. If any fail, do NOT begin Phase 1.

---

## Gate 1 — CPU Schwarzschild reference image

**Triggered after:** Phase 1 (raytrace_cpu.cpp complete)

### What to run
```bash
./build/debug/kerr-physics-validate --mode schwarzschild --output validation/reference_images/schwarzschild_ref.png
```
(Or equivalent invocation for your CPU raytrace binary.)

### Checks

1. **Shadow is a circle.** Open the PNG. The shadow boundary must be circular, not elliptical.
   If it is not circular, the observer inclination is wrong or the metric is not Schwarzschild.

2. **Shadow angular radius matches analytic formula.**
   For an observer at r=1000M, the shadow angular radius is:
   `θ_shadow ≈ 3√3 M / r_obs = 3√3 / 1000 rad ≈ 5.196 × 10⁻³ rad`
   Measure the pixel radius of the shadow in the output image and verify it matches
   this value within 2% (accounting for FOV and image resolution).

3. **Disk is present between r_ISCO=6M and r_outer.**
   If disk is enabled, it should appear as a ring with Doppler brightness asymmetry
   (approaching side brighter) even in Schwarzschild, because the disk matter orbits.

4. **Store the output.** Save the reference PNG to `validation/reference_images/schwarzschild_ref.png`.
   This is the CPU ground truth for GPU diff-testing in Phase 6.

### Gate verdict
Shadow is circular, angular size within 2% of formula, image saved. Only then begin Phase 2.

---

## Gate 2 — Kerr geodesics correctness

**Triggered after:** Phase 2 (geodesics.cpp extended to full Kerr)

### What to run
Re-run the full test suite:
```bash
ctest --preset debug --output-on-failure
```

All three tests from Gate 0 must still pass. Additionally verify manually (or with a dedicated
test) that Carter's constant Q is not just a fixed input but is actually conserved along
trajectories that would precess in θ (i.e., use initial conditions with non-zero Q).

### Kerr-specific check
Integrate a Kerr geodesic with a*=0.9, E=1, L_z=0.5, Q=2, μ=0 from r=20M, θ=π/2+0.1.
The photon should oscillate in θ (because Q > 0 allows off-equatorial motion). Verify:
- θ oscillates (does not monotonically decrease)
- Turning points in θ are found when Θ(θ)=0 and ± sign correctly flips
- Q conserved to 1e-6 throughout

### Gate verdict
All Gate 0 tests still green. Kerr θ-oscillation observed. Q conserved. Proceed to Phase 3.

---

## Gate 3 — Kerr CPU ray tracer (lopsided shadow)

**Triggered after:** Phase 3 (raytrace_cpu.cpp extended to Kerr)

### What to run
```bash
./build/debug/kerr-physics-validate \
  --mode kerr --spin 0.94 --inclination 30 \
  --output validation/reference_images/kerr_a094_i30.png
```

### Checks

1. **Shadow is NOT a perfect circle.** The Kerr shadow must be:
   - Flattened on the prograde (co-rotating) side
   - Slightly extended on the retrograde side
   - Visually similar to EHT's published Sgr A* shadow shape

   If the shadow is a perfect circle for a*=0.94, the spin terms are broken. Do not proceed.

2. **Compare against a*=0 output.** The Kerr shadow at a*=0.94 must differ measurably from
   the Schwarzschild shadow. If they look identical, spin is not affecting the photon paths.

3. **Disk beaming.** The approaching (co-rotating) side of the accretion disk must be visibly
   brighter than the receding side. If brightness is symmetric, Doppler beaming is not applied.

4. **Save the reference.** `validation/reference_images/kerr_a094_i30.png` becomes the
   Kerr CPU ground truth for GPU diff-testing.

### Gate verdict
Lopsided shadow confirmed, disk beaming asymmetric, reference image saved. Proceed to Phase 4.

---

## Gate 4 — Disk emission physics

**Triggered after:** Phase 4 (disk_shading integrated into CPU ray tracer)

### Checks (visual + quantitative)

1. **ISCO inner edge.** The disk's inner edge must be at r_ISCO(a*), not at a hardcoded r=3M
   or similar. Verify: for a*=0, inner edge at 6M; for a*=0.94, inner edge is much closer to r₊.

2. **Temperature profile.** The disk must be hotter (bluer/whiter) near the inner edge and
   cooler (redder) at the outer edge. If color is uniform, the profile is not applied.

3. **Photon ring.** The second image of the disk (the n=1 photon ring) should be visible as
   a thin bright arc just outside the shadow. This is the lensed image of the far side of
   the disk passing over the top of the black hole. If absent, the ray tracer is terminating
   rays too early.

4. **Beaming ratio.** Measure the brightness of the approaching versus receding disk limb.
   For a*=0 (Schwarzschild), the ratio from pure Keplerian Doppler beaming at r=6M is:
   `I_approaching / I_receding ≈ [(1+β)/(1-β)]⁴` where β = orbital speed at ISCO.
   For Schwarzschild ISCO: β ≈ 0.5c, so ratio ≈ [(1.5)/(0.5)]⁴ = 81.
   Your result should be in the right ballpark (order of magnitude correct).

### Gate verdict
ISCO position correct, temperature gradient visible, photon ring visible, beaming ratio
order-of-magnitude correct. Proceed to Phase 5.

---

## Gate 5 — Vulkan engine stand-up

**Triggered after:** Phase 5 (Vulkan plumbing, no physics)

### What to run
```bash
cmake --preset debug && cmake --build --preset debug --target kerr-engine
VK_INSTANCE_LAYERS=VK_LAYER_KHRONOS_validation ./build/debug/kerr-engine
```

### Checks

1. **Application launches without crash.** Window appears, solid color renders.

2. **Zero validation layer errors.** The terminal output must contain no lines beginning with
   `[VALIDATION]` or similar. If any appear, fix them before adding any physics.

3. **Dynamic rendering path confirmed.** Verify with RenderDoc that the compute dispatch
   writes directly to a storage image and that image is presented without a render pass.
   There should be no `vkCreateFramebuffer` or `vkCreateRenderPass` calls in the trace.

4. **Bindless descriptor set active.** In RenderDoc's resource inspector, verify one global
   descriptor set is bound (not rebound per draw/dispatch).

### Gate verdict
Zero validation errors, dynamic rendering confirmed, bindless confirmed. Proceed to Phase 6.

---

## Gate 6 — GPU geodesic correctness (CPU vs GPU diff)

**Triggered after:** Phase 6 (geodesic.comp.glsl ported from CPU)

### What to run
```bash
./build/debug/compare_shadow_size \
  --cpu-reference validation/reference_images/schwarzschild_ref.png \
  --gpu-output <current-frame-capture> \
  --resolution 64x64
```

### Checks

1. **Per-pixel L2 error < 0.01 (1% of full range) for Schwarzschild (a*=0).**
   The GPU and CPU must produce nearly identical images at low resolution where numerical
   integration differences are small.

2. **Shadow boundary pixel error < 2 pixels at 64×64 resolution.**

3. **Repeat for a*=0.94.** Compare against `kerr_a094_i30.png` from Gate 3.

4. **Zero validation errors throughout the GPU capture.**

### Gate verdict
L2 error < 0.01, shadow boundary within 2px, no validation errors. Proceed to Phase 7.

---

## Gate 8 — Sgr A* calibration (final)

**Triggered after:** Phase 8 (full Sgr A* render)

### What to run
```bash
./build/release/kerr-engine \
  --mass 4.297e6 \          # solar masses
  --distance 8277 \         # parsecs
  --spin 0.94 \
  --inclination 30          # degrees from spin axis
```
Export a frame and measure the photon ring diameter in pixels.

### Calculation
```
pixel_diameter_of_ring → angular_diameter_μas =
    pixel_diameter / image_resolution_pixels × camera_FOV_arcsec × 1e6
```

### Acceptance criterion
```
angular_diameter_μas ∈ [41.7, 55.7]   (48.7 ± 7 μas, EHT 2022 shadow)
angular_diameter_μas ∈ [49.5, 54.1]   (51.8 ± 2.3 μas, EHT photon ring, tighter)
```

Your result should fall within the shadow range, and ideally within the photon ring range.

If the diameter is off by more than 2×, check:
- Are you using correct Sgr A* mass (4.297×10⁶ M☉, NOT 4.3×10⁶)?
- Is the distance 8,277 pc (NOT 8,500)?
- Is the FOV calculation using radians consistently?
- Did you forget to project from geometric units (M) to physical meters before converting to angle?

---

## Summary table

| Gate | Phase completed | Key test | Accept if... |
|---|---|---|---|
| 0 | Metric + geodesics | Catch2 suite | 0 failures, E/L_z/Q drift < 1e-6 |
| 1 | CPU Schwarzschild tracer | Shadow circle | Circular, size within 2% |
| 2 | Kerr geodesics | Catch2 + manual | θ-oscillation, Q conserved |
| 3 | Kerr CPU tracer | Shadow lopsided | Not circular for a*>0 |
| 4 | Disk emission | Visual + beaming ratio | ISCO correct, photon ring visible |
| 5 | Vulkan stand-up | RenderDoc + VL | 0 VL errors, dynamic rendering |
| 6 | GPU geodesic port | CPU vs GPU diff | L2 < 0.01 at 64×64 |
| 8 | Sgr A* calibration | Angular size | 41.7–55.7 μas |
