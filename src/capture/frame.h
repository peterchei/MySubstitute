#pragma once

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif
#include <vector>
#include <memory>

/**
 * Frame data structure for video frames
 */
struct Frame {
#ifdef HAVE_OPENCV
    cv::Mat data;           // OpenCV matrix containing pixel data
#else
    std::vector<unsigned char> data;  // Raw pixel data when OpenCV not available
#endif
    int width;              // Frame width in pixels
    int height;             // Frame height in pixels
    int channels;           // Number of color channels (1=grayscale, 3=RGB, 4=RGBA)
    double timestamp;       // Timestamp in milliseconds
    int format;             // Pixel format
    
    Frame() : width(0), height(0), channels(0), timestamp(0.0), format(0) {}
    
    Frame(int w, int h, int c, int fmt = 0) 
        : width(w), height(h), channels(c), timestamp(0.0), format(fmt) {
#ifdef HAVE_OPENCV
        data = cv::Mat::zeros(h, w, fmt);
#else
        data.resize(w * h * c);
#endif
    }
    
#ifdef HAVE_OPENCV
    Frame(const cv::Mat& mat) 
        : data(mat), timestamp(0.0) {
        width = mat.cols;
        height = mat.rows;
        channels = mat.channels();
        format = mat.type();
    }
#endif
    
    /**
     * Check if frame contains valid data
     */
    bool IsValid() const {
#ifdef HAVE_OPENCV
        return !data.empty() && width > 0 && height > 0;
#else
        return !data.empty() && width > 0 && height > 0;
#endif
    }
    
    /**
     * Get frame size in bytes
     */
    size_t GetSizeInBytes() const {
#ifdef HAVE_OPENCV
        return data.total() * data.elemSize();
#else
        return data.size();
#endif
    }
    
    /**
     * Clone the frame (deep copy)
     */
    Frame Clone() const {
        Frame result;
#ifdef HAVE_OPENCV
        result.data = data.clone();
#else
        result.data = data;  // Vector copy
#endif
        result.width = width;
        result.height = height;
        result.channels = channels;
        result.timestamp = timestamp;
        result.format = format;
        return result;
    }
    
    /**
     * Convert frame to different color space
     */
    bool ConvertTo(int newFormat, Frame& output) const;
    
    /**
     * Resize frame to new dimensions
     */
    bool ResizeTo(int newWidth, int newHeight, Frame& output) const;
};

/**
 * Camera device information
 */
struct CameraDevice {
    int id;                 // Device ID for selection
    std::string name;       // Human readable device name
    std::string description;// Device description
    bool isAvailable;       // Whether device is currently available
    
    // Supported capabilities
    struct Capability {
        int width, height;
        int fps;
        int format;
    };
    
    std::vector<Capability> supportedFormats;
    
    CameraDevice() : id(-1), isAvailable(false) {}
    
    CameraDevice(int deviceId, const std::string& deviceName) 
        : id(deviceId), name(deviceName), isAvailable(true) {}
};