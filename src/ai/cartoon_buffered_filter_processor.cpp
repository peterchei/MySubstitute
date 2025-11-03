#include "cartoon_buffered_filter_processor.h"
#include <iostream>
#include <chrono>
#include <algorithm>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

CartoonBufferedFilterProcessor::CartoonBufferedFilterProcessor()
    : m_style(SIMPLE),
      m_edgeThreshold(100),
      m_smoothingLevel(3),
      m_colorLevels(6),
      m_bufferSize(5),  // 5 frames at 30fps = ~0.17 seconds delay (reduced from 10 for performance)
      m_frameCounter(0),
      m_processingTime(0.0)
{
    std::cout << "[CartoonBufferedFilterProcessor] Initializing with buffer size: " << m_bufferSize << std::endl;
}

CartoonBufferedFilterProcessor::~CartoonBufferedFilterProcessor()
{
    Cleanup();
}

bool CartoonBufferedFilterProcessor::Initialize()
{
    std::cout << "[CartoonBufferedFilterProcessor] Initialize called" << std::endl;
    std::cout << "[CartoonBufferedFilterProcessor] Style: " << static_cast<int>(m_style) << std::endl;
    std::cout << "[CartoonBufferedFilterProcessor] Edge Threshold: " << m_edgeThreshold << std::endl;
    std::cout << "[CartoonBufferedFilterProcessor] Smoothing Level: " << m_smoothingLevel << std::endl;
    std::cout << "[CartoonBufferedFilterProcessor] Color Levels: " << m_colorLevels << std::endl;
    std::cout << "[CartoonBufferedFilterProcessor] Buffer Size: " << m_bufferSize << " frames" << std::endl;
    return true;
}

void CartoonBufferedFilterProcessor::Cleanup()
{
    std::cout << "[CartoonBufferedFilterProcessor] Cleanup called" << std::endl;
#ifdef HAVE_OPENCV
    m_frameBuffer.clear();
    m_edgeBuffer.clear();
    m_quantizedBuffer.clear();
#endif
}

Frame CartoonBufferedFilterProcessor::ProcessFrame(const Frame& input)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    Frame output = input;

#ifdef HAVE_OPENCV
    if (!input.data.empty()) {
        cv::Mat workingFrame = input.data.clone();
        
        // Add frame to buffer
        AddFrameToBuffer(workingFrame);
        
        // Apply buffered cartoon effect
        ApplyBufferedCartoon(workingFrame);
        
        workingFrame.copyTo(output.data);
    }
#endif

    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    m_frameCounter++;
    return output;
}

#ifdef HAVE_OPENCV

void CartoonBufferedFilterProcessor::AddFrameToBuffer(const cv::Mat& frame)
{
    if (frame.empty()) return;
    
    // Apply bilateral filtering for smooth colors
    cv::Mat smoothed = frame.clone();
    for (int i = 0; i < m_smoothingLevel; ++i) {
        cv::Mat temp;
        cv::bilateralFilter(smoothed, temp, 7, 40, 40);
        temp.copyTo(smoothed);
    }
    
    // Enhance saturation
    cv::Mat hsv;
    cv::cvtColor(smoothed, hsv, cv::COLOR_BGR2HSV);
    for (int y = 0; y < hsv.rows; ++y) {
        cv::Vec3b* row = hsv.ptr<cv::Vec3b>(y);
        for (int x = 0; x < hsv.cols; ++x) {
            row[x][1] = cv::saturate_cast<uint8_t>(row[x][1] * 1.5f);
        }
    }
    cv::cvtColor(hsv, smoothed, cv::COLOR_HSV2BGR);
    
    // Store smoothed frame
    m_frameBuffer.push_back(smoothed.clone());
    if (m_frameBuffer.size() > static_cast<size_t>(m_bufferSize)) {
        m_frameBuffer.pop_front();
    }
    
    // Detect and store edges
    cv::Mat edges = DetectEdges(smoothed);
    m_edgeBuffer.push_back(edges.clone());
    if (m_edgeBuffer.size() > static_cast<size_t>(m_bufferSize)) {
        m_edgeBuffer.pop_front();
    }
    
    // Quantize and store colors
    cv::Mat quantized = QuantizeColors(smoothed, m_colorLevels);
    m_quantizedBuffer.push_back(quantized.clone());
    if (m_quantizedBuffer.size() > static_cast<size_t>(m_bufferSize)) {
        m_quantizedBuffer.pop_front();
    }
}

cv::Mat CartoonBufferedFilterProcessor::ComputeTemporalMedianEdges()
{
    if (m_edgeBuffer.empty()) {
        return cv::Mat();
    }
    
    if (m_edgeBuffer.size() == 1) {
        return m_edgeBuffer[0].clone();
    }
    
    // Use weighted temporal blending instead of median for performance
    // Start with most recent frame and blend with older frames
    cv::Mat result = m_edgeBuffer.back().clone();
    
    // Blend with previous frames using exponential decay
    double alpha = 0.7;  // Weight for current accumulated result
    for (int i = m_edgeBuffer.size() - 2; i >= 0; --i) {
        cv::addWeighted(result, alpha, m_edgeBuffer[i], 1.0 - alpha, 0, result);
        alpha *= 0.9;  // Decay older frames
    }
    
    return result;
}

cv::Mat CartoonBufferedFilterProcessor::ComputeTemporalMedianColors()
{
    if (m_quantizedBuffer.empty()) {
        return cv::Mat();
    }
    
    if (m_quantizedBuffer.size() == 1) {
        return m_quantizedBuffer[0].clone();
    }
    
    // Use weighted temporal blending instead of median for performance
    // Start with most recent frame and blend with older frames
    cv::Mat result = m_quantizedBuffer.back().clone();
    
    // Blend with previous frames using exponential decay
    double alpha = 0.7;  // Weight for current accumulated result
    for (int i = m_quantizedBuffer.size() - 2; i >= 0; --i) {
        cv::addWeighted(result, alpha, m_quantizedBuffer[i], 1.0 - alpha, 0, result);
        alpha *= 0.9;  // Decay older frames
    }
    
    return result;
}

void CartoonBufferedFilterProcessor::ApplyBufferedCartoon(cv::Mat& outputFrame)
{
    // Start temporal filtering as soon as we have 2 frames (reduced from half-buffer)
    if (m_frameBuffer.size() < 2) {
        // Not enough frames yet, just process current frame normally
        cv::Mat edges = DetectEdges(outputFrame);
        cv::Mat quantized = QuantizeColors(outputFrame, m_colorLevels);
        CombineEdgesWithColors(quantized, edges);
        quantized.copyTo(outputFrame);
        return;
    }
    
    try {
        // Compute temporally filtered edges using weighted blending
        cv::Mat blendedEdges = ComputeTemporalMedianEdges();
        
        // Compute temporally filtered colors using weighted blending
        cv::Mat blendedColors = ComputeTemporalMedianColors();
        
        if (blendedEdges.empty() || blendedColors.empty()) {
            return;  // Failed to compute blending
        }
        
        // Apply edges to colors
        CombineEdgesWithColors(blendedColors, blendedEdges);
        
        // Copy result to output
        blendedColors.copyTo(outputFrame);
        
    } catch (const std::exception& e) {
        std::cerr << "[CartoonBufferedFilterProcessor] ApplyBufferedCartoon error: " << e.what() << std::endl;
    }
}

cv::Mat CartoonBufferedFilterProcessor::DetectEdges(const cv::Mat& src)
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

    // Apply Laplacian operator for edge detection
    cv::Laplacian(blurred, laplacian, CV_16S, 1);
    cv::convertScaleAbs(laplacian, laplacian);

    // Moderate threshold for stable edges
    int threshold = std::max(15, m_edgeThreshold / 6);
    cv::threshold(laplacian, edges, threshold, 255, cv::THRESH_BINARY);

    // Invert: we want white areas where edges are
    cv::bitwise_not(edges, edges);

    return edges;
}

cv::Mat CartoonBufferedFilterProcessor::QuantizeColors(const cv::Mat& src, int levels)
{
    if (src.empty()) {
        return src.clone();
    }

    cv::Mat dst = src.clone();
    int divideValue = std::max(1, 256 / levels);

    // Fast quantization using pointer-based access
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

void CartoonBufferedFilterProcessor::CombineEdgesWithColors(cv::Mat& frame, const cv::Mat& edges)
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
                // Darker outlines for cartoon look
                frameRow[x][0] = cv::saturate_cast<uint8_t>(frameRow[x][0] * 0.3f);
                frameRow[x][1] = cv::saturate_cast<uint8_t>(frameRow[x][1] * 0.3f);
                frameRow[x][2] = cv::saturate_cast<uint8_t>(frameRow[x][2] * 0.3f);
            }
        }
    }
}

#endif  // HAVE_OPENCV

bool CartoonBufferedFilterProcessor::SetParameter(const std::string& name, const std::string& value)
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
        } else if (name == "buffer_size") {
            int size = std::stoi(value);
            SetBufferSize(size);
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "[CartoonBufferedFilterProcessor] SetParameter error: " << e.what() << std::endl;
        return false;
    }
    return false;
}

std::map<std::string, std::string> CartoonBufferedFilterProcessor::GetParameters() const
{
    std::map<std::string, std::string> params;
    params["style"] = std::to_string(static_cast<int>(m_style));
    params["edge_threshold"] = std::to_string(m_edgeThreshold);
    params["smoothing_level"] = std::to_string(m_smoothingLevel);
    params["color_levels"] = std::to_string(m_colorLevels);
    params["buffer_size"] = std::to_string(m_bufferSize);
    return params;
}

double CartoonBufferedFilterProcessor::GetExpectedProcessingTime() const
{
    return m_processingTime;
}

void CartoonBufferedFilterProcessor::SetCartoonStyle(int style)
{
    m_style = static_cast<CartoonStyle>(std::max(0, std::min(2, style)));
    std::cout << "[CartoonBufferedFilterProcessor] Style changed to: " << static_cast<int>(m_style) << std::endl;
}

void CartoonBufferedFilterProcessor::SetEdgeThreshold(int threshold)
{
    m_edgeThreshold = std::max(0, std::min(255, threshold));
}

void CartoonBufferedFilterProcessor::SetSmoothingLevel(int level)
{
    m_smoothingLevel = std::max(1, std::min(10, level));
}

void CartoonBufferedFilterProcessor::SetColorLevels(int levels)
{
    m_colorLevels = std::max(2, std::min(32, levels));
}

void CartoonBufferedFilterProcessor::SetBufferSize(int size)
{
    m_bufferSize = std::max(3, std::min(30, size));  // Between 3 and 30 frames
    std::cout << "[CartoonBufferedFilterProcessor] Buffer size changed to: " << m_bufferSize << " frames" << std::endl;
}
