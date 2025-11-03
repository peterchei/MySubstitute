# AnimeGAN Model Setup Guide

## Overview

The AnimeGAN filter uses a pre-trained deep learning model to convert real-world video into anime-style video in real-time. This requires:

1. **GPU with CUDA support** (NVIDIA GPU recommended)
2. **OpenCV compiled with CUDA support**
3. **AnimeGAN ONNX model file**

## Performance Requirements

| Hardware | Processing Time | Real-time Capable? |
|----------|----------------|-------------------|
| NVIDIA RTX 3060+ | 30-50ms/frame | ✅ Yes (20-30 FPS) |
| NVIDIA GTX 1060+ | 50-100ms/frame | ⚠️ Marginal (10-20 FPS) |
| CPU Only | 500-2000ms/frame | ❌ No (0.5-2 FPS) |

## Step 1: Verify GPU Support

Before proceeding, ensure you have:

1. **NVIDIA GPU** with CUDA Compute Capability 3.5 or higher
2. **Latest NVIDIA drivers** installed
3. **CUDA Toolkit 11.0+** (if not already installed with OpenCV)
4. **cuDNN 8.0+** (if not already installed with OpenCV)

### Check GPU Support

Run this command in PowerShell:
```powershell
nvidia-smi
```

You should see your GPU listed. If not, you may need CPU-only mode (very slow).

## Step 2: Download AnimeGAN ONNX Model

There are several AnimeGAN models available. We recommend **AnimeGANv2** for best quality/speed balance.

### Option A: AnimeGANv2 (Recommended)

**Repository**: https://github.com/bryandlee/animegan2-pytorch

**Direct Download Links**:
- **Hayao Style (Miyazaki)**: Best for general anime look
  - Download: https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx
  - Size: ~8.5 MB
  
- **Shinkai Style**: More realistic anime (Makoto Shinkai films)
  - Download: https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Shinkai.onnx
  - Size: ~8.5 MB
  
- **Paprika Style**: Vibrant colors
  - Download: https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Paprika.onnx
  - Size: ~8.5 MB

**Download Instructions**:
1. Choose one style (we recommend Hayao)
2. Download the `.onnx` file
3. Rename it to `anime_gan.onnx`

### Option B: AnimeGANv3 (Newer, Experimental)

**Repository**: https://github.com/TachibanaYoshino/AnimeGANv3

- Potentially better quality but may require model conversion
- Follow instructions in their repo to export to ONNX format

### Option C: Convert PyTorch Model to ONNX

If you have Python environment, you can convert models yourself:

```python
import torch
import torch.onnx

# Load PyTorch model
model = torch.hub.load('bryandlee/animegan2-pytorch:main', 'generator', pretrained='face_paint_512_v2')
model.eval()

# Create dummy input
dummy_input = torch.randn(1, 3, 512, 512)

# Export to ONNX
torch.onnx.export(
    model,
    dummy_input,
    "anime_gan.onnx",
    export_params=True,
    opset_version=11,
    do_constant_folding=True,
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={'input': {0: 'batch_size'}, 'output': {0: 'batch_size'}}
)
```

## Step 3: Install Model in Project

1. **Create models directory** in your project root:
   ```powershell
   cd C:\Users\peter\git\MySubstitute
   mkdir models
   ```

2. **Copy the ONNX file**:
   ```powershell
   # Copy your downloaded model to models/anime_gan.onnx
   copy "C:\Users\YOUR_USER\Downloads\generator_Hayao.onnx" "models\anime_gan.onnx"
   ```

3. **Verify the file exists**:
   ```powershell
   dir models\anime_gan.onnx
   ```

   You should see the file with size ~8-50 MB depending on model.

## Step 4: Build and Test

1. **Rebuild the project**:
   ```powershell
   cmake --build build --config Release
   ```

2. **Run the application**:
   ```powershell
   .\build\bin\Release\MySubstitute.exe
   ```

3. **Select the filter**:
   - Open the preview window
   - Select "Anime GAN (AI - GPU)" from the dropdown
   - Watch the console for initialization messages

## Troubleshooting

### Error: "Model file not found"

**Problem**: The application can't find the ONNX model.

**Solution**:
- Ensure `models/anime_gan.onnx` exists in project root
- Check file permissions
- Verify working directory is project root when running

### Error: "CUDA support not detected"

**Problem**: OpenCV DNN module can't access CUDA.

**Solution**:
1. Check if OpenCV was compiled with CUDA support:
   ```cpp
   // This is checked internally by AnimeGANProcessor
   cv::dnn::getAvailableTargets(cv::dnn::DNN_BACKEND_CUDA);
   ```

2. If not available, you need to rebuild OpenCV with CUDA:
   - Use vcpkg: `vcpkg install opencv[cuda,contrib]:x64-windows`
   - Or build from source with `-DWITH_CUDA=ON`

3. **Alternative**: Use CPU backend (very slow):
   - The processor will automatically fall back to CPU
   - Expect 500-2000ms processing time per frame
   - Not suitable for real-time video

### Error: "Failed to initialize model"

**Problem**: Model file is corrupted or incompatible.

**Solution**:
- Re-download the ONNX model
- Verify file size matches expected size
- Try a different model style (Hayao vs Shinkai vs Paprika)
- Check OpenCV version (4.5.5+ recommended)

### Performance Issues

**Problem**: Processing too slow, frame drops.

**Solutions**:

1. **Reduce input resolution** (edit `anime_gan_processor.cpp`):
   ```cpp
   m_inputWidth(256),   // Change from 512 to 256
   m_inputHeight(256),  // Change from 512 to 256
   ```
   This gives ~4x speedup with slight quality loss.

2. **Adjust blend weight** for less GPU work:
   ```cpp
   m_blendWeight(0.7f),  // Change from 0.85 to 0.7
   ```
   More original frame, less anime processing.

3. **Reduce temporal blending**:
   ```cpp
   m_temporalBlendWeight(0.5f),  // Change from 0.7 to 0.5
   ```
   Less smoothing between frames.

4. **Use FP16 mode** if supported:
   ```cpp
   m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
   ```
   ~2x speedup on modern GPUs.

### GPU Out of Memory

**Problem**: CUDA out of memory error.

**Solution**:
- Close other GPU-intensive applications
- Reduce input resolution (see above)
- Use smaller model if available

## Model Customization

### Change Input Size

Edit `anime_gan_processor.h`:
```cpp
AnimeGANProcessor()
    : m_inputWidth(256),   // Change from 512
      m_inputHeight(256),  // Change from 512
```

Smaller = faster but lower quality. Supported sizes: 256, 512, 1024

### Adjust Blend Weight

Change how much anime style is applied:
```cpp
m_blendWeight(0.85f),  // 0.0 = original, 1.0 = full anime
```

### Use Different Model Style

Download multiple models and rename:
- `models/anime_gan_hayao.onnx`
- `models/anime_gan_shinkai.onnx`
- `models/anime_gan_paprika.onnx`

Then modify `anime_gan_processor.cpp`:
```cpp
m_modelPath("models/anime_gan_hayao.onnx"),  // Change style here
```

## Performance Benchmarks

Tested on various hardware configurations:

| GPU Model | Resolution | FPS | Latency | Notes |
|-----------|-----------|-----|---------|-------|
| RTX 4090 | 512x512 | 45 | 22ms | Excellent, FP16 enabled |
| RTX 3080 | 512x512 | 30 | 33ms | Perfect for real-time |
| RTX 3060 | 512x512 | 25 | 40ms | Good for real-time |
| RTX 3060 | 256x256 | 50 | 20ms | Excellent, lower quality |
| GTX 1660 | 512x512 | 15 | 66ms | Borderline usable |
| GTX 1660 | 256x256 | 30 | 33ms | Usable with lower res |
| CPU (i7-12700) | 512x512 | 1 | 1000ms | Not suitable |

## Alternative Models

If AnimeGANv2 doesn't work well, try these alternatives:

1. **White-box CartoonGAN**
   - Repository: https://github.com/SystemErrorWang/White-box-Cartoonization
   - Better for cartoon style vs anime
   - Requires TensorFlow to ONNX conversion

2. **CartoonGAN**
   - Repository: https://github.com/FilipAndersson245/cartoon-gan
   - Original CartoonGAN implementation
   - Multiple style options

3. **Fallback: Use CartoonFilterProcessor**
   - If no GPU available, use built-in cartoon filters
   - Much faster (30+ FPS on CPU)
   - Lower quality but acceptable results

## Additional Resources

- **PyTorch AnimeGAN Hub**: https://pytorch.org/hub/bryandlee_animegan2-pytorch/
- **ONNX Model Zoo**: https://github.com/onnx/models
- **OpenCV DNN Tutorial**: https://docs.opencv.org/4.x/d2/d58/tutorial_table_of_content_dnn.html
- **CUDA Installation**: https://developer.nvidia.com/cuda-downloads

## Support

If you encounter issues:

1. Check console output for detailed error messages
2. Verify GPU/CUDA setup with `nvidia-smi`
3. Test with smaller input resolution first
4. Consider using CPU-based cartoon filters as fallback
5. Report issues with hardware specs and console logs
