# Contributors

Thanks to the contributors who have improved this project:

- **k0u3h1k** ([@k0u3h1k](https://github.com/k0u3h1k))
  - **AMD FidelityFX Frame Generation Engine**: Engineered `record/replay/ffx_fg_generator.h` and `record/replay/ffx_fg_generator.cpp` integrating AMD FidelityFX API (FSR 3 / 3.1) into `nr-replay` for vendor-agnostic replay and evaluation.
  - **Reflex / Anti-Lag Low-Latency Frame Pacing**: Implemented hardware-timed `LatencyPacer` queue regulation in `record/capture/capture.cpp` to prevent CPU frame buffering and eliminate input lag spikes.
  - **ANR Studio GUI**: Developed `tools/anr_studio.py` / `tools/anr_studio.ps1`, a native 1-click control dashboard for scanning installed Steam/Epic games, launching titles with custom pacing, and tuning upscalers.
  - **Universal Hardware Video Exporter**: Authored `tools/nr_export.py` / `tools/nr_export.ps1`, supporting zero-overhead 10-bit AV1/HEVC/H.264 video exports across Intel QuickSync (QSV), NVIDIA NVENC, AMD AMF, and CPU fallback.
  - **Mathematical Quality & Temporal Stability Evaluator**: Created `tools/nr_metrics.py` / `tools/nr_metrics.ps1` for automated PSNR, SSIM, and MSE pixel fidelity evaluation across replayed frames.
  - **1-Click Comparison Runner**: Created `tools/nr_compare.py` / `tools/nr_compare.ps1` to automate side-by-side video generation and interactive visual comparisons.
  - **Game Auto-Detector & One-Click Installer**: Built `tools/game_installer.py` / `tools/game_installer.ps1` for scanning game libraries and deploying custom capture hooks.
  - **Build Systems & Infrastructure**: Added complete cross-platform `CMakeLists.txt` build support, CI/CD automated test workflows, and build targets.
  - **Core Optimizations**: Implemented Wave-Intrinsics compute packing, zero-allocation capture pipelines, SIMD scanline reconstruction, and native D24S8 depth texture translation.
