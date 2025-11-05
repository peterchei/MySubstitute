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
      m_solidColor(200, 200, 200)
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
            // Fall through to face detection fallback
            mask = cv::Mat::zeros(frame.size(), CV_8U);
        }
    } else {
        std::cout << "[VirtualBackgroundProcessor::SegmentPerson] Using face detection + body estimation" << std::endl;
        
        // Use face detection to find the person
        cv::CascadeClassifier faceCascade;
        std::string faceCascadePath = "C:/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml";
        
        // Try multiple common paths for Haar cascade
        std::vector<std::string> possiblePaths = {
            faceCascadePath,
            "haarcascade_frontalface_default.xml",
            "C:/opencv/sources/data/haarcascades/haarcascade_frontalface_default.xml",
            "D:/DevTools/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml"
        };
        
        bool cascadeLoaded = false;
        for (const auto& path : possiblePaths) {
            if (faceCascade.load(path)) {
                cascadeLoaded = true;
                std::cout << "[VirtualBackgroundProcessor] Loaded face cascade from: " << path << std::endl;
                break;
            }
        }
        
        mask = cv::Mat::zeros(frame.size(), CV_8U);
        
        if (cascadeLoaded) {
            // Detect faces
            std::vector<cv::Rect> faces;
            cv::Mat gray;
            cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
            cv::equalizeHist(gray, gray);
            
            faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));
            
            if (!faces.empty()) {
                std::cout << "[VirtualBackgroundProcessor] Detected " << faces.size() << " face(s)" << std::endl;
                
                // Use the largest face (closest to camera)
                cv::Rect largestFace = faces[0];
                for (const auto& face : faces) {
                    if (face.area() > largestFace.area()) {
                        largestFace = face;
                    }
                }
                
                // Estimate body region based on face
                // Typical body proportions: body is ~3.5x head height, ~2.5x head width
                int faceHeight = largestFace.height;
                int faceWidth = largestFace.width;
                
                // Expand to include body
                int bodyWidth = static_cast<int>(faceWidth * 2.8);
                int bodyHeight = static_cast<int>(faceHeight * 4.0);
                
                int bodyX = largestFace.x + faceWidth / 2 - bodyWidth / 2;
                int bodyY = largestFace.y - static_cast<int>(faceHeight * 0.3); // Include some area above head
                
                // Clamp to frame boundaries
                bodyX = std::max(0, std::min(bodyX, frame.cols - bodyWidth));
                bodyY = std::max(0, std::min(bodyY, frame.rows - bodyHeight));
                bodyWidth = std::min(bodyWidth, frame.cols - bodyX);
                bodyHeight = std::min(bodyHeight, frame.rows - bodyY);
                
                cv::Rect bodyRect(bodyX, bodyY, bodyWidth, bodyHeight);
                
                // Create elliptical mask for more natural shape
                cv::Point center(bodyX + bodyWidth / 2, bodyY + bodyHeight / 2);
                cv::Size axes(bodyWidth / 2, bodyHeight / 2);
                cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255), -1);
                
                std::cout << "[VirtualBackgroundProcessor] Created body mask from face detection" << std::endl;
            } else {
                std::cout << "[VirtualBackgroundProcessor] No face detected, using center region fallback" << std::endl;
                // Fallback to center region
                int centerWidth = static_cast<int>(frame.cols * 0.5);
                int centerHeight = static_cast<int>(frame.rows * 0.7);
                int startX = (frame.cols - centerWidth) / 2;
                int startY = (frame.rows - centerHeight) / 2;
                
                cv::Point center(frame.cols / 2, frame.rows / 2);
                cv::Size axes(centerWidth / 2, centerHeight / 2);
                cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255), -1);
            }
        } else {
            std::cout << "[VirtualBackgroundProcessor] Face cascade not found, using center region" << std::endl;
            // Fallback to center ellipse if cascade can't be loaded
            int centerWidth = static_cast<int>(frame.cols * 0.5);
            int centerHeight = static_cast<int>(frame.rows * 0.7);
            
            cv::Point center(frame.cols / 2, frame.rows / 2);
            cv::Size axes(centerWidth / 2, centerHeight / 2);
            cv::ellipse(mask, center, axes, 0, 0, 360, cv::Scalar(255), -1);
        }
        
        // Apply Gaussian blur to make edges soft
        cv::GaussianBlur(mask, mask, cv::Size(51, 51), 0);
        
        std::cout << "[VirtualBackgroundProcessor::SegmentPerson] Segmentation complete, person pixels: " 
                  << cv::countNonZero(mask) << " / " << (mask.rows * mask.cols) << std::endl;
    }
    
    return mask;
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
