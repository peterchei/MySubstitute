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
      m_bgSubtractorInitialized(false)
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
        // Load semantic segmentation model (U-Net or DeepLab style)
        // Using OpenCV's built-in person segmentation via MobileNetV2
        std::string modelPath = "models/deeplabv3_mnv2_pascal_train_aug.pb";
        std::string configPath = "models/deeplabv3_mnv2_pascal_train_aug.pbtxt";
        
        // Try to load the model if available
        if (std::ifstream(modelPath).good()) {
            std::cout << "[VirtualBackgroundProcessor] Loading semantic segmentation model..." << std::endl;
            m_segmentationNet = cv::dnn::readNetFromTensorflow(modelPath, configPath);
            
            if (!m_segmentationNet.empty()) {
                std::cout << "[VirtualBackgroundProcessor] Segmentation model loaded successfully" << std::endl;
                m_segmentationNet.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
                m_segmentationNet.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
                m_modelLoaded = true;
            } else {
                std::cout << "[VirtualBackgroundProcessor] Model file exists but failed to load, using simple approach" << std::endl;
                m_modelLoaded = false;
            }
        } else {
            std::cout << "[VirtualBackgroundProcessor] Segmentation model not found, using color-based segmentation" << std::endl;
            m_modelLoaded = false;
        }
        
        // Don't override background mode here - it should be set by SetBackgroundMode() before Initialize()
        
        std::cout << "[VirtualBackgroundProcessor] Processor initialized successfully" << std::endl;
        std::cout << "[VirtualBackgroundProcessor] Background Mode: " << (int)m_backgroundMode << std::endl;
        std::cout << "[VirtualBackgroundProcessor] Segmentation Threshold: " << m_segmentationThreshold << std::endl;
        std::cout << "[VirtualBackgroundProcessor] Blend Alpha: " << m_blendAlpha << std::endl;
        
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
    // Create mask where 255 = person (foreground), 0 = background
    cv::Mat mask;
    
    if (m_modelLoaded && !m_segmentationNet.empty()) {
        std::cout << "[VirtualBackgroundProcessor::SegmentPerson] Using DNN model" << std::endl;
        try {
            // Prepare input blob
            cv::Mat inputBlob = cv::dnn::blobFromImage(frame, 1.0 / 255.0, cv::Size(513, 513),
                                                       cv::Scalar(0, 0, 0), false, false);
            
            m_segmentationNet.setInput(inputBlob);
            
            // Get output (semantic segmentation map)
            cv::Mat output = m_segmentationNet.forward();
            
            // Extract person class (usually class 15 in Pascal VOC)
            // Output shape is [1, numClasses, H, W]
            int numClasses = output.size[1];
            int height = output.size[2];
            int width = output.size[3];
            
            // Get person channel and resize back to frame size
            cv::Mat personMap(height, width, CV_32F, output.ptr<float>(0) + 15 * height * width);
            cv::Mat personMaskSmall;
            cv::threshold(personMap, personMaskSmall, m_segmentationThreshold, 255, cv::THRESH_BINARY);
            personMaskSmall.convertTo(personMaskSmall, CV_8U);
            
            // Resize mask to frame size
            cv::resize(personMaskSmall, mask, frame.size());
            
            // Apply morphological operations to clean up mask
            cv::Mat kernel = cv::getStructuringElement(cv::MORPH_ELLIPSE, cv::Size(5, 5));
            cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);
            cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);
            
            std::cout << "[VirtualBackgroundProcessor::SegmentPerson] DNN segmentation complete" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "[VirtualBackgroundProcessor] Segmentation error: " << e.what() << std::endl;
            // Fall through to advanced fallback
            mask = cv::Mat::zeros(frame.size(), CV_8U);
        }
    } else {
        // Use combination of methods for better segmentation
        mask = DetectPersonUsingMotionAndFace(frame);
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
        "C:/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml",
        "haarcascade_frontalface_default.xml",
        "C:/opencv/sources/data/haarcascades/haarcascade_frontalface_default.xml",
        "D:/DevTools/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml",
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
    
    // Step 7: Edge refinement using bilateral filter for better edge quality
    cv::Mat refinedMask;
    personMask.convertTo(refinedMask, CV_32F, 1.0 / 255.0);
    
    // Apply bilateral filter to preserve edges while smoothing
    cv::Mat smoothMask;
    cv::bilateralFilter(refinedMask, smoothMask, 9, 75, 75);
    
    // Convert back and apply light Gaussian blur
    smoothMask.convertTo(personMask, CV_8U, 255.0);
    cv::GaussianBlur(personMask, personMask, cv::Size(15, 15), 0);
    
    int nonZeroPixels = cv::countNonZero(personMask);
    int totalPixels = personMask.rows * personMask.cols;
    double percentage = 100.0 * nonZeroPixels / totalPixels;
    
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
            float alpha = maskFloat.at<float>(y, x) * m_blendAlpha;
            
            if (foreground.channels() == 3) {
                cv::Vec3b fgPixel = foreground.at<cv::Vec3b>(y, x);
                cv::Vec3b bgPixel = background.at<cv::Vec3b>(y, x);
                
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
