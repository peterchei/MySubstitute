"""
Download InsightFace pre-trained ONNX models for face swapping.

InsightFace provides production-ready face swap models that work directly
with ONNX Runtime - no conversion needed!
"""

import os
import urllib.request
import sys

def download_file(url, output_path):
    """Download file with progress indicator"""
    def report_progress(block_num, block_size, total_size):
        downloaded = block_num * block_size
        percent = min(100, (downloaded / total_size) * 100)
        sys.stdout.write(f'\rDownloading... {percent:.1f}%')
        sys.stdout.flush()
    
    print(f"Downloading from {url}")
    urllib.request.urlretrieve(url, output_path, reporthook=report_progress)
    print("\n✅ Download complete!")

def main():
    # Create models directory
    models_dir = os.path.join(os.path.dirname(__file__), '..', 'models')
    os.makedirs(models_dir, exist_ok=True)
    
    print("=" * 60)
    print("InsightFace Model Downloader for Face Swapping")
    print("=" * 60)
    
    # InsightFace inswapper model (128MB, production-ready)
    models = {
        'inswapper_128.onnx': {
            'url': 'https://github.com/facefusion/facefusion-assets/releases/download/models/inswapper_128.onnx',
            'description': 'InsightFace face swap model (128MB)',
            'size': '128 MB'
        }
    }
    
    for model_name, info in models.items():
        output_path = os.path.join(models_dir, model_name)
        
        if os.path.exists(output_path):
            print(f"\n✓ {model_name} already exists")
            continue
        
        print(f"\n📥 Downloading {model_name}")
        print(f"   Description: {info['description']}")
        print(f"   Size: {info['size']}")
        
        try:
            download_file(info['url'], output_path)
            print(f"   Saved to: {output_path}")
        except Exception as e:
            print(f"\n❌ Failed to download {model_name}: {e}")
            print(f"   Please download manually from: {info['url']}")
    
    print("\n" + "=" * 60)
    print("✅ Model setup complete!")
    print("=" * 60)
    print("\nNext steps:")
    print("1. Rebuild MySubstitute: build.bat")
    print("2. Run application: run.bat")
    print("3. Select 'AI Face Swap' filter")
    print("\nThe AI model will provide much smoother face replacement!")

if __name__ == '__main__':
    main()
