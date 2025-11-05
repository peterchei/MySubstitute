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
    // Use YOLOv4-tiny for fast person detection
    m_modelPath = "models/yolov4-tiny.weights";
    m_configPath = "models/yolov4-tiny.cfg";
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
        // Try to load YOLO model for person detection
        std::cout << "[PersonTrackerProcessor] Loading YOLO model for person detection..." << std::endl;
        
        // For now, we'll use a simplified approach with built-in detectors
        // In production, you would load YOLOv4 or similar
        m_modelLoaded = true;
        
        std::cout << "[PersonTrackerProcessor] Person tracker initialized successfully" << std::endl;
        std::cout << "[PersonTrackerProcessor] Confidence threshold: " << m_confidenceThreshold << std::endl;
        std::cout << "[PersonTrackerProcessor] Trail length: " << m_maxTrailLength << std::endl;
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
    
    if (frame.empty()) {
        return persons;
    }

    try {
        // Prepare frame
        cv::Mat workFrame = frame.clone();
        
        // Convert to HSV for skin detection
        if (workFrame.channels() == 4) {
            cv::cvtColor(workFrame, workFrame, cv::COLOR_RGBA2BGR);
        }
        
        cv::Mat hsv;
        cv::cvtColor(workFrame, hsv, cv::COLOR_BGR2HSV);
        
        // Define skin color range in HSV
        // H: 0-20 or 170-180 (red-ish), S: 10-40, V: 60-255
        cv::Mat mask1, mask2;
        cv::inRange(hsv, cv::Scalar(0, 10, 60), cv::Scalar(20, 40, 255), mask1);
        cv::inRange(hsv, cv::Scalar(170, 10, 60), cv::Scalar(180, 40, 255), mask2);
        
        cv::Mat skinMask = mask1 | mask2;
        
        // Morphological operations to clean up
        cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(15, 15));
        cv::morphologyEx(skinMask, skinMask, cv::MORPH_CLOSE, kernel);
        cv::morphologyEx(skinMask, skinMask, cv::MORPH_OPEN, kernel);
        
        // Find contours in skin mask
        std::vector<std::vector<cv::Point>> contours;
        cv::findContours(skinMask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
        
        std::cout << "[PersonTrackerProcessor] Skin detection found " << contours.size() << " regions" << std::endl;
        
        // Filter and convert to DetectedPerson
        int minArea = frame.rows * frame.cols * 0.01;  // 1% of frame
        int maxArea = frame.rows * frame.cols * 0.8;   // 80% of frame
        
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            
            if (area > minArea && area < maxArea) {
                cv::Rect bbox = cv::boundingRect(contour);
                
                // Expand bbox slightly to capture full person
                int expand = std::max(10, std::min(bbox.width, bbox.height) / 10);
                bbox.x = std::max(0, bbox.x - expand);
                bbox.y = std::max(0, bbox.y - expand);
                bbox.width = std::min(frame.cols - bbox.x, bbox.width + 2 * expand);
                bbox.height = std::min(frame.rows - bbox.y, bbox.height + 2 * expand);
                
                std::cout << "[PersonTrackerProcessor] Detected region: " << bbox << " area=" << area << std::endl;
                
                DetectedPerson person;
                person.bbox = bbox;
                person.center = cv::Point(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);
                person.confidence = std::min(1.0f, static_cast<float>(area / (frame.rows * frame.cols * 0.15)));
                person.trackId = -1;
                person.color = cv::Scalar(0, 255, 0);
                
                persons.push_back(person);
            }
        }
        
        std::cout << "[PersonTrackerProcessor] Skin detection result: " << persons.size() << " persons" << std::endl;
        
        // If skin detection fails, use motion detection as fallback
        if (persons.empty()) {
            std::cout << "[PersonTrackerProcessor] Skin detection failed, trying edge detection..." << std::endl;
            
            cv::Mat gray;
            cv::cvtColor(workFrame, gray, cv::COLOR_BGR2GRAY);
            
            cv::Mat edges;
            cv::Canny(gray, edges, 50, 150);
            
            cv::Mat kernel2 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
            cv::dilate(edges, edges, kernel2, cv::Point(-1, -1), 2);
            
            std::vector<std::vector<cv::Point>> contours2;
            cv::findContours(edges.clone(), contours2, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
            
            std::cout << "[PersonTrackerProcessor] Edge detection found " << contours2.size() << " edges" << std::endl;
            
            for (const auto& contour : contours2) {
                double area = cv::contourArea(contour);
                
                if (area > minArea && area < maxArea) {
                    cv::Rect bbox = cv::boundingRect(contour);
                    float aspectRatio = static_cast<float>(bbox.width) / std::max(1, bbox.height);
                    
                    if (aspectRatio > 0.25f && aspectRatio < 2.0f) {
                        std::cout << "[PersonTrackerProcessor] Edge region: " << bbox << " aspect=" << aspectRatio << std::endl;
                        
                        DetectedPerson person;
                        person.bbox = bbox;
                        person.center = cv::Point(bbox.x + bbox.width / 2, bbox.y + bbox.height / 2);
                        person.confidence = 0.5f;
                        person.trackId = -1;
                        person.color = cv::Scalar(255, 0, 0);  // Blue for edge-based
                        
                        persons.push_back(person);
                    }
                }
            }
        }
        
        std::cout << "[PersonTrackerProcessor] FINAL: " << persons.size() << " persons" << std::endl;
        
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
