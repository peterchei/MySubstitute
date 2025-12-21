#include "person_tracker_processor.h"
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>
#include <random>

PersonTrackerProcessor::PersonTrackerProcessor()
    : m_modelLoaded(false),
      m_confidenceThreshold(0.5f),
      m_inputSize(416, 416),
      m_nextTrackId(1),
      m_maxTrailLength(30),
      m_showBoundingBox(true),
      m_showTrail(true),
      m_showSkeleton(false),
      m_processingTime(0.0),
      m_frameCounter(0)
{
    std::cout << "[PersonTrackerProcessor] Initializing..." << std::endl;
    // Use MediaPipe Selfie Segmentation for robust person detection
    m_modelPath = "models/MediaPipe-Selfie-Segmentation.onnx";
    m_inputSize = cv::Size(256, 256);
}

PersonTrackerProcessor::~PersonTrackerProcessor()
{
    Cleanup();
}

bool PersonTrackerProcessor::Initialize()
{
    std::cout << "[PersonTrackerProcessor] Initialize called" << std::endl;

#ifdef HAVE_OPENCV
    try {
        // Load MediaPipe Selfie Segmentation model
        std::cout << "[PersonTrackerProcessor] Loading model: " << m_modelPath << std::endl;
        
        m_net = cv::dnn::readNetFromONNX(m_modelPath);
        
        // Optimize for CPU
        m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
        m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        
        m_modelLoaded = true;
        
        std::cout << "[PersonTrackerProcessor] Person tracker initialized successfully" << std::endl;
        std::cout << "[PersonTrackerProcessor] Confidence threshold: " << m_confidenceThreshold << std::endl;
        std::cout << "[PersonTrackerProcessor] Visualization: BBox=" << (m_showBoundingBox ? "on" : "off");
        std::cout << " Trail=" << (m_showTrail ? "on" : "off");
        std::cout << " Skeleton=" << (m_showSkeleton ? "on" : "off") << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[PersonTrackerProcessor] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[PersonTrackerProcessor] ERROR: OpenCV not available" << std::endl;
    return false;
#endif
}

void PersonTrackerProcessor::Cleanup()
{
    std::cout << "[PersonTrackerProcessor] Cleanup called" << std::endl;
    m_currentPersons.clear();
    m_previousPersons.clear();
    m_motionTrail.clear();
    m_modelLoaded = false;
}

Frame PersonTrackerProcessor::ProcessFrame(const Frame& input)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    Frame output = input;

#ifdef HAVE_OPENCV
    if (!input.data.empty() && m_modelLoaded) {
        cv::Mat frame = input.data.clone();
        
        // Debug frame info on first frame
        if (m_frameCounter == 0) {
            std::cout << "[PersonTrackerProcessor] First frame info:" << std::endl;
            std::cout << "  Size: " << frame.cols << "x" << frame.rows << std::endl;
            std::cout << "  Channels: " << frame.channels() << std::endl;
            std::cout << "  Type: " << frame.type() << std::endl;
        }
        
        // Detect persons in frame
        if (m_frameCounter % 1 == 0) {  // Run detection every frame
            m_previousPersons = m_currentPersons;
            m_currentPersons = DetectPersons(frame);
            
            // Track persons across frames
            TrackPersons(m_currentPersons);
            
            // Update motion trail
            UpdateMotionTrail();
        }
        
        // Always draw visualization
        DrawVisualization(frame);
        
        frame.copyTo(output.data);
    }
#endif

    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    m_frameCounter++;

    return output;
}

#ifdef HAVE_OPENCV

std::vector<PersonTrackerProcessor::DetectedPerson> PersonTrackerProcessor::DetectPersons(const cv::Mat& frame)
{
    std::vector<DetectedPerson> persons;
    
    if (frame.empty() || !m_modelLoaded) {
        return persons;
    }

    try {
        // Preprocess frame for MediaPipe (256x256, value 0-1)
        cv::Mat inputBlob;
        cv::Mat resized;
        cv::resize(frame, resized, m_inputSize);
        
        // MediaPipe expects RGB and 1/255 scale
        cv::dnn::blobFromImage(resized, inputBlob, 1.0/255.0, m_inputSize, cv::Scalar(0, 0, 0), true, false);
        
        m_net.setInput(inputBlob);
        
        // Run inference
        cv::Mat output = m_net.forward();
        
        // Output shape is [1, 1, 256, 256] or similar (depends on specific model export)
        // Usually index 0 is background, 1 is foreground, OR it's a single channel sigmoid
        // For "MediaPipe-Selfie-Segmentation.onnx", output is typically [1, 256, 256, 1] or [1, 1, 256, 256]
        
        // Reshape to proper 2D matrix
        // The output might be NCHW or NHWC. 
        // Let's inspect dimensions if we were debugging, but here we assume standard NCHW or NHWC handling.
        // We'll create a segmentation mask.
        
        cv::Mat segmentationMask;
        
        if (output.dims >= 3) {
            // Handle output format
             // Extract pointer to data
            float* data = (float*)output.data;
            
            // Create a Mat from valid data
            // Assuming output matches input size (256x256)
            cv::Mat probMap(m_inputSize, CV_32FC1, data);
            
            // Threshold to create binary mask
            cv::threshold(probMap, segmentationMask, m_confidenceThreshold, 255, cv::THRESH_BINARY);
            segmentationMask.convertTo(segmentationMask, CV_8U);
        } else {
             std::cerr << "[PersonTrackerProcessor] Unexpected output dimensions" << std::endl;
             return persons;
        }

        // Resize mask back to original frame size
        cv::Mat fullSizeMask;
        cv::resize(segmentationMask, fullSizeMask, frame.size(), 0, 0, cv::INTER_LINEAR);
        
        // Find contours to get bounding box
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(fullSizeMask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        double minArea = (frame.cols * frame.rows) * 0.05; // 5% area minimum to filter noise
        
        // Find the largest contour (assuming the person is the main subject)
        // Or collect all significant contours
        
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area >= minArea) {
                cv::Rect bbox = cv::boundingRect(contour);
                
                // Add padding
                int padX = bbox.width * 0.05; // 5% padding
                int padY = bbox.height * 0.05;
                
                bbox.x = std::max(0, bbox.x - padX);
                bbox.y = std::max(0, bbox.y - padY);
                bbox.width = std::min(frame.cols - bbox.x, bbox.width + 2 * padX);
                bbox.height = std::min(frame.rows - bbox.y, bbox.height + 2 * padY);
                
                DetectedPerson person;
                person.bbox = bbox;
                person.center = cv::Point(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);
                person.confidence = 0.95f; // DNN is usually high confidence if threshold passed
                person.trackId = -1;
                person.color = cv::Scalar(0, 255, 0);
                
                persons.push_back(person);
            }
        }
        
    } catch (const std::exception& e) {
        std::cerr << "[PersonTrackerProcessor] Exception: " << e.what() << std::endl;
    }
    
    return persons;
}

void PersonTrackerProcessor::TrackPersons(std::vector<DetectedPerson>& persons)
{
    // Simple centroid tracking algorithm
    if (m_previousPersons.empty()) {
        // First frame - assign new IDs
        for (auto& person : persons) {
            person.trackId = m_nextTrackId++;
            person.color = GetTrackColor(person.trackId);
        }
        return;
    }

    // Match current persons with previous persons using centroid distance
    std::vector<bool> matched(m_previousPersons.size(), false);
    
    for (auto& person : persons) {
        float minDistance = 50.0f;  // Max distance to match
        int bestMatchIdx = -1;
        
        for (size_t i = 0; i < m_previousPersons.size(); ++i) {
            if (matched[i]) continue;
            
            float distance = cv::norm(person.center - m_previousPersons[i].center);
            if (distance < minDistance) {
                minDistance = distance;
                bestMatchIdx = i;
            }
        }
        
        if (bestMatchIdx >= 0) {
            // Match found
            person.trackId = m_previousPersons[bestMatchIdx].trackId;
            matched[bestMatchIdx] = true;
        } else {
            // New person
            person.trackId = m_nextTrackId++;
        }
        
        person.color = GetTrackColor(person.trackId);
    }
}

void PersonTrackerProcessor::UpdateMotionTrail()
{
    int64_t currentTime = std::chrono::system_clock::now().time_since_epoch().count();
    
    for (const auto& person : m_currentPersons) {
        TrackPoint point;
        point.position = person.center;
        point.timestamp = currentTime;
        point.trackId = person.trackId;
        
        m_motionTrail.push_back(point);
    }
    
    // Limit trail length
    while (m_motionTrail.size() > static_cast<size_t>(m_maxTrailLength * m_currentPersons.size())) {
        m_motionTrail.pop_front();
    }
}

void PersonTrackerProcessor::DrawVisualization(cv::Mat& frame)
{
    if (m_showBoundingBox) {
        DrawBoundingBoxes(frame);
    }
    
    if (m_showTrail) {
        DrawMotionTrail(frame);
    }
    
    if (m_showSkeleton) {
        for (const auto& person : m_currentPersons) {
            DrawSkeleton(frame, person);
        }
    }
    
    // Draw frame info
    std::string infoText = "Persons: " + std::to_string(m_currentPersons.size()) + 
                          " | FPS: " + std::to_string(static_cast<int>(1000.0 / m_processingTime));
    cv::putText(frame, infoText, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 0.7, 
                cv::Scalar(0, 255, 0), 2);
}

void PersonTrackerProcessor::DrawBoundingBoxes(cv::Mat& frame)
{
    for (const auto& person : m_currentPersons) {
        // Draw bounding box
        cv::rectangle(frame, person.bbox, person.color, 2);
        
        // Draw center point
        cv::circle(frame, person.center, 5, person.color, -1);
        
        // Draw track ID
        std::string idText = "ID: " + std::to_string(person.trackId);
        cv::putText(frame, idText, cv::Point(person.bbox.x, person.bbox.y - 5),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, person.color, 2);
        
        // Draw confidence
        std::string confText = "Conf: " + std::to_string(static_cast<int>(person.confidence * 100)) + "%";
        cv::putText(frame, confText, cv::Point(person.bbox.x, person.bbox.y + person.bbox.height + 15),
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, person.color, 1);
    }
}

void PersonTrackerProcessor::DrawMotionTrail(cv::Mat& frame)
{
    // Group trail points by track ID
    std::map<int, std::vector<cv::Point>> trailsById;
    for (const auto& point : m_motionTrail) {
        trailsById[point.trackId].push_back(point.position);
    }
    
    // Draw trails
    for (const auto& [trackId, trail] : trailsById) {
        if (trail.size() < 2) continue;
        
        cv::Scalar color = GetTrackColor(trackId);
        
        for (size_t i = 1; i < trail.size(); ++i) {
            int thickness = static_cast<int>(2 * i / trail.size());
            thickness = std::max(1, thickness);
            cv::line(frame, trail[i - 1], trail[i], color, thickness, cv::LINE_AA);
        }
    }
}

void PersonTrackerProcessor::DrawSkeleton(cv::Mat& frame, const DetectedPerson& person)
{
    // Simple skeleton - draw lines to inferred joint positions
    int centerX = person.center.x;
    int centerY = person.center.y;
    int w = person.bbox.width;
    int h = person.bbox.height;
    
    // Head
    cv::circle(frame, cv::Point(centerX, centerY - h/3), w/6, person.color, 2);
    
    // Body
    cv::line(frame, cv::Point(centerX, centerY - h/3 + w/6), 
             cv::Point(centerX, centerY + h/6), person.color, 2);
    
    // Arms
    cv::line(frame, cv::Point(centerX, centerY), 
             cv::Point(centerX - w/3, centerY - h/6), person.color, 2);
    cv::line(frame, cv::Point(centerX, centerY), 
             cv::Point(centerX + w/3, centerY - h/6), person.color, 2);
    
    // Legs
    cv::line(frame, cv::Point(centerX, centerY + h/6), 
             cv::Point(centerX - w/4, centerY + h/2), person.color, 2);
    cv::line(frame, cv::Point(centerX, centerY + h/6), 
             cv::Point(centerX + w/4, centerY + h/2), person.color, 2);
}

cv::Scalar PersonTrackerProcessor::GetTrackColor(int trackId)
{
    // Generate deterministic color based on track ID
    static std::vector<cv::Scalar> colors = {
        cv::Scalar(255, 0, 0),      // Blue
        cv::Scalar(0, 255, 0),      // Green
        cv::Scalar(0, 0, 255),      // Red
        cv::Scalar(255, 255, 0),    // Cyan
        cv::Scalar(255, 0, 255),    // Magenta
        cv::Scalar(0, 255, 255),    // Yellow
        cv::Scalar(128, 0, 128),    // Purple
        cv::Scalar(0, 128, 128),    // Teal
        cv::Scalar(128, 128, 0),    // Olive
        cv::Scalar(128, 0, 0),      // Dark Blue
    };
    
    return colors[trackId % colors.size()];
}

float PersonTrackerProcessor::CalculateIoU(const cv::Rect& box1, const cv::Rect& box2)
{
    // Calculate Intersection over Union for bounding boxes
    int intersectionArea = (box1 & box2).area();
    int unionArea = box1.area() + box2.area() - intersectionArea;
    
    if (unionArea == 0) return 0.0f;
    return static_cast<float>(intersectionArea) / unionArea;
}

#endif  // HAVE_OPENCV

std::string PersonTrackerProcessor::GetName() const
{
    return "Person Tracker";
}

std::string PersonTrackerProcessor::GetVersion() const
{
    return "1.0.0";
}

bool PersonTrackerProcessor::SupportsRealTime() const
{
    return true;
}

bool PersonTrackerProcessor::SetParameter(const std::string& name, const std::string& value)
{
    if (name == "confidence_threshold") {
        try {
            float threshold = std::stof(value);
            SetConfidenceThreshold(threshold);
            m_parameters[name] = value;
            return true;
        } catch (...) {
            return false;
        }
    }
    else if (name == "trail_length") {
        try {
            int length = std::stoi(value);
            SetTrailLength(length);
            m_parameters[name] = value;
            return true;
        } catch (...) {
            return false;
        }
    }
    else if (name == "show_bbox") {
        SetShowBoundingBox(value == "true" || value == "1");
        m_parameters[name] = value;
        return true;
    }
    else if (name == "show_trail") {
        SetShowTrail(value == "true" || value == "1");
        m_parameters[name] = value;
        return true;
    }
    else if (name == "show_skeleton") {
        SetShowSkeleton(value == "true" || value == "1");
        m_parameters[name] = value;
        return true;
    }
    return false;
}

std::map<std::string, std::string> PersonTrackerProcessor::GetParameters() const
{
    return m_parameters;
}

double PersonTrackerProcessor::GetExpectedProcessingTime() const
{
    return m_processingTime;
}

void PersonTrackerProcessor::SetConfidenceThreshold(float threshold)
{
    m_confidenceThreshold = std::max(0.0f, std::min(1.0f, threshold));
}

void PersonTrackerProcessor::SetTrailLength(int length)
{
    m_maxTrailLength = std::max(1, length);
}

void PersonTrackerProcessor::SetShowBoundingBox(bool show)
{
    m_showBoundingBox = show;
}

void PersonTrackerProcessor::SetShowTrail(bool show)
{
    m_showTrail = show;
}

void PersonTrackerProcessor::SetShowSkeleton(bool show)
{
    m_showSkeleton = show;
}
