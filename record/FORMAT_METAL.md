# Apple Silicon & Metal Format Specification

This document outlines the architecture, buffer formats, and integration pathways for running **Apex Neural Rendering (ANR)** frame generation and replay natively on **Apple Silicon (M1/M2/M3/M4 series)** using the **Metal** graphics and compute API.

---

## 1. Overview & Architecture

ANR Frame Generation on macOS / Apple Silicon targets:
1. **Native Metal Backend**: Compute pipeline driving `MTLComputePipelineState` using SIMDgroup matrix operations (`simdgroup_matrix`) and FP16 math on Apple Silicon GPU cores.
2. **CrossOver / Wine Compatibility**: DXVK-macOS and MoltenVK translation layers bridging DirectX 12 / Vulkan calls to Metal.

---

## 2. Resource Formats (Metal Equivalent)

| Role | DXGI Format | Vulkan Format | Metal Format (`MTLPixelFormat`) | Description |
|---|---|---|---|---|
| **Backbuffer (SDR)** | `R8G8B8A8_UNORM` | `VK_FORMAT_R8G8B8A8_UNORM` | `MTLPixelFormatRGBA8Unorm` | Display color buffer (sRGB) |
| **Backbuffer (HDR)** | `R10G10B10A2_UNORM` | `VK_FORMAT_A2B10G10R10_UNORM_PACK32` | `MTLPixelFormatRGB10A2Unorm` | HDR10 / PQ10 wide-gamut |
| **Backbuffer (scRGB)**| `R16G16B16A16_FLOAT` | `VK_FORMAT_R16G16B16A16_SFLOAT` | `MTLPixelFormatRGBA16Float` | 16-bit scene-linear HDR |
| **Depth** | `R32_FLOAT` / `D32_FLOAT` | `VK_FORMAT_D32_SFLOAT` | `MTLPixelFormatDepth32Float` | Reversed floating-point depth |
| **Motion Vectors** | `R16G16_FLOAT` | `VK_FORMAT_R16G16_SFLOAT` | `MTLPixelFormatRG16Float` | 2D screen-space pixel velocity |
| **HUD-less** | `R8G8B8A8_UNORM` | `VK_FORMAT_R8G8B8A8_UNORM` | `MTLPixelFormatRGBA8Unorm` | Clean scene render without UI |

---

## 3. Shader Compilation: Slang to Metal Shading Language (MSL)

ANR shaders written in Slang (`record/record_pack.slang`) cross-compile to Metal Shading Language using Slang's `-target metal` or via SPIR-V to MSL using SPIRV-Cross / `metal-shader-converter`:

```bash
# Compile Slang compute kernel to Metal Shading Language (MSL 3.0+)
slangc record/record_pack.slang -target metal -entry record_pack -profile sm_6_6 -o record_pack.metal

# Compile MSL to Metal library
xcrun -sdk macosx metal -c record_pack.metal -o record_pack.air
xcrun -sdk macosx metallib record_pack.air -o record_pack.metallib
```

---

## 4. Hardware Matrix Acceleration on Apple Silicon

Apple M-series GPUs leverage SIMDgroup matrix multiply-accumulate intrinsics (`simdgroup_matrix<half, 8, 8>`):
- **Apple M1 / M2 / M3 / M4**: Hardware support for 8x8 half-precision SIMDgroup execution.
- **Threadgroup Memory**: Unified Memory Architecture (UMA) enables zero-copy texture sharing between CPU host code and GPU compute encoders.

---

## 5. Building & Running Test Kits on macOS

To build and execute ANR on macOS:
```bash
# Build native macOS target with Clang
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
cmake --build . --config Release

# Run kit benchmark on Apple Silicon
./bin/nr-record --benchmark
```
