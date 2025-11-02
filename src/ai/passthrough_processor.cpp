#include "passthrough_processor.h"
#include <opencv2/opencv.hpp>
#include <chrono>
#include <iomanip>
#include <sstream>

PassthroughProcessor::PassthroughProcessor() 
    : m_addTimestamp(false)
    , m_addWatermark(false)
    , m_addCaption(true)  // Enable caption by default
    , m_captionText("MySubstitute Virtual Camera")
    , m_captionX(10)
    , m_captionY(30)  // Distance from bottom
    , m_captionColor(255, 255, 255)  // White
    , m_captionScale(0.8)
    , m_captionThickness(2)
    , m_frameCounter(0)
{
}

PassthroughProcessor::~PassthroughProcessor() {
    Cleanup();
}

bool PassthroughProcessor::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    // No special initialization needed for passthrough
    m_initialized = true;
    return true;
}

Frame PassthroughProcessor::ProcessFrame(const Frame& input) {
    if (!m_initialized || !input.IsValid()) {
        return input;
    }
    
    // Create a copy of the input frame
    Frame output = input.Clone();
    
#if HAVE_OPENCV
    // Convert frame data to OpenCV Mat if needed
    cv::Mat frame = output.data;
    if (frame.empty()) {
        return output;  // Return original if conversion failed
    }
    
    // Add overlays
    if (m_addTimestamp) {
        AddTimestamp(frame);
    }
    
    if (m_addWatermark) {
        AddWatermark(frame);
    }
    
    if (m_addCaption) {
        AddCaption(frame);
    }
    
    // Update the output frame data
    output.data = frame;
#endif
    
    return output;
}

void PassthroughProcessor::Cleanup() {
    m_initialized = false;
}

std::string PassthroughProcessor::GetName() const {
    return "Passthrough Processor";
}

std::string PassthroughProcessor::GetVersion() const {
    return "1.0.0";
}

bool PassthroughProcessor::SupportsRealTime() const {
    return true;
}

bool PassthroughProcessor::SetParameter(const std::string& name, const std::string& value) {
    if (name == "add_timestamp") {
        m_addTimestamp = (value == "true" || value == "1");
        m_parameters[name] = value;
        return true;
    } else if (name == "add_watermark") {
        m_addWatermark = (value == "true" || value == "1");
        m_parameters[name] = value;
        return true;
    } else if (name == "add_caption") {
        m_addCaption = (value == "true" || value == "1");
        m_parameters[name] = value;
        return true;
    } else if (name == "caption_text") {
        m_captionText = value;
        m_parameters[name] = value;
        return true;
    }
    
    return false;
}

std::map<std::string, std::string> PassthroughProcessor::GetParameters() const {
    return m_parameters;
}

double PassthroughProcessor::GetExpectedProcessingTime() const {
    // Passthrough is very fast, usually < 1ms
    return 0.5;
}

void PassthroughProcessor::SetCaptionText(const std::string& text) {
    m_captionText = text;
}

void PassthroughProcessor::SetCaptionEnabled(bool enabled) {
    m_addCaption = enabled;
}

void PassthroughProcessor::SetCaptionPosition(int x, int y) {
    m_captionX = x;
    m_captionY = y;
}

void PassthroughProcessor::AddTimestamp(cv::Mat& frame) {
#if HAVE_OPENCV
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    
    cv::putText(frame, ss.str(), cv::Point(10, 30), 
               cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    
    // Add semi-transparent background for better readability
    cv::Size textSize = cv::getTextSize(ss.str(), cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, nullptr);
    cv::rectangle(frame, cv::Point(5, 5), cv::Point(15 + textSize.width, 35 + textSize.height), 
                 cv::Scalar(0, 0, 0, 128), -1);
#endif
}

void PassthroughProcessor::AddWatermark(cv::Mat& frame) {
#if HAVE_OPENCV
    std::string watermark = "MySubstitute";
    cv::Size textSize = cv::getTextSize(watermark, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
    cv::Point position(frame.cols - textSize.width - 10, frame.rows - 10);
    
    cv::putText(frame, watermark, position,
               cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200, 180), 1);
#endif
}

void PassthroughProcessor::AddCaption(cv::Mat& frame) {
#if HAVE_OPENCV
    if (m_captionText.empty()) {
        return;
    }
    
    // Generate current timestamp with high precision
    auto now = std::chrono::high_resolution_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    auto us = std::chrono::duration_cast<std::chrono::microseconds>(now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%H:%M:%S");
    ss << "." << std::setfill('0') << std::setw(3) << ms.count();
    ss << std::setfill('0') << std::setw(3) << us.count();  // Add microseconds for more precision
    
    // Increment frame counter for this frame
    uint64_t currentFrame = ++m_frameCounter;
    
    // Append timestamp and frame number to caption text
    std::string fullCaptionText = m_captionText + " - " + ss.str() + " (Frame #" + std::to_string(currentFrame) + ")";
    
    // Calculate text size for positioning
    cv::Size textSize = cv::getTextSize(fullCaptionText, cv::FONT_HERSHEY_SIMPLEX, 
                                       m_captionScale, m_captionThickness, nullptr);
    
    // Position at bottom center by default, or use custom position
    cv::Point position;
    if (m_captionX == 10 && m_captionY == 30) {  // Default position
        position.x = (frame.cols - textSize.width) / 2;  // Center horizontally
        position.y = frame.rows - m_captionY;  // Distance from bottom
    } else {
        position.x = m_captionX;
        position.y = frame.rows - m_captionY;
    }
    
    // Add semi-transparent background rectangle for better readability
    cv::Point bg_p1(position.x - 10, position.y - textSize.height - 10);
    cv::Point bg_p2(position.x + textSize.width + 10, position.y + 5);
    
    // Create overlay for transparency
    cv::Mat overlay;
    frame.copyTo(overlay);
    cv::rectangle(overlay, bg_p1, bg_p2, cv::Scalar(0, 0, 0), -1);
    cv::addWeighted(frame, 0.7, overlay, 0.3, 0, frame);
    
    // Add the caption text with timestamp
    cv::putText(frame, fullCaptionText, position, cv::FONT_HERSHEY_SIMPLEX, 
               m_captionScale, m_captionColor, m_captionThickness);
#endif
}