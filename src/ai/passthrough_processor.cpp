#include "passthrough_processor.h"
#include <opencv2/opencv.hpp>

PassthroughProcessor::PassthroughProcessor() 
    : m_addTimestamp(false), m_addWatermark(false) {
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
    
    // Add timestamp if enabled
    if (m_addTimestamp) {
        std::string timeText = "Time: " + std::to_string(static_cast<long long>(output.timestamp));
        cv::putText(output.data, timeText, cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(255, 255, 255), 2);
    }
    
    // Add watermark if enabled
    if (m_addWatermark) {
        std::string watermark = "MySubstitute";
        cv::Size textSize = cv::getTextSize(watermark, cv::FONT_HERSHEY_SIMPLEX, 0.5, 1, nullptr);
        cv::Point position(output.width - textSize.width - 10, output.height - 10);
        
        cv::putText(output.data, watermark, position,
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(200, 200, 200), 1);
    }
    
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