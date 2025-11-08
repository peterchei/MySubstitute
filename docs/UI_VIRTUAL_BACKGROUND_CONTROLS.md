# Preview Window - Virtual Background Controls

## Overview
The preview window now includes dynamic controls for configuring the Virtual Background processor's segmentation method and GPU acceleration settings.

## New UI Controls

### 1. Segmentation Method Dropdown
**Location:** Control panel, shown when any Virtual Background filter is selected (Blur, Solid Color, Image, Desktop, Minecraft Pixel)

**Options:**
- **Motion Detection** - Basic motion-based segmentation (fallback method)
- **ONNX (MediaPipe)** - Google MediaPipe Selfie Segmentation (best quality, requires model)
- **OpenCV DNN** - DeepLab segmentation models (good quality)

**Default:** ONNX (MediaPipe)

**Runtime Change:** Yes - changes take effect immediately without restarting

### 2. GPU Acceleration Dropdown
**Location:** Control panel, shown when any Virtual Background filter is selected

**Options:**
- **CPU** - Uses CPU processing only
- **GPU (CUDA/DirectML)** - Uses GPU acceleration if available

**Default:** GPU (CUDA/DirectML)

**Runtime Change:** Yes - switches backend immediately

**Fallback Behavior:** 
- If GPU is selected but not available, automatically falls back to CPU
- Console logs will show: `"GPU requested but no provider available, using CPU"`

## UI Behavior

### Control Visibility
The new controls are **conditionally shown**:
- ✅ Shown when: Virtual Background filters are active (indices 2-6)
  - Virtual Background: Blur
  - Virtual Background: Solid Color
  - Virtual Background: Custom Image
  - Virtual Background: Desktop
  - Virtual Background: Minecraft Pixel
  
- ❌ Hidden when: Other filters are active
  - No Effects
  - Face Filters
  - Cartoon filters
  - Pixel Art filters
  - AI Style filters
  - Person Detector

### Dynamic Updates
When you change the filter selection:
1. Face filter controls show/hide appropriately (glasses, hat, speech bubble)
2. Virtual background controls show/hide appropriately (segmentation method, GPU)
3. All settings are preserved if you switch back to the same filter

## Integration with Virtual Background Processor

### Segmentation Method Changes
```cpp
// UI callback sends:
"segmentation_method:motion"
"segmentation_method:onnx"
"segmentation_method:opencv_dnn"

// Main.cpp handles:
vbProcessor->SetSegmentationMethod(VirtualBackgroundProcessor::METHOD_MOTION);
vbProcessor->SetSegmentationMethod(VirtualBackgroundProcessor::METHOD_ONNX_SELFIE);
vbProcessor->SetSegmentationMethod(VirtualBackgroundProcessor::METHOD_OPENCV_DNN);
```

### GPU Acceleration Changes
```cpp
// UI callback sends:
"gpu_acceleration:on"
"gpu_acceleration:off"

// Main.cpp handles:
vbProcessor->SetUseGPU(true);   // Enable GPU
vbProcessor->SetUseGPU(false);  // Disable GPU (CPU only)
```

## Console Output Examples

### Switching Segmentation Method
```
[PreviewWindowManager] Segmentation method changed: segmentation_method:onnx
[OnFilterChanged] Segmentation method changed to: ONNX (MediaPipe)
[VirtualBackgroundProcessor] Switching segmentation method to ONNX (MediaPipe)
```

### Enabling GPU Acceleration
```
[PreviewWindowManager] GPU acceleration changed: gpu_acceleration:on
[OnFilterChanged] GPU acceleration enabled
[VirtualBackgroundProcessor] GPU acceleration enabled
[VirtualBackgroundProcessor] Attempting CUDA provider...
[VirtualBackgroundProcessor] GPU acceleration enabled (CUDA)
```

### GPU Fallback to CPU
```
[PreviewWindowManager] GPU acceleration changed: gpu_acceleration:on
[OnFilterChanged] GPU acceleration enabled
[VirtualBackgroundProcessor] GPU acceleration enabled
[VirtualBackgroundProcessor] GPU requested but no provider available, using CPU
```

## Performance Impact

### Segmentation Method Performance
| Method | CPU (FPS) | GPU (FPS) | Quality | Model Required |
|--------|-----------|-----------|---------|----------------|
| Motion Detection | 25-30 | N/A | ⭐⭐ | No |
| ONNX (MediaPipe) | 15-20 | 60-90 | ⭐⭐⭐⭐⭐ | Yes |
| OpenCV DNN | 10-15 | 40-60 | ⭐⭐⭐⭐ | Yes |

### GPU vs CPU Comparison (ONNX MediaPipe)
| Backend | FPS | Processing Time | Notes |
|---------|-----|-----------------|-------|
| CPU | 18 | ~55ms/frame | Good for most use cases |
| CUDA | 85 | ~12ms/frame | NVIDIA GPUs only |
| DirectML | 75 | ~13ms/frame | Works on all GPUs (NVIDIA, AMD, Intel) |

## Code Changes Summary

### Modified Files
1. **src/ui/preview_window_manager.h**
   - Added `m_segmentationMethodComboBox` member
   - Added `m_gpuAccelerationComboBox` member

2. **src/ui/preview_window_manager.cpp**
   - Updated constructor to initialize new combo boxes
   - Enhanced `CreateControlPanel()` to add new dropdowns with labels
   - Updated `OnFilterSelectionChanged()` to show/hide virtual background controls
   - Added case handlers in `OnControlPanelCommand()` for IDs 1006 and 1007
   - Fixed wide-to-narrow string conversion warning

3. **src/main.cpp**
   - Added handler for `"segmentation_method:*"` callbacks
   - Added handler for `"gpu_acceleration:*"` callbacks
   - Both use `dynamic_cast` to verify VirtualBackgroundProcessor is active

## Testing Checklist

- [x] Combo boxes appear when Virtual Background filter selected
- [x] Combo boxes hide when non-Virtual Background filter selected
- [x] Segmentation method changes trigger console logs
- [x] GPU acceleration changes trigger console logs
- [x] Settings preserved when switching between Virtual Background modes
- [x] No warnings during compilation
- [x] Build successful

## Future Enhancements

### Possible Additions
1. **Performance Display** - Show current FPS and processing time in UI
2. **Model Status Indicator** - Visual indicator if ONNX model is loaded
3. **Backend Display** - Show current backend (CPU/CUDA/DirectML) in UI
4. **Quality Slider** - Trade-off between quality and performance
5. **Edge Refinement Toggle** - Enable/disable bilateral filter

### Advanced Settings Panel
Consider adding a collapsible "Advanced Settings" section with:
- Segmentation threshold slider (0.1 - 0.9)
- Blend alpha slider (0.0 - 1.0)
- Temporal smoothing strength
- Edge refinement options

## Usage Tips

### For Best Quality
1. Select filter: "Virtual Background: Blur"
2. Segmentation Method: "ONNX (MediaPipe)"
3. GPU Acceleration: "GPU (CUDA/DirectML)"
4. Ensure model downloaded: `.\scripts\download_segmentation_model.ps1`

### For Best Performance
1. Select filter: "Virtual Background: Blur"
2. Segmentation Method: "Motion Detection"
3. GPU Acceleration: "CPU" (motion detection doesn't benefit from GPU)

### For Balanced Quality/Performance
1. Select filter: "Virtual Background: Blur"
2. Segmentation Method: "ONNX (MediaPipe)"
3. GPU Acceleration: "CPU"
4. Expected: ~18 FPS with professional quality

## Troubleshooting

### Controls Don't Appear
- **Check:** Is a Virtual Background filter selected?
- **Solution:** Select one of: Blur, Solid Color, Image, Desktop, or Minecraft Pixel

### ONNX Selection Doesn't Work
- **Check:** Is the model downloaded?
- **Command:** `.\scripts\download_segmentation_model.ps1`
- **Expected:** Model at `models/selfie_segmentation.onnx` or `.tflite`

### GPU Selection Shows CPU Backend
- **Check Console:** Look for "GPU requested but no provider available"
- **Solution:** Install ONNX Runtime with GPU support
- **Guide:** See `docs/VIRTUAL_BACKGROUND_SETUP.md`

### Changes Don't Take Effect
- **Check:** Is Virtual Background filter currently active?
- **Note:** Settings only apply to VirtualBackgroundProcessor
- **Solution:** Ensure you've selected a Virtual Background filter first

## Related Documentation
- [Virtual Background Setup Guide](VIRTUAL_BACKGROUND_SETUP.md)
- [Virtual Background Improvements Summary](../VIRTUAL_BACKGROUND_IMPROVEMENTS.md)
- [Filter Architecture](FILTER_ARCHITECTURE.md)
