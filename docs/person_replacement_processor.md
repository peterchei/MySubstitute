# PersonReplacementProcessor Documentation

## Overview

The **PersonReplacementProcessor** is an advanced AI-powered processor for the MySubstitute virtual camera application that enables DeepSeek-style person manipulation capabilities including face swapping, full body replacement, face enhancement, and super-resolution.

## Table of Contents

1. [Features](#features)
2. [Architecture](#architecture)
3. [Processing Modes](#processing-modes)
4. [Getting Started](#getting-started)
5. [Model Integration](#model-integration)
6. [API Reference](#api-reference)
7. [Performance Optimization](#performance-optimization)
8. [Troubleshooting](#troubleshooting)

---

## Features

### Core Capabilities

- ✅ **Face Swapping** - DeepFake-style face replacement with seamless blending
- ✅ **Full Body Replacement** - Complete person substitution using AI segmentation
- ✅ **Face Enhancement** - GFPGAN/CodeFormer-style face restoration and quality improvement
- ✅ **Super Resolution** - Real-ESRGAN powered AI upscaling (2x-4x)
- ✅ **Style Transfer** - Apply artistic styles to video frames (future)

### Technical Features

- **ONNX Runtime Integration** - High-performance AI inference with CPU/GPU support
- **OpenCV Fallbacks** - Works without AI models using traditional computer vision
- **Real-time Processing** - Optimized for video streaming applications
- **Configurable Blending** - Adjustable blend strength (0.0-1.0) for natural results
- **Multi-Model Support** - Load and switch between different AI models dynamically
- **Video Target Support** - Animate face swaps using video as target source

---

## Architecture

### Class Structure

```cpp
class PersonReplacementProcessor : public AIProcessor
{
public:
    enum ReplacementMode {
        FACE_SWAP,           // Replace only the face (DeepFake style)
        FULL_BODY_REPLACE,   // Replace entire person
        FACE_ENHANCE,        // Enhance face quality (GFPGAN/CodeFormer)
        SUPER_RESOLUTION,    // Upscale resolution (Real-ESRGAN)
        STYLE_TRANSFER       // Apply artistic style
    };
    
    // Core AIProcessor interface
    virtual bool Initialize() override;
    virtual void Cleanup() override;
    virtual Frame ProcessFrame(const Frame& input) override;
    
    // Configuration methods
    void SetReplacementMode(ReplacementMode mode);
    void SetTargetPersonImage(const std::string& imagePath);
    void SetTargetPersonVideo(const std::string& videoPath);
    void SetBlendStrength(float strength);  // 0.0 to 1.0
    void SetEnableEnhancement(bool enable);
    void SetUseGPU(bool useGPU);
    
    // Model loading
    bool LoadFaceSwapModel(const std::string& modelPath);
    bool LoadSuperResolutionModel(const std::string& modelPath);
    bool LoadFaceEnhancementModel(const std::string& modelPath);
    bool LoadSegmentationModel(const std::string& modelPath);
};
```

### Processing Pipeline

```
Input Frame
    ↓
Mode Selection
    ↓
┌─────────────────────────────────────────┐
│  FACE_SWAP        → Face Detection      │
│  FULL_BODY_REPLACE → Person Segmentation│
│  FACE_ENHANCE     → Face Detection      │
│  SUPER_RESOLUTION → Direct Processing   │
│  STYLE_TRANSFER   → Neural Style        │
└─────────────────────────────────────────┘
    ↓
AI Model Inference (ONNX) or OpenCV Fallback
    ↓
Blending & Compositing
    ↓
Output Frame
```

### Dependencies

- **OpenCV 4.x** - Required for all image processing
- **ONNX Runtime 1.16+** - Optional, for AI model inference
- **CUDA Toolkit** (optional) - For GPU acceleration
- **Haar Cascades** - For face detection fallback

---

## Processing Modes

### 1. Face Swap Mode (`FACE_SWAP`)

Replaces detected faces in the video with faces from a target image or video.

**Use Cases:**
- Virtual identity in video calls
- Privacy protection
- Entertainment/content creation

**Requirements:**
- Face swap ONNX model (SimSwap, FaceSwap, InsightFace)
- Target person image/video with clear face

**Configuration:**
```cpp
auto processor = std::make_unique<PersonReplacementProcessor>();
processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
processor->SetTargetPersonImage("path/to/target_face.jpg");
processor->SetBlendStrength(0.9f);  // High blend for realistic swap
processor->LoadFaceSwapModel("models/simswap.onnx");
processor->Initialize();
```

**Fallback Behavior (without ONNX model):**
- Uses Haar Cascade for face detection
- Simple weighted blending between source and target faces
- No landmark alignment or color matching

---

### 2. Full Body Replacement (`FULL_BODY_REPLACE`)

Replaces the entire person in the video with another person using AI segmentation.

**Use Cases:**
- Virtual avatars
- Costume changes
- Body anonymization

**Requirements:**
- Person segmentation model (MediaPipe, U-Net, DeepLab)
- Target person image (full body)

**Configuration:**
```cpp
auto processor = std::make_unique<PersonReplacementProcessor>();
processor->SetReplacementMode(PersonReplacementProcessor::FULL_BODY_REPLACE);
processor->SetTargetPersonImage("path/to/full_body.jpg");
processor->SetBlendStrength(0.85f);
processor->LoadSegmentationModel("models/MediaPipe-Selfie-Segmentation.onnx");
processor->Initialize();
```

**Fallback Behavior:**
- Creates simple center-region mask (assumes person in center)
- Basic alpha blending without accurate segmentation

---

### 3. Face Enhancement (`FACE_ENHANCE`)

Enhances face quality using AI restoration models like GFPGAN or CodeFormer.

**Use Cases:**
- Improve video quality in low-light conditions
- Remove compression artifacts
- Professional appearance in video calls

**Requirements:**
- Face enhancement model (GFPGAN, CodeFormer, RestoreFormer)

**Configuration:**
```cpp
auto processor = std::make_unique<PersonReplacementProcessor>();
processor->SetReplacementMode(PersonReplacementProcessor::FACE_ENHANCE);
processor->SetEnableEnhancement(true);
processor->LoadFaceEnhancementModel("models/gfpgan.onnx");
processor->Initialize();
```

**Fallback Behavior:**
- Bilateral filter for noise reduction
- Unsharp masking for sharpness enhancement
- Basic edge-preserving smoothing

---

### 4. Super Resolution (`SUPER_RESOLUTION`)

Upscales video resolution using AI models like Real-ESRGAN.

**Use Cases:**
- Improve low-resolution webcam output
- Enhance video quality for streaming
- 2x-4x resolution increase

**Requirements:**
- Super-resolution model (Real-ESRGAN, ESRGAN, SRResNet)

**Configuration:**
```cpp
auto processor = std::make_unique<PersonReplacementProcessor>();
processor->SetReplacementMode(PersonReplacementProcessor::SUPER_RESOLUTION);
processor->LoadSuperResolutionModel("models/realesrgan_x2.onnx");
processor->Initialize();
```

**Fallback Behavior:**
- Bicubic interpolation (2x upscaling)
- OpenCV's INTER_CUBIC resize algorithm

---

### 5. Style Transfer (`STYLE_TRANSFER`)

Applies artistic neural style transfer to video frames (future implementation).

**Use Cases:**
- Artistic video effects
- Creative content creation
- Brand-specific visual styles

**Status:** Placeholder - to be implemented

---

## Getting Started

### Basic Usage

#### 1. Create and Initialize Processor

```cpp
#include "ai/person_replacement_processor.h"

// Create processor
auto processor = std::make_unique<PersonReplacementProcessor>();

// Set mode
processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);

// Configure
processor->SetBlendStrength(0.9f);
processor->SetTargetPersonImage("assets/target_face.jpg");

// Initialize (loads Haar Cascade)
if (!processor->Initialize()) {
    std::cerr << "Failed to initialize processor" << std::endl;
    return false;
}
```

#### 2. Load AI Models (Optional)

```cpp
// Load face swap model
if (!processor->LoadFaceSwapModel("models/simswap.onnx")) {
    std::cerr << "Failed to load face swap model, using fallback" << std::endl;
}

// Enable GPU acceleration (requires CUDA)
processor->SetUseGPU(true);
```

#### 3. Process Frames

```cpp
// Get frame from camera
Frame inputFrame = camera->CaptureFrame();

// Process with AI
Frame outputFrame = processor->ProcessFrame(inputFrame);

// Display or stream output
DisplayFrame(outputFrame);
```

#### 4. Switch Modes Dynamically

```cpp
// Switch to face enhancement
processor->SetReplacementMode(PersonReplacementProcessor::FACE_ENHANCE);

// Process next frame with new mode
Frame enhanced = processor->ProcessFrame(nextFrame);
```

### UI Integration

The processor is integrated into the preview window dropdown menu:

1. Launch MySubstitute: `.\run_debug.bat`
2. Right-click system tray icon → **Show Preview**
3. Select filter from dropdown:
   - **AI Face Swap (DeepSeek)** - Face swapping mode
   - **AI Full Body Replace** - Full body replacement
   - **AI Face Enhancement (GFPGAN)** - Face quality improvement
   - **AI Super Resolution** - Resolution upscaling

### Command-Line Parameter Setting

```cpp
// Using SetParameter API
processor->SetParameter("mode", "face_swap");
processor->SetParameter("blend_strength", "0.9");
processor->SetParameter("target_image", "assets/celebrity.jpg");
processor->SetParameter("use_gpu", "true");
processor->SetParameter("enable_enhancement", "true");
```

---

## Model Integration

### Supported Model Types

| Model Type | Purpose | Recommended Models | Input Size | Output Format |
|------------|---------|-------------------|------------|---------------|
| Face Swap | Face replacement | SimSwap, InsightFace, FaceSwap | 256x256 RGB | Face image |
| Segmentation | Person masking | MediaPipe, U-Net, DeepLab | 256x256 RGB | Binary mask |
| Face Enhancement | Quality improvement | GFPGAN, CodeFormer, RestoreFormer | 512x512 RGB | Enhanced face |
| Super Resolution | Upscaling | Real-ESRGAN, ESRGAN, SRResNet | Variable | 2x-4x resolution |

### Model Download & Conversion

#### 1. Face Swap - SimSwap

**Download Pre-trained Model:**
```bash
# Clone SimSwap repository
git clone https://github.com/neuralchen/SimSwap.git
cd SimSwap

# Download checkpoint
# Follow their instructions to download arcface_checkpoint.tar
```

**Convert to ONNX:**
```python
import torch
import torch.onnx
from models.models import create_model

# Load PyTorch model
model = create_model(opt)
model.load_state_dict(torch.load('checkpoints/simswap.pth'))
model.eval()

# Create dummy input
dummy_input = torch.randn(1, 3, 256, 256)

# Export to ONNX
torch.onnx.export(
    model,
    dummy_input,
    "simswap.onnx",
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={'input': {0: 'batch'}, 'output': {0: 'batch'}},
    opset_version=11
)
```

**Place in MySubstitute:**
```
models/simswap.onnx
```

#### 2. Face Enhancement - GFPGAN

**Download:**
```bash
# Download from official repository
wget https://github.com/TencentARC/GFPGAN/releases/download/v1.3.0/GFPGANv1.3.pth

# Or use Hugging Face
wget https://huggingface.co/spaces/Xintao/GFPGAN/resolve/main/GFPGANv1.3.pth
```

**Convert to ONNX:**
```python
import torch
from gfpgan import GFPGANer

# Load model
restorer = GFPGANer(
    model_path='GFPGANv1.3.pth',
    upscale=1,
    arch='clean',
    channel_multiplier=2,
    bg_upsampler=None
)

# Export to ONNX
dummy_input = torch.randn(1, 3, 512, 512)
torch.onnx.export(
    restorer.gfpgan,
    dummy_input,
    "gfpgan_v1.3.onnx",
    input_names=['input'],
    output_names=['output'],
    opset_version=11
)
```

**Place in MySubstitute:**
```
models/gfpgan_v1.3.onnx
```

#### 3. Super Resolution - Real-ESRGAN

**Download:**
```bash
# Download Real-ESRGAN x2
wget https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.1/RealESRGAN_x2plus.pth
```

**Convert to ONNX:**
```python
import torch
from basicsr.archs.rrdbnet_arch import RRDBNet

# Load model
model = RRDBNet(num_in_ch=3, num_out_ch=3, num_feat=64, num_block=23, num_grow_ch=32, scale=2)
model.load_state_dict(torch.load('RealESRGAN_x2plus.pth')['params'], strict=True)
model.eval()

# Export to ONNX
dummy_input = torch.randn(1, 3, 256, 256)
torch.onnx.export(
    model,
    dummy_input,
    "realesrgan_x2.onnx",
    input_names=['input'],
    output_names=['output'],
    opset_version=11,
    dynamic_axes={'input': {2: 'height', 3: 'width'}}
)
```

**Place in MySubstitute:**
```
models/realesrgan_x2.onnx
```

#### 4. Person Segmentation - MediaPipe

**Already Available:**
The MediaPipe Selfie Segmentation model is already in your project:
```
models/MediaPipe-Selfie-Segmentation.onnx
```

**Alternative - Download Custom:**
```bash
# Download from Hugging Face
wget https://huggingface.co/models/mediapipe/selfie_segmentation/resolve/main/model.onnx
```

### Model Directory Structure

```
MySubstitute/
├── models/
│   ├── simswap.onnx                    # Face swap model
│   ├── gfpgan_v1.3.onnx               # Face enhancement
│   ├── realesrgan_x2.onnx             # Super resolution (2x)
│   ├── realesrgan_x4.onnx             # Super resolution (4x)
│   ├── MediaPipe-Selfie-Segmentation.onnx  # Person segmentation
│   ├── insightface_swap.onnx          # Alternative face swap
│   └── codeformer.onnx                # Alternative face enhancement
```

### Loading Models in Code

```cpp
// In main.cpp or filter initialization
auto processor = std::make_unique<PersonReplacementProcessor>();

// Load all models
processor->LoadFaceSwapModel("models/simswap.onnx");
processor->LoadFaceEnhancementModel("models/gfpgan_v1.3.onnx");
processor->LoadSuperResolutionModel("models/realesrgan_x2.onnx");
processor->LoadSegmentationModel("models/MediaPipe-Selfie-Segmentation.onnx");

// Models are loaded on-demand when mode is switched
processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
processor->Initialize();
```

### Model Performance Comparison

| Model | Size | FPS (CPU) | FPS (GPU) | Quality | Memory |
|-------|------|-----------|-----------|---------|--------|
| SimSwap | 150MB | 5-8 | 25-30 | ⭐⭐⭐⭐⭐ | 2GB |
| InsightFace | 200MB | 4-6 | 20-25 | ⭐⭐⭐⭐ | 2.5GB |
| GFPGAN v1.3 | 350MB | 3-5 | 15-20 | ⭐⭐⭐⭐⭐ | 3GB |
| CodeFormer | 300MB | 4-6 | 18-22 | ⭐⭐⭐⭐⭐ | 2.8GB |
| Real-ESRGAN x2 | 65MB | 8-12 | 30-40 | ⭐⭐⭐⭐ | 1.5GB |
| Real-ESRGAN x4 | 65MB | 2-4 | 10-15 | ⭐⭐⭐⭐⭐ | 1.5GB |
| MediaPipe Seg | 2MB | 40-60 | 100+ | ⭐⭐⭐⭐ | 200MB |

---

## API Reference

### Constructor & Destructor

```cpp
PersonReplacementProcessor::PersonReplacementProcessor()
PersonReplacementProcessor::~PersonReplacementProcessor()
```

### Core Methods

#### `bool Initialize()`
Initializes the processor, loads Haar Cascades, and sets up ONNX Runtime.

**Returns:** `true` if successful, `false` otherwise

**Example:**
```cpp
if (!processor->Initialize()) {
    std::cerr << "Initialization failed" << std::endl;
}
```

#### `void Cleanup()`
Releases all resources, models, and video captures.

**Example:**
```cpp
processor->Cleanup();  // Called automatically in destructor
```

#### `Frame ProcessFrame(const Frame& input)`
Processes a single video frame based on the current mode.

**Parameters:**
- `input` - Input frame to process

**Returns:** Processed output frame

**Example:**
```cpp
Frame output = processor->ProcessFrame(inputFrame);
```

### Configuration Methods

#### `void SetReplacementMode(ReplacementMode mode)`
Sets the processing mode.

**Parameters:**
- `mode` - One of: `FACE_SWAP`, `FULL_BODY_REPLACE`, `FACE_ENHANCE`, `SUPER_RESOLUTION`, `STYLE_TRANSFER`

**Example:**
```cpp
processor->SetReplacementMode(PersonReplacementProcessor::FACE_ENHANCE);
```

#### `void SetTargetPersonImage(const std::string& imagePath)`
Sets the target person image for face/body replacement.

**Parameters:**
- `imagePath` - Path to target image file

**Example:**
```cpp
processor->SetTargetPersonImage("assets/celebrity.jpg");
```

#### `void SetTargetPersonVideo(const std::string& videoPath)`
Sets the target person video for animated replacements.

**Parameters:**
- `videoPath` - Path to target video file

**Example:**
```cpp
processor->SetTargetPersonVideo("assets/avatar_animation.mp4");
```

#### `void SetBlendStrength(float strength)`
Sets the blending strength between original and processed frames.

**Parameters:**
- `strength` - Value between 0.0 (original) and 1.0 (full effect)

**Example:**
```cpp
processor->SetBlendStrength(0.8f);  // 80% effect, 20% original
```

#### `void SetEnableEnhancement(bool enable)`
Enables/disables additional enhancement processing.

**Parameters:**
- `enable` - `true` to enable, `false` to disable

#### `void SetUseGPU(bool useGPU)`
Enables/disables GPU acceleration (requires CUDA).

**Parameters:**
- `useGPU` - `true` to use GPU, `false` for CPU

**Example:**
```cpp
processor->SetUseGPU(true);  // Enable CUDA acceleration
```

### Model Loading Methods

#### `bool LoadFaceSwapModel(const std::string& modelPath)`
Loads a face swap ONNX model.

**Returns:** `true` if loaded successfully

**Example:**
```cpp
if (processor->LoadFaceSwapModel("models/simswap.onnx")) {
    std::cout << "Face swap model loaded" << std::endl;
}
```

#### `bool LoadSuperResolutionModel(const std::string& modelPath)`
Loads a super-resolution ONNX model.

#### `bool LoadFaceEnhancementModel(const std::string& modelPath)`
Loads a face enhancement ONNX model.

#### `bool LoadSegmentationModel(const std::string& modelPath)`
Loads a person segmentation ONNX model.

### Information Methods

#### `std::string GetReplacementInfo() const`
Returns detailed information about current processor state.

**Returns:** Multi-line string with mode, settings, and status

**Example:**
```cpp
std::cout << processor->GetReplacementInfo() << std::endl;
```

#### `ReplacementMode GetMode() const`
Returns the current replacement mode.

#### `float GetBlendStrength() const`
Returns the current blend strength.

### Parameter Interface

#### `bool SetParameter(const std::string& name, const std::string& value)`
Sets a parameter by name.

**Supported Parameters:**
- `"mode"` - Values: `"face_swap"`, `"full_body"`, `"face_enhance"`, `"super_res"`, `"style_transfer"`
- `"blend_strength"` - Values: `"0.0"` to `"1.0"`
- `"enable_enhancement"` - Values: `"true"`, `"false"`, `"1"`, `"0"`
- `"use_gpu"` - Values: `"true"`, `"false"`
- `"target_image"` - Path to image file
- `"target_video"` - Path to video file

**Example:**
```cpp
processor->SetParameter("mode", "face_swap");
processor->SetParameter("blend_strength", "0.9");
processor->SetParameter("use_gpu", "true");
```

#### `std::map<std::string, std::string> GetParameters() const`
Returns all current parameters as a map.

---

## Performance Optimization

### CPU Optimization

1. **Use Smaller Models:**
   ```cpp
   // Use lighter models for CPU
   processor->LoadFaceSwapModel("models/simswap_lite.onnx");
   ```

2. **Reduce Input Resolution:**
   ```cpp
   // Process at lower resolution, upscale result
   cv::Mat small;
   cv::resize(input.data, small, cv::Size(640, 480));
   Frame smallFrame(small);
   Frame processed = processor->ProcessFrame(smallFrame);
   ```

3. **Skip Frames:**
   ```cpp
   static int frameCounter = 0;
   if (frameCounter++ % 2 == 0) {
       // Process every other frame
       output = processor->ProcessFrame(input);
   } else {
       output = input;  // Use previous result
   }
   ```

### GPU Optimization

1. **Enable GPU Acceleration:**
   ```cpp
   processor->SetUseGPU(true);
   ```

2. **Batch Processing (if supported):**
   ```cpp
   // Process multiple frames together
   std::vector<Frame> batch = {frame1, frame2, frame3};
   // Model-specific batching needed
   ```

3. **Use CUDA-Optimized Models:**
   - Convert models with TensorRT optimization
   - Use FP16 precision for faster inference

### Memory Optimization

1. **Release Unused Models:**
   ```cpp
   processor->Cleanup();  // Release all models
   processor->LoadFaceSwapModel("models/simswap.onnx");  // Load only needed
   ```

2. **Use Smaller Batch Sizes:**
   - Process 1 frame at a time for real-time applications

3. **Monitor Memory Usage:**
   ```cpp
   double time = processor->GetExpectedProcessingTime();
   if (time > 100.0) {  // > 100ms per frame
       std::cerr << "Performance issue detected" << std::endl;
   }
   ```

---

## Troubleshooting

### Common Issues

#### 1. "Failed to load face detection cascade"

**Cause:** Haar Cascade XML file not found

**Solution:**
```cpp
// Ensure OpenCV Haar Cascades are available
// Download from: https://github.com/opencv/opencv/tree/master/data/haarcascades
// Place in: D:/DevTools/opencv/build/etc/haarcascades/
```

#### 2. "ONNX Runtime not available"

**Cause:** ONNX Runtime not installed or `USE_ONNX` not enabled

**Solution:**
```bash
# Rebuild with ONNX support
cmake -B build -DUSE_ONNX=ON
cmake --build build --config Debug
```

#### 3. "GPU not available, falling back to CPU"

**Cause:** CUDA not installed or incompatible GPU

**Solution:**
- Install CUDA Toolkit 11.x or 12.x
- Use CPU-only ONNX Runtime for non-NVIDIA GPUs
- Download from: https://github.com/microsoft/onnxruntime/releases

#### 4. "No faces detected in source frame"

**Cause:** Poor lighting, face at angle, or Haar Cascade limitations

**Solution:**
```cpp
// Try DNN-based face detection (future improvement)
// Or adjust Haar Cascade parameters
std::vector<cv::Rect> faces;
m_faceCascade.detectMultiScale(
    gray, faces,
    1.1,  // Scale factor (lower = more detections, slower)
    3,    // Min neighbors (lower = more false positives)
    0,
    cv::Size(30, 30)  // Min face size
);
```

#### 5. Low FPS / Performance Issues

**Solutions:**
- Use CPU-optimized models
- Enable GPU acceleration
- Reduce input resolution
- Skip frames (process every 2nd or 3rd frame)
- Use lighter models (SimSwap Lite, etc.)

#### 6. "Invalid Feed Input Name" ONNX Error

**Cause:** Model input/output names don't match expected names

**Solution:**
The processor auto-detects input/output names:
```cpp
// Automatic detection in LoadFaceSwapModel()
m_faceSwapInputName = m_faceSwapSession->GetInputNameAllocated(0, allocator).get();
m_faceSwapOutputName = m_faceSwapSession->GetOutputNameAllocated(0, allocator).get();
```

---

## Examples

### Example 1: Basic Face Swap

```cpp
#include "ai/person_replacement_processor.h"

int main() {
    // Create processor
    auto processor = std::make_unique<PersonReplacementProcessor>();
    
    // Configure for face swap
    processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
    processor->SetTargetPersonImage("assets/celebrity.jpg");
    processor->SetBlendStrength(0.95f);
    
    // Initialize
    if (!processor->Initialize()) {
        return -1;
    }
    
    // Load AI model (optional, fallback to OpenCV if not available)
    processor->LoadFaceSwapModel("models/simswap.onnx");
    
    // Process video
    while (running) {
        Frame input = camera->CaptureFrame();
        Frame output = processor->ProcessFrame(input);
        DisplayFrame(output);
    }
    
    return 0;
}
```

### Example 2: Animated Face Swap from Video

```cpp
// Use video as target source
processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
processor->SetTargetPersonVideo("assets/animated_avatar.mp4");
processor->SetBlendStrength(0.9f);
processor->Initialize();

// Each frame will use next frame from video
// Video loops automatically when it reaches the end
while (running) {
    Frame output = processor->ProcessFrame(camera->CaptureFrame());
    DisplayFrame(output);
}
```

### Example 3: Face Enhancement for Video Calls

```cpp
// Enhance face quality for professional appearance
processor->SetReplacementMode(PersonReplacementProcessor::FACE_ENHANCE);
processor->SetEnableEnhancement(true);
processor->LoadFaceEnhancementModel("models/gfpgan_v1.3.onnx");
processor->Initialize();

while (inVideoCall) {
    Frame enhanced = processor->ProcessFrame(webcamFrame);
    SendToVideoCall(enhanced);
}
```

### Example 4: Super Resolution Upscaling

```cpp
// Upscale 720p to 1440p
processor->SetReplacementMode(PersonReplacementProcessor::SUPER_RESOLUTION);
processor->LoadSuperResolutionModel("models/realesrgan_x2.onnx");
processor->SetUseGPU(true);  // Use GPU for faster processing
processor->Initialize();

Frame lowRes = CaptureFrame();  // 1280x720
Frame highRes = processor->ProcessFrame(lowRes);  // 2560x1440
```

### Example 5: Dynamic Mode Switching

```cpp
// Switch between modes based on user input
void OnUserCommand(const std::string& command) {
    if (command == "enhance") {
        processor->SetReplacementMode(PersonReplacementProcessor::FACE_ENHANCE);
    } else if (command == "swap") {
        processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
    } else if (command == "upscale") {
        processor->SetReplacementMode(PersonReplacementProcessor::SUPER_RESOLUTION);
    }
}
```

---

## Best Practices

1. **Always Check Initialization:**
   ```cpp
   if (!processor->Initialize()) {
       // Handle error
   }
   ```

2. **Load Models Before Processing:**
   ```cpp
   processor->LoadFaceSwapModel("models/simswap.onnx");
   processor->Initialize();  // Then initialize
   ```

3. **Use Appropriate Blend Strength:**
   - Face Swap: 0.85-0.95 (high for realism)
   - Full Body: 0.75-0.90
   - Face Enhance: 0.6-0.8 (keep some original)

4. **Monitor Performance:**
   ```cpp
   double time = processor->GetExpectedProcessingTime();
   std::cout << "Processing time: " << time << " ms" << std::endl;
   ```

5. **Cleanup Resources:**
   ```cpp
   processor->Cleanup();  // When done
   ```

---

## Future Enhancements

- [ ] Style Transfer implementation
- [ ] DNN-based face detection (more accurate than Haar)
- [ ] Face landmark detection for better alignment
- [ ] Multi-face tracking and swapping
- [ ] Real-time background removal integration
- [ ] Custom model training pipeline
- [ ] Model quantization for faster CPU inference
- [ ] TensorRT integration for NVIDIA GPUs
- [ ] DirectML support for AMD/Intel GPUs

---

## References

- [SimSwap GitHub](https://github.com/neuralchen/SimSwap)
- [GFPGAN GitHub](https://github.com/TencentARC/GFPGAN)
- [Real-ESRGAN GitHub](https://github.com/xinntao/Real-ESRGAN)
- [MediaPipe Segmentation](https://google.github.io/mediapipe/solutions/selfie_segmentation)
- [ONNX Runtime Documentation](https://onnxruntime.ai/docs/)
- [OpenCV Documentation](https://docs.opencv.org/)

---

## Support

For issues, questions, or contributions:
- GitHub Issues: [MySubstitute Issues](https://github.com/peterchei/MySubstitute/issues)
- Email: support@mysubstitute.com

---

**Last Updated:** November 9, 2025
**Version:** 1.0.0
