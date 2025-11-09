# AI Model Conversion Guide for PersonReplacementProcessor

Complete guide for converting PyTorch models to ONNX format for use with MySubstitute PersonReplacementProcessor.

---

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [General Conversion Process](#general-conversion-process)
3. [SimSwap (Face Swap)](#simswap-face-swap)
4. [GFPGAN (Face Enhancement)](#gfpgan-face-enhancement)
5. [Real-ESRGAN (Super Resolution)](#real-esrgan-super-resolution)
6. [MediaPipe (Person Segmentation)](#mediapipe-person-segmentation)
7. [Alternative Models](#alternative-models)
8. [Testing ONNX Models](#testing-onnx-models)
9. [Troubleshooting](#troubleshooting)

---

## Prerequisites

### Python Environment Setup

```bash
# Create virtual environment
python -m venv onnx_conversion_env
source onnx_conversion_env/bin/activate  # On Windows: onnx_conversion_env\Scripts\activate

# Install core dependencies
pip install torch torchvision torchaudio --index-url https://download.pytorch.org/whl/cu118
pip install onnx onnxruntime onnxsim
pip install opencv-python numpy
```

### Required Tools

- **Python 3.8+**
- **PyTorch 1.13+**
- **ONNX 1.14+**
- **ONNX Runtime 1.16+**
- **ONNX Simplifier** (optional, for optimization)

---

## General Conversion Process

### Basic PyTorch to ONNX Conversion

```python
import torch
import torch.onnx

# 1. Load your PyTorch model
model = YourModel()
model.load_state_dict(torch.load('model.pth'))
model.eval()

# 2. Create dummy input (match expected input shape)
dummy_input = torch.randn(1, 3, 256, 256)

# 3. Export to ONNX
torch.onnx.export(
    model,                          # Model to export
    dummy_input,                    # Example input
    "model.onnx",                   # Output file
    input_names=['input'],          # Input tensor names
    output_names=['output'],        # Output tensor names
    dynamic_axes={                  # Optional: allow dynamic sizes
        'input': {0: 'batch'},
        'output': {0: 'batch'}
    },
    opset_version=11                # ONNX opset version
)

print("✅ Model exported to model.onnx")
```

### Simplify ONNX Model (Optimize)

```python
import onnx
from onnxsim import simplify

# Load ONNX model
model = onnx.load("model.onnx")

# Simplify
model_simp, check = simplify(model)

if check:
    onnx.save(model_simp, "model_simplified.onnx")
    print("✅ Model simplified successfully")
else:
    print("❌ Simplification failed")
```

---

## SimSwap (Face Swap)

### Download SimSwap

```bash
# Clone repository
git clone https://github.com/neuralchen/SimSwap.git
cd SimSwap

# Install dependencies
pip install -r requirements.txt

# Download pre-trained models
# Follow instructions at: https://github.com/neuralchen/SimSwap#preparation
# Download:
# - arcface_checkpoint.tar (face recognition)
# - checkpoints/people (face swapping weights)
```

### Convert SimSwap to ONNX

Create `convert_simswap.py`:

```python
import torch
import torch.onnx
import sys
import os

# Add SimSwap to path
sys.path.append('./SimSwap')

from models.models import create_model
from options.test_options import TestOptions

def convert_simswap_to_onnx():
    """Convert SimSwap model to ONNX format"""
    
    # Parse options (modify as needed)
    opt = TestOptions().parse()
    opt.isTrain = False
    opt.no_simswaplogo = True
    opt.Arc_path = 'arcface_checkpoint.tar'
    opt.pic_a_path = './demo_file/Iron_man.jpg'  # Dummy source
    opt.pic_b_path = './demo_file/multi_people.jpg'  # Dummy target
    opt.pic_specific_path = './demo_file/specific2.png'
    opt.multisepcific_dir = './demo_file/multispecific/'
    opt.name = 'people'
    
    # Create model
    print("Creating SimSwap model...")
    model = create_model(opt)
    model.eval()
    
    # Extract just the generator (face swapping network)
    generator = model.netG
    generator.eval()
    
    # Create dummy inputs
    # SimSwap expects source and target face embeddings
    dummy_source = torch.randn(1, 3, 224, 224)  # Source face
    dummy_target = torch.randn(1, 3, 224, 224)  # Target face
    
    # Export to ONNX
    print("Exporting to ONNX...")
    torch.onnx.export(
        generator,
        (dummy_source, dummy_target),
        "simswap.onnx",
        input_names=['source_face', 'target_face'],
        output_names=['swapped_face'],
        dynamic_axes={
            'source_face': {0: 'batch'},
            'target_face': {0: 'batch'},
            'swapped_face': {0: 'batch'}
        },
        opset_version=11,
        do_constant_folding=True
    )
    
    print("✅ SimSwap exported to simswap.onnx")
    
    # Verify export
    import onnx
    model_onnx = onnx.load("simswap.onnx")
    onnx.checker.check_model(model_onnx)
    print("✅ ONNX model validated")

if __name__ == "__main__":
    convert_simswap_to_onnx()
```

### Run Conversion

```bash
python convert_simswap.py

# Output:
# Creating SimSwap model...
# Exporting to ONNX...
# ✅ SimSwap exported to simswap.onnx
# ✅ ONNX model validated
```

### Simplified Alternative (InsightFace ArcFace)

If SimSwap is too complex, use InsightFace for simpler face swapping:

```python
import torch
import onnx
from insightface.model_zoo import get_model

# Download pre-trained model
model = get_model('buffalo_l')
model.prepare(ctx_id=0)

# The model is already in ONNX format!
# Just copy to your models folder:
# cp ~/.insightface/models/buffalo_l/w600k_r50.onnx models/insightface_swap.onnx
```

---

## GFPGAN (Face Enhancement)

### Download GFPGAN

```bash
# Clone repository
git clone https://github.com/TencentARC/GFPGAN.git
cd GFPGAN

# Install
pip install basicsr
pip install facexlib
pip install -r requirements.txt

# Download pre-trained model
wget https://github.com/TencentARC/GFPGAN/releases/download/v1.3.0/GFPGANv1.3.pth
```

### Convert GFPGAN to ONNX

Create `convert_gfpgan.py`:

```python
import torch
import torch.onnx
from basicsr.archs.rrdbnet_arch import RRDBNet
from gfpganv1_arch import GFPGANv1

def convert_gfpgan_to_onnx():
    """Convert GFPGAN v1.3 to ONNX format"""
    
    # Load GFPGAN model
    print("Loading GFPGAN model...")
    model = GFPGANv1(
        out_size=512,
        num_style_feat=512,
        channel_multiplier=2,
        decoder_load_path=None,
        fix_decoder=False,
        num_mlp=8,
        input_is_latent=True,
        different_w=True,
        narrow=1,
        sft_half=True
    )
    
    # Load checkpoint
    checkpoint = torch.load('GFPGANv1.3.pth', map_location='cpu')
    model.load_state_dict(checkpoint['params_ema'], strict=True)
    model.eval()
    
    # Create dummy input (512x512 face image)
    dummy_input = torch.randn(1, 3, 512, 512)
    
    # Export to ONNX
    print("Exporting to ONNX...")
    torch.onnx.export(
        model,
        dummy_input,
        "gfpgan_v1.3.onnx",
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={
            'input': {0: 'batch', 2: 'height', 3: 'width'},
            'output': {0: 'batch', 2: 'height', 3: 'width'}
        },
        opset_version=11,
        do_constant_folding=True
    )
    
    print("✅ GFPGAN exported to gfpgan_v1.3.onnx")
    
    # Verify
    import onnx
    model_onnx = onnx.load("gfpgan_v1.3.onnx")
    onnx.checker.check_model(model_onnx)
    print("✅ ONNX model validated")
    
    # Get model size
    import os
    size_mb = os.path.getsize("gfpgan_v1.3.onnx") / (1024 * 1024)
    print(f"📦 Model size: {size_mb:.2f} MB")

if __name__ == "__main__":
    convert_gfpgan_to_onnx()
```

### Run Conversion

```bash
cd GFPGAN
python convert_gfpgan.py

# Move to MySubstitute models folder
mv gfpgan_v1.3.onnx ../../MySubstitute/models/
```

### Alternative: CodeFormer (Better Quality)

```bash
# Clone CodeFormer
git clone https://github.com/sczhou/CodeFormer.git
cd CodeFormer

# Download weights
python scripts/download_pretrained_models.py facelib
python scripts/download_pretrained_models.py CodeFormer

# Convert (similar process to GFPGAN)
```

---

## Real-ESRGAN (Super Resolution)

### Download Real-ESRGAN

```bash
# Clone repository
git clone https://github.com/xinntao/Real-ESRGAN.git
cd Real-ESRGAN

# Install
pip install basicsr
pip install facexlib
pip install gfpgan
pip install -r requirements.txt

# Download pre-trained models
wget https://github.com/xinntao/Real-ESRGAN/releases/download/v0.2.1/RealESRGAN_x2plus.pth
wget https://github.com/xinntao/Real-ESRGAN/releases/download/v0.1.0/RealESRGAN_x4plus.pth
```

### Convert Real-ESRGAN to ONNX

Create `convert_realesrgan.py`:

```python
import torch
import torch.onnx
from basicsr.archs.rrdbnet_arch import RRDBNet

def convert_realesrgan_to_onnx(scale=2):
    """Convert Real-ESRGAN to ONNX format
    
    Args:
        scale: Upscaling factor (2 or 4)
    """
    
    print(f"Converting Real-ESRGAN x{scale}...")
    
    # Create model
    model = RRDBNet(
        num_in_ch=3,
        num_out_ch=3,
        num_feat=64,
        num_block=23,
        num_grow_ch=32,
        scale=scale
    )
    
    # Load checkpoint
    model_path = f'RealESRGAN_x{scale}plus.pth'
    checkpoint = torch.load(model_path, map_location='cpu')
    
    # Load state dict (handle 'params' or 'params_ema' keys)
    if 'params_ema' in checkpoint:
        model.load_state_dict(checkpoint['params_ema'], strict=True)
    elif 'params' in checkpoint:
        model.load_state_dict(checkpoint['params'], strict=True)
    else:
        model.load_state_dict(checkpoint, strict=True)
    
    model.eval()
    
    # Create dummy input (variable size)
    dummy_input = torch.randn(1, 3, 256, 256)
    
    # Export to ONNX
    output_path = f"realesrgan_x{scale}.onnx"
    print(f"Exporting to {output_path}...")
    
    torch.onnx.export(
        model,
        dummy_input,
        output_path,
        input_names=['input'],
        output_names=['output'],
        dynamic_axes={
            'input': {0: 'batch', 2: 'height', 3: 'width'},
            'output': {0: 'batch', 2: 'height', 3: 'width'}
        },
        opset_version=11,
        do_constant_folding=True
    )
    
    print(f"✅ Real-ESRGAN x{scale} exported to {output_path}")
    
    # Verify
    import onnx
    model_onnx = onnx.load(output_path)
    onnx.checker.check_model(model_onnx)
    print("✅ ONNX model validated")
    
    # Simplify (optional but recommended)
    from onnxsim import simplify
    model_simp, check = simplify(model_onnx)
    if check:
        output_simp = f"realesrgan_x{scale}_simplified.onnx"
        onnx.save(model_simp, output_simp)
        print(f"✅ Simplified model saved to {output_simp}")
    
    # Get model size
    import os
    size_mb = os.path.getsize(output_path) / (1024 * 1024)
    print(f"📦 Model size: {size_mb:.2f} MB")

if __name__ == "__main__":
    # Convert both x2 and x4 versions
    convert_realesrgan_to_onnx(scale=2)
    convert_realesrgan_to_onnx(scale=4)
```

### Run Conversion

```bash
cd Real-ESRGAN
python convert_realesrgan.py

# Move to MySubstitute models folder
mv realesrgan_x2.onnx ../../MySubstitute/models/
mv realesrgan_x4.onnx ../../MySubstitute/models/
```

---

## MediaPipe (Person Segmentation)

MediaPipe models are already available in ONNX format!

### Download MediaPipe Selfie Segmentation

```bash
# Option 1: Download from Google's CDN
wget https://storage.googleapis.com/mediapipe-models/image_segmenter/selfie_segmenter/float16/latest/selfie_segmenter.tflite

# Convert TFLite to ONNX (requires tf2onnx)
pip install tf2onnx

python -m tf2onnx.convert \
    --tflite selfie_segmenter.tflite \
    --output mediapipe_selfie_segmentation.onnx \
    --opset 11
```

### Or Use Pre-converted Version

```bash
# Download from Hugging Face (community conversions)
wget https://huggingface.co/models/mediapipe/selfie_segmentation/resolve/main/model.onnx \
    -O mediapipe_selfie_segmentation.onnx
```

### Verify MediaPipe Model

```python
import onnx

# Load model
model = onnx.load("mediapipe_selfie_segmentation.onnx")

# Check model
onnx.checker.check_model(model)
print("✅ MediaPipe model is valid")

# Print input/output info
print("\nInputs:")
for input in model.graph.input:
    print(f"  Name: {input.name}")
    print(f"  Shape: {[d.dim_value for d in input.type.tensor_type.shape.dim]}")

print("\nOutputs:")
for output in model.graph.output:
    print(f"  Name: {output.name}")
    print(f"  Shape: {[d.dim_value for d in output.type.tensor_type.shape.dim]}")
```

---

## Alternative Models

### FaceSwap - InsightFace (Simpler than SimSwap)

```python
# Install InsightFace
pip install insightface

# Download and use
from insightface.app import FaceAnalysis

app = FaceAnalysis(providers=['CUDAExecutionProvider', 'CPUExecutionProvider'])
app.prepare(ctx_id=0, det_size=(640, 640))

# Models are downloaded to ~/.insightface/models/
# Copy ONNX files to MySubstitute:
# cp ~/.insightface/models/buffalo_l/*.onnx models/
```

### Face Enhancement - RestoreFormer

```bash
git clone https://github.com/wzhouxiff/RestoreFormer.git
cd RestoreFormer

# Download checkpoint
# https://github.com/wzhouxiff/RestoreFormer/releases

# Convert (similar to GFPGAN)
```

### Super Resolution - SwinIR (Transformer-based)

```bash
git clone https://github.com/JingyunLiang/SwinIR.git
cd SwinIR

# Download pre-trained models
python main_download_pretrained_models.py

# Convert to ONNX
```

---

## Testing ONNX Models

### Verify Model Structure

```python
import onnx

model = onnx.load("model.onnx")

# Check validity
onnx.checker.check_model(model)
print("✅ Model is valid")

# Print model info
print(f"\nProducer: {model.producer_name}")
print(f"Graph name: {model.graph.name}")
print(f"Inputs: {len(model.graph.input)}")
print(f"Outputs: {len(model.graph.output)}")

# Print input details
for input in model.graph.input:
    print(f"\nInput: {input.name}")
    print(f"  Type: {input.type.tensor_type.elem_type}")
    shape = [d.dim_value if d.dim_value > 0 else 'dynamic' 
             for d in input.type.tensor_type.shape.dim]
    print(f"  Shape: {shape}")
```

### Test Inference

```python
import onnxruntime as ort
import numpy as np

# Load model
session = ort.InferenceSession("model.onnx")

# Get input name and shape
input_name = session.get_inputs()[0].name
input_shape = session.get_inputs()[0].shape
print(f"Input: {input_name}, Shape: {input_shape}")

# Create dummy input
dummy_input = np.random.randn(*input_shape).astype(np.float32)

# Run inference
output = session.run(None, {input_name: dummy_input})
print(f"✅ Inference successful!")
print(f"Output shape: {output[0].shape}")
```

### Benchmark Performance

```python
import time
import numpy as np
import onnxruntime as ort

def benchmark_model(model_path, iterations=100):
    """Benchmark ONNX model inference speed"""
    
    # Load model
    session = ort.InferenceSession(model_path)
    input_name = session.get_inputs()[0].name
    input_shape = session.get_inputs()[0].shape
    
    # Create dummy input
    dummy_input = np.random.randn(*input_shape).astype(np.float32)
    
    # Warmup
    for _ in range(10):
        session.run(None, {input_name: dummy_input})
    
    # Benchmark
    start = time.time()
    for _ in range(iterations):
        session.run(None, {input_name: dummy_input})
    end = time.time()
    
    avg_time = (end - start) / iterations * 1000  # ms
    fps = 1000 / avg_time
    
    print(f"📊 Benchmark Results:")
    print(f"  Average inference time: {avg_time:.2f} ms")
    print(f"  FPS: {fps:.2f}")
    
    return avg_time, fps

# Test
benchmark_model("realesrgan_x2.onnx")
```

---

## Troubleshooting

### Issue: "Unsupported ONNX opset version"

**Solution:**
```python
# Use older opset version
torch.onnx.export(
    model, dummy_input, "model.onnx",
    opset_version=11  # Try 11, 12, or 13
)
```

### Issue: "Model has dynamic shapes"

**Solution:**
```python
# Fix input shape
torch.onnx.export(
    model, dummy_input, "model.onnx",
    input_names=['input'],
    output_names=['output'],
    dynamic_axes={
        'input': {0: 'batch'},  # Only batch dimension dynamic
        'output': {0: 'batch'}
    }
)
```

### Issue: "Model too large"

**Solution:**
```python
# Simplify and optimize
from onnxsim import simplify
import onnx

model = onnx.load("model.onnx")
model_simp, check = simplify(model)

if check:
    onnx.save(model_simp, "model_optimized.onnx")
    
# Or quantize to FP16
from onnxruntime.quantization import quantize_dynamic

quantize_dynamic(
    "model.onnx",
    "model_quantized.onnx",
    weight_type=QuantType.QUInt8
)
```

### Issue: "CUDA out of memory"

**Solution:**
```python
# Use CPU for conversion
model = model.cpu()
dummy_input = dummy_input.cpu()

# Or reduce batch size
dummy_input = torch.randn(1, 3, 256, 256)  # Batch size = 1
```

---

## Model Checklist

Before deploying to MySubstitute, verify:

- [ ] Model exports without errors
- [ ] ONNX validation passes (`onnx.checker.check_model`)
- [ ] Input/output names are clear and documented
- [ ] Model size is reasonable (< 500MB ideally)
- [ ] Inference test succeeds with dummy data
- [ ] Performance is acceptable (FPS > 5 for real-time)
- [ ] Model works with ONNX Runtime CPU
- [ ] Model works with ONNX Runtime GPU (if CUDA available)

---

## Quick Reference Table

| Model | PyTorch File | ONNX Output | Input Size | Output Size |
|-------|--------------|-------------|------------|-------------|
| SimSwap | `checkpoints/people/latest_net_G.pth` | `simswap.onnx` | 224x224x3 | 224x224x3 |
| GFPGAN | `GFPGANv1.3.pth` | `gfpgan_v1.3.onnx` | 512x512x3 | 512x512x3 |
| Real-ESRGAN x2 | `RealESRGAN_x2plus.pth` | `realesrgan_x2.onnx` | Variablex3 | 2x input |
| Real-ESRGAN x4 | `RealESRGAN_x4plus.pth` | `realesrgan_x4.onnx` | Variablex3 | 4x input |
| MediaPipe | `selfie_segmenter.tflite` | `mediapipe_selfie_segmentation.onnx` | 256x256x3 | 256x256x1 |

---

## Batch Conversion Script

Save as `convert_all_models.sh`:

```bash
#!/bin/bash

echo "🚀 Converting all models to ONNX..."

# Create output directory
mkdir -p onnx_models

# Convert SimSwap
echo "📦 Converting SimSwap..."
cd SimSwap && python convert_simswap.py && cd ..
mv SimSwap/simswap.onnx onnx_models/

# Convert GFPGAN
echo "📦 Converting GFPGAN..."
cd GFPGAN && python convert_gfpgan.py && cd ..
mv GFPGAN/gfpgan_v1.3.onnx onnx_models/

# Convert Real-ESRGAN
echo "📦 Converting Real-ESRGAN..."
cd Real-ESRGAN && python convert_realesrgan.py && cd ..
mv Real-ESRGAN/realesrgan_*.onnx onnx_models/

echo "✅ All models converted!"
echo "📁 Models saved to: onnx_models/"

# Copy to MySubstitute
echo "📋 Copying to MySubstitute/models/..."
cp onnx_models/*.onnx ../MySubstitute/models/

echo "🎉 Done! Models ready to use."
```

Run with:
```bash
chmod +x convert_all_models.sh
./convert_all_models.sh
```

---

**Last Updated:** November 9, 2025
**Author:** MySubstitute Development Team
