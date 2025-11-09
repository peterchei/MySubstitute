#include "virtual_background_processor.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <chrono>
#include <cmath>
#include <algorithm>

VirtualBackgroundProcessor::VirtualBackgroundProcessor()
    : m_modelLoaded(false),
      m_backgroundMode(BLUR),
      m_segmentationThreshold(0.5f),
      m_blendAlpha(0.8f),
      m_blurStrength(21),
      m_processingTime(0.0),
      m_frameCounter(0),
      m_cachedWidth(0),
      m_cachedHeight(0),
      m_solidColor(200, 200, 200),
      m_bgSubtractorInitialized(false),
      m_stableFrameCount(0),
      m_segmentationMethod(METHOD_ONNX_SELFIE),  // Default fallback method
      m_useGPU(true),  // Enable GPU by default
      m_useGuidedFilter(true),
      m_backend("CPU")
{
    std::cout << "[VirtualBackgroundProcessor] Initializing..." << std::endl;
}

VirtualBackgroundProcessor::~VirtualBackgroundProcessor()
{
    Cleanup();
}

bool VirtualBackgroundProcessor::Initialize()
{
    std::cout << "[VirtualBackgroundProcessor] Initialize called" << std::endl;

#ifdef HAVE_OPENCV
    try {
        // Try to load best available segmentation model
        std::vector<std::pair<std::string, std::string>> modelPaths = {
            // MediaPipe Selfie Segmentation (256x256) - BEST for real-time
            {"models/MediaPipe-Selfie-Segmentation.onnx", "MediaPipe Selfie Segmentation"},
            {"models/selfie_segmentation_mediapipe.onnx", "MediaPipe Selfie (ONNX)"},
            {"models/selfie_segmentation.onnx", "ONNX MediaPipe Selfie"},
            {"models/selfie_segmentation.tflite", "TFLite MediaPipe (not supported)"},
            
            // OpenCV DNN models
            {"models/segmentation_model_fp16.onnx", "ONNX FP16"},
            {"models/deeplabv3_mnv2_pascal_train_aug.pb", "DeepLab MobileNetV2"},
            
            // BodyPix models
            {"models/bodypix_mobilenet.onnx", "BodyPix MobileNet"}
        };
        
        bool modelFound = false;
        for (const auto& [path, name] : modelPaths) {
            if (std::ifstream(path).good()) {
                std::cout << "[VirtualBackgroundProcessor] Found model: " << name << std::endl;
                
                if (path.find(".onnx") != std::string::npos) {
#ifdef HAVE_ONNX
                    if (LoadSegmentationModelONNX(path)) {
                        m_segmentationMethod = METHOD_ONNX_SELFIE;
                        m_modelPath = path;
                        modelFound = true;
                        std::cout << "[VirtualBackgroundProcessor] ✅ Using ONNX model: " << name << std::endl;
                        break;
                    }
#else
                    std::cout << "[VirtualBackgroundProcessor] ⚠️  ONNX model found but ONNX Runtime not available" << std::endl;
#endif
                } else if (path.find(".pb") != std::string::npos) {
                    if (LoadSegmentationModelOpenCVDNN(path)) {
                        m_segmentationMethod = METHOD_OPENCV_DNN;
                        m_modelPath = path;
                        modelFound = true;
                        std::cout << "[VirtualBackgroundProcessor] ✅ Using OpenCV DNN model: " << name << std::endl;
                        break;
                    }
                }
            }
        }
        
        if (!modelFound) {
            std::cout << "[VirtualBackgroundProcessor] ⚠️  No segmentation model found, using motion detection fallback" << std::endl;
            std::cout << "[VirtualBackgroundProcessor] 💡 For better quality, download MediaPipe Selfie Segmentation:" << std::endl;
            std::cout << "[VirtualBackgroundProcessor]    Run: .\\scripts\\download_segmentation_model.ps1" << std::endl;
            m_segmentationMethod = METHOD_MOTION;
        }
        
        std::cout << "[VirtualBackgroundProcessor] Processor initialized successfully" << std::endl;
        std::cout << "[VirtualBackgroundProcessor] Configuration:" << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   Background Mode: " << (int)m_backgroundMode << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   Segmentation Method: " << (m_segmentationMethod == METHOD_ONNX_SELFIE ? "ONNX" : m_segmentationMethod == METHOD_OPENCV_DNN ? "OpenCV DNN" : "Motion+Face") << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   Segmentation Threshold: " << m_segmentationThreshold << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   Blend Alpha: " << m_blendAlpha << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   GPU Enabled: " << (m_useGPU ? "YES" : "NO") << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   Backend: " << m_backend << std::endl;
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[VirtualBackgroundProcessor] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[VirtualBackgroundProcessor] ERROR: OpenCV not available" << std::endl;
    return false;
#endif
}

void VirtualBackgroundProcessor::Cleanup()
{
    std::cout << "[VirtualBackgroundProcessor] Cleanup called" << std::endl;
    m_backgroundImage.release();
    m_cachedBackground.release();
    m_modelLoaded = false;
}

Frame VirtualBackgroundProcessor::ProcessFrame(const Frame& input)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    Frame output = input;

#ifdef HAVE_OPENCV
    if (!input.data.empty()) {
        cv::Mat frame = input.data.clone();
        
        // Create segmentation mask (person vs background)
        cv::Mat mask = SegmentPerson(frame);
        
        // Get background frame (blur, solid color, or custom image)
        cv::Mat background = GetBackgroundFrame(frame);
        
        // Blend foreground and background
        cv::Mat result = BlendFrames(frame, background, mask);
        
        result.copyTo(output.data);
    }
#endif

    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    m_frameCounter++;

    return output;
}

#ifdef HAVE_OPENCV

cv::Mat VirtualBackgroundProcessor::SegmentPerson(const cv::Mat& frame)
{
    cv::Mat mask;
    
    // Use best available method
    switch (m_segmentationMethod) {
#ifdef HAVE_ONNX
        case METHOD_ONNX_SELFIE:
            mask = SegmentPersonWithONNX(frame);
            break;
#endif
        case METHOD_OPENCV_DNN:
            if (m_modelLoaded && !m_segmentationNet.empty()) {
                mask = SegmentPersonWithOpenCVDNN(frame);
            } else {
                mask = DetectPersonUsingMotionAndFace(frame);
            }
            break;
            
        case METHOD_MOTION:
        default:
            mask = DetectPersonUsingMotionAndFace(frame);
            break;
    }
    
    return mask;
}

cv::Mat VirtualBackgroundProcessor::DetectPersonUsingMotionAndFace(const cv::Mat& frame)
{
    std::cout << "[VirtualBackgroundProcessor] Using motion + face detection for segmentation" << std::endl;
    
    cv::Mat mask = cv::Mat::zeros(frame.size(), CV_8U);
    cv::Mat personMask = cv::Mat::zeros(frame.size(), CV_8U);
    
    // Step 1: Initialize background subtractor on first frame
    if (!m_bgSubtractorInitialized) {
        // Use MOG2 with better parameters for person detection
        auto mog2 = cv::createBackgroundSubtractorMOG2(300, 25, true);  // history=300, threshold=25
        mog2->setDetectShadows(true);  // Detect shadows but don't mark them as foreground
        mog2->setShadowValue(0);       // Shadow pixels set to 0
        mog2->setShadowThreshold(0.5); // Shadow detection threshold
        m_bgSubtractor = mog2;
        
        m_previousFrame = frame.clone();
        m_bgSubtractorInitialized = true;
        std::cout << "[VirtualBackgroundProcessor] Background subtractor initialized" << std::endl;
        
        // Return center ellipse for first frame
        cv::Point center(frame.cols / 2, frame.rows / 2);
        cv::Size axes(frame.cols / 4, frame.rows / 3);
        cv::ellipse(personMask, center, axes, 0, 0, 360, cv::Scalar(255), -1);
        cv::GaussianBlur(personMask, personMask, cv::Size(21, 21), 0);
        return personMask;
    }
    
    // Step 2: Apply background subtraction with low learning rate
    cv::Mat fgMask;
    m_bgSubtractor->apply(frame, fgMask, 0.0005);  // Very low learning rate for stable background
    
    // Remove shadow pixels (they are marked as 127)
    cv::threshold(fgMask, fgMask, 200, 255, cv::THRESH_BINARY);
    
    // Step 3: Clean up noise with morphological operations
    cv::Mat kernel3 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::Mat kernel5 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::Mat kernel7 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    
    // Remove small noise
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_OPEN, kernel3);
    // Fill small holes
    cv::morphologyEx(fgMask, fgMask, cv::MORPH_CLOSE, kernel5);
    // Expand slightly to ensure we capture full person
    cv::dilate(fgMask, fgMask, kernel7, cv::Point(-1, -1), 1);
    
    // Step 4: Find contours and filter by size/location
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(fgMask.clone(), contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    bool personDetected = false;
    
    if (!contours.empty()) {
        // Filter contours by area and aspect ratio
        std::vector<std::vector<cv::Point>> validContours;
        double minArea = frame.cols * frame.rows * 0.03;  // At least 3% of frame
        double maxArea = frame.cols * frame.rows * 0.85;  // At most 85% of frame
        
        for (const auto& contour : contours) {
            double area = cv::contourArea(contour);
            if (area > minArea && area < maxArea) {
                // Check aspect ratio (person should be taller than wide)
                cv::Rect bbox = cv::boundingRect(contour);
                double aspectRatio = static_cast<double>(bbox.height) / bbox.width;
                
                // Person typically has aspect ratio between 0.8 and 3.0
                if (aspectRatio > 0.7 && aspectRatio < 3.5) {
                    validContours.push_back(contour);
                }
            }
        }
        
        if (!validContours.empty()) {
            // If multiple valid contours, use the one closest to center or largest
            if (validContours.size() == 1) {
                cv::drawContours(personMask, validContours, 0, cv::Scalar(255), -1);
                personDetected = true;
            } else {
                // Find contour closest to center of frame
                cv::Point frameCenter(frame.cols / 2, frame.rows / 2);
                double minDist = std::numeric_limits<double>::max();
                int bestIdx = 0;
                
                for (size_t i = 0; i < validContours.size(); i++) {
                    cv::Moments m = cv::moments(validContours[i]);
                    cv::Point centroid(m.m10 / m.m00, m.m01 / m.m00);
                    double dist = cv::norm(centroid - frameCenter);
                    
                    if (dist < minDist) {
                        minDist = dist;
                        bestIdx = i;
                    }
                }
                
                cv::drawContours(personMask, validContours, bestIdx, cv::Scalar(255), -1);
                personDetected = true;
                
                std::cout << "[VirtualBackgroundProcessor] Selected contour " << bestIdx 
                          << " from " << validContours.size() << " valid contours" << std::endl;
            }
            
            std::cout << "[VirtualBackgroundProcessor] Detected person contour from motion" << std::endl;
        }
    }
    
    // Step 5: Face detection to refine or fallback
    cv::CascadeClassifier faceCascade;
    std::vector<std::string> possiblePaths = {
        "D:/DevTools/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml",
        "C:/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml",
        "haarcascade_frontalface_default.xml",
        "C:/opencv/sources/data/haarcascades/haarcascade_frontalface_default.xml",
        "data/haarcascades/haarcascade_frontalface_default.xml"
    };
    
    bool cascadeLoaded = false;
    for (const auto& path : possiblePaths) {
        if (faceCascade.load(path)) {
            cascadeLoaded = true;
            break;
        }
    }
    
    if (cascadeLoaded) {
        std::vector<cv::Rect> faces;
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
        cv::equalizeHist(gray, gray);
        
        faceCascade.detectMultiScale(gray, faces, 1.1, 4, 0, cv::Size(40, 40));
        
        if (!faces.empty()) {
            cv::Rect largestFace = faces[0];
            for (const auto& face : faces) {
                if (face.area() > largestFace.area()) {
                    largestFace = face;
                }
            }
            
            // If no motion detected but face found, create body estimate
            if (!personDetected) {
                std::cout << "[VirtualBackgroundProcessor] No motion contour, using face detection" << std::endl;
                
                // More conservative body estimation
                int bodyWidth = static_cast<int>(largestFace.width * 2.5);
                int bodyHeight = static_cast<int>(largestFace.height * 4.2);
                int bodyX = largestFace.x + largestFace.width / 2 - bodyWidth / 2;
                int bodyY = largestFace.y - static_cast<int>(largestFace.height * 0.4);
                
                bodyX = std::max(0, std::min(bodyX, frame.cols - bodyWidth));
                bodyY = std::max(0, std::min(bodyY, frame.rows - bodyHeight));
                bodyWidth = std::min(bodyWidth, frame.cols - bodyX);
                bodyHeight = std::min(bodyHeight, frame.rows - bodyY);
                
                // Use ellipse but with tighter bounds
                cv::Point center(bodyX + bodyWidth / 2, bodyY + bodyHeight / 2);
                cv::Size axes(bodyWidth / 2, bodyHeight / 2);
                cv::ellipse(personMask, center, axes, 0, 0, 360, cv::Scalar(255), -1);
                
                personDetected = true;
            } else {
                // If we have both motion and face, ensure face region is included
                cv::Rect expandedFace(
                    std::max(0, largestFace.x - largestFace.width / 2),
                    std::max(0, largestFace.y - largestFace.height / 2),
                    std::min(largestFace.width * 2, frame.cols),
                    std::min(largestFace.height * 2, frame.rows)
                );
                
                // Make sure face area is definitely in the mask
                cv::Point faceCenter(largestFace.x + largestFace.width / 2, 
                                    largestFace.y + largestFace.height / 2);
                cv::circle(personMask, faceCenter, largestFace.width, cv::Scalar(255), -1);
            }
        }
    }
    
    // Step 6: Last resort fallback to center region (only if really nothing detected)
    if (!personDetected) {
        std::cout << "[VirtualBackgroundProcessor] No detection, using center fallback" << std::endl;
        cv::Point center(frame.cols / 2, frame.rows / 2);
        cv::Size axes(frame.cols / 5, frame.rows / 3);  // Smaller fallback
        cv::ellipse(personMask, center, axes, 0, 0, 360, cv::Scalar(255), -1);
    }
    
    // Step 7: Calculate percentage for adaptive smoothing
    int nonZeroPixels = cv::countNonZero(personMask);
    int totalPixels = personMask.rows * personMask.cols;
    double percentage = 100.0 * nonZeroPixels / totalPixels;
    
    // Step 8: Edge refinement using bilateral filter for better edge quality
    cv::Mat refinedMask;
    personMask.convertTo(refinedMask, CV_32F, 1.0 / 255.0);
    
    // Step 9: Temporal smoothing - blend with previous mask to reduce flickering
    if (!m_previousMask.empty() && m_previousMask.size() == personMask.size()) {
        cv::Mat prevMaskFloat;
        m_previousMask.convertTo(prevMaskFloat, CV_32F, 1.0 / 255.0);
        
        // Adaptive blending based on detection confidence
        double alpha = 0.7;  // Weight for current frame
        double beta = 0.3;   // Weight for previous frame
        
        // If current detection is weak (too small or too large), rely more on previous
        if (percentage < 5.0 || percentage > 80.0) {
            alpha = 0.4;  // Less weight on current (probably bad detection)
            beta = 0.6;   // More weight on previous
        } else if (percentage > 10.0 && percentage < 70.0) {
            // Good detection range, can trust current frame more
            alpha = 0.8;
            beta = 0.2;
            m_stableFrameCount++;
        } else {
            m_stableFrameCount = 0;
        }
        
        // Blend current and previous masks
        cv::addWeighted(refinedMask, alpha, prevMaskFloat, beta, 0.0, refinedMask);
        
        std::cout << "[VirtualBackgroundProcessor] Temporal smoothing: alpha=" << alpha 
                  << ", stable_frames=" << m_stableFrameCount << std::endl;
    } else {
        std::cout << "[VirtualBackgroundProcessor] First mask, no temporal smoothing" << std::endl;
    }
    
    // Apply bilateral filter to preserve edges while smoothing
    cv::Mat smoothMask;
    cv::bilateralFilter(refinedMask, smoothMask, 9, 75, 75);
    
    // Convert back and apply light Gaussian blur
    smoothMask.convertTo(personMask, CV_8U, 255.0);
    cv::GaussianBlur(personMask, personMask, cv::Size(15, 15), 0);
    
    // Store current mask for next frame
    m_previousMask = personMask.clone();
    
    std::cout << "[VirtualBackgroundProcessor] Person mask: " << nonZeroPixels << " / " << totalPixels 
              << " (" << percentage << "%) - Detection: " 
              << (personDetected ? "Motion/Face" : "Fallback") << std::endl;
    
    return personMask;
}

cv::Mat VirtualBackgroundProcessor::GetBackgroundFrame(const cv::Mat& frame)
{
    cv::Mat background = frame.clone();
    
    std::cout << "[VirtualBackgroundProcessor::GetBackgroundFrame] Mode=" << (int)m_backgroundMode << std::endl;
    
    switch (m_backgroundMode) {
        case BLUR: {
            // Apply strong blur to background
            int kernelSize = m_blurStrength;
            if (kernelSize % 2 == 0) kernelSize++;  // Ensure odd number
            kernelSize = std::max(3, std::min(kernelSize, 99));
            
            cv::GaussianBlur(frame, background, cv::Size(kernelSize, kernelSize), 0);
            std::cout << "[VirtualBackgroundProcessor] Applying BLUR mode, kernel=" << kernelSize << std::endl;
            break;
        }
        
        case SOLID_COLOR: {
            // Create solid color background
            background = cv::Mat(frame.size(), frame.type(), m_solidColor);
            std::cout << "[VirtualBackgroundProcessor] Applying SOLID_COLOR mode: (" 
                      << m_solidColor[0] << "," << m_solidColor[1] << "," << m_solidColor[2] << ")" << std::endl;
            break;
        }
        
        case CUSTOM_IMAGE: {
            // Use custom background image
            if (!m_backgroundImage.empty()) {
                background = ResizeBackgroundToFrame(frame);
                std::cout << "[VirtualBackgroundProcessor] Applying CUSTOM_IMAGE mode" << std::endl;
            } else {
                std::cout << "[VirtualBackgroundProcessor] CUSTOM_IMAGE: No image loaded, fallback to blur" << std::endl;
                // Fallback to blur
                int kernelSize = std::max(3, std::min(m_blurStrength, 99));
                if (kernelSize % 2 == 0) kernelSize++;
                cv::GaussianBlur(frame, background, cv::Size(kernelSize, kernelSize), 0);
            }
            break;
        }
        
        case DESKTOP_CAPTURE: {
            // Use desktop screenshot as background
            std::cout << "[VirtualBackgroundProcessor] DESKTOP_CAPTURE: Capturing desktop..." << std::endl;
            CaptureDesktopBackground();
            if (!m_backgroundImage.empty()) {
                background = ResizeBackgroundToFrame(frame);
                std::cout << "[VirtualBackgroundProcessor] Desktop captured and applied" << std::endl;
            } else {
                std::cout << "[VirtualBackgroundProcessor] DESKTOP_CAPTURE: Failed, fallback to solid color" << std::endl;
                // Fallback to solid color
                background = cv::Mat(frame.size(), frame.type(), m_solidColor);
            }
            break;
        }
        
        case MINECRAFT_PIXEL: {
            // Create Minecraft-style pixelated background
            background = CreateMinecraftPixelBackground(frame);
            std::cout << "[VirtualBackgroundProcessor] Applying MINECRAFT_PIXEL mode" << std::endl;
            break;
        }
        
        default:
            break;
    }
    
    return background;
}

cv::Mat VirtualBackgroundProcessor::ResizeBackgroundToFrame(const cv::Mat& frame)
{
    if (m_backgroundImage.empty()) {
        return frame.clone();
    }
    
    // Check if we need to resize
    if (m_cachedWidth != frame.cols || m_cachedHeight != frame.rows || m_cachedBackground.empty()) {
        // Aspect ratio aware resizing - fit to fill
        double frameAspect = static_cast<double>(frame.cols) / frame.rows;
        double bgAspect = static_cast<double>(m_backgroundImage.cols) / m_backgroundImage.rows;
        
        cv::Mat resized;
        if (bgAspect > frameAspect) {
            // Background is wider - fit by height
            int newWidth = static_cast<int>(frame.rows * bgAspect);
            cv::resize(m_backgroundImage, resized, cv::Size(newWidth, frame.rows));
            // Crop center
            int startX = (newWidth - frame.cols) / 2;
            m_cachedBackground = resized(cv::Rect(startX, 0, frame.cols, frame.rows)).clone();
        } else {
            // Background is taller - fit by width
            int newHeight = static_cast<int>(frame.cols / bgAspect);
            cv::resize(m_backgroundImage, resized, cv::Size(frame.cols, newHeight));
            // Crop center
            int startY = (newHeight - frame.rows) / 2;
            m_cachedBackground = resized(cv::Rect(0, startY, frame.cols, frame.rows)).clone();
        }
        
        m_cachedWidth = frame.cols;
        m_cachedHeight = frame.rows;
    }
    
    return m_cachedBackground.clone();
}

cv::Mat VirtualBackgroundProcessor::BlendFrames(const cv::Mat& foreground, const cv::Mat& background, const cv::Mat& mask)
{
    cv::Mat result = background.clone();
    
    if (mask.empty()) {
        return result;
    }
    
    // Normalize mask to 0-1 range
    cv::Mat maskFloat;
    mask.convertTo(maskFloat, CV_32F, 1.0 / 255.0);
    
    // Apply morphological operations for smoother edges
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::morphologyEx(maskFloat, maskFloat, cv::MORPH_CLOSE, kernel);
    
    // Gaussian blur for smooth edge transition
    cv::GaussianBlur(maskFloat, maskFloat, cv::Size(7, 7), 0);
    
    // Blend using alpha mask
    cv::Mat fgFloat, bgFloat;
    foreground.convertTo(fgFloat, CV_32F);
    background.convertTo(bgFloat, CV_32F);
    
    // Apply mask to foreground
    std::vector<cv::Mat> fgChannels;
    cv::split(fgFloat, fgChannels);
    std::vector<cv::Mat> bgChannels;
    cv::split(bgFloat, bgChannels);
    
    for (int i = 0; i < fgChannels.size(); i++) {
        result.create(foreground.size(), foreground.type());
        cv::Mat blended = fgChannels[i].mul(maskFloat) + bgChannels[i].mul(1.0 - maskFloat);
        blended.convertTo(blended, CV_8U);
        
        std::vector<cv::Mat> resultChannels;
        cv::split(result, resultChannels);
        if (i < resultChannels.size()) {
            blended.copyTo(resultChannels[i]);
            cv::merge(resultChannels, result);
        }
    }
    
    // Simpler blend approach
    cv::Mat result2 = background.clone();
    for (int y = 0; y < foreground.rows; y++) {
        for (int x = 0; x < foreground.cols; x++) {
            float alpha = maskFloat.at<float>(y, x);
            
            if (foreground.channels() == 3) {
                cv::Vec3b fgPixel = foreground.at<cv::Vec3b>(y, x);
                cv::Vec3b bgPixel = background.at<cv::Vec3b>(y, x);
                
                // If alpha is high (person), use foreground directly
                // If alpha is low (background), use background
                // Only blend at the edges
                result2.at<cv::Vec3b>(y, x) = cv::Vec3b(
                    static_cast<uchar>(fgPixel[0] * alpha + bgPixel[0] * (1.0 - alpha)),
                    static_cast<uchar>(fgPixel[1] * alpha + bgPixel[1] * (1.0 - alpha)),
                    static_cast<uchar>(fgPixel[2] * alpha + bgPixel[2] * (1.0 - alpha))
                );
            }
        }
    }
    
    return result2;
}

void VirtualBackgroundProcessor::CaptureDesktopBackground()
{
    // Get desktop DC
    HDC desktopDC = GetDC(nullptr);
    if (!desktopDC) {
        std::cerr << "[VirtualBackgroundProcessor] Failed to get desktop DC" << std::endl;
        return;
    }
    
    // Get screen dimensions
    int screenWidth = GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = GetSystemMetrics(SM_CYSCREEN);
    
    // Create compatible DC and bitmap
    HDC memDC = CreateCompatibleDC(desktopDC);
    HBITMAP memBitmap = CreateCompatibleBitmap(desktopDC, screenWidth, screenHeight);
    SelectObject(memDC, memBitmap);
    
    // Copy screen
    BitBlt(memDC, 0, 0, screenWidth, screenHeight, desktopDC, 0, 0, SRCCOPY);
    
    // Convert to OpenCV Mat
    BITMAPINFO bmpInfo = {};
    bmpInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmpInfo.bmiHeader.biWidth = screenWidth;
    bmpInfo.bmiHeader.biHeight = -screenHeight;  // Negative for top-down
    bmpInfo.bmiHeader.biPlanes = 1;
    bmpInfo.bmiHeader.biBitCount = 24;
    bmpInfo.bmiHeader.biCompression = BI_RGB;
    
    cv::Mat screenshot(screenHeight, screenWidth, CV_8UC3);
    GetDIBits(memDC, memBitmap, 0, screenHeight, screenshot.data, &bmpInfo, DIB_RGB_COLORS);
    
    // Convert BGR (GetDIBits returns RGB)
    cv::cvtColor(screenshot, m_backgroundImage, cv::COLOR_RGB2BGR);
    
    std::cout << "[VirtualBackgroundProcessor] Desktop captured: " << screenWidth << "x" << screenHeight << std::endl;
    
    // Cleanup
    DeleteObject(memBitmap);
    DeleteDC(memDC);
    ReleaseDC(nullptr, desktopDC);
}

bool VirtualBackgroundProcessor::LoadBackgroundImage(const std::string& imagePath)
{
    m_backgroundImage = cv::imread(imagePath);
    
    if (m_backgroundImage.empty()) {
        std::cerr << "[VirtualBackgroundProcessor] Failed to load image: " << imagePath << std::endl;
        return false;
    }
    
    std::cout << "[VirtualBackgroundProcessor] Background image loaded: " << imagePath << std::endl;
    std::cout << "[VirtualBackgroundProcessor] Image size: " << m_backgroundImage.cols << "x" << m_backgroundImage.rows << std::endl;
    
    // Reset cache to force resize on next frame
    m_cachedWidth = 0;
    m_cachedHeight = 0;
    m_cachedBackground.release();
    
    return true;
}

// New segmentation methods for improved quality

#ifdef HAVE_ONNX
bool VirtualBackgroundProcessor::LoadSegmentationModelONNX(const std::string& modelPath)
{
    try {
        // Initialize ONNX Runtime
        m_onnxEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "VirtualBackground");
        m_sessionOptions = std::make_unique<Ort::SessionOptions>();
        
        // Enable GPU if available
        if (m_useGPU) {
            try {
                // Try CUDA first
                #ifdef USE_CUDA
                OrtCUDAProviderOptions cuda_options;
                m_sessionOptions->AppendExecutionProvider_CUDA(cuda_options);
                m_backend = "CUDA";
                std::cout << "[VirtualBackgroundProcessor] GPU acceleration enabled (CUDA)" << std::endl;
                #elif defined(USE_DIRECTML)
                m_sessionOptions->AppendExecutionProvider_DML(0);
                m_backend = "DirectML";
                std::cout << "[VirtualBackgroundProcessor] GPU acceleration enabled (DirectML)" << std::endl;
                #else
                m_backend = "CPU";
                std::cout << "[VirtualBackgroundProcessor] GPU requested but no provider available, using CPU" << std::endl;
                #endif
            } catch (...) {
                m_backend = "CPU";
                std::cout << "[VirtualBackgroundProcessor] GPU initialization failed, using CPU" << std::endl;
            }
        } else {
            m_backend = "CPU";
        }
        
        // Set optimization level
        m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        m_sessionOptions->SetIntraOpNumThreads(4);
        
        // Load model
        #ifdef _WIN32
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        m_onnxSession = std::make_unique<Ort::Session>(*m_onnxEnv, wModelPath.c_str(), *m_sessionOptions);
        #else
        m_onnxSession = std::make_unique<Ort::Session>(*m_onnxEnv, modelPath.c_str(), *m_sessionOptions);
        #endif
        
        // Store input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        m_onnxInputName = m_onnxSession->GetInputNameAllocated(0, allocator).get();
        m_onnxOutputName = m_onnxSession->GetOutputNameAllocated(0, allocator).get();
        
        m_modelLoaded = true;
        std::cout << "[VirtualBackgroundProcessor] ONNX model loaded successfully" << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   Input name: " << m_onnxInputName << std::endl;
        std::cout << "[VirtualBackgroundProcessor]   Output name: " << m_onnxOutputName << std::endl;
        std::cout << "[VirtualBackgroundProcessor] Backend: " << m_backend << std::endl;
        
        return true;
        
    } catch (const Ort::Exception& e) {
        std::cerr << "[VirtualBackgroundProcessor] ONNX error: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat VirtualBackgroundProcessor::SegmentPersonWithONNX(const cv::Mat& frame)
{
    if (!m_onnxSession) {
        return DetectPersonUsingMotionAndFace(frame);
    }
    
    try {
        // MediaPipe Selfie Segmentation expects 256x256 input
        const int inputSize = 256;
        
        // IMPORTANT: Use letterboxing to preserve aspect ratio
        // Calculate scale to fit frame into 256x256 square with higher precision
        double scale = std::min(
            static_cast<double>(inputSize) / frame.cols,
            static_cast<double>(inputSize) / frame.rows
        );
        
        // Use rounding for better accuracy (not truncation)
        int scaledWidth = static_cast<int>(std::round(frame.cols * scale));
        int scaledHeight = static_cast<int>(std::round(frame.rows * scale));
        
        // Ensure we don't exceed inputSize due to rounding
        scaledWidth = std::min(scaledWidth, inputSize);
        scaledHeight = std::min(scaledHeight, inputSize);
        
        // Resize preserving aspect ratio using high-quality interpolation
        cv::Mat scaled;
        cv::resize(frame, scaled, cv::Size(scaledWidth, scaledHeight), 0, 0, cv::INTER_AREA);
        
        // Create 256x256 canvas and center the scaled image (letterboxing)
        // CRITICAL: Use integer division for offsets (consistent with crop operation)
        // If we use rounding here, we must use the SAME offsets when cropping
        cv::Mat letterboxed = cv::Mat::zeros(inputSize, inputSize, frame.type());
        int offsetX = (inputSize - scaledWidth) / 2;
        int offsetY = (inputSize - scaledHeight) / 2;
        
        // Debug output (first frame only)
        static bool debugPrinted = false;
        if (!debugPrinted) {
            std::cout << "[VirtualBackgroundProcessor] Letterbox math:" << std::endl;
            std::cout << "  Original: " << frame.cols << "x" << frame.rows << std::endl;
            std::cout << "  Scale: " << scale << std::endl;
            std::cout << "  Scaled: " << scaledWidth << "x" << scaledHeight << std::endl;
            std::cout << "  Offset: (" << offsetX << ", " << offsetY << ")" << std::endl;
            debugPrinted = true;
        }
        
        // Ensure offsets are valid
        if (offsetX >= 0 && offsetY >= 0 && 
            offsetX + scaledWidth <= inputSize && 
            offsetY + scaledHeight <= inputSize) {
            scaled.copyTo(letterboxed(cv::Rect(offsetX, offsetY, scaledWidth, scaledHeight)));
        }
        
        // Convert to RGB and normalize to [0, 1]
        cv::Mat rgb;
        cv::cvtColor(letterboxed, rgb, cv::COLOR_BGR2RGB);
        rgb.convertTo(rgb, CV_32F, 1.0 / 255.0);
        
        // Prepare input tensor [1, 3, 256, 256]
        std::vector<float> inputTensorValues(1 * 3 * inputSize * inputSize);
        
        // Convert HWC to CHW format
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    inputTensorValues[c * inputSize * inputSize + h * inputSize + w] = 
                        rgb.at<cv::Vec3f>(h, w)[c];
                }
            }
        }
        
        // Create input tensor
        std::vector<int64_t> inputShape = {1, 3, inputSize, inputSize};
        auto memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, inputTensorValues.data(), inputTensorValues.size(),
            inputShape.data(), inputShape.size()
        );
        
        // Run inference using stored names
        const char* inputNames[] = {m_onnxInputName.c_str()};
        const char* outputNames[] = {m_onnxOutputName.c_str()};
        
        auto outputTensors = m_onnxSession->Run(
            Ort::RunOptions{nullptr},
            inputNames, &inputTensor, 1,
            outputNames, 1
        );
        
        // Get output [1, 1, 256, 256] or [1, 256, 256, 1]
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto tensorInfo = outputTensors[0].GetTensorTypeAndShapeInfo();
        auto shape = tensorInfo.GetShape();
        
        // Convert to OpenCV Mat
        cv::Mat maskSmall(inputSize, inputSize, CV_32F, outputData);
        
        // IMPORTANT: Remove letterboxing padding - crop to scaled region
        // Validate crop rectangle before using it
        if (offsetX >= 0 && offsetY >= 0 && 
            scaledWidth > 0 && scaledHeight > 0 &&
            offsetX + scaledWidth <= inputSize && 
            offsetY + scaledHeight <= inputSize) {
            
            cv::Rect cropRect(offsetX, offsetY, scaledWidth, scaledHeight);
            cv::Mat maskCropped = maskSmall(cropRect).clone();
            
            // Resize cropped mask back to original frame size
            // Use INTER_LINEAR instead of INTER_CUBIC - may have better alignment
            cv::Mat mask;
            cv::resize(maskCropped, mask, frame.size(), 0, 0, cv::INTER_LINEAR);
            
            // Convert to 8-bit [0, 255]
            mask.convertTo(mask, CV_8U, 255.0);
            
            // Apply horizontal shift correction using cv::warpAffine for sub-pixel accuracy
            // Shift mask slightly left to compensate for observed right-side bias
            cv::Mat shiftedMask;
            float shiftX = -1.5f;  // Negative = shift left (sub-pixel precision)
            cv::Mat M = (cv::Mat_<float>(2, 3) << 1, 0, shiftX, 0, 1, 0);
            cv::warpAffine(mask, shiftedMask, M, mask.size(), cv::INTER_LINEAR, cv::BORDER_REPLICATE);
            
            // Post-process for better quality
            return PostProcessMask(shiftedMask, frame);
        } else {
            // Fallback: use entire mask if crop validation fails
            std::cerr << "[VirtualBackgroundProcessor] Invalid crop rect, using full mask" << std::endl;
            cv::Mat mask;
            cv::resize(maskSmall, mask, frame.size(), 0, 0, cv::INTER_CUBIC);
            mask.convertTo(mask, CV_8U, 255.0);
            return PostProcessMask(mask, frame);
        }
        
    } catch (const Ort::Exception& e) {
        std::cerr << "[VirtualBackgroundProcessor] ONNX inference error: " << e.what() << std::endl;
        return DetectPersonUsingMotionAndFace(frame);
    } catch (const std::exception& e) {
        std::cerr << "[VirtualBackgroundProcessor] Error in ONNX segmentation: " << e.what() << std::endl;
        return DetectPersonUsingMotionAndFace(frame);
    }
}
#endif // HAVE_ONNX

bool VirtualBackgroundProcessor::LoadSegmentationModelOpenCVDNN(const std::string& modelPath)
{
    try {
        std::string configPath = modelPath;
        size_t pos = configPath.rfind(".pb");
        if (pos != std::string::npos) {
            configPath.replace(pos, 3, ".pbtxt");
        }
        
        m_segmentationNet = cv::dnn::readNetFromTensorflow(modelPath, configPath);
        
        if (m_segmentationNet.empty()) {
            return false;
        }
        
        // Try to use GPU
        if (m_useGPU) {
            try {
                m_segmentationNet.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
                m_segmentationNet.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA_FP16);
                m_backend = "CUDA FP16";
                std::cout << "[VirtualBackgroundProcessor] GPU acceleration enabled (OpenCV CUDA)" << std::endl;
            } catch (...) {
                m_segmentationNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
                m_segmentationNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
                m_backend = "CPU";
                std::cout << "[VirtualBackgroundProcessor] CUDA not available, using CPU" << std::endl;
            }
        } else {
            m_segmentationNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_segmentationNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
            m_backend = "CPU";
        }
        
        m_modelLoaded = true;
        std::cout << "[VirtualBackgroundProcessor] OpenCV DNN model loaded successfully" << std::endl;
        std::cout << "[VirtualBackgroundProcessor] Backend: " << m_backend << std::endl;
        
        return true;
        
    } catch (const cv::Exception& e) {
        std::cerr << "[VirtualBackgroundProcessor] OpenCV DNN error: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat VirtualBackgroundProcessor::SegmentPersonWithOpenCVDNN(const cv::Mat& frame)
{
    if (m_segmentationNet.empty()) {
        return DetectPersonUsingMotionAndFace(frame);
    }
    
    try {
        // Prepare input blob (DeepLab expects 513x513)
        cv::Mat inputBlob = cv::dnn::blobFromImage(
            frame, 1.0 / 255.0, cv::Size(513, 513),
            cv::Scalar(0, 0, 0), false, false
        );
        
        m_segmentationNet.setInput(inputBlob);
        cv::Mat output = m_segmentationNet.forward();
        
        // Extract person class (class 15 in Pascal VOC)
        int numClasses = output.size[1];
        int height = output.size[2];
        int width = output.size[3];
        
        cv::Mat personMap(height, width, CV_32F, output.ptr<float>(0) + 15 * height * width);
        
        // Threshold and resize
        cv::Mat maskSmall;
        cv::threshold(personMap, maskSmall, m_segmentationThreshold, 255, cv::THRESH_BINARY);
        maskSmall.convertTo(maskSmall, CV_8U);
        
        cv::Mat mask;
        cv::resize(maskSmall, mask, frame.size(), 0, 0, cv::INTER_LINEAR);
        
        return PostProcessMask(mask, frame);
        
    } catch (const cv::Exception& e) {
        std::cerr << "[VirtualBackgroundProcessor] DNN inference error: " << e.what() << std::endl;
        return DetectPersonUsingMotionAndFace(frame);
    }
}

cv::Mat VirtualBackgroundProcessor::PostProcessMask(const cv::Mat& rawMask, const cv::Mat& frame)
{
    cv::Mat mask = rawMask.clone();
    
    // 1. Gentle morphological operations to clean up mask WITHOUT shrinking
    cv::Mat kernel3 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(3, 3));
    cv::Mat kernel5 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
    cv::Mat kernel7 = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(7, 7));
    
    // Fill small holes first (before removing noise to preserve person boundary)
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel5, cv::Point(-1, -1), 2);
    
    // Remove very small noise (use small kernel to avoid shrinking person)
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel3, cv::Point(-1, -1), 1);
    
    // IMPORTANT: Slightly expand the mask to ensure we capture the full person (especially head)
    // This prevents cutting off the top of the head
    cv::dilate(mask, mask, kernel7, cv::Point(-1, -1), 1);
    
    // 2. Edge refinement using guided filter or bilateral filter
    EdgeRefinement(mask, frame);
    
    // 3. Temporal smoothing to reduce flickering
    TemporalSmoothing(mask);
    
    // 4. Final soft edge with Gaussian blur
    cv::GaussianBlur(mask, mask, cv::Size(9, 9), 0);
    
    return mask;
}

void VirtualBackgroundProcessor::EdgeRefinement(cv::Mat& mask, const cv::Mat& frame)
{
    if (!m_useGuidedFilter) {
        return;
    }
    
    // Use bilateral filter for edge-aware smoothing
    cv::Mat refined;
    cv::bilateralFilter(mask, refined, 9, 75, 75);
    refined.copyTo(mask);
    
    // Optional: Use guided filter if available (better quality but slower)
    // This would require opencv_contrib ximgproc module:
    // #ifdef HAVE_OPENCV_XIMGPROC
    // cv::ximgproc::guidedFilter(frame, mask, mask, 8, 0.1);
    // #endif
}

void VirtualBackgroundProcessor::TemporalSmoothing(cv::Mat& mask)
{
    // Add current mask to history
    m_maskHistory.push_back(mask.clone());
    
    // Keep only last N masks
    if (m_maskHistory.size() > MAX_MASK_HISTORY) {
        m_maskHistory.pop_front();
    }
    
    // If we have enough history, do temporal averaging
    if (m_maskHistory.size() >= 3) {
        cv::Mat avgMask = cv::Mat::zeros(mask.size(), CV_32F);
        
        // Weighted average (more recent frames have more weight)
        float totalWeight = 0.0f;
        for (size_t i = 0; i < m_maskHistory.size(); ++i) {
            float weight = static_cast<float>(i + 1) / m_maskHistory.size();  // 0.2, 0.4, 0.6, 0.8, 1.0
            
            cv::Mat maskFloat;
            m_maskHistory[i].convertTo(maskFloat, CV_32F);
            avgMask += maskFloat * weight;
            totalWeight += weight;
        }
        
        avgMask /= totalWeight;
        avgMask.convertTo(mask, CV_8U);
    }
}

void VirtualBackgroundProcessor::SetSegmentationMethod(SegmentationMethod method)
{
    std::string methodName = (method == METHOD_ONNX_SELFIE ? "ONNX (MediaPipe)" : 
                              method == METHOD_OPENCV_DNN ? "OpenCV DNN" : "Motion+Face");
    
    std::cout << "[VirtualBackgroundProcessor] Segmentation method changed to: " << methodName << std::endl;
    
    // Check if the requested method is available
    if (method == METHOD_OPENCV_DNN) {
        if (!m_modelLoaded || m_segmentationNet.empty()) {
            std::cout << "[VirtualBackgroundProcessor] ⚠️  OpenCV DNN model not loaded!" << std::endl;
            std::cout << "[VirtualBackgroundProcessor]    Falling back to Motion+Face detection" << std::endl;
            std::cout << "[VirtualBackgroundProcessor] 💡 To use OpenCV DNN, download DeepLab model:" << std::endl;
            std::cout << "[VirtualBackgroundProcessor]    1. Download: deeplabv3_mnv2_pascal_train_aug.pb" << std::endl;
            std::cout << "[VirtualBackgroundProcessor]    2. Place in: models/ folder" << std::endl;
            std::cout << "[VirtualBackgroundProcessor]    3. Restart application" << std::endl;
        } else {
            std::cout << "[VirtualBackgroundProcessor] ✅ OpenCV DNN ready" << std::endl;
        }
    }
    
#ifdef HAVE_ONNX
    if (method == METHOD_ONNX_SELFIE) {
        if (!m_modelLoaded) {
            std::cout << "[VirtualBackgroundProcessor] ⚠️  ONNX model not loaded!" << std::endl;
            std::cout << "[VirtualBackgroundProcessor]    Falling back to Motion+Face detection" << std::endl;
            std::cout << "[VirtualBackgroundProcessor] 💡 To use ONNX, run: .\\scripts\\download_mediapipe_onnx.ps1" << std::endl;
        } else {
            std::cout << "[VirtualBackgroundProcessor] ✅ ONNX (MediaPipe) ready with " << m_backend << std::endl;
        }
    }
#endif
    
    m_segmentationMethod = method;
}

void VirtualBackgroundProcessor::SetUseGPU(bool useGPU)
{
    m_useGPU = useGPU;
    std::cout << "[VirtualBackgroundProcessor] GPU usage " << (useGPU ? "enabled" : "disabled") << std::endl;
    
    // If model is already loaded, may need to reinitialize
    if (m_modelLoaded) {
        std::cout << "[VirtualBackgroundProcessor] ⚠️  GPU setting changed. Reinitialize processor for changes to take effect." << std::endl;
    }
}

bool VirtualBackgroundProcessor::LoadSegmentationModel(const std::string& modelPath)
{
    m_modelPath = modelPath;
    
    if (modelPath.find(".onnx") != std::string::npos) {
#ifdef HAVE_ONNX
        return LoadSegmentationModelONNX(modelPath);
#else
        std::cerr << "[VirtualBackgroundProcessor] ONNX model specified but ONNX Runtime not available" << std::endl;
        return false;
#endif
    } else if (modelPath.find(".pb") != std::string::npos) {
        return LoadSegmentationModelOpenCVDNN(modelPath);
    } else {
        std::cerr << "[VirtualBackgroundProcessor] Unsupported model format: " << modelPath << std::endl;
        return false;
    }
}

std::string VirtualBackgroundProcessor::GetSegmentationInfo() const
{
    std::string info;
    info += "Segmentation Method: ";
    
    switch (m_segmentationMethod) {
        case METHOD_ONNX_SELFIE:
            info += "ONNX (MediaPipe Selfie Segmentation)\n";
            break;
        case METHOD_OPENCV_DNN:
            info += "OpenCV DNN (DeepLab/BodyPix)\n";
            break;
        case METHOD_MOTION:
            info += "Motion + Face Detection (Fallback)\n";
            break;
    }
    
    info += "Backend: " + m_backend + "\n";
    info += "GPU Enabled: " + std::string(m_useGPU ? "Yes" : "No") + "\n";
    info += "Model Loaded: " + std::string(m_modelLoaded ? "Yes" : "No") + "\n";
    
    if (m_modelLoaded) {
        info += "Model Path: " + m_modelPath + "\n";
    }
    
    info += "Temporal Smoothing: Enabled (" + std::to_string(m_maskHistory.size()) + " frame history)\n";
    info += "Edge Refinement: " + std::string(m_useGuidedFilter ? "Enabled" : "Disabled") + "\n";
    
    if (m_frameCounter > 0) {
        info += "Performance: " + std::to_string(m_processingTime) + " ms/frame\n";
        float fps = (m_processingTime > 0) ? (1000.0f / m_processingTime) : 0.0f;
        info += "FPS: " + std::to_string(fps) + "\n";
    }
    
    return info;
}

#endif

std::string VirtualBackgroundProcessor::GetName() const
{
    return "Virtual Background Processor";
}

std::string VirtualBackgroundProcessor::GetVersion() const
{
    return "1.0.0";
}

bool VirtualBackgroundProcessor::SupportsRealTime() const
{
    return true;
}

void VirtualBackgroundProcessor::SetBackgroundMode(BackgroundMode mode)
{
    m_backgroundMode = mode;
    std::cout << "[VirtualBackgroundProcessor] Background mode changed to: " << (int)mode << std::endl;
}

void VirtualBackgroundProcessor::SetBackgroundImage(const std::string& imagePath)
{
#ifdef HAVE_OPENCV
    if (LoadBackgroundImage(imagePath)) {
        m_backgroundMode = CUSTOM_IMAGE;
    }
#endif
}

void VirtualBackgroundProcessor::SetBlurStrength(int kernelSize)
{
    m_blurStrength = std::max(1, std::min(kernelSize, 100));
    std::cout << "[VirtualBackgroundProcessor] Blur strength set to: " << m_blurStrength << std::endl;
}

#ifdef HAVE_OPENCV
void VirtualBackgroundProcessor::SetSolidColor(cv::Scalar color)
{
    m_solidColor = color;
    m_backgroundMode = SOLID_COLOR;
    std::cout << "[VirtualBackgroundProcessor] Solid color set to: (" << color[0] << "," << color[1] << "," << color[2] << ")" << std::endl;
}
#endif

void VirtualBackgroundProcessor::SetSegmentationThreshold(float threshold)
{
    m_segmentationThreshold = std::max(0.0f, std::min(1.0f, threshold));
}

void VirtualBackgroundProcessor::SetBlendAlpha(float alpha)
{
    m_blendAlpha = std::max(0.0f, std::min(1.0f, alpha));
}

bool VirtualBackgroundProcessor::SetParameter(const std::string& name, const std::string& value)
{
    if (name == "background_mode") {
        try {
            int mode = std::stoi(value);
            SetBackgroundMode(static_cast<BackgroundMode>(mode));
            m_parameters[name] = value;
            return true;
        } catch (...) {
            return false;
        }
    }
    else if (name == "background_image") {
        SetBackgroundImage(value);
        m_parameters[name] = value;
        return true;
    }
    else if (name == "blur_strength") {
        try {
            int strength = std::stoi(value);
            SetBlurStrength(strength);
            m_parameters[name] = value;
            return true;
        } catch (...) {
            return false;
        }
    }
    else if (name == "segmentation_threshold") {
        try {
            float threshold = std::stof(value);
            SetSegmentationThreshold(threshold);
            m_parameters[name] = value;
            return true;
        } catch (...) {
            return false;
        }
    }
    else if (name == "blend_alpha") {
        try {
            float alpha = std::stof(value);
            SetBlendAlpha(alpha);
            m_parameters[name] = value;
            return true;
        } catch (...) {
            return false;
        }
    }
    return false;
}

std::map<std::string, std::string> VirtualBackgroundProcessor::GetParameters() const
{
    return m_parameters;
}

double VirtualBackgroundProcessor::GetExpectedProcessingTime() const
{
    return m_processingTime;
}

cv::Mat VirtualBackgroundProcessor::CreateMinecraftPixelBackground(const cv::Mat& frame)
{
    // Minecraft-style pixelated effect with sharp, stable pixels
    // Using the same approach as PixelArtProcessor for consistency
    
    const int pixelSize = 8;  // 8x8 blocks for Minecraft style
    const int colorLevels = 6;  // Limited color palette
    
    cv::Mat result = frame.clone();
    
    // Step 1: Enhance saturation for vibrant Minecraft-like colors
    cv::Mat hsv;
    cv::cvtColor(result, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    channels[1] = channels[1] * 1.4; // Boost saturation
    cv::merge(channels, hsv);
    cv::cvtColor(hsv, result, cv::COLOR_HSV2BGR);
    
    // Step 2: Pixelate - downsample then upsample with nearest neighbor
    int newWidth = std::max(1, result.cols / pixelSize);
    int newHeight = std::max(1, result.rows / pixelSize);
    
    cv::Mat small;
    cv::resize(result, small, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_LINEAR);
    
    // Upsample back with nearest neighbor for sharp, blocky effect
    cv::Mat pixelated;
    cv::resize(small, pixelated, result.size(), 0, 0, cv::INTER_NEAREST);
    
    // Step 3: Quantize colors to fewer levels for blocky color palette
    int step = 256 / colorLevels;
    for (int y = 0; y < pixelated.rows; ++y) {
        cv::Vec3b* row = pixelated.ptr<cv::Vec3b>(y);
        for (int x = 0; x < pixelated.cols; ++x) {
            for (int c = 0; c < 3; ++c) {
                int val = row[x][c];
                row[x][c] = (val / step) * step + step / 2;
            }
        }
    }
    
    // Step 4: Add strong black edge outlines (optional but looks good)
    cv::Mat gray, edges;
    cv::cvtColor(pixelated, gray, cv::COLOR_BGR2GRAY);
    cv::Canny(gray, edges, 50, 150);
    
    // Dilate edges slightly
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::dilate(edges, edges, kernel);
    
    // Apply black outlines where edges are detected
    for (int y = 0; y < pixelated.rows; ++y) {
        cv::Vec3b* row = pixelated.ptr<cv::Vec3b>(y);
        const uint8_t* edgeRow = edges.ptr<uint8_t>(y);
        
        for (int x = 0; x < pixelated.cols; ++x) {
            if (edgeRow[x] > 0) {
                row[x] = cv::Vec3b(0, 0, 0);  // Black outline
            }
        }
    }
    
    return pixelated;
}

