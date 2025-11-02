#include "face_filter_processor.h"
#include <iostream>
#include <filesystem>
#include <cmath>

#ifdef HAVE_OPENCV
#include <opencv2/objdetect.hpp>
#include <opencv2/imgproc.hpp>
#endif

FaceFilterProcessor::FaceFilterProcessor()
    : m_glassesEnabled(true)
    , m_hatEnabled(true)
    , m_speechBubbleEnabled(true)
    , m_speechBubbleText("Hello Meeting!")
    , m_frameCounter(0)
{
}

FaceFilterProcessor::~FaceFilterProcessor() {
    Cleanup();
}

bool FaceFilterProcessor::Initialize() {
#ifdef HAVE_OPENCV
    try {
        // Load face detection cascade
        std::string cascadePath = "haarcascade_frontalface_default.xml";

        // Try multiple possible locations for the cascade file
        std::vector<std::string> possiblePaths = {
            cascadePath,
            "data/" + cascadePath,
            "../data/" + cascadePath,
            "D:/DevTools/opencv/build/etc/haarcascades/" + cascadePath,
            "D:/DevTools/opencv/sources/data/haarcascades/" + cascadePath,
            "C:/opencv/data/haarcascades/" + cascadePath
        };

        bool cascadeLoaded = false;
        for (const auto& path : possiblePaths) {
            if (faceCascade.load(path)) {
                std::cout << "[FaceFilter] Loaded face cascade from: " << path << std::endl;
                cascadeLoaded = true;
                break;
            }
        }

        if (!cascadeLoaded) {
            std::cerr << "[FaceFilter] Failed to load face cascade classifier!" << std::endl;
            std::cerr << "[FaceFilter] Please ensure OpenCV data files are available." << std::endl;
            return false;
        }

        std::cout << "[FaceFilter] Face cascade loaded successfully" << std::endl;

        // Load accessory images
        glassesImage = LoadAccessoryImage("glasses.png");
        hatImage = LoadAccessoryImage("funny_hat.png");

        std::cout << "[FaceFilter] Face Filter Processor initialized successfully" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[FaceFilter] Initialization error: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[FaceFilter] OpenCV not available - Face Filter Processor disabled" << std::endl;
    return false;
#endif
}

Frame FaceFilterProcessor::ProcessFrame(const Frame& input) {
    Frame output = input; // Start with a copy

#ifdef HAVE_OPENCV
    if (input.data.empty()) {
        return output;
    }

    try {
        // Detect faces
        std::vector<cv::Rect> faces;
        DetectFaces(input.data, faces);

        std::cout << "[FaceFilter] Detected " << faces.size() << " faces in frame " << m_frameCounter << std::endl;

        // Add status text to show filter is active
        cv::putText(output.data, "FACE FILTER ACTIVE", cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 1.0, cv::Scalar(0, 255, 0), 2);

        // Apply effects to detected faces
        for (const auto& face : faces) {
            std::cout << "[FaceFilter] Processing face at (" << face.x << "," << face.y << ") size " << face.width << "x" << face.height << std::endl;
            
            // Always draw a visible rectangle around detected faces for debugging
            cv::rectangle(output.data, face, cv::Scalar(0, 255, 0), 3); // Green rectangle
            
            if (m_glassesEnabled) {
                AddVirtualGlasses(output.data, face);
            }
            if (m_hatEnabled) {
                AddFunnyHat(output.data, face);
            }
            if (m_speechBubbleEnabled && !m_speechBubbleText.empty()) {
                AddSpeechBubble(output.data, face, m_speechBubbleText);
            }
        }

        m_frameCounter++;

    } catch (const std::exception& e) {
        std::cerr << "[FaceFilter] Frame processing error: " << e.what() << std::endl;
        // Return original frame on error
    }
#endif

    return output;
}

void FaceFilterProcessor::Cleanup() {
#ifdef HAVE_OPENCV
    // Release resources
    glassesImage.release();
    hatImage.release();
    faceCascade = cv::CascadeClassifier();
#endif
    std::cout << "[FaceFilter] Face Filter Processor cleaned up" << std::endl;
}

std::string FaceFilterProcessor::GetName() const {
    return "Face Filter Processor";
}

std::string FaceFilterProcessor::GetVersion() const {
    return "1.0.0";
}

bool FaceFilterProcessor::SupportsRealTime() const {
    return true;
}

bool FaceFilterProcessor::SetParameter(const std::string& name, const std::string& value) {
    if (name == "glasses_enabled") {
        m_glassesEnabled = (value == "true" || value == "1");
        return true;
    } else if (name == "hat_enabled") {
        m_hatEnabled = (value == "true" || value == "1");
        return true;
    } else if (name == "speech_bubble_enabled") {
        m_speechBubbleEnabled = (value == "true" || value == "1");
        return true;
    } else if (name == "speech_bubble_text") {
        m_speechBubbleText = value;
        return true;
    }
    return false;
}

std::map<std::string, std::string> FaceFilterProcessor::GetParameters() const {
    return {
        {"glasses_enabled", m_glassesEnabled ? "true" : "false"},
        {"hat_enabled", m_hatEnabled ? "true" : "false"},
        {"speech_bubble_enabled", m_speechBubbleEnabled ? "true" : "false"},
        {"speech_bubble_text", m_speechBubbleText}
    };
}

double FaceFilterProcessor::GetExpectedProcessingTime() const {
    return 50.0; // ~50ms per frame (rough estimate for face detection)
}

void FaceFilterProcessor::SetGlassesEnabled(bool enabled) {
    m_glassesEnabled = enabled;
}

void FaceFilterProcessor::SetHatEnabled(bool enabled) {
    m_hatEnabled = enabled;
}

void FaceFilterProcessor::SetSpeechBubbleEnabled(bool enabled) {
    m_speechBubbleEnabled = enabled;
}

void FaceFilterProcessor::SetSpeechBubbleText(const std::string& text) {
    m_speechBubbleText = text;
}

#ifdef HAVE_OPENCV

void FaceFilterProcessor::DetectFaces(const cv::Mat& frame, std::vector<cv::Rect>& faces) {
    if (faceCascade.empty()) {
        std::cerr << "[FaceFilter] Face cascade not loaded!" << std::endl;
        return;
    }

    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    // Detect faces
    faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));
    
    std::cout << "[FaceFilter] Face detection completed, found " << faces.size() << " faces" << std::endl;
}

void FaceFilterProcessor::AddVirtualGlasses(cv::Mat& frame, const cv::Rect& face) {
    // Calculate glass dimensions based on face size
    int eyeY = face.y + face.height * 0.35;
    int glassesWidth = face.width * 0.7;
    int glassesHeight = glassesWidth * 0.4;
    int glassesX = face.x + face.width * 0.15;
    int glassesY = eyeY - glassesHeight / 2;

    // Draw left lens
    cv::ellipse(frame, 
                cv::Point(glassesX + glassesWidth / 4, glassesY + glassesHeight / 2),
                cv::Size(glassesWidth / 4, glassesHeight / 2),
                0, 0, 360, cv::Scalar(100, 200, 255), 3);  // Cyan outline
    cv::ellipse(frame, 
                cv::Point(glassesX + glassesWidth / 4, glassesY + glassesHeight / 2),
                cv::Size(glassesWidth / 4 - 3, glassesHeight / 2 - 3),
                0, 0, 360, cv::Scalar(100, 200, 255), -1);  // Filled cyan

    // Draw right lens
    cv::ellipse(frame, 
                cv::Point(glassesX + 3 * glassesWidth / 4, glassesY + glassesHeight / 2),
                cv::Size(glassesWidth / 4, glassesHeight / 2),
                0, 0, 360, cv::Scalar(100, 200, 255), 3);  // Cyan outline
    cv::ellipse(frame, 
                cv::Point(glassesX + 3 * glassesWidth / 4, glassesY + glassesHeight / 2),
                cv::Size(glassesWidth / 4 - 3, glassesHeight / 2 - 3),
                0, 0, 360, cv::Scalar(100, 200, 255), -1);  // Filled cyan

    // Draw bridge between lenses
    int bridgeY = glassesY + glassesHeight / 2;
    cv::line(frame,
             cv::Point(glassesX + glassesWidth / 4 + glassesWidth / 8, bridgeY),
             cv::Point(glassesX + 3 * glassesWidth / 4 - glassesWidth / 8, bridgeY),
             cv::Scalar(100, 200, 255), 2);

    // Add shine effect (animation based on frame counter)
    int shineOffset = (m_frameCounter / 5) % (glassesWidth / 4);
    cv::line(frame,
             cv::Point(glassesX + glassesWidth / 4 - glassesWidth / 8 + shineOffset, glassesY + glassesHeight / 4),
             cv::Point(glassesX + glassesWidth / 4 - glassesWidth / 8 + shineOffset + glassesWidth / 8, glassesY + glassesHeight / 4 - glassesHeight / 8),
             cv::Scalar(255, 255, 200), 2);  // Light yellow shine
}

void FaceFilterProcessor::AddFunnyHat(cv::Mat& frame, const cv::Rect& face) {
    // Draw a party hat above the face
    int hatX = face.x + face.width / 2;
    int hatY = face.y - face.height * 0.3;
    int hatWidth = face.width * 0.6;
    int hatHeight = face.height * 0.4;

    // Hat color changes based on frame counter for animation
    int colorCycle = (m_frameCounter / 10) % 3;
    cv::Scalar hatColor;
    switch (colorCycle) {
        case 0: hatColor = cv::Scalar(0, 255, 255);    break;  // Cyan
        case 1: hatColor = cv::Scalar(255, 0, 255);    break;  // Magenta
        case 2: hatColor = cv::Scalar(255, 255, 0);    break;  // Yellow
        default: hatColor = cv::Scalar(0, 255, 255);   break;  // Cyan
    }

    // Draw hat body (triangle)
    std::vector<cv::Point> hatPoints = {
        cv::Point(hatX, hatY + hatHeight),           // Bottom middle
        cv::Point(hatX - hatWidth / 2, hatY),        // Top left
        cv::Point(hatX + hatWidth / 2, hatY)         // Top right
    };
    cv::fillConvexPoly(frame, hatPoints, hatColor);
    std::vector<std::vector<cv::Point>> hatPointsVec = {hatPoints};
    cv::polylines(frame, hatPointsVec, true, cv::Scalar(0, 0, 0), 2, cv::LINE_AA);  // Black outline

    // Draw hat base
    cv::rectangle(frame,
                  cv::Point(hatX - hatWidth / 2 - 5, hatY + hatHeight - 10),
                  cv::Point(hatX + hatWidth / 2 + 5, hatY + hatHeight + 5),
                  hatColor, -1);
    cv::rectangle(frame,
                  cv::Point(hatX - hatWidth / 2 - 5, hatY + hatHeight - 10),
                  cv::Point(hatX + hatWidth / 2 + 5, hatY + hatHeight + 5),
                  cv::Scalar(0, 0, 0), 2);

    // Draw pom-pom on top (bobbing animation)
    int pompomOffset = (int)(3 * sin(m_frameCounter * 0.1));  // Sine wave bob
    int pompomRadius = hatWidth / 8;
    cv::circle(frame,
               cv::Point(hatX, hatY - pompomRadius + pompomOffset),
               pompomRadius,
               cv::Scalar(255, 100, 100), -1);  // Red pom-pom
    cv::circle(frame,
               cv::Point(hatX, hatY - pompomRadius + pompomOffset),
               pompomRadius,
               cv::Scalar(0, 0, 0), 2);  // Black outline
}

void FaceFilterProcessor::AddSpeechBubble(cv::Mat& frame, const cv::Rect& face, const std::string& text) {
    if (text.empty()) return;

    // Position speech bubble above the face
    int bubbleX = face.x + face.width / 2;
    int bubbleY = face.y - 30;

    // Calculate bubble size based on text length
    int bubbleWidth = std::max(120, static_cast<int>(text.length() * 10 + 20));
    int bubbleHeight = 50;

    // Clamp to frame boundaries
    int drawX = bubbleX - bubbleWidth / 2;
    int drawY = bubbleY - bubbleHeight;
    
    if (drawX < 10) drawX = 10;
    if (drawY < 10) drawY = 10;
    if (drawX + bubbleWidth > frame.cols - 10) drawX = frame.cols - bubbleWidth - 10;

    // Draw bubble background with gradient effect
    cv::Scalar bubbleBG = cv::Scalar(255, 255, 200);    // Light yellow
    cv::Scalar bubbleBorder = cv::Scalar(0, 0, 0);      // Black border
    cv::Scalar textColor = cv::Scalar(0, 0, 0);         // Black text

    // Draw filled rectangle
    cv::rectangle(frame,
                  cv::Point(drawX, drawY),
                  cv::Point(drawX + bubbleWidth, drawY + bubbleHeight),
                  bubbleBG, -1);

    // Draw border
    cv::rectangle(frame,
                  cv::Point(drawX, drawY),
                  cv::Point(drawX + bubbleWidth, drawY + bubbleHeight),
                  bubbleBorder, 2);

    // Draw pointer triangle to face
    int pointerX = face.x + face.width / 2;
    int pointerY = bubbleY + bubbleHeight;
    
    std::vector<cv::Point> pointerPoints = {
        cv::Point(pointerX, pointerY + 15),
        cv::Point(pointerX - 10, pointerY),
        cv::Point(pointerX + 10, pointerY)
    };
    cv::fillConvexPoly(frame, pointerPoints, bubbleBG);
    std::vector<std::vector<cv::Point>> pointerPointsVec = {pointerPoints};
    cv::polylines(frame, pointerPointsVec, true, bubbleBorder, 2);

    // Add text with wrapping
    std::string displayText = text;
    if (displayText.length() > 20) {
        displayText = displayText.substr(0, 17) + "...";
    }

    cv::putText(frame, displayText,
                cv::Point(drawX + 10, drawY + bubbleHeight - 12),
                cv::FONT_HERSHEY_SIMPLEX, 0.5, textColor, 1);

    // Add animated dots (pulsing effect)
    int pulse = (m_frameCounter / 5) % 4;
    std::string dots = "";
    for (int i = 0; i < pulse; i++) dots += ".";
    
    if (!dots.empty()) {
        cv::putText(frame, dots,
                    cv::Point(drawX + bubbleWidth - 30, drawY + bubbleHeight - 12),
                    cv::FONT_HERSHEY_SIMPLEX, 0.6, cv::Scalar(100, 100, 255), 1);
    }
}

cv::Mat FaceFilterProcessor::LoadAccessoryImage(const std::string& filename) {
    cv::Mat image;

    // Try multiple possible locations
    std::vector<std::string> possiblePaths = {
        filename,
        "assets/" + filename,
        "images/" + filename,
        "../assets/" + filename,
        "../images/" + filename
    };

    for (const auto& path : possiblePaths) {
        image = cv::imread(path, cv::IMREAD_UNCHANGED);
        if (!image.empty()) {
            std::cout << "[FaceFilter] Loaded accessory: " << path << std::endl;
            break;
        }
    }

    if (image.empty()) {
        std::cout << "[FaceFilter] Could not load accessory: " << filename << std::endl;
        std::cout << "[FaceFilter] Creating placeholder accessory..." << std::endl;

        // Create a simple colored rectangle as placeholder
        if (filename.find("glasses") != std::string::npos) {
            image = cv::Mat(50, 100, CV_8UC4, cv::Scalar(0, 255, 255, 128)); // Semi-transparent cyan
        } else if (filename.find("hat") != std::string::npos) {
            image = cv::Mat(60, 120, CV_8UC4, cv::Scalar(255, 0, 255, 128)); // Semi-transparent magenta
        }
    }

    return image;
}

void FaceFilterProcessor::OverlayImage(cv::Mat& background, const cv::Mat& overlay, const cv::Point& position) {
    if (overlay.empty()) return;

    // Ensure overlay fits within background bounds
    cv::Rect roi(position.x, position.y, overlay.cols, overlay.rows);
    roi = roi & cv::Rect(0, 0, background.cols, background.rows);

    if (roi.width <= 0 || roi.height <= 0) return;

    // Extract the region of interest
    cv::Mat backgroundROI = background(roi);

    // Handle different overlay formats
    if (overlay.channels() == 4) { // RGBA overlay
        cv::Mat overlayROI = overlay(cv::Rect(0, 0, roi.width, roi.height));

        // Split channels
        std::vector<cv::Mat> overlayChannels;
        cv::split(overlayROI, overlayChannels);

        // Create mask from alpha channel
        cv::Mat mask = overlayChannels[3];

        // Blend images
        cv::Mat blended;
        cv::addWeighted(backgroundROI, 1.0, overlayROI, 0.0, 0.0, blended);

        // Apply mask
        overlayChannels[0].copyTo(blended, mask);
        overlayChannels[1].copyTo(blended, mask);
        overlayChannels[2].copyTo(blended, mask);

        blended.copyTo(backgroundROI);
    } else { // Regular BGR overlay
        cv::Mat overlayROI = overlay(cv::Rect(0, 0, roi.width, roi.height));
        overlayROI.copyTo(backgroundROI);
    }
}

#endif // HAVE_OPENCV