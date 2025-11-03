# Alternative Model Download Methods

The automated download script may fail due to HuggingFace authentication. Here are several alternative methods:

## Method 1: Direct Browser Download (Easiest)

1. **Open this URL in your browser:**
   ```
   https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx
   ```

2. **The file will download automatically** (generator_Hayao.onnx - 8.5 MB)

3. **Move and rename the file:**
   ```powershell
   # From Downloads folder
   Move-Item "$env:USERPROFILE\Downloads\generator_Hayao.onnx" "C:\Users\peter\git\MySubstitute\models\anime_gan.onnx"
   ```

## Method 2: Use curl (Windows Built-in)

```powershell
cd C:\Users\peter\git\MySubstitute
curl -L "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx" -o "models\anime_gan.onnx"
```

## Method 3: Use wget (If Installed)

```powershell
wget "https://huggingface.co/bryandlee/animegan2-pytorch/resolve/main/generator_Hayao.onnx" -O "models\anime_gan.onnx"
```

## Method 4: Git LFS (If You Have Git)

```powershell
# Clone the model repository
git lfs install
git clone https://huggingface.co/bryandlee/animegan2-pytorch temp_models

# Copy the model
Copy-Item "temp_models\generator_Hayao.onnx" "models\anime_gan.onnx"

# Cleanup
Remove-Item -Recurse -Force temp_models
```

## Method 5: Alternative Model Sources

If HuggingFace doesn't work, try these mirror sites:

### GitHub Release (If Available)
```
https://github.com/bryandlee/animegan2-pytorch/releases
```

### Google Drive Links (Community Mirrors)
Sometimes community members upload to Google Drive. Search for:
- "AnimeGANv2 ONNX model download"
- "generator_Hayao.onnx download"

## Verify Your Download

After downloading, verify the file:

```powershell
# Check file exists
Test-Path "C:\Users\peter\git\MySubstitute\models\anime_gan.onnx"

# Check file size (should be around 8-9 MB)
(Get-Item "models\anime_gan.onnx").Length / 1MB
```

Should return:
```
True
8.47  (approximately)
```

## If All Else Fails: Use Cartoon Filters

The AnimeGAN filter requires the model download. If you can't get it, the built-in Cartoon filters work great without any downloads:

1. **Cartoon (Anime)** - Built-in anime-style filter
2. **Cartoon (Detailed)** - More detailed cartoon effect
3. **Pixel Art (Anime)** - Anime pixel art style

These run at 30+ FPS on CPU with no model required!

## Test the Model

Once you have the model file, test it:

```powershell
.\build\bin\Release\MySubstitute.exe
```

In the preview window, select "Anime GAN (AI - GPU)" and check the console for:
```
[AnimeGANProcessor] Model loaded successfully
[AnimeGANProcessor] Using CUDA backend for GPU acceleration
```

## Common Issues

### "Invalid username or password"
- HuggingFace may require authentication
- Use **Method 1** (browser download) instead

### "SSL/TLS secure channel"
- Try: `[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12`
- Then retry the download

### "Access Denied"
- Make sure you have write permissions to the models folder
- Try running PowerShell as Administrator

## Need Help?

If you still can't download:
1. Try **Method 1** (browser download) - most reliable
2. Check if you can access https://huggingface.co in your browser
3. Try using a VPN if HuggingFace is blocked in your region
4. Use the built-in Cartoon filters as an excellent alternative
