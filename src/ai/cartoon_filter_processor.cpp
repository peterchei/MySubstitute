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
        // Step 1: Apply bilateral filter iteratively
        cv::Mat smoothed = frame.clone();
        for (int i = 0; i < m_smoothingLevel; ++i) {
            cv::Mat temp;
            cv::bilateralFilter(smoothed, temp, 7, 50, 50);
            temp.copyTo(smoothed);
        }

        // Step 2: Quantize colors to reduce palette
        cv::Mat quantized = QuantizeColors(smoothed, m_colorLevels);

        // Step 3: Detect edges
        cv::Mat edges = DetectEdges(smoothed);

        // Step 4: Apply edges
        if (!edges.empty()) {
            CombineEdgesWithColors(quantized, edges);
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
        // More aggressive cartoon effect
        cv::Mat smoothed = frame.clone();
        int iterations = std::max(m_smoothingLevel - 1, 1);
        for (int i = 0; i < iterations; ++i) {
            cv::Mat temp;
            cv::bilateralFilter(smoothed, temp, 9, 40, 40);
            temp.copyTo(smoothed);
        }

        // Fewer colors for more pronounced cartoon effect
        cv::Mat quantized = QuantizeColors(smoothed, std::max(m_colorLevels - 2, 4));

        // Stronger edge detection
        cv::Mat edges = DetectEdges(smoothed);

        // Enhance edges for more visible outlines
        if (!edges.empty()) {
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
            cv::dilate(edges, edges, kernel, cv::Point(-1, -1), 1);
        }

        if (!edges.empty() && !quantized.empty()) {
            CombineEdgesWithColors(quantized, edges);
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
        // Maximum smoothing for anime look
        cv::Mat smoothed = frame.clone();
        int iterations = std::max(m_smoothingLevel, 2);
        for (int i = 0; i < iterations; ++i) {
            cv::Mat temp;
            cv::bilateralFilter(smoothed, temp, 11, 60, 60);
            temp.copyTo(smoothed);
        }

        // Aggressive color reduction for vibrant anime palette
        cv::Mat quantized = QuantizeColors(smoothed, std::min(m_colorLevels, 6));

        // Strong edge detection for anime outlines
        cv::Mat edges = DetectEdges(smoothed);
        if (!edges.empty()) {
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
            cv::dilate(edges, edges, kernel, cv::Point(-1, -1), 2);
        }

        if (!edges.empty() && !quantized.empty()) {
            CombineEdgesWithColors(quantized, edges);
        }

        if (!quantized.empty()) {
            quantized.copyTo(frame);
        }
    } catch (const std::exception& e) {
        std::cerr << "[CartoonFilterProcessor] ApplyAnimeStyle error: " << e.what() << std::endl;
    }
}

cv::Mat CartoonFilterProcessor::DetectEdges(const cv::Mat& src)
{
    if (src.empty()) {
        return cv::Mat::zeros(src.size(), CV_8UC1);
    }

    cv::Mat gray, laplacian, edges, result;

    // Convert to grayscale
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src.clone();
    }

    // Apply Gaussian blur to reduce noise
    cv::GaussianBlur(gray, gray, cv::Size(3, 3), 0);

    // Apply Laplacian operator for edge detection (use CV_16S for proper output)
    cv::Laplacian(gray, laplacian, CV_16S, 1);

    // Convert back to 8-bit
    cv::convertScaleAbs(laplacian, laplacian);

    // Apply threshold to get binary edges - adjust based on m_edgeThreshold
    int threshold = std::max(20, m_edgeThreshold / 3);  // Better scaling
    cv::threshold(laplacian, edges, threshold, 255, cv::THRESH_BINARY);

    // Invert: we want white edges (255) to apply as black lines
    cv::bitwise_not(edges, edges);

    // Dilate to strengthen edges
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(2, 2));
    cv::dilate(edges, edges, kernel, cv::Point(-1, -1), 1);

    return edges;
}

cv::Mat CartoonFilterProcessor::QuantizeColors(const cv::Mat& src, int levels)
{
    if (src.empty()) {
        return src.clone();
    }

    cv::Mat dst = src.clone();
    int divideValue = std::max(1, 256 / levels);

    // Optimize: use pointer-based access for speed
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

    // Use pointer-based access for speed
    for (int y = 0; y < frame.rows; ++y) {
        cv::Vec3b* frameRow = frame.ptr<cv::Vec3b>(y);
        const uint8_t* edgesRow = edges.ptr<uint8_t>(y);
        
        for (int x = 0; x < frame.cols; ++x) {
            if (edgesRow[x] < 200) {  // Edge detected (dark region)
                frameRow[x] = cv::Vec3b(0, 0, 0);  // Pure black outline
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
