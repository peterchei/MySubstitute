# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this project is

MySubstitute is a Windows-only C++17 application that captures a physical webcam, runs the video through a pluggable AI/image-processing pipeline, and re-publishes the processed stream as a DirectShow virtual camera device (`MySubstitute Virtual Camera`) consumable by Chrome, OBS, Zoom (desktop), Teams (desktop), etc.

The executable also hosts a system tray UI and a mobile-shaped live preview window. There is no service; everything runs in the user-mode `MySubstitute.exe` process plus a separately-registered COM DLL.

## Build, register, run

All scripts must be run from the repo root in a shell that has the VS 2022 C++ toolchain on PATH (`build.bat` will auto-call `VsDevCmd.bat` if not).

```cmd
setup.bat                 :: sanity-check VS + CMake + (optional) OpenCV
build.bat                 :: configures CMake (Visual Studio 17 2022, x64, -DUSE_ONNX=ON), builds Debug
rebuild.bat               :: cmake --build build --config Release (with retry-on-locked-DLL loop)
run.bat                   :: launches Debug (preferred) or Release exe
run_debug.bat             :: launches with --debug to attach an AllocConsole for std::cout/cerr
run_as_admin.bat          :: elevates run.bat — needed for virtual camera registration
```

Default build is **Debug** with `_d` postfix: outputs land in `build\bin\Debug\MySubstitute_d.exe` and `MySubstituteVirtualCamera_d.dll`. Release artifacts are in `build\bin\Release\`. The DLL is copied into the exe directory via `scripts\copy_dll_safe.ps1`, which handles the case where the DLL is locked because it is currently COM-registered.

### Virtual camera registration

The COM filter must be registered before any host app sees the device. Either:
- Run the exe with `--register` (preferred — also runs `DirectShowVirtualCameraManager` fallback chain), OR
- `regsvr32 MySubstituteVirtualCamera.dll` from the build output (requires elevation)

`--test-virtual-camera` runs registration in a console-only diagnostic mode.

### Command-line flags

`WinMain` parses `lpCmdLine` directly (no real CLI parser):
- `--debug` / `-d` — allocate a console for log output
- `--register` — registration flow described above
- `--test-virtual-camera` — diagnostic-only registration test

### Tests

Three standalone test executables are wired into the top-level `CMakeLists.txt` and build alongside the main target:
- `test_face_filter` (`scripts/test_face_filter.cpp`)
- `test_filter_callback` (`scripts/test_filter_callback.cpp`)
- `test_anime_gpu` (`scripts/test_anime_gpu.cpp`)

There is no test framework — they are interactive console programs. Run individually from `build\bin\<Config>\`. There is no `ctest` integration and no single-test selector.

## Architecture

```
Physical Camera ──► CameraCapture (DirectShow + OpenCV)
                         │ Frame callback (BGR Mat)
                         ▼
                    AIProcessor (mutex-guarded; one active at a time)
                         │ Processed Frame
                         ├─► PreviewWindowManager (in-process GDI render)
                         └─► VirtualCameraManager
                                 │ writes RGB24 640×480
                                 ▼
                         Shared memory: "MySubstituteVirtualCameraFrames" (921,600 bytes)
                                 ▲
                                 │ reads
                         MySubstituteVirtualCameraDLL (separate process, loaded by host app)
                                 │ IBaseFilter / IAMStreamConfig / IKsPropertySet
                                 ▼
                         Host application (Chrome, OBS, Zoom, …)
```

Key cross-process invariant: the main exe and the DirectShow DLL communicate **only** through the named shared-memory buffer. The DLL has no link-time dependency on the rest of the codebase; its sources (`virtual_camera_directshow.cpp`, `directshow_dll_main.cpp`, `frame.cpp`) are listed independently in `CMakeLists.txt` and it is compiled with `/MT` (static runtime) to avoid CRT version issues when loaded into arbitrary host processes.

### Source tree

- `src/main.cpp` — `WinMain` entry, globals (`g_camera`, `g_processor`, `g_virtualCamera*`, `g_previewManager`, `g_trayManager`), and the giant `OnFilterChanged(filterName)` dispatcher that is the single source of truth for filter selection. The dispatcher also handles parameterized commands like `speech_text:<text>`, `mask_offset_x:<float>`, `gpu_acceleration:on|off`, `segmentation_method:motion|onnx|opencv_dnn`.
- `src/capture/` — `CameraCapture` (DirectShow enum + OpenCV `VideoCapture`), `Frame` (BGR `cv::Mat` wrapper).
- `src/ai/` — Built as a separate static library `AIProcessor` (see `src/ai/CMakeLists.txt`). All processors derive from `AIProcessor` (`ai_processor.h`) and follow `Initialize → ProcessFrame → Cleanup` lifecycle. Current processors: `PassthroughProcessor`, `FaceFilterProcessor`, `VirtualBackgroundProcessor`, `CartoonFilterProcessor`, `CartoonBufferedFilterProcessor`, `PixelArtProcessor`, `AnimeGANProcessor` (loads `.t7` Torch / `.onnx` ESPNet-style style-transfer models), `PersonTrackerProcessor`, `PersonReplacementProcessor` (face-swap / full-body-replace / face-enhance / super-res).
- `src/virtual_camera/` — Several historical implementation attempts coexist. The **live** path is `virtual_camera_directshow.{h,cpp}` + `class_factory.cpp` + `directshow_dll_main.cpp` (DLL side) and `virtual_camera_manager.cpp` + `directshow_virtual_camera_manager.cpp` (exe side). Older files (`simple_registry_virtual_camera`, `media_foundation_camera`, `working_directshow_filter`, `real_virtual_camera_registration`, `simple_virtual_camera`, `directshow_filter_dll`) are fallback / dead code still referenced by the CMake source list — do not delete without checking the link graph.
- `src/ui/` — `SystemTrayManager` (Shell_NotifyIcon + context menu, callbacks set in `WinMain`), `PreviewWindowManager` (270×480 GDI window, owns the filter-selection combo box and exposes `OnFilterChanged` callback).
- `src/service/background_service.{h,cpp}` — Skeleton, not currently active in `WinMain`.

### Where the virtual-camera CLSID lives

`{B3F3A1C4-8F9E-4A2D-9B5C-7E6F8D4C9A3B}` — defined once in `virtual_camera_directshow.h` as `CLSID_MySubstituteVirtualCamera`. Friendly name is `"MySubstitute Virtual Camera"`. Shared memory name is `"MySubstituteVirtualCameraFrames"`. Frame format is hard-coded RGB24 at 640×480 (`SHARED_BUFFER_SIZE = 640 * 480 * 3`).

## Conventions worth knowing

- **Filter switching is mutex-protected** by `g_processorMutex` in `main.cpp`. Always lock it when replacing `g_processor`, otherwise the capture thread can call `ProcessFrame` on a half-destroyed processor.
- **Adding a new filter**: implement `AIProcessor` in `src/ai/`, add it to `AI_SOURCES`/`AI_HEADERS` in `src/ai/CMakeLists.txt`, then add a new `else if (filterName == "your_id")` branch in `OnFilterChanged` and a matching combo-box entry in `PreviewWindowManager`. Always fall back to `PassthroughProcessor` on init failure (existing branches all do this).
- **OpenCV is optional** at compile time but everything practical requires it. The `HAVE_OPENCV` preprocessor flag gates `cv::Mat` use; `#if HAVE_OPENCV` blocks appear throughout. Don't break the `HAVE_OPENCV=0` build path.
- **ONNX is gated by `-DUSE_ONNX=ON`** (defaulted on in `build.bat`). `HAVE_ONNX` definition controls whether the ONNX-based segmentation in `VirtualBackgroundProcessor` and AI inference in `PersonReplacementProcessor` are compiled in.
- **OpenCV DLL discovery** in `CMakeLists.txt` is hard-coded to `D:/DevTools/opencv/build/x64/vc16/bin` for the post-build DLL copy step. ONNX paths similarly hard-coded to `D:/DevTools/onnxruntime-win-x64-...`. Adjust per machine.
- **Models** live in `models/` (gitignored). Filenames referenced from code: `models/{candy,mosaic,starry_night,la_muse,feathers}.t7` (style transfer), `models/MediaPipe-Selfie-Segmentation.onnx` / `selfie_segmentation.tflite` (segmentation), `models/simswap.onnx` / `crossface_simswap.onnx` (face swap), `models/GFPGANv1.3.pth` / `codeformer.onnx` (face enhance), `models/RealESRGAN_x2plus.pth` (super-res). Missing models cause processor init failure → fallback to passthrough.
- **Assets** referenced by hard-coded paths relative to CWD: `assets/background.jpg`, `assets/default_face.jpg`, `assets/default_person.jpg`. The exe expects CWD = repo root or `build\bin\<Config>\` depending on how it was launched.
- **COM**: every entry point (`WinMain`, test-mode branches) brackets work with `CoInitialize` / `CoUninitialize`. Don't skip this in new entry points.
- **Logging**: `std::cout` / `std::cerr` are only visible when launched with `--debug` (`AllocConsole`). The codebase liberally uses `[ComponentName] message` log lines, often with emoji prefixes — keep that style when adding logs in main flows.

## Reference docs

`docs/` contains markdown for setup, individual filters, and historical fix notes. Most useful: `docs/QUICKSTART.md`, `docs/development_setup.md`, `docs/FILTER_ARCHITECTURE.md`, `docs/VIRTUAL_BACKGROUND_SETUP.md`, `docs/UWP_COMPATIBILITY.md` (UWP/Store apps need Frame Server — not supported).
