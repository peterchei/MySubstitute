# Quick Start: Using AnimeGAN Filter

## What is the AnimeGAN Filter?

The AnimeGAN filter uses a deep learning model (similar to video2anime) to convert your real-time webcam video into anime-style video. Unlike the built-in cartoon filters (which use fast image processing), this uses a trained neural network for higher-quality anime conversion.

## Before You Start

**IMPORTANT**: This filter requires:
- ✅ **NVIDIA GPU with CUDA support** (RTX 2060 or better recommended)
- ✅ **OpenCV compiled with CUDA** (should already be in your build)
- ✅ **AnimeGAN ONNX model file** (download required - see below)

**Performance Expectations**:
- GPU: 30-100ms per frame (real-time capable)
- CPU: 500-2000ms per frame (TOO SLOW for real-time)

## Quick Setup (5 minutes)

### Step 1: Download the Model

1. **Download AnimeGANv2 (Hayao Style)** - 8.5 MB:
   ```
   https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx
   ```
   
   Or use PowerShell:
   ```powershell
   Invoke-WebRequest -Uri "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx" -OutFile "generator_Hayao.onnx"
   ```

### Step 2: Create Models Directory

```powershell
cd C:\Users\peter\git\MySubstitute
mkdir models
```

### Step 3: Copy and Rename Model

```powershell
# Rename to anime_gan.onnx
copy "C:\Users\peter\Downloads\generator_Hayao.onnx" "models\anime_gan.onnx"

# Verify
dir models\anime_gan.onnx
```

You should see the file (~8.5 MB).

### Step 4: Run MySubstitute

```powershell
.\build\bin\Release\MySubstitute.exe
```

### Step 5: Select AnimeGAN Filter

1. Open the preview window
2. In the dropdown menu, select: **"Anime GAN (AI - GPU)"**
3. Watch the console output for:
   - ✅ "CUDA support detected"
   - ✅ "Model loaded successfully"
   - ✅ "Using CUDA backend for GPU acceleration"

### Step 6: Check Performance

The console will show processing times every 100 frames:
```
[AnimeGANProcessor] Frame 100 | Processing time: 35ms
```

**Good**: 20-100ms (real-time capable)
**Slow**: 100-200ms (marginal)
**Too Slow**: 200ms+ (need to optimize - see below)

## Troubleshooting

### "Model file not found"

**Fix**: Make sure `models/anime_gan.onnx` exists in your project root directory.

```powershell
# Check if file exists
Test-Path "C:\Users\peter\git\MySubstitute\models\anime_gan.onnx"
```

Should return `True`.

### "CUDA support not detected" (Will use slow CPU mode)

**Problem**: OpenCV can't access your GPU.

**Quick Check**:
```powershell
nvidia-smi
```

Should show your GPU. If not, update NVIDIA drivers.

**Fix**: OpenCV needs to be built with CUDA support. Check your OpenCV build configuration.

### Processing Too Slow (>100ms per frame)

**Solution 1 - Reduce Resolution**:

Edit `src/ai/anime_gan_processor.cpp` line 11:
```cpp
m_inputWidth(256),   // Change from 512 to 256
m_inputHeight(256),  // Change from 512 to 256
```

This gives ~4x speedup with only slight quality loss.

**Solution 2 - Reduce Blend Weight**:

Edit line 13:
```cpp
m_blendWeight(0.7f),  // Change from 0.85 to 0.7
```

More original video, less anime processing.

Then rebuild:
```powershell
cmake --build build --config Release
```

## Alternative Models

Don't like the Hayao (Miyazaki) style? Try these alternatives:

### Shinkai Style (More Realistic)
```powershell
Invoke-WebRequest -Uri "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Shinkai.onnx" -OutFile "models\anime_gan.onnx"
```

### Paprika Style (Vibrant Colors)
```powershell
Invoke-WebRequest -Uri "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Paprika.onnx" -OutFile "models\anime_gan.onnx"
```

## Comparison with Built-in Filters

| Filter | Method | Speed | Quality | Requirements |
|--------|--------|-------|---------|--------------|
| **Cartoon (Simple/Detailed/Anime)** | Image Processing | 30+ FPS | Good | CPU only |
| **Pixel Art** | Image Processing | 30+ FPS | Stylized | CPU only |
| **AnimeGAN** | Deep Learning | 10-30 FPS | Excellent | GPU required |

**When to use AnimeGAN**:
- ✅ You have NVIDIA GPU
- ✅ Want highest quality anime conversion
- ✅ Don't mind 30-100ms latency
- ✅ Willing to download 8-50MB model

**When to use Cartoon filters instead**:
- ✅ No GPU / laptop user
- ✅ Need fastest possible performance
- ✅ Don't want to download models
- ✅ Good-enough quality is acceptable

## Full Documentation

For detailed setup, model options, performance tuning, and troubleshooting:

📖 See: `docs/anime_gan_setup.md`

## Support

Having issues? Check:

1. **Console output** for detailed error messages
2. **GPU availability**: Run `nvidia-smi` 
3. **Model file size**: Should be 8-50 MB, not corrupted
4. **Working directory**: Make sure you run from project root

Still stuck? The AnimeGAN filter is optional - you can use the built-in Cartoon filters which work great on CPU-only systems!
