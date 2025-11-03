# AnimeGAN Model Issue - Incompatible ONNX Format

## Problem
The AnimeGANv2_Hayao.onnx model from HuggingFace (bryandlee/animegan2-pytorch) is incompatible with OpenCV DNN module.

## Error
```
Number of input channels should be multiple of 3 but got 512
```

This error occurs because:
1. The model's internal layers expect 3-channel RGB input
2. But OpenCV DNN is passing 512-channel intermediate feature maps
3. This indicates the ONNX model wasn't exported properly for OpenCV compatibility

## Solution Options

### Option 1: Use PyTorch Model Export (Recommended)
Download the PyTorch model and export it correctly for OpenCV:

```python
import torch
import torch.onnx
from animegan2_pytorch import Generator

# Load model
model = Generator()
model.load_state_dict(torch.load('generator_Hayao.pth'))
model.eval()

# Export with correct settings for OpenCV
dummy_input = torch.randn(1, 3, 512, 512)
torch.onnx.export(
    model,
    dummy_input,
    "anime_gan_opencv.onnx",
    export_params=True,
    opset_version=11,  # OpenCV supports opset 11
    do_constant_folding=True,
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={
        'input': {0: 'batch_size', 2: 'height', 3: 'width'},
        'output': {0: 'batch_size', 2: 'height', 3: 'width'}
    }
)
```

### Option 2: Use Pre-exported OpenCV-Compatible Model
Find models specifically exported for OpenCV DNN:
- https://github.com/opencv/opencv_zoo (official OpenCV models)
- Look for models with "opencv" or "dnn" in the name
- Check model's opset version (OpenCV supports up to opset 11-13)

### Option 3: Use Different Style Transfer Model
Try alternative anime style transfer models:
1. **Fast Neural Style Transfer** - OpenCV has built-in support
2. **CartoonGAN** - Simpler architecture, better OpenCV compatibility
3. **White-Box CartoonGAN** - Specifically designed for real-time

### Option 4: Use TensorFlow Lite (Not OpenCV)
If you have TensorFlow support compiled:
- Download .tflite models
- Use OpenCV's readNetFromTensorflow()
- Requires OpenCV compiled with TensorFlow support

## Current Model Details
- **File**: models/anime_gan.onnx (AnimeGANv2_Hayao.onnx)
- **Size**: 8.25 MB
- **Source**: HuggingFace bryandlee/animegan2-pytorch
- **Issue**: Not compatible with OpenCV DNN module
- **Error Location**: First convolution layer expects 3 channels but gets 512

## Recommended Action
1. **Quick Fix**: Switch to simpler cartoon filter (already working: Cartoon, Pixel Art)
2. **Better Solution**: Re-export AnimeGAN model with proper ONNX settings
3. **Best Solution**: Use OpenCV Zoo models designed for OpenCV DNN

## Testing Commands
```powershell
# Try running with debug output
.\build\bin\Release\MySubstitute.exe 2>&1 | Select-String "ERROR"

# Check model with Python
python -c "import onnx; model=onnx.load('models/anime_gan.onnx'); print(model.graph.input[0])"
```

## Alternative: Use Existing Filters
The project already has working stylization filters:
- **Cartoon Filter** - Edge-preserving smoothing + edge detection
- **Watercolor** - Bilateral filter + saturation boost
- **Pixel Art** - Block averaging + color quantization
- **Oil Painting** - Bilateral filter + edge preservation

These provide artistic styles without requiring deep learning models.
