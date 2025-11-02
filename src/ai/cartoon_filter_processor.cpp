#include "cartoon_filter_processor.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

CartoonFilterProcessor::CartoonFilterProcessor()
    : m_style(SIMPLE),
      m_edgeThreshold(100),
      m_smoothingLevel(3),
      m_colorLevels(8),
      m_addOutlineOnly(false),
      m_preserveDetails(true),
      m_frameCounter(0),
      m_processingTime(0.0)
{
    std::cout << "[CartoonFilterProcessor] Initializing..." << std::endl;
}

CartoonFilterProcessor::~CartoonFilterProcessor()
{
    Cleanup();
}

bool CartoonFilterProcessor::Initialize()
{
    std::cout << "[CartoonFilterProcessor] Initialize called" << std::endl;
    std::cout << "[CartoonFilterProcessor] Style: " << static_cast<int>(m_style) << std::endl;
    std::cout << "[CartoonFilterProcessor] Edge Threshold: " << m_edgeThreshold << std::endl;
    std::cout << "[CartoonFilterProcessor] Smoothing Level: " << m_smoothingLevel << std::endl;
    std::cout << "[CartoonFilterProcessor] Color Levels: " << m_colorLevels << std::endl;
    return true;
}

void CartoonFilterProcessor::Cleanup()
{
    std::cout << "[CartoonFilterProcessor] Cleanup called" << std::endl;
    m_previousEdges.release();
    m_previousFrame.release();
    m_previousQuantized.release();
}

Frame CartoonFilterProcessor::ProcessFrame(const Frame& input)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    Frame output = input;

#ifdef HAVE_OPENCV
    if (!input.data.empty()) {
        cv::Mat workingFrame = input.data.clone();
        ApplyCartoonEffect(workingFrame);
        workingFrame.copyTo(output.data);
    }
#endif

    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    m_frameCounter++;
    return output;
}

#ifdef HAVE_OPENCV

void CartoonFilterProcessor::ApplyCartoonEffect(cv::Mat& frame)
{
    switch (m_style) {
        case SIMPLE:
            ApplySimpleCartoon(frame);
            break;
        case DETAILED:
            ApplyDetailedCartoon(frame);
            break;
        case ANIME:
            ApplyAnimeStyle(frame);
            break;
        default:
            ApplySimpleCartoon(frame);
    }
}

void CartoonFilterProcessor::ApplySimpleCartoon(cv::Mat& frame)
{
    if (frame.empty()) {
        return;
    }

    try {
        // Apply bilateral filtering for smooth colors
        cv::Mat smoothed = frame.clone();
        
        // Multiple passes with moderate parameters for better quality
        for (int i = 0; i < m_smoothingLevel; ++i) {
            cv::Mat temp;
            // Use smaller sigma for more detailed smoothing
            cv::bilateralFilter(smoothed, temp, 7, 40, 40);
            temp.copyTo(smoothed);
        }

        // Enhance saturation for more vibrant cartoon colors
        cv::Mat hsv;
        cv::cvtColor(smoothed, hsv, cv::COLOR_BGR2HSV);
        for (int y = 0; y < hsv.rows; ++y) {
            cv::Vec3b* row = hsv.ptr<cv::Vec3b>(y);
            for (int x = 0; x < hsv.cols; ++x) {
                row[x][1] = cv::saturate_cast<uint8_t>(row[x][1] * 1.5f);  // Stronger saturation
            }
        }
        cv::cvtColor(hsv, smoothed, cv::COLOR_HSV2BGR);

        // More aggressive color quantization for cartoon look
        cv::Mat quantized = QuantizeColors(smoothed, 6);  // Fewer colors = more cartoon
        
        // Smooth color transitions between frames
        if (!m_previousQuantized.empty() && quantized.size() == m_previousQuantized.size()) {
            cv::addWeighted(quantized, 0.6, m_previousQuantized, 0.4, 0, quantized);
        }
        quantized.copyTo(m_previousQuantized);

        // Edge detection
        cv::Mat edges = DetectEdges(smoothed);
        
        // Stabilize edges across frames to reduce flickering
        cv::Mat stableEdges = StabilizeEdges(edges);

        // Apply edges
        if (!stableEdges.empty()) {
            CombineEdgesWithColors(quantized, stableEdges);
        }

        if (!quantized.empty()) {
            quantized.copyTo(frame);
        }
    } catch (const std::exception& e) {
        std::cerr << "[CartoonFilterProcessor] ApplySimpleCartoon error: " << e.what() << std::endl;
    }
}

void CartoonFilterProcessor::ApplyDetailedCartoon(cv::Mat& frame)
{
    if (frame.empty()) {
        return;
    }

    try {
        // Smoothing for detailed cartoon effect
        cv::Mat smoothed = frame.clone();
        for (int i = 0; i < m_smoothingLevel; ++i) {
            cv::Mat temp;
            // Smaller sigma for more detail preservation
            cv::bilateralFilter(smoothed, temp, 8, 50, 50);
            temp.copyTo(smoothed);
        }

        // Enhance saturation aggressively
        cv::Mat hsv;
        cv::cvtColor(smoothed, hsv, cv::COLOR_BGR2HSV);
        for (int y = 0; y < hsv.rows; ++y) {
            cv::Vec3b* row = hsv.ptr<cv::Vec3b>(y);
            for (int x = 0; x < hsv.cols; ++x) {
                row[x][1] = cv::saturate_cast<uint8_t>(row[x][1] * 1.6f);  // Much stronger saturation
            }
        }
        cv::cvtColor(hsv, smoothed, cv::COLOR_HSV2BGR);

        // Aggressive color reduction
        cv::Mat quantized = QuantizeColors(smoothed, 5);  // Very few colors
        
        // Smooth color transitions between frames
        if (!m_previousQuantized.empty() && quantized.size() == m_previousQuantized.size()) {
            cv::addWeighted(quantized, 0.6, m_previousQuantized, 0.4, 0, quantized);
        }
        quantized.copyTo(m_previousQuantized);

        // Edge detection
        cv::Mat edges = DetectEdges(smoothed);
        
        // Stabilize edges across frames to reduce flickering
        cv::Mat stableEdges = StabilizeEdges(edges);

        if (!stableEdges.empty() && !quantized.empty()) {
            CombineEdgesWithColors(quantized, stableEdges);
        }

        if (!quantized.empty()) {
            quantized.copyTo(frame);
        }
    } catch (const std::exception& e) {
        std::cerr << "[CartoonFilterProcessor] ApplyDetailedCartoon error: " << e.what() << std::endl;
    }
}

void CartoonFilterProcessor::ApplyAnimeStyle(cv::Mat& frame)
{
    if (frame.empty()) {
        return;
    }

    try {
        // Strong smoothing for anime look
        cv::Mat smoothed = frame.clone();
        for (int i = 0; i < m_smoothingLevel; ++i) {
            cv::Mat temp;
            // Use multiple passes with smaller sigma instead of large single pass
            cv::bilateralFilter(smoothed, temp, 9, 60, 60);
            temp.copyTo(smoothed);
        }

        // Enhance saturation for vibrant anime colors
        cv::Mat hsv;
        cv::cvtColor(smoothed, hsv, cv::COLOR_BGR2HSV);
        for (int y = 0; y < hsv.rows; ++y) {
            cv::Vec3b* row = hsv.ptr<cv::Vec3b>(y);
            for (int x = 0; x < hsv.cols; ++x) {
                row[x][1] = cv::saturate_cast<uint8_t>(row[x][1] * 1.8f);  // Maximum saturation boost
            }
        }
        cv::cvtColor(hsv, smoothed, cv::COLOR_HSV2BGR);

        // Extreme color reduction for anime palette
        cv::Mat quantized = QuantizeColors(smoothed, 4);  // Very few colors for anime style
        
        // Smooth color transitions between frames
        if (!m_previousQuantized.empty() && quantized.size() == m_previousQuantized.size()) {
            cv::addWeighted(quantized, 0.6, m_previousQuantized, 0.4, 0, quantized);
        }
        quantized.copyTo(m_previousQuantized);

        // Edge detection
        cv::Mat edges = DetectEdges(smoothed);
        
        // Stabilize edges across frames to reduce flickering
        cv::Mat stableEdges = StabilizeEdges(edges);

        if (!stableEdges.empty() && !quantized.empty()) {
            CombineEdgesWithColors(quantized, stableEdges);
        }

        if (!quantized.empty()) {
            quantized.copyTo(frame);
        }
    } catch (const std::exception& e) {
        std::cerr << "[CartoonFilterProcessor] ApplyAnimeStyle error: " << e.what() << std::endl;
    }
}

cv::Mat CartoonFilterProcessor::StabilizeEdges(const cv::Mat& currentEdges)
{
    // First frame - just return current edges
    if (m_previousEdges.empty()) {
        m_previousEdges = currentEdges.clone();
        return currentEdges.clone();
    }

    if (currentEdges.size() != m_previousEdges.size()) {
        m_previousEdges = currentEdges.clone();
        return currentEdges.clone();
    }

    // Use hysteresis-like approach for more stable edges
    // Keep edges that are strong in current frame OR were present in previous frame
    cv::Mat stabilized = cv::Mat::ones(currentEdges.size(), CV_8UC1) * 255;  // Start with "no edge"
    
    for (int y = 0; y < currentEdges.rows; ++y) {
        const uint8_t* currRow = currentEdges.ptr<uint8_t>(y);
        const uint8_t* prevRow = m_previousEdges.ptr<uint8_t>(y);
        uint8_t* stabRow = stabilized.ptr<uint8_t>(y);
        
        for (int x = 0; x < currentEdges.cols; ++x) {
            uint8_t currVal = currRow[x];
            uint8_t prevVal = prevRow[x];
            
            // Strong hysteresis: keep edges if:
            // 1. Strong edge in current frame (< 100), OR
            // 2. Edge in previous frame AND reasonable edge in current (< 150)
            if (currVal < 100) {
                // Strong current edge - always keep
                stabRow[x] = currVal;
            } else if (currVal < 150 && prevVal < 200) {
                // Moderate current edge and previous edge present
                stabRow[x] = cv::saturate_cast<uint8_t>((currVal + prevVal) / 2);
            } else if (prevVal < 150) {
                // Weak current but strong previous edge - propagate it
                stabRow[x] = cv::saturate_cast<uint8_t>(prevVal * 0.9f);
            } else {
                // No significant edge
                stabRow[x] = 255;
            }
        }
    }
    
    // Dilate slightly to fill small gaps and make edges more continuous
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::dilate(stabilized, stabilized, kernel, cv::Point(-1, -1), 1);
    
    // Store current for next frame
    stabilized.copyTo(m_previousEdges);
    
    return stabilized;
}

cv::Mat CartoonFilterProcessor::DetectEdges(const cv::Mat& src)
{
    if (src.empty()) {
        return cv::Mat::zeros(src.size(), CV_8UC1);
    }

    cv::Mat gray, blurred, laplacian, edges;

    // Convert to grayscale
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src.clone();
    }

    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(gray, blurred, cv::Size(5, 5), 1.0);

    // Apply Laplacian operator for edge detection (fast)
    cv::Laplacian(blurred, laplacian, CV_16S, 1);
    cv::convertScaleAbs(laplacian, laplacian);

    // Use very low threshold for more responsive edges
    // Lower values = more edge detection, but more noise
    int threshold = std::max(5, m_edgeThreshold / 8);  // Even lower threshold
    cv::threshold(laplacian, edges, threshold, 255, cv::THRESH_BINARY);

    // Invert: we want white areas where edges are (for masking)
    cv::bitwise_not(edges, edges);

    return edges;
}

cv::Mat CartoonFilterProcessor::QuantizeColors(const cv::Mat& src, int levels)
{
    if (src.empty()) {
        return src.clone();
    }

    cv::Mat dst = src.clone();
    int divideValue = std::max(1, 256 / levels);

    // Fast quantization using pointer-based access
    // This is much faster than k-means for real-time processing
    for (int y = 0; y < dst.rows; ++y) {
        cv::Vec3b* row = dst.ptr<cv::Vec3b>(y);
        for (int x = 0; x < dst.cols; ++x) {
            row[x][0] = (row[x][0] / divideValue) * divideValue;
            row[x][1] = (row[x][1] / divideValue) * divideValue;
            row[x][2] = (row[x][2] / divideValue) * divideValue;
        }
    }

    return dst;
}

void CartoonFilterProcessor::CombineEdgesWithColors(cv::Mat& frame, const cv::Mat& edges)
{
    if (frame.empty() || edges.empty()) {
        return;
    }

    if (frame.size() != edges.size()) {
        return;  // Size mismatch
    }

    // Use pointer-based access for maximum speed
    for (int y = 0; y < frame.rows; ++y) {
        cv::Vec3b* frameRow = frame.ptr<cv::Vec3b>(y);
        const uint8_t* edgesRow = edges.ptr<uint8_t>(y);
        
        for (int x = 0; x < frame.cols; ++x) {
            uint8_t edgeVal = edgesRow[x];
            if (edgeVal < 220) {  // Edge detected
                // Darker but more visible outlines - better cartoon look
                frameRow[x][0] = cv::saturate_cast<uint8_t>(frameRow[x][0] * 0.3f);
                frameRow[x][1] = cv::saturate_cast<uint8_t>(frameRow[x][1] * 0.3f);
                frameRow[x][2] = cv::saturate_cast<uint8_t>(frameRow[x][2] * 0.3f);
            }
        }
    }
}

#endif  // HAVE_OPENCV

bool CartoonFilterProcessor::SetParameter(const std::string& name, const std::string& value)
{
    try {
        if (name == "style") {
            int style = std::stoi(value);
            SetCartoonStyle(style);
            return true;
        } else if (name == "edge_threshold") {
            int threshold = std::stoi(value);
            SetEdgeThreshold(threshold);
            return true;
        } else if (name == "smoothing_level") {
            int level = std::stoi(value);
            SetSmoothingLevel(level);
            return true;
        } else if (name == "color_levels") {
            int levels = std::stoi(value);
            SetColorLevels(levels);
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "[CartoonFilterProcessor] SetParameter error: " << e.what() << std::endl;
        return false;
    }
    return false;
}

std::map<std::string, std::string> CartoonFilterProcessor::GetParameters() const
{
    std::map<std::string, std::string> params;
    params["style"] = std::to_string(static_cast<int>(m_style));
    params["edge_threshold"] = std::to_string(m_edgeThreshold);
    params["smoothing_level"] = std::to_string(m_smoothingLevel);
    params["color_levels"] = std::to_string(m_colorLevels);
    return params;
}

double CartoonFilterProcessor::GetExpectedProcessingTime() const
{
    return m_processingTime;
}

void CartoonFilterProcessor::SetCartoonStyle(int style)
{
    m_style = static_cast<CartoonStyle>(std::max(0, std::min(2, style)));
    std::cout << "[CartoonFilterProcessor] Style changed to: " << static_cast<int>(m_style) << std::endl;
}

void CartoonFilterProcessor::SetEdgeThreshold(int threshold)
{
    m_edgeThreshold = std::max(0, std::min(255, threshold));
}

void CartoonFilterProcessor::SetSmoothingLevel(int level)
{
    m_smoothingLevel = std::max(1, std::min(10, level));
}

void CartoonFilterProcessor::SetColorLevels(int levels)
{
    m_colorLevels = std::max(2, std::min(32, levels));
}
