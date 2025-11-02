# Face Filter Processor

A fun AI processor that adds virtual accessories to detected faces in video streams. Perfect for video calls and meetings!

## Features

- **Face Detection**: Uses OpenCV's Haar cascades for robust face detection
- **Virtual Glasses**: Adds semi-transparent glasses overlay
- **Funny Hats**: Places animated hats above detected faces
- **Speech Bubbles**: Adds customizable text bubbles above faces
- **Real-time Processing**: Optimized for 30+ FPS performance
- **Configurable**: Enable/disable individual effects via parameters

## Usage

### Basic Setup

```cpp
#include "face_filter_processor.h"

// Create and initialize
auto processor = std::make_unique<FaceFilterProcessor>();
if (processor->Initialize()) {
    // Configure effects
    processor->SetGlassesEnabled(true);
    processor->SetHatEnabled(true);
    processor->SetSpeechBubbleText("Hello Meeting!");

    // Process frames
    Frame processedFrame = processor->ProcessFrame(inputFrame);
}
```

### Configuration Parameters

| Parameter | Type | Description |
|-----------|------|-------------|
| `glasses_enabled` | bool | Enable/disable virtual glasses |
| `hat_enabled` | bool | Enable/disable funny hats |
| `speech_bubble_enabled` | bool | Enable/disable speech bubbles |
| `speech_bubble_text` | string | Text to display in speech bubbles |

### Runtime Configuration

```cpp
// Enable/disable effects at runtime
processor->SetParameter("glasses_enabled", "false");
processor->SetParameter("speech_bubble_text", "Good morning!");

// Get current settings
auto params = processor->GetParameters();
for (const auto& param : params) {
    std::cout << param.first << ": " << param.second << std::endl;
}
```

## Asset Files

The processor looks for accessory images in these locations:
- `glasses.png` - Virtual glasses overlay
- `funny_hat.png` - Hat accessory

If images are not found, the processor creates colored placeholder rectangles.

## Performance

- **Expected Processing Time**: ~50ms per frame
- **Real-time Capable**: Yes
- **Dependencies**: OpenCV with face detection cascades

## Integration

The Face Filter Processor integrates seamlessly with the MySubstitute virtual camera pipeline:

1. **Camera Capture** → **AI Processing** → **Virtual Camera Output**
2. Faces detected in real-time
3. Virtual accessories added to video stream
4. Output sent to meeting applications via virtual camera

## Example Output

When a face is detected, the processor will:
- Add semi-transparent glasses positioned on the eyes
- Place a funny hat above the head
- Display a speech bubble with customizable text

Perfect for adding personality to your video calls! 🎭✨