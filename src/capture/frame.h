#pragma once

#include <opencv2/opencv.hpp>
#include <vector>
#include <memory>

/**
 * Frame data structure for video frames
 */
struct Frame {
    cv::Mat data;           // OpenCV matrix containing pixel data
    int width;              // Frame width in pixels
    int height;             // Frame height in pixels
    int channels;           // Number of color channels (1=grayscale, 3=RGB, 4=RGBA)
    double timestamp;       // Timestamp in milliseconds
    int format;             // Pixel format (CV_8UC3, etc.)
    
    Frame() : width(0), height(0), channels(0), timestamp(0.0), format(0) {}
    
    Frame(int w, int h, int c, int fmt = CV_8UC3) 
        : width(w), height(h), channels(c), timestamp(0.0), format(fmt) {
        data = cv::Mat::zeros(h, w, fmt);
    }
    
    Frame(const cv::Mat& mat) 
        : data(mat), timestamp(0.0) {
        width = mat.cols;
        height = mat.rows;
        channels = mat.channels();
        format = mat.type();
    }
    
    /**
     * Check if frame contains valid data
     */
    bool IsValid() const {
        return !data.empty() && width > 0 && height > 0;
    }
    
    /**
     * Get frame size in bytes
     */
    size_t GetSizeInBytes() const {
        return data.total() * data.elemSize();
    }
    
    /**
     * Clone the frame (deep copy)
     */
    Frame Clone() const {
        Frame result;
        result.data = data.clone();
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