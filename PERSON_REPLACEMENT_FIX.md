# Person Replacement Quick Fix Guide

## Problem: "No target person image set for full body replacement!"

This error occurs when you select the **AI Face Swap** or **AI Full Body Replace** filters without providing a target image.

## Quick Solution

### Option 1: Generate Placeholder Images (Recommended for Testing)

Run the placeholder generation script:

```powershell
.\create_placeholders.bat
```

This will:
1. Install required Python packages (if needed)
2. Generate placeholder images in `assets/` folder
3. Create `assets/default_face.jpg` (for face swap)
4. Create `assets/default_person.jpg` (for full body replacement)

### Option 2: Use Your Own Images

1. Create an `assets` folder if it doesn't exist
2. Add your images:
   - **For Face Swap**: Save as `assets/default_face.jpg`
     - Should be a frontal face photo
     - At least 512x512 pixels
     - Well-lit and clear
   
   - **For Full Body Replace**: Save as `assets/default_person.jpg`
     - Should show the entire person
     - At least 512x768 pixels (portrait orientation)
     - Good lighting and clear background

### Option 3: Just Test It Out (No Setup Required)

The filters now work without target images! They will:
- Display the original camera feed with a yellow message
- Show where to place the target image
- Work as a passthrough filter until you add images

## How to Use

1. **Launch MySubstitute**:
   ```powershell
   .\run.bat
   ```

2. **Open Preview Window**:
   - Right-click system tray icon
   - Select "Show Preview"

3. **Select Filter**:
   - **AI Face Swap (DeepSeek)** - Replaces your face with target face
   - **AI Full Body Replace** - Replaces your entire body
   - **AI Face Enhancement (GFPGAN)** - Improves face quality (no target needed)
   - **AI Super Resolution** - Upscales video (no target needed)

4. **See Results**:
   - With target images: AI-powered replacement
   - Without target images: Helpful message showing where to add images

## Tips for Best Results

### Face Swap Target Images
- Use frontal-facing photos
- Good lighting (avoid shadows)
- Neutral expression works best
- High resolution (512x512 or larger)

### Full Body Target Images
- Entire person visible head-to-toe
- Standing position works best
- Clean background preferred
- Portrait orientation (taller than wide)

### Enhancement Filters (No Target Needed)
- **Face Enhancement**: Works immediately, improves face clarity
- **Super Resolution**: Works immediately, upscales video 2x

## What Changed

We've updated the PersonReplacementProcessor to:

1. ✅ **Gracefully handle missing target images** - No more errors, just helpful messages
2. ✅ **Display instructions on screen** - Shows where to place target images
3. ✅ **Auto-detect images** - Automatically uses `assets/default_face.jpg` and `assets/default_person.jpg`
4. ✅ **Optional target images** - Enhancement and super-resolution work without targets

## File Locations

```
MySubstitute/
├── assets/                      # Target images folder
│   ├── default_face.jpg        # For face swap (create this)
│   └── default_person.jpg      # For full body replace (create this)
├── create_placeholders.bat     # Generate placeholder images
└── create_placeholder_images.py # Python script for placeholders
```

## Advanced: Using AI Models

For professional-quality results, you can download AI models:

1. See: `docs/person_replacement_processor.md`
2. See: `docs/model_conversion_guide.md`
3. Download models like SimSwap, GFPGAN, Real-ESRGAN
4. Convert to ONNX format
5. Load in application for AI-powered processing

**Note**: OpenCV fallback implementations work immediately without AI models!

## Support

- **Full Documentation**: See `docs/person_replacement_processor.md`
- **Quick Start**: See `docs/person_replacement_quickstart.md`
- **Model Guide**: See `docs/model_conversion_guide.md`

---

**Status**: ✅ Fixed! Person replacement now works with or without target images.
