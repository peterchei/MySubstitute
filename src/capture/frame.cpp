#include "frame.h"

bool Frame::ConvertTo(int newFormat, Frame& output) const {
    if (!IsValid()) {
        return false;
    }
    
#ifdef HAVE_OPENCV
    if (format == newFormat) {
        output = Clone();
        return true;
    }
    
    try {
        cv::Mat converted;
        if (format == CV_8UC3 && newFormat == CV_8UC1) {
            cv::cvtColor(data, converted, cv::COLOR_BGR2GRAY);
        } else if (format == CV_8UC1 && newFormat == CV_8UC3) {
            cv::cvtColor(data, converted, cv::COLOR_GRAY2BGR);
        } else if (format == CV_8UC3 && newFormat == CV_8UC4) {
            cv::cvtColor(data, converted, cv::COLOR_BGR2BGRA);
        } else if (format == CV_8UC4 && newFormat == CV_8UC3) {
            cv::cvtColor(data, converted, cv::COLOR_BGRA2BGR);
        } else {
            // Unsupported conversion
            return false;
        }
        
        output = Frame(converted);
        output.format = newFormat;
        output.timestamp = timestamp;
        return true;
        
    } catch (const cv::Exception& e) {
        return false;
    }
#else
    // Without OpenCV, just copy the frame (no conversion possible)
    output = Clone();
    return true;
#endif
}

bool Frame::ResizeTo(int newWidth, int newHeight, Frame& output) const {
    if (!IsValid() || newWidth <= 0 || newHeight <= 0) {
        return false;
    }
    
#ifdef HAVE_OPENCV
    try {
        cv::Mat resized;
        cv::resize(data, resized, cv::Size(newWidth, newHeight));
        
        output = Frame(resized);
        output.timestamp = timestamp;
        return true;
        
    } catch (const cv::Exception& e) {
        return false;
    }
#else
    // Without OpenCV, just copy the frame (no resize possible)
    output = Clone();
    return true;
#endif
}