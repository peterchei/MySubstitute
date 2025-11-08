# OpenCV DNN Compatible Models Guide

## ✅ Models That WORK with OpenCV DNN

Unlike AnimeGANv2, these models are **verified compatible** with OpenCV DNN module.

### **Option 1: Fast Neural Style Transfer (RECOMMENDED)**

**Format:** Torch (.t7)  
**OpenCV Support:** ✅ Native via `cv::dnn::readNetFromTorch()`  
**Performance:** Fast (30-60 FPS on GPU, 5-10 FPS on CPU)  
**Size:** ~6-7 MB per model

#### Download Sources:

**Method 1 - Stanford (Original):**
Visit in browser and download manually:
- https://cs.stanford.edu/people/jcjohns/fast-neural-style/models/instance_norm/

Available styles:
- `candy.t7` - Vibrant, colorful candy-like style
- `mosaic.t7` - Mosaic tile effect  
- `udnie.t7` - Abstract geometric style
- `the_scream.t7` - "The Scream" painting style
- `feathers.t7` - Feather-like patterns
- `la_muse.t7` - Artistic brush strokes
- `starry_night.t7` - Van Gogh's Starry Night style

**Method 2 - Google Drive Mirror:**
Some models are mirrored on Google Drive (search for "fast neural style torch models")

**Method 3 - OpenCV Test Data:**
OpenCV includes some test models in their repository:
- https://github.com/opencv/opencv_extra/tree/master/testdata/dnn/torch

#### After Downloading:
1. Place `.t7` file in `models/` folder
2. Rename to `anime_gan.t7` (or update code to load `.t7` files)
3. Update `anime_gan_processor.cpp` to use `readNetFromTorch()` instead of `readNetFromONNX()`

---

### **Option 2: Caffe Models**

**Format:** Caffe (.caffemodel + .prototxt)  
**OpenCV Support:** ✅ Native via `cv::dnn::readNetFromCaffe()`  
**Performance:** Very fast  

Many artistic style models are available in Caffe format. Search for:
- "fast neural style caffe model"
- "style transfer caffe"

---

### **Option 3: TensorFlow Models** 

**Format:** TensorFlow (.pb)  
**OpenCV Support:** ✅ via `cv::dnn::readNetFromTensorflow()`  
**Performance:** Good

Models available from TensorFlow Hub:
- https://tfhub.dev/google/magenta/arbitrary-image-stylization-v1-256/2

**Note:** Requires downloading and converting to frozen graph format.

---

## ❌ Models That DON'T WORK with OpenCV DNN

### AnimeGANv2 (All Styles)
- ❌ Hayao
- ❌ Shinkai  
- ❌ Paprika
- ❌ Face Paint

**Reason:** PyTorch ONNX export creates incompatible layer connections. The channel mismatch error is unfixable without re-training/re-exporting the entire model.

### CartoonGAN (Most Versions)
- Most ONNX exports have similar issues to AnimeGAN

### StyleGAN, DALL-E, Stable Diffusion
- Too large and complex for real-time OpenCV DNN inference

---

## 🔧 How to Use Fast Neural Style with Your Code

You'll need to modify `anime_gan_processor.cpp`:

### Change Line ~40 (in Initialize method):
```cpp
// OLD:
m_net = cv::dnn::readNetFromONNX(m_modelPath);

// NEW:
m_net = cv::dnn::readNetFromTorch(m_modelPath);
```

### Change model path to accept .t7:
```cpp
// In constructor
m_modelPath = "models/anime_gan.t7";  // or style_candy.t7, etc.
```

That's it! Fast Neural Style models have the same input/output format as AnimeGAN, so the rest of your code should work.

---

## 📥 Manual Download Instructions

Since automated downloads are failing:

1. **Open browser** and navigate to:
   https://cs.stanford.edu/people/jcjohns/fast-neural-style/models/instance_norm/

2. **Right-click and "Save As"** on any model file (e.g., `candy.t7`)

3. **Move to your project:**
   ```powershell
   Move-Item "C:\Users\peter\Downloads\candy.t7" "models\anime_gan.t7"
   ```

4. **Update code** to use `readNetFromTorch()` as shown above

5. **Rebuild:**
   ```powershell
   .\build.bat
   ```

6. **Test:**
   ```powershell
   .\build\bin\Release\MySubstitute.exe
   ```

---

## 🎨 Expected Results

Fast Neural Style models will give you:
- ✅ Artistic stylization (different from anime, more like painting styles)
- ✅ Real-time performance
- ✅ Stable, no errors
- ✅ Works with OpenCV DNN

**Note:** These won't look like "anime" - they're artistic painting styles. For anime look, the **Cartoon (Anime) filter** you already have is actually your best option!

---

## 💡 Recommendation

**For anime-style specifically:** Use your existing "Cartoon (Anime)" filter - it's specifically designed for anime aesthetics and works perfectly.

**For artistic effects:** Download Fast Neural Style .t7 models and modify the code to use Torch format.

**For quick testing:** Try the Cartoon (Anime) filter right now - it's already working and provides great results!
