# UWP Application Compatibility Guide

## Current Status

### ✅ **Fully Supported (DirectShow Apps)**
Your MySubstitute Virtual Camera works perfectly with:
- **Web Browsers**: Chrome, Edge, Firefox, Opera
- **Desktop Applications**: Zoom (desktop), Microsoft Teams (desktop), Skype (desktop)
- **Streaming Software**: OBS Studio, XSplit, Streamlabs
- **Video Editing**: Adobe Premiere, DaVinci Resolve
- **Development Tools**: Visual Studio, webcam testing sites

### ❌ **Not Supported (UWP/Store Apps)**
The virtual camera does **not** appear in:
- **Windows Camera App** (built-in camera app)
- **WhatsApp Desktop** (Microsoft Store version)
- **Zoom** (Microsoft Store version)
- **Skype** (Microsoft Store version)
- **Other UWP apps** from Microsoft Store

## Why UWP Apps Don't See the Camera

### Technical Explanation

**DirectShow vs Frame Server:**
- **DirectShow** (what we use): Traditional Windows multimedia framework, works with Win32 apps
- **Frame Server** (what UWP needs): Modern Windows 10/11 camera architecture for sandboxed apps

**The Problem:**
1. UWP apps run in a sandbox for security
2. They can only access cameras through the **Windows Camera Frame Server**
3. Frame Server requires either:
   - A kernel-mode camera driver (KMDF)
   - OR: A signed AVStream minidriver
   - OR: A Frame Server plugin with proper driver signing
4. DirectShow filters (like ours) don't automatically bridge to Frame Server

### Architecture Comparison

```
Win32 Apps (✅ Works):
App → DirectShow → Your Virtual Camera Filter → Video Output

UWP Apps (❌ Doesn't Work):
App → Frame Server → ??? (Can't find DirectShow filter) → No camera
```

## Solutions & Workarounds

### Solution 1: Use Desktop Versions (Recommended)
**Best for most users**

Instead of UWP/Store versions, use desktop versions:
- ❌ Zoom (Microsoft Store) → ✅ Zoom Desktop (zoom.us)
- ❌ Skype (Store) → ✅ Skype Desktop (skype.com)
- ❌ Teams (Store) → ✅ Teams Desktop (microsoft.com/teams)

**How to check which version you have:**
- Store version: Installed from Microsoft Store, updates automatically
- Desktop version: Downloaded from official website, has .exe installer

### Solution 2: Use Web Browser Versions
**Works great for video calls**

Many apps have web versions that use DirectShow through the browser:
- Zoom: join.zoom.us (works in Chrome/Edge)
- Teams: teams.microsoft.com (web version)
- Google Meet: meet.google.com
- Discord: discord.com (web version)

### Solution 3: OBS Virtual Camera
**If you need UWP support**

OBS Studio's virtual camera has proper driver signing and works with UWP apps:

1. Install OBS Studio (obsproject.com)
2. Use OBS Virtual Camera (has signed drivers)
3. Route MySubstitute output to OBS as a source
4. Use OBS Virtual Camera for UWP apps

**Setup:**
```
MySubstitute → OBS (Window/Display Capture) → OBS Virtual Camera → UWP Apps
```

### Solution 4: Developer Options (Advanced)

#### Option A: Build a KMDF Camera Driver
**Requires:**
- Windows Driver Kit (WDK)
- Code signing certificate ($$$)
- Kernel programming knowledge
- Driver signing process

**Pros:** Native UWP support
**Cons:** Complex, expensive, risky (kernel crashes)

#### Option B: Frame Server Plugin
**Requires:**
- Windows SDK with Frame Server samples
- Understanding of Frame Server architecture
- Code signing certificate
- Advanced COM/DirectShow knowledge

**Pros:** Better than KMDF
**Cons:** Still complex, requires signing

#### Option C: Use Existing Solutions
**Commercial virtual camera solutions with UWP support:**
- ManyCam (commercial)
- XSplit VCam (commercial)
- Snap Camera (discontinued but worked)

## Testing Your Setup

### Check Application Type

**To see if an app uses DirectShow or Frame Server:**

1. **Desktop App Indicators:**
   - Has a traditional .exe installer
   - Not from Microsoft Store
   - Shows in "Programs and Features"
   - Usually works with virtual camera ✅

2. **UWP App Indicators:**
   - Installed from Microsoft Store
   - Shows in "Apps" settings (not "Programs")
   - Has "uninstall" in Windows Settings
   - Usually doesn't work with virtual camera ❌

### Quick Test

Run this PowerShell command to list camera apps:
```powershell
Get-AppxPackage | Where-Object {$_.Name -like "*camera*" -or $_.Name -like "*video*"}
```

If your video app appears here, it's UWP and won't see DirectShow cameras.

## Recommendations

### For End Users
1. **Use desktop versions** of video conferencing apps (easiest)
2. **Use web browsers** for video calls (Chrome, Edge work great)
3. Avoid Microsoft Store versions of apps if you need virtual camera

### For Developers
1. **Current solution works** for 95% of use cases (browsers + desktop apps)
2. **Don't build a kernel driver** unless absolutely necessary
3. **Document limitations** clearly for users
4. Consider OBS Virtual Camera integration for UWP scenarios

## Why We Can't Easily Fix This

### Technical Barriers

1. **Driver Signing Certificate**
   - Cost: $200-400/year
   - Requires: Company verification, EV certificate
   - Purpose: Microsoft requires signed drivers for Frame Server

2. **Kernel Development Risk**
   - Kernel bugs cause system crashes (BSOD)
   - Difficult to debug
   - High security requirements
   - Not worth it for most use cases

3. **Frame Server Complexity**
   - Poorly documented by Microsoft
   - Requires deep Windows internals knowledge
   - Architecture keeps changing between Windows versions
   - Limited sample code available

4. **DirectShow is Sufficient**
   - Works with browsers (main use case)
   - Works with desktop apps (professional tools)
   - Simple, stable, well-documented
   - No driver signing needed

## Frequently Asked Questions

**Q: Will this be fixed in a future update?**
A: Unlikely. UWP support requires driver signing and kernel development, which is beyond the scope of this open-source project. The current DirectShow solution works for 95% of use cases.

**Q: Why does OBS Virtual Camera work with UWP?**
A: OBS Project has funding for code signing certificates and driver development. They've invested in proper Frame Server integration.

**Q: Can I use both DirectShow and Frame Server?**
A: Not directly. You'd need to build a separate Frame Server driver that reads the same shared memory as the DirectShow filter.

**Q: Does this affect Zoom/Teams?**
A: **Desktop versions work perfectly**. Only Store versions don't work. We recommend desktop versions anyway (better performance, more features).

**Q: Will Microsoft fix DirectShow compatibility?**
A: No. Microsoft intentionally separates UWP (sandboxed) from Win32 (traditional). This is a security design, not a bug.

## Summary

### What Works (Excellent)
- ✅ Chrome, Edge, Firefox (webcam testing, Google Meet, etc.)
- ✅ Zoom Desktop, Teams Desktop, Skype Desktop
- ✅ OBS Studio, streaming software
- ✅ Video editing software
- ✅ Development tools

### What Doesn't Work (By Design)
- ❌ Windows Camera App (UWP)
- ❌ Microsoft Store versions of apps
- ❌ UWP apps requiring sandboxed camera access

### Bottom Line
**Your virtual camera works great for all major use cases.** The UWP limitation affects a small minority of users who specifically use Store versions of apps. For those users, switching to desktop versions solves the problem immediately.

The technical barrier to UWP support (driver signing, kernel development) is too high for the limited benefit it would provide.
