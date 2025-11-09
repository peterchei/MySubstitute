# PersonReplacementProcessor Quick Start Guide

## 5-Minute Setup

Get DeepSeek-style face swapping and AI enhancements working in 5 minutes!

---

## Prerequisites

✅ **Already Installed (from your working setup):**
- Visual Studio 2022 with C++ Desktop Development
- CMake 3.16+
- OpenCV 4.12.0 (D:\DevTools\opencv)
- ONNX Runtime 1.16.3 (D:\DevTools\onnxruntime-win-x64-1.16.3)

---

## Step 1: Verify Installation ✅

Your PersonReplacementProcessor is already integrated! Just verify:

```powershell
# Check if build succeeded
cd C:\Users\peter\git\MySubstitute
.\build.bat

# You should see:
# ✅ Found ONNX Runtime
# Build completed successfully!
```

---

## Step 2: Download AI Models (Optional but Recommended)

### Option A: Quick Start (No AI Models)

The processor works out-of-the-box with OpenCV fallbacks:
- ✅ Face detection via Haar Cascades
- ✅ Basic face blending
- ✅ Bilateral filter enhancement
- ✅ Bicubic upscaling

**Just run the app:**
```powershell
.\run_debug.bat
```

### Option B: Full AI Power (Download Models)

Download pre-converted ONNX models for best results:

#### Face Swap Model (SimSwap)
```powershell
# Download from Hugging Face (example - adjust URL)
$url = "https://huggingface.co/spaces/examples/SimSwap/resolve/main/simswap.onnx"
Invoke-WebRequest -Uri $url -OutFile "models\simswap.onnx"
```

#### Face Enhancement (GFPGAN)
```powershell
# Download GFPGAN v1.3
$url = "https://github.com/TencentARC/GFPGAN/releases/download/v1.3.0/GFPGANv1.3.pth"
Invoke-WebRequest -Uri $url -OutFile "models\GFPGANv1.3.pth"

# Then convert to ONNX (requires Python + torch)
python scripts/convert_gfpgan_to_onnx.py
```

#### Super Resolution (Real-ESRGAN)
```powershell
# Download Real-ESRGAN x2
$url = "https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.1/RealESRGAN_x2plus.pth"
Invoke-WebRequest -Uri $url -OutFile "models\RealESRGAN_x2plus.pth"
```

**Note:** Most models are in PyTorch format (.pth) and need conversion to ONNX. See full documentation for conversion scripts.

---

## Step 3: Run the Application

```powershell
# Launch with debug console to see AI processing info
.\run_debug.bat
```

**What you'll see:**
```
Initializing PersonReplacementProcessor...
Loaded Haar Cascade from: D:/DevTools/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml
ONNX Runtime initialized successfully
PersonReplacementProcessor initialized successfully!
```

---

## Step 4: Use Person Replacement Features

### In the Preview Window:

1. **Right-click system tray icon** → **Show Preview**

2. **Select a filter from dropdown:**

   | Filter Option | What It Does | Model Required |
   |--------------|--------------|----------------|
   | **AI Face Swap (DeepSeek)** | Replace your face with target person | Optional |
   | **AI Full Body Replace** | Replace entire person | Optional |
   | **AI Face Enhancement (GFPGAN)** | Improve face quality | Optional |
   | **AI Super Resolution** | Upscale video 2x | Optional |

3. **See the effect in real-time!**

### Without AI Models (OpenCV Fallback):

- **Face Swap:** Uses simple weighted blending between detected faces
- **Face Enhancement:** Bilateral filter + sharpening
- **Super Resolution:** Bicubic interpolation (2x)
- **Full Body:** Simple center-region mask

### With AI Models (Full Quality):

- **Face Swap:** DeepFake-quality face replacement with landmark alignment
- **Face Enhancement:** GFPGAN-level restoration and detail enhancement
- **Super Resolution:** Real-ESRGAN AI upscaling
- **Full Body:** MediaPipe accurate person segmentation

---

## Step 5: Test Face Swap

### Prepare Target Image:

```powershell
# Create assets folder if not exists
mkdir -p assets

# Place a target face image
# Copy any image with a clear frontal face to:
# assets/target_face.jpg
```

### Configure in Code (Optional):

Edit `src/main.cpp` to set default target:

```cpp
} else if (filterName == "person_replace_face_swap") {
    auto processor = std::make_unique<PersonReplacementProcessor>();
    processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
    processor->SetTargetPersonImage("assets/target_face.jpg");  // Your target
    processor->SetBlendStrength(0.9f);
    
    // Load model if available
    if (!processor->LoadFaceSwapModel("models/simswap.onnx").empty()) {
        std::cout << "Using AI face swap model" << std::endl;
    } else {
        std::cout << "Using OpenCV fallback face swap" << std::endl;
    }
    
    g_processor = std::move(processor);
    g_processor->Initialize();
}
```

### Rebuild & Test:

```powershell
.\build.bat
.\run_debug.bat

# Select "AI Face Swap (DeepSeek)" from dropdown
# You should see your face replaced with the target!
```

---

## Quick Test Checklist

- [ ] Build succeeds without errors
- [ ] Application launches with preview window
- [ ] "AI Face Swap (DeepSeek)" appears in dropdown
- [ ] Selecting filter doesn't crash
- [ ] Console shows "PersonReplacementProcessor initialized successfully!"
- [ ] Face detection works (bounding boxes or face swap visible)

---

## Performance Expectations

### Without AI Models (OpenCV Fallback):
- **FPS:** 30-60 (very fast)
- **Quality:** Basic/Good
- **Memory:** ~200 MB

### With AI Models (CPU):
- **FPS:** 3-8 (slow but usable)
- **Quality:** Excellent
- **Memory:** 2-3 GB

### With AI Models (GPU - CUDA):
- **FPS:** 20-30 (smooth)
- **Quality:** Excellent
- **Memory:** 2-3 GB GPU RAM

---

## Troubleshooting Quick Fixes

### Issue: "Failed to load face detection cascade"

**Quick Fix:**
```powershell
# Verify Haar Cascade exists
Test-Path "D:\DevTools\opencv\build\etc\haarcascades\haarcascade_frontalface_default.xml"

# If false, reinstall OpenCV or download manually from:
# https://github.com/opencv/opencv/tree/master/data/haarcascades
```

### Issue: "No faces detected in source frame"

**Quick Fix:**
- Ensure good lighting (face clearly visible)
- Face must be frontal (not profile view)
- Face must be reasonably large in frame (at least 100x100 pixels)
- Try adjusting camera angle

### Issue: Low FPS / Choppy Video

**Quick Fix:**
```cpp
// In main.cpp, reduce blend strength to skip some processing
processor->SetBlendStrength(0.5f);  // Lighter processing

// Or process every 2nd frame:
static int frameSkip = 0;
if (frameSkip++ % 2 == 0) {
    output = processor->ProcessFrame(input);
}
```

### Issue: "ONNX Runtime not available"

**Quick Fix:**
```powershell
# Rebuild with ONNX enabled
cmake -B build -DUSE_ONNX=ON
cmake --build build --config Debug
```

---

## Next Steps

### 1. Download Real AI Models

Follow the full model download guide in `person_replacement_processor.md`

### 2. Enable GPU Acceleration

```cpp
processor->SetUseGPU(true);  // Requires CUDA Toolkit
```

### 3. Try All Modes

- Face Swap
- Full Body Replacement
- Face Enhancement
- Super Resolution

### 4. Create Custom Target Images

```
assets/
  ├── celebrity_face.jpg
  ├── avatar_animated.mp4
  ├── professional_headshot.jpg
  └── background_office.jpg
```

### 5. Optimize Performance

See "Performance Optimization" section in main documentation

---

## Resources

- **Full Documentation:** `docs/person_replacement_processor.md`
- **Model Downloads:** See "Model Integration" section
- **API Reference:** See "API Reference" section
- **Examples:** See "Examples" section

---

## Live Demo

### Face Swap Test:

```powershell
# 1. Run app
.\run_debug.bat

# 2. Open Preview Window (right-click tray icon)

# 3. Select "AI Face Swap (DeepSeek)"

# 4. Watch console output:
# [OnFilterChanged] Switched to: Person Replacement (Face Swap)
# Initializing PersonReplacementProcessor...
# PersonReplacementProcessor initialized successfully!

# 5. See your face replaced!
```

### Face Enhancement Test:

```powershell
# 1. Select "AI Face Enhancement (GFPGAN)"

# 2. Console shows:
# [OnFilterChanged] Switched to: Face Enhancement (AI)
# Replacement mode set to: 2

# 3. Your face should look sharper and clearer!
```

---

## Common Use Cases

### Use Case 1: Privacy in Video Calls
**Mode:** Face Swap  
**Setup:** Replace your face with generic face for privacy

### Use Case 2: Professional Appearance
**Mode:** Face Enhancement  
**Setup:** Improve lighting and skin quality for meetings

### Use Case 3: Low-Quality Webcam Fix
**Mode:** Super Resolution  
**Setup:** Upscale 720p to 1440p for better quality

### Use Case 4: Virtual Avatar
**Mode:** Full Body Replacement  
**Setup:** Replace entire appearance with avatar image

---

## Success Criteria

✅ **You're Ready When:**
1. App launches without errors
2. Preview window shows video feed
3. Person replacement filters appear in dropdown
4. Selecting a filter processes video (even with fallback)
5. Console shows processing time logs every 30 frames

**Example Console Output:**
```
[OnFilterChanged] Switched to: Face Enhancement (AI)
Initializing PersonReplacementProcessor...
Loaded Haar Cascade from: D:/DevTools/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml
ONNX Runtime initialized successfully
PersonReplacementProcessor initialized successfully!
Person Replacement Processing Time: 45.2 ms
Person Replacement Processing Time: 43.8 ms
```

---

## Get Help

- 📖 **Full Docs:** `docs/person_replacement_processor.md`
- 🐛 **Issues:** Check console output for error messages
- 💬 **Questions:** Review troubleshooting section

---

**Ready to start!** 🚀

Just run:
```powershell
.\run_debug.bat
```

And select "AI Face Swap (DeepSeek)" from the dropdown!

---

**Last Updated:** November 9, 2025
