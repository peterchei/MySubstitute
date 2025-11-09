# Person Replacement Error Fix - Summary

**Date**: November 9, 2025  
**Issue**: "No target person image set for full body replacement!" error  
**Status**: ✅ FIXED

---

## What Was Fixed

### Problem
When selecting "AI Face Swap" or "AI Full Body Replace" filters, the application would show an error:
```
No target person image set for full body replacement!
No target person image set for face swap!
```

The console would flood with error messages and the filter wouldn't work.

### Solution
Made three key improvements:

1. **Graceful Error Handling** - No more console errors
   - Changed from `std::cerr` error messages to helpful on-screen overlays
   - Application continues working even without target images
   - Shows yellow text overlay with helpful instructions

2. **Auto-Detection of Target Images** - Smart image loading
   - Automatically looks for `assets/default_face.jpg`
   - Automatically looks for `assets/default_person.jpg`
   - Falls back to passthrough mode if images not found
   - Shows helpful tip messages in console

3. **Placeholder Image Generator** - Easy setup
   - Created `create_placeholder_images.py` script
   - Created `create_placeholders.bat` for easy execution
   - Generates simple placeholder images for testing
   - Users can replace with their own photos

---

## Files Changed

### 1. `src/main.cpp`
**Changes:**
- Added `#include <fstream>` for file checking
- Updated `person_replace_face_swap` handler to check for `assets/default_face.jpg`
- Updated `person_replace_full_body` handler to check for `assets/default_person.jpg`
- Added helpful console messages when images are found or missing

**Before:**
```cpp
processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
processor->SetBlendStrength(0.9f);
g_processor = std::move(processor);
```

**After:**
```cpp
processor->SetReplacementMode(PersonReplacementProcessor::FACE_SWAP);
processor->SetBlendStrength(0.9f);

// Try to load default target image
std::string defaultTargetFace = "assets/default_face.jpg";
if (std::ifstream(defaultTargetFace).good()) {
    processor->SetTargetPersonImage(defaultTargetFace);
    std::cout << "[OnFilterChanged] Using default target face: " << defaultTargetFace << std::endl;
} else {
    std::cout << "[OnFilterChanged] No target face found. Face swap will use fallback mode." << std::endl;
    std::cout << "[OnFilterChanged] Tip: Place a face image at 'assets/default_face.jpg'!" << std::endl;
}

g_processor = std::move(processor);
```

### 2. `src/ai/person_replacement_processor.cpp`
**Changes:**
- Replaced error messages with helpful on-screen overlays
- Shows yellow text with instructions when no target image is set
- Message tells user exactly where to place target images

**Before:**
```cpp
if (!m_targetPersonImage.empty()) {
    result = ReplaceFullBody(frame, m_targetPersonImage);
} else {
    std::cerr << "No target person image set for full body replacement!" << std::endl;
    result = frame.clone();
}
```

**After:**
```cpp
if (!m_targetPersonImage.empty()) {
    result = ReplaceFullBody(frame, m_targetPersonImage);
} else {
    // No target image - show helpful message
    result = frame.clone();
    std::string msg = "No target person image set. Place image at assets/default_person.jpg";
    cv::putText(result, msg, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 
               0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
}
```

---

## New Files Created

### 1. `create_placeholder_images.py`
Python script that generates simple placeholder images:
- **Face placeholder**: 512x512 image with simple drawn face
- **Person placeholder**: 512x768 image with simple stick figure
- Saves to `assets/default_face.jpg` and `assets/default_person.jpg`

### 2. `create_placeholders.bat`
Windows batch file for easy execution:
- Checks if Python is installed
- Installs OpenCV if needed
- Runs the placeholder creation script
- Shows success message and instructions

### 3. `PERSON_REPLACEMENT_FIX.md`
Quick fix guide with:
- Problem description
- 3 solution options
- Step-by-step instructions
- Tips for best results
- File locations

---

## How to Use (User Instructions)

### Quick Start - Generate Placeholders
```powershell
# 1. Generate placeholder images
.\create_placeholders.bat

# 2. Run MySubstitute
.\run.bat

# 3. Open Preview Window
# Right-click tray icon → "Show Preview"

# 4. Select "AI Face Swap (DeepSeek)" or "AI Full Body Replace"
# You'll see the placeholder being used!
```

### Best Results - Use Your Own Photos
```powershell
# 1. Create assets folder (if needed)
mkdir assets

# 2. Add your images
# - assets/default_face.jpg (frontal face photo, 512x512+)
# - assets/default_person.jpg (full body photo, 512x768+)

# 3. Run MySubstitute
.\run.bat

# 4. Select Person Replacement filters
# Now uses your real photos!
```

### No Setup - Just Test It
```powershell
# 1. Run MySubstitute
.\run.bat

# 2. Select any Person Replacement filter
# You'll see a helpful message showing where to add images
# Filter works as passthrough until you add images
```

---

## Technical Details

### On-Screen Message Display
When no target image is set, the filter shows:
- **Yellow text** (easy to see, not intrusive)
- **Top-left corner** (cv::Point(10, 30))
- **Clear instructions**: "No target person image set. Place image at assets/default_person.jpg"
- **Font**: Hershey Simplex, 0.5 scale, 1px thickness, anti-aliased

### Console Messages
The application now shows helpful guidance:
```
[OnFilterChanged] No target face found. Face swap will use fallback mode.
[OnFilterChanged] Tip: Place a face image at 'assets/default_face.jpg' for better results!
```

Or when images are found:
```
[OnFilterChanged] Using default target face: assets/default_face.jpg
```

### Image Detection
Uses `std::ifstream` to check if files exist:
```cpp
if (std::ifstream(defaultTargetImage).good()) {
    processor->SetTargetPersonImage(defaultTargetImage);
} else {
    // Show helpful message instead of error
}
```

---

## Testing Results

✅ **Build**: Successful (0 warnings, 0 errors)  
✅ **Application Launch**: Successful  
✅ **Face Swap Filter**: Works with helpful message when no image  
✅ **Full Body Replace**: Works with helpful message when no image  
✅ **Face Enhancement**: Works normally (no target needed)  
✅ **Super Resolution**: Works normally (no target needed)

---

## User Benefits

### Before Fix
- ❌ Confusing error messages in console
- ❌ Unclear what to do
- ❌ No guidance on where to place images
- ❌ Filter appeared broken

### After Fix
- ✅ Clear on-screen instructions
- ✅ Helpful console tips
- ✅ Automatic image detection
- ✅ Easy placeholder generation
- ✅ Filter works gracefully without errors

---

## Next Steps for Users

### Immediate Testing (5 minutes)
1. Run `.\create_placeholders.bat`
2. Launch application
3. Test all 4 Person Replacement filters
4. See placeholders in action

### Better Quality (15 minutes)
1. Take a good frontal face photo
2. Take a full-body photo
3. Save as `assets/default_face.jpg` and `assets/default_person.jpg`
4. Restart application
5. Enjoy better replacement quality

### Professional Quality (with AI models)
1. See `docs/model_conversion_guide.md`
2. Download and convert AI models (SimSwap, GFPGAN, Real-ESRGAN)
3. Load models in application
4. Get DeepFake-quality results

---

## Related Documentation

- **Quick Fix Guide**: `PERSON_REPLACEMENT_FIX.md`
- **Full Documentation**: `docs/person_replacement_processor.md`
- **Quick Start**: `docs/person_replacement_quickstart.md`
- **Model Conversion**: `docs/model_conversion_guide.md`
- **Summary**: `docs/PERSON_REPLACEMENT_SUMMARY.md`

---

**Result**: Person Replacement filters now work smoothly with or without target images! 🎉
