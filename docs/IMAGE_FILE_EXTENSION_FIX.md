# Image File Extension Issue - Fixed

## Problem
You placed images in the `assets/` folder but the application still couldn't find them.

## Root Cause
The image files had **double extensions**:
- ❌ `default_face.jpg.jpg` (wrong)
- ❌ `default_person.jpg.jpg` (wrong)

But the application was looking for:
- ✅ `assets/default_face.jpg`
- ✅ `assets/default_person.jpg`

This happens when Windows hides file extensions by default, so when you saved the file as "default_face.jpg", Windows added another `.jpg` extension.

## Solution Applied

### 1. Renamed the Files
```powershell
# Removed the double extension
default_face.jpg.jpg → default_face.jpg
default_person.jpg.jpg → default_person.jpg
```

### 2. Added Better Debugging
Updated the application to show:
- ✓ Current working directory
- ✓ Full path where it's looking for images
- ✓ Clear success/failure messages with checkmarks

Now when you select a Person Replacement filter, the console shows:
```
[OnFilterChanged] Current working directory: C:\Users\peter\git\MySubstitute\build\bin\Debug
[OnFilterChanged] Looking for face image at: assets/default_face.jpg
[OnFilterChanged] ✓ Using default target face: assets/default_face.jpg
```

Or if the file is missing:
```
[OnFilterChanged] Current working directory: C:\Users\peter\git\MySubstitute\build\bin\Debug
[OnFilterChanged] Looking for person image at: assets/default_person.jpg
[OnFilterChanged] ✗ No target image found at: assets/default_person.jpg
[OnFilterChanged] Tip: Place an image at 'C:\Users\peter\git\MySubstitute\build\bin\Debug\assets\default_person.jpg'
```

## How to Prevent This in Future

### Option 1: Show File Extensions in Windows
1. Open File Explorer
2. Click "View" menu
3. Check "File name extensions"
4. Now you'll see the real file names

### Option 2: Use Command Line to Check
```powershell
cd c:\Users\peter\git\MySubstitute\assets
Get-ChildItem *.jpg | Select-Object Name
```

This shows the actual file names including all extensions.

## Current Status

✅ **Files Fixed**: Renamed to correct extensions  
✅ **Application Updated**: Better debugging messages  
✅ **Build Successful**: Rebuilt with improvements  
✅ **Ready to Test**: Launch app and select Person Replacement filters

## What Happens Now

When you run the application and select "AI Face Swap" or "AI Full Body Replace":

1. **Application starts** from: `C:\Users\peter\git\MySubstitute\build\bin\Debug\`
2. **Looks for images** at: `build\bin\Debug\assets\default_face.jpg`
3. **But wait!** Your images are at: `C:\Users\peter\git\MySubstitute\assets\`

### Important Discovery!

The working directory is `build\bin\Debug\`, not the project root!

## Final Fix Needed

You need to either:

### Option A: Copy Images to Correct Location
```powershell
# Create assets folder in build directory
mkdir c:\Users\peter\git\MySubstitute\build\bin\Debug\assets

# Copy images
copy c:\Users\peter\git\MySubstitute\assets\*.jpg c:\Users\peter\git\MySubstitute\build\bin\Debug\assets\
```

### Option B: Use Absolute Paths in Code
Update `main.cpp` to use full project paths:
```cpp
std::string projectRoot = "c:\\Users\\peter\\git\\MySubstitute\\";
std::string defaultTargetFace = projectRoot + "assets/default_face.jpg";
```

### Option C: Change Working Directory at Launch
Update `run_debug.bat` to run from project root instead of build directory.

## Quick Fix Command

Run this to copy images to the correct location:
```powershell
mkdir c:\Users\peter\git\MySubstitute\build\bin\Debug\assets -Force
copy c:\Users\peter\git\MySubstitute\assets\default_*.jpg c:\Users\peter\git\MySubstitute\build\bin\Debug\assets\
```

Then the application will find your images!

---

**Status**: Files renamed, application updated with better debugging. Now copy images to `build\bin\Debug\assets\` folder.
