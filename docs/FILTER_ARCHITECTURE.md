# MySubstitute Filter Architecture - Now with AI Anime!

## Complete Filter Pipeline

```
┌─────────────────┐
│  Physical       │
│  Camera         │
│  (Webcam)       │
└────────┬────────┘
         │ Raw Video
         ▼
┌─────────────────┐
│ CameraCapture   │
│ (DirectShow)    │
└────────┬────────┘
         │ Frame Buffer
         ▼
┌─────────────────────────────────────────────────────────────┐
│                    AI Processor Layer                       │
│                                                             │
│  ┌────────────────┐  ┌──────────────────┐  ┌────────────┐ │
│  │ No Effects     │  │ Face Filters     │  │  Virtual   │ │
│  │ (Passthrough)  │  │ • Glasses        │  │ Background │ │
│  │                │  │ • Hats           │  │            │ │
│  │ 30+ FPS (CPU)  │  │ • Speech Bubbles │  │ 30+ FPS    │ │
│  └────────────────┘  └──────────────────┘  └────────────┘ │
│                                                             │
│  ┌────────────────┐  ┌──────────────────┐  ┌────────────┐ │
│  │ Cartoon Simple │  │ Cartoon Detailed │  │  Cartoon   │ │
│  │ • 6 colors     │  │ • 5 colors       │  │   Anime    │ │
│  │ • 1.5x sat     │  │ • 1.6x sat       │  │ • 6 colors │ │
│  │                │  │                  │  │ • 1.8x sat │ │
│  │ 30+ FPS (CPU)  │  │ 30+ FPS (CPU)    │  │ 30+ FPS    │ │
│  └────────────────┘  └──────────────────┘  └────────────┘ │
│                                                             │
│  ┌────────────────┐  ┌──────────────────┐  ┌────────────┐ │
│  │ Cartoon        │  │ Pixel Art        │  │  Pixel Art │ │
│  │ Buffered       │  │ (Minecraft)      │  │   Anime    │ │
│  │ • 5 frame buf  │  │ • 8x8 pixels     │  │ • 4x4 pix  │ │
│  │ • Temporal avg │  │ • 6 colors       │  │ • Anime pal│ │
│  │                │  │ • 1.4x sat       │  │ • 8 colors │ │
│  │ 26+ FPS (CPU)  │  │ 30+ FPS (CPU)    │  │ 30+ FPS    │ │
│  └────────────────┘  └──────────────────┘  └────────────┘ │
│                                                             │
│  ┌────────────────┐  ┌──────────────────────────────────┐ │
│  │ Pixel Art      │  │ ⭐ AnimeGAN (AI - NEW!)         │ │
│  │ Retro 16-bit   │  │ • Deep Learning (ONNX)          │ │
│  │ • 6x6 pixels   │  │ • GPU Accelerated (CUDA)        │ │
│  │ • 5 colors     │  │ • 512x512 inference             │ │
│  │ • Dithering    │  │ • 3 styles: Hayao/Shinkai/Paprika│ │
│  │                │  │ • 85% anime blend               │ │
│  │ 30+ FPS (CPU)  │  │ • 70% temporal smoothing        │ │
│  │                │  │                                  │ │
│  └────────────────┘  │ 20-100ms (GPU REQUIRED!)        │ │
│                      │ 10-30 FPS on RTX 3060+          │ │
│                      └──────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────┘
         │ Processed Frame
         ▼
┌─────────────────┐
│ Virtual Camera  │
│ Filter          │
│ (DirectShow)    │
└────────┬────────┘
         │ Virtual Device
         ▼
┌─────────────────┐
│ Applications    │
│ • Zoom          │
│ • Teams         │
│ • Browsers      │
│ • OBS           │
└─────────────────┘
```

## Filter Comparison Table

| Filter Name | Method | Speed (CPU) | Speed (GPU) | Quality | Memory | Requirements |
|-------------|--------|-------------|-------------|---------|--------|--------------|
| **No Effects** | Passthrough | 30+ FPS | N/A | Original | <1 MB | None |
| **Face Filters** | OpenCV Haar | 30+ FPS | N/A | Good | ~5 MB | None |
| **Person Detector**| MediaPipe DNN | 30+ FPS | N/A | Excellent | ~2 MB | None |
| **Virtual Background** | Segmentation | 25+ FPS | N/A | Good | ~10 MB | None |
| **Cartoon (Simple)** | Bilateral+Edges | 30+ FPS | N/A | Good | <1 MB | None |
| **Cartoon (Detailed)** | Bilateral+Edges | 30+ FPS | N/A | Good | <1 MB | None |
| **Cartoon (Anime)** | Bilateral+Edges | 30+ FPS | N/A | Good | <1 MB | None |
| **Cartoon Buffered** | Multi-frame avg | 26+ FPS | N/A | Better | ~5 MB | None |
| **Pixel Art (Minecraft)** | Quantize+Edges | 30+ FPS | N/A | Stylized | <1 MB | None |
| **Pixel Art (Anime)** | Quantize+Palette | 30+ FPS | N/A | Stylized | <1 MB | None |
| **Pixel Art (Retro)** | Dithering | 30+ FPS | N/A | Stylized | <1 MB | None |
| **⭐ AnimeGAN** | Deep Learning | 1-2 FPS ❌ | 10-30 FPS ✅ | Excellent | ~10 MB | **NVIDIA GPU** |

## AnimeGAN Processing Pipeline

```
Input Frame (640x480 BGR)
          │
          ▼
┌─────────────────────┐
│ Preprocess          │
│ • Resize to 512x512 │
│ • BGR → RGB         │
│ • Normalize [-1,1]  │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Create Blob         │
│ [1, 3, 512, 512]    │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ ONNX Model          │
│ (GPU Inference)     │
│                     │
│ AnimeGANv2:         │
│ • Generator network │
│ • 8.5 MB model      │
│ • CUDA backend      │
│                     │
│ ⏱ 20-100ms         │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Postprocess         │
│ • Denormalize       │
│ • RGB → BGR         │
│ • Resize to 640x480 │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Blend with Original │
│ • 85% anime         │
│ • 15% original      │
└──────────┬──────────┘
           │
           ▼
┌─────────────────────┐
│ Temporal Smoothing  │
│ • 70% current frame │
│ • 30% previous frame│
└──────────┬──────────┘
           │
           ▼
Output Frame (640x480 BGR)
```

## Performance Characteristics

### CPU-Based Filters (Image Processing)
```
Processing Time: <1ms per filter operation
Total Pipeline: 5-10ms
Frame Rate: 30-60 FPS (real-time)
Latency: Negligible (<10ms)

Suitable for: ALL users
```

### GPU-Based Filter (Deep Learning)
```
Processing Time: 20-100ms per frame
GPU Memory: ~500 MB
Frame Rate: 10-30 FPS (real-time capable)
Latency: Noticeable (20-100ms)

Suitable for: Users with NVIDIA GPU (RTX 2060+)
```

## When to Use Each Filter

### Use CPU Filters When:
- ✅ No GPU available (laptop users)
- ✅ Need maximum performance (30+ FPS)
- ✅ Minimum latency required (<10ms)
- ✅ Multiple applications running
- ✅ Good-enough quality acceptable

**Best Options**: Cartoon (Simple/Detailed/Anime), Pixel Art variants

### Use AnimeGAN When:
- ✅ Have NVIDIA GPU with CUDA
- ✅ Want highest quality anime conversion
- ✅ Can accept 20-100ms latency
- ✅ Willing to download 8.5 MB model
- ✅ Professional streaming/recording

**Best Option**: AnimeGAN with appropriate style (Hayao/Shinkai/Paprika)

## Filter Selection Flow

```
Start
  │
  ▼
Do you have NVIDIA GPU?
  │
  ├─ No ──────────────────┐
  │                       ▼
  │              Use Cartoon Filters
  │              (Simple/Detailed/Anime)
  │              • 30+ FPS on CPU
  │              • Good quality
  │              • Zero config
  │
  └─ Yes ─────────────────┐
                          ▼
           Want highest quality anime?
                          │
                 ├─ No ───┴─ Yes ─┐
                 │                 ▼
                 │        Download AnimeGAN model
                 │        Use AnimeGAN filter
                 │        • 10-30 FPS on GPU
                 │        • Excellent quality
                 │        • 8.5 MB download
                 │
                 ▼
        Use Cartoon or Pixel Art
        (Still good quality!)
```

## Total Filter Count: 11 Filters

1. No Effects (passthrough with captions)
2. Face Filters (glasses, hats, speech)
3. Person Detector (Deep Learning)
4. Virtual Background
4. Cartoon (Simple)
5. Cartoon (Detailed)
6. Cartoon (Anime)
7. Cartoon Buffered
8. Pixel Art (Minecraft)
9. Pixel Art (Anime)
10. Pixel Art (Retro 16-bit)
11. **⭐ AnimeGAN (AI - GPU)** ← NEW!

## Architecture Highlights

### Strengths
- ✅ **Modular**: Easy to add new filters
- ✅ **Fallback**: GPU filters degrade gracefully to CPU
- ✅ **Thread-safe**: Mutex protection on filter switching
- ✅ **Flexible**: Runtime filter selection
- ✅ **Documented**: Comprehensive setup guides

### Design Patterns
- **Factory Pattern**: `std::make_unique<FilterType>()`
- **Strategy Pattern**: `AIProcessor` interface
- **RAII**: Smart pointers for automatic cleanup
- **Thread Safety**: Mutex guards (`g_processorMutex`)

### DirectShow Integration
```
MySubstitute.exe
    └─ Virtual Camera DLL (DirectShow filter)
        ├─ Registered in Windows registry
        ├─ Appears as "MySubstitute Virtual Camera"
        └─ Accessed by all applications

Zoom/Teams/Browser → DirectShow → Virtual Camera → MySubstitute → Processed Frame
```

## Next Steps

1. **Download a model**: Run `.\download_anime_model.ps1`
2. **Try the filter**: Select "Anime GAN (AI - GPU)" 
3. **Compare quality**: Test vs Cartoon filters
4. **Tune performance**: Adjust resolution if needed
5. **Experiment**: Try different styles (Hayao/Shinkai/Paprika)

Enjoy your new AI-powered anime filter! 🎨✨
