#include "face_filter_processor.h"
#include <iostream>
#include <filesystem>

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
    if (glassesImage.empty()) return;

    // Position glasses on the face (centered on eyes area)
    int glassesWidth = face.width * 0.8;
    int glassesHeight = glassesWidth * glassesImage.rows / glassesImage.cols;

    cv::Point position(
        face.x + face.width * 0.1,
        face.y + face.height * 0.35
    );

    // Resize glasses to fit
    cv::Mat resizedGlasses;
    cv::resize(glassesImage, resizedGlasses, cv::Size(glassesWidth, glassesHeight));

    OverlayImage(frame, resizedGlasses, position);
}

void FaceFilterProcessor::AddFunnyHat(cv::Mat& frame, const cv::Rect& face) {
    if (hatImage.empty()) return;

    // Position hat above the face
    int hatWidth = face.width * 1.2;
    int hatHeight = hatWidth * hatImage.rows / hatImage.cols;

    cv::Point position(
        face.x + face.width * 0.5 - hatWidth * 0.5,
        face.y - hatHeight * 0.3
    );

    // Resize hat to fit
    cv::Mat resizedHat;
    cv::resize(hatImage, resizedHat, cv::Size(hatWidth, hatHeight));

    OverlayImage(frame, resizedHat, position);
}

void FaceFilterProcessor::AddSpeechBubble(cv::Mat& frame, const cv::Rect& face, const std::string& text) {
    // Position speech bubble above the face
    cv::Point bubblePosition(face.x + face.width / 2, face.y - 20);

    // Draw speech bubble background
    int bubbleWidth = std::max(150, static_cast<int>(text.length() * 12));
    int bubbleHeight = 40;

    cv::Rect bubbleRect(
        bubblePosition.x - bubbleWidth / 2,
        bubblePosition.y - bubbleHeight,
        bubbleWidth,
        bubbleHeight
    );

    // Draw rounded rectangle for bubble
    cv::Scalar bubbleColor(255, 255, 255); // White background
    cv::Scalar textColor(0, 0, 0); // Black text

    cv::rectangle(frame, bubbleRect, bubbleColor, cv::FILLED);
    cv::rectangle(frame, bubbleRect, cv::Scalar(0, 0, 0), 2); // Black border

    // Add text
    cv::putText(frame, text,
                cv::Point(bubbleRect.x + 10, bubbleRect.y + bubbleRect.height - 10),
                cv::FONT_HERSHEY_SIMPLEX, 0.7, textColor, 2);

    // Draw pointer to face
    cv::Point pointerPoints[3] = {
        cv::Point(bubblePosition.x, bubblePosition.y),
        cv::Point(bubblePosition.x - 10, bubblePosition.y - 10),
        cv::Point(bubblePosition.x + 10, bubblePosition.y - 10)
    };

    cv::fillConvexPoly(frame, pointerPoints, 3, bubbleColor);
    std::vector<std::vector<cv::Point>> pointerContours = {std::vector<cv::Point>(pointerPoints, pointerPoints + 3)};
    cv::polylines(frame, pointerContours, true, cv::Scalar(0, 0, 0), 2);
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