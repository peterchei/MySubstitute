#include "person_replacement_processor.h"
#include <chrono>
#include <iostream>

#ifdef HAVE_OPENCV
#include <opencv2/imgproc.hpp>
#include <opencv2/photo.hpp>
#endif

PersonReplacementProcessor::PersonReplacementProcessor()
    : m_mode(FACE_SWAP)
    , m_blendStrength(0.8f)
    , m_enableEnhancement(true)
    , m_useGPU(false)
    , m_backend("ONNX")
    , m_processingTime(0.0)
    , m_frameCounter(0)
    , m_framesWithoutDetection(0)  // Face tracking initialization
    , m_useVideoTarget(false)
    , m_useDNNFaceDetection(false)
    , m_modelLoaded(false)
#ifdef HAVE_ONNX
    , m_faceSwapLoaded(false)
    , m_faceEmbeddingLoaded(false)
    , m_superResLoaded(false)
    , m_faceEnhanceLoaded(false)
    , m_segmentationLoaded(false)
#endif
{
}

PersonReplacementProcessor::~PersonReplacementProcessor()
{
    Cleanup();
}

bool PersonReplacementProcessor::Initialize()
{
#ifndef HAVE_OPENCV
    std::cerr << "PersonReplacementProcessor requires OpenCV support!" << std::endl;
    return false;
#else

    std::cout << "Initializing PersonReplacementProcessor..." << std::endl;

    // Initialize ONNX Runtime
#ifdef HAVE_ONNX
    try {
        m_onnxEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "PersonReplacementProcessor");
        m_sessionOptions = std::make_unique<Ort::SessionOptions>();
        
        if (m_useGPU) {
            // Try to enable CUDA if available
            try {
                OrtCUDAProviderOptions cuda_options;
                m_sessionOptions->AppendExecutionProvider_CUDA(cuda_options);
                std::cout << "GPU acceleration enabled" << std::endl;
            }
            catch (const std::exception& e) {
                std::cerr << "GPU not available, falling back to CPU: " << e.what() << std::endl;
                m_useGPU = false;
            }
        }
        
        // Set optimization level
        m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        
        std::cout << "ONNX Runtime initialized successfully" << std::endl;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to initialize ONNX Runtime: " << e.what() << std::endl;
        return false;
    }
#endif

    // Try to load face detection cascade (OpenCV fallback)
    std::vector<std::string> cascadePaths = {
        "D:/DevTools/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml",
        "C:/opencv/build/etc/haarcascades/haarcascade_frontalface_default.xml",
        "haarcascade_frontalface_default.xml"
    };

    bool cascadeLoaded = false;
    for (const auto& path : cascadePaths) {
        if (m_faceCascade.load(path)) {
            std::cout << "Loaded Haar Cascade from: " << path << std::endl;
            cascadeLoaded = true;
            break;
        }
    }

    if (!cascadeLoaded) {
        std::cerr << "Warning: Could not load face detection cascade. Face detection may not work." << std::endl;
    }

    // Auto-load face swap models if available
#ifdef HAVE_ONNX
    // Load ArcFace embedding model first (required for crossface_simswap)
    std::vector<std::string> arcfacePaths = {
        "models/simswap_arcface_backbone.onnx",
        "../../../models/simswap_arcface_backbone.onnx"
    };
    
    for (const auto& modelPath : arcfacePaths) {
        if (LoadFaceEmbeddingModel(modelPath)) {
            std::cout << "✅ Loaded face embedding model: " << modelPath << std::endl;
            break;
        }
    }

    // Load face swap model
    std::vector<std::string> faceSwapPaths = {
        "models/simswap.onnx",                        // Standard SimSwap (4D input, works directly)
        "../../../models/simswap.onnx",
        "models/inswapper_128.onnx",                  // InsightFace (standalone)
        "../../../models/inswapper_128.onnx"
        // Note: crossface_simswap.onnx needs embeddings, not images - skip for now
    };

    for (const auto& modelPath : faceSwapPaths) {
        if (LoadFaceSwapModel(modelPath)) {
            std::cout << "✅ Auto-loaded face swap model: " << modelPath << std::endl;
            break;
        }
    }

    if (!m_faceSwapLoaded) {
        std::cout << "ℹ️ No AI face swap model found - using OpenCV fallback" << std::endl;
        std::cout << "   For better quality, place crossface_simswap.onnx in models/ folder" << std::endl;
    }
#endif

    std::cout << "PersonReplacementProcessor initialized successfully!" << std::endl;
    return true;
#endif
}

void PersonReplacementProcessor::Cleanup()
{
#ifdef HAVE_OPENCV
    // Release video capture
    if (m_targetPersonVideo.isOpened()) {
        m_targetPersonVideo.release();
    }

    // Clear images
    m_targetPersonImage.release();
    m_currentTargetFrame.release();

#ifdef HAVE_ONNX
    // ONNX sessions will auto-cleanup via unique_ptr
    m_faceSwapSession.reset();
    m_faceEmbeddingSession.reset();
    m_superResSession.reset();
    m_faceEnhanceSession.reset();
    m_segmentationSession.reset();
    m_sessionOptions.reset();
    m_onnxEnv.reset();
    
    m_faceSwapLoaded = false;
    m_faceEmbeddingLoaded = false;
    m_superResLoaded = false;
    m_faceEnhanceLoaded = false;
    m_segmentationLoaded = false;
#endif

    m_modelLoaded = false;
#endif
}

Frame PersonReplacementProcessor::ProcessFrame(const Frame& input)
{
#ifndef HAVE_OPENCV
    std::cerr << "PersonReplacementProcessor requires OpenCV!" << std::endl;
    return input;
#else
    auto startTime = std::chrono::high_resolution_clock::now();

    // Get cv::Mat from input frame
    cv::Mat frame = input.data.clone();

    cv::Mat result;

    try {
        // Process based on selected mode
        switch (m_mode) {
            case FACE_SWAP:
                if (!m_targetPersonImage.empty()) {
                    result = ReplaceFace(frame, m_targetPersonImage);
                } else {
                    // No target image set - just pass through original frame with a message overlay
                    result = frame.clone();
                    std::string msg = "No target face image set. Place image at assets/default_face.jpg";
                    cv::putText(result, msg, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 
                               0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
                }
                break;

            case FULL_BODY_REPLACE:
                if (!m_targetPersonImage.empty()) {
                    result = ReplaceFullBody(frame, m_targetPersonImage);
                } else {
                    // No target image set - just pass through original frame with a message overlay
                    result = frame.clone();
                    std::string msg = "No target person image set. Place image at assets/default_person.jpg";
                    cv::putText(result, msg, cv::Point(10, 30), cv::FONT_HERSHEY_SIMPLEX, 
                               0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
                }
                break;

            case FACE_ENHANCE:
                result = EnhanceFaceInFrame(frame);
                break;

            case SUPER_RESOLUTION:
                result = SuperResolve(frame);
                break;

            case STYLE_TRANSFER:
                result = ApplyStyleTransfer(frame);
                break;

            default:
                result = frame.clone();
                break;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing frame: " << e.what() << std::endl;
        result = frame.clone();
    }

    // Calculate processing time
    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    m_frameCounter++;
    if (m_frameCounter % 30 == 0) {
        std::cout << "Person Replacement Processing Time: " << m_processingTime << " ms" << std::endl;
    }

    // Convert back to Frame
    Frame output(result);
    output.timestamp = input.timestamp;

    return output;
#endif
}

void PersonReplacementProcessor::SetReplacementMode(ReplacementMode mode)
{
    m_mode = mode;
    std::cout << "Replacement mode set to: " << static_cast<int>(mode) << std::endl;
}

void PersonReplacementProcessor::SetTargetPersonImage(const std::string& imagePath)
{
#ifdef HAVE_OPENCV
    m_targetPersonImage = cv::imread(imagePath);
    if (m_targetPersonImage.empty()) {
        std::cerr << "Failed to load target person image: " << imagePath << std::endl;
    } else {
        std::cout << "Target person image loaded: " << imagePath << std::endl;
        m_useVideoTarget = false;
    }
#endif
}

void PersonReplacementProcessor::SetTargetPersonVideo(const std::string& videoPath)
{
#ifdef HAVE_OPENCV
    m_targetPersonVideo.open(videoPath);
    if (!m_targetPersonVideo.isOpened()) {
        std::cerr << "Failed to open target person video: " << videoPath << std::endl;
    } else {
        std::cout << "Target person video opened: " << videoPath << std::endl;
        m_useVideoTarget = true;
    }
#endif
}

void PersonReplacementProcessor::SetBlendStrength(float strength)
{
    m_blendStrength = std::max(0.0f, std::min(1.0f, strength));
}

void PersonReplacementProcessor::SetEnableEnhancement(bool enable)
{
    m_enableEnhancement = enable;
}

void PersonReplacementProcessor::SetUseGPU(bool useGPU)
{
    m_useGPU = useGPU;
}

bool PersonReplacementProcessor::LoadFaceSwapModel(const std::string& modelPath)
{
#ifndef HAVE_ONNX
    std::cerr << "ONNX Runtime not available!" << std::endl;
    return false;
#else
    try {
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        m_faceSwapSession = std::make_unique<Ort::Session>(*m_onnxEnv, wModelPath.c_str(), *m_sessionOptions);
        
        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        m_faceSwapInputName = m_faceSwapSession->GetInputNameAllocated(0, allocator).get();
        m_faceSwapOutputName = m_faceSwapSession->GetOutputNameAllocated(0, allocator).get();
        
        m_faceSwapLoaded = true;
        std::cout << "Face swap model loaded: " << modelPath << std::endl;
        std::cout << "  Input: " << m_faceSwapInputName << ", Output: " << m_faceSwapOutputName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load face swap model: " << e.what() << std::endl;
        return false;
    }
#endif
}

bool PersonReplacementProcessor::LoadFaceEmbeddingModel(const std::string& modelPath)
{
#ifndef HAVE_ONNX
    std::cerr << "ONNX Runtime not available!" << std::endl;
    return false;
#else
    try {
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        m_faceEmbeddingSession = std::make_unique<Ort::Session>(*m_onnxEnv, wModelPath.c_str(), *m_sessionOptions);
        
        m_faceEmbeddingLoaded = true;
        std::cout << "Face embedding model (ArcFace) loaded: " << modelPath << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load face embedding model: " << e.what() << std::endl;
        return false;
    }
#endif
}

bool PersonReplacementProcessor::LoadSuperResolutionModel(const std::string& modelPath)
{
#ifndef HAVE_ONNX
    std::cerr << "ONNX Runtime not available!" << std::endl;
    return false;
#else
    try {
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        m_superResSession = std::make_unique<Ort::Session>(*m_onnxEnv, wModelPath.c_str(), *m_sessionOptions);
        
        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        m_superResInputName = m_superResSession->GetInputNameAllocated(0, allocator).get();
        m_superResOutputName = m_superResSession->GetOutputNameAllocated(0, allocator).get();
        
        m_superResLoaded = true;
        std::cout << "Super-resolution model loaded: " << modelPath << std::endl;
        std::cout << "  Input: " << m_superResInputName << ", Output: " << m_superResOutputName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load super-resolution model: " << e.what() << std::endl;
        return false;
    }
#endif
}

bool PersonReplacementProcessor::LoadFaceEnhancementModel(const std::string& modelPath)
{
#ifndef HAVE_ONNX
    std::cerr << "ONNX Runtime not available!" << std::endl;
    return false;
#else
    try {
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        m_faceEnhanceSession = std::make_unique<Ort::Session>(*m_onnxEnv, wModelPath.c_str(), *m_sessionOptions);
        
        // Get input/output names
        Ort::AllocatorWithDefaultOptions allocator;
        m_enhanceInputName = m_faceEnhanceSession->GetInputNameAllocated(0, allocator).get();
        m_enhanceOutputName = m_faceEnhanceSession->GetOutputNameAllocated(0, allocator).get();
        
        m_faceEnhanceLoaded = true;
        std::cout << "Face enhancement model loaded: " << modelPath << std::endl;
        std::cout << "  Input: " << m_enhanceInputName << ", Output: " << m_enhanceOutputName << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load face enhancement model: " << e.what() << std::endl;
        return false;
    }
#endif
}

bool PersonReplacementProcessor::LoadSegmentationModel(const std::string& modelPath)
{
#ifndef HAVE_ONNX
    std::cerr << "ONNX Runtime not available!" << std::endl;
    return false;
#else
    try {
        std::wstring wModelPath(modelPath.begin(), modelPath.end());
        m_segmentationSession = std::make_unique<Ort::Session>(*m_onnxEnv, wModelPath.c_str(), *m_sessionOptions);
        
        m_segmentationLoaded = true;
        std::cout << "Segmentation model loaded: " << modelPath << std::endl;
        return true;
    }
    catch (const std::exception& e) {
        std::cerr << "Failed to load segmentation model: " << e.what() << std::endl;
        return false;
    }
#endif
}

std::string PersonReplacementProcessor::GetReplacementInfo() const
{
    std::string info = "Person Replacement Processor\n";
    info += "Mode: ";
    switch (m_mode) {
        case FACE_SWAP: info += "Face Swap"; break;
        case FULL_BODY_REPLACE: info += "Full Body Replacement"; break;
        case FACE_ENHANCE: info += "Face Enhancement"; break;
        case SUPER_RESOLUTION: info += "Super Resolution"; break;
        case STYLE_TRANSFER: info += "Style Transfer"; break;
    }
    info += "\nBlend Strength: " + std::to_string(m_blendStrength);
    info += "\nEnhancement: " + std::string(m_enableEnhancement ? "Enabled" : "Disabled");
    info += "\nGPU: " + std::string(m_useGPU ? "Enabled" : "Disabled");
    return info;
}

bool PersonReplacementProcessor::SetParameter(const std::string& name, const std::string& value)
{
    m_parameters[name] = value;
    
    if (name == "mode") {
        if (value == "face_swap") SetReplacementMode(FACE_SWAP);
        else if (value == "full_body") SetReplacementMode(FULL_BODY_REPLACE);
        else if (value == "face_enhance") SetReplacementMode(FACE_ENHANCE);
        else if (value == "super_res") SetReplacementMode(SUPER_RESOLUTION);
        else if (value == "style_transfer") SetReplacementMode(STYLE_TRANSFER);
        return true;
    }
    else if (name == "blend_strength") {
        SetBlendStrength(std::stof(value));
        return true;
    }
    else if (name == "enable_enhancement") {
        SetEnableEnhancement(value == "true" || value == "1");
        return true;
    }
    else if (name == "use_gpu") {
        SetUseGPU(value == "true" || value == "1");
        return true;
    }
    else if (name == "target_image") {
        SetTargetPersonImage(value);
        return true;
    }
    else if (name == "target_video") {
        SetTargetPersonVideo(value);
        return true;
    }
    
    return false;
}

std::map<std::string, std::string> PersonReplacementProcessor::GetParameters() const
{
    return m_parameters;
}

#ifdef HAVE_OPENCV

cv::Mat PersonReplacementProcessor::ReplaceFace(const cv::Mat& frame, const cv::Mat& targetImage)
{
    cv::Mat result = frame.clone();

    // Detect faces in both source and target
    std::vector<cv::Rect> sourceFaces = DetectFaces(frame);
    std::vector<cv::Rect> targetFaces = DetectFaces(targetImage);

    if (sourceFaces.empty()) {
        // Don't spam console - just draw helpful message on frame
        static int framesSinceLastWarning = 0;
        if (framesSinceLastWarning == 0) {
            std::cout << "[Face Swap] No faces detected. Try: face camera directly, better lighting, remove glasses/mask" << std::endl;
        }
        framesSinceLastWarning = (framesSinceLastWarning + 1) % 60; // Log every 2 seconds at 30fps
        
        // Draw helpful message on frame
        cv::putText(result, "No face detected - face camera directly", cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
        cv::putText(result, "Try: better lighting, remove glasses/mask", cv::Point(10, 50), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
        return result;
    }

    if (targetFaces.empty()) {
        // Check target image only once
        static bool targetWarningShown = false;
        if (!targetWarningShown) {
            std::cerr << "[Face Swap] No faces detected in target image. Use a clear frontal face photo." << std::endl;
            targetWarningShown = true;
        }
        cv::putText(result, "No face in target image - use frontal face photo", cv::Point(10, 30), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        return result;
    }

    // Process each detected face
    for (size_t i = 0; i < sourceFaces.size() && i < targetFaces.size(); ++i) {
        const cv::Rect& sourceFaceRect = sourceFaces[i];
        const cv::Rect& targetFaceRect = targetFaces[i];

        // Expand face rect to include more context (forehead, chin, ears)
        // This gives better blending at edges
        int expandX = sourceFaceRect.width * 0.2;   // 20% expansion
        int expandY = sourceFaceRect.height * 0.3;  // 30% expansion (more for forehead/chin)
        
        cv::Rect expandedSourceRect = sourceFaceRect;
        expandedSourceRect.x = std::max(0, sourceFaceRect.x - expandX);
        expandedSourceRect.y = std::max(0, sourceFaceRect.y - expandY);
        expandedSourceRect.width = std::min(frame.cols - expandedSourceRect.x, 
                                           sourceFaceRect.width + 2 * expandX);
        expandedSourceRect.height = std::min(frame.rows - expandedSourceRect.y, 
                                             sourceFaceRect.height + 2 * expandY);

        // Extract face regions with expansion
        cv::Mat sourceFace = frame(expandedSourceRect);
        cv::Mat targetFace = targetImage(targetFaceRect);

        // Resize target face to match expanded source size
        cv::Mat resizedTarget;
        cv::resize(targetFace, resizedTarget, sourceFace.size(), 0, 0, cv::INTER_CUBIC);

#ifdef HAVE_ONNX
        // Use ONNX model if loaded
        if (m_faceSwapLoaded) {
            cv::Mat swappedFace = RunFaceSwapInference(sourceFace, resizedTarget);
            if (!swappedFace.empty()) {
                resizedTarget = swappedFace;
            }
        }
#endif

        // Apply color correction to match source lighting
        cv::Mat colorCorrected = MatchColorHistogram(resizedTarget, sourceFace);
        
        // Create feathered mask for smooth blending
        cv::Mat mask = CreateFeatheredMask(colorCorrected.size());
        
        // Seamless clone for better blending (Poisson blending)
        cv::Mat blended;
        try {
            // Calculate center point for seamless clone
            cv::Point center(expandedSourceRect.width / 2, expandedSourceRect.height / 2);
            
            // Convert mask to 8-bit for seamlessClone
            cv::Mat mask8bit;
            mask.convertTo(mask8bit, CV_8U, 255.0);
            
            // Use seamless clone for natural blending
            cv::seamlessClone(colorCorrected, sourceFace, mask8bit, center, blended, cv::MIXED_CLONE);
        }
        catch (const cv::Exception& e) {
            // Fallback to feathered alpha blending if seamlessClone fails
            blended = AlphaBlendWithMask(sourceFace, colorCorrected, mask, m_blendStrength);
        }

        // Copy blended result back
        blended.copyTo(result(expandedSourceRect));
        
        // Draw subtle indicator (optional, comment out for production)
        cv::rectangle(result, sourceFaceRect, cv::Scalar(0, 255, 0), 1);
    }

    return result;
}

cv::Mat PersonReplacementProcessor::ReplaceFullBody(const cv::Mat& frame, const cv::Mat& targetPerson)
{
    cv::Mat result = frame.clone();

    // Segment the person in the frame
    cv::Mat mask = SegmentPerson(frame);

    if (mask.empty()) {
        std::cerr << "Person segmentation failed" << std::endl;
        return result;
    }

    // Resize target person to match frame size
    cv::Mat resizedTarget;
    cv::resize(targetPerson, resizedTarget, frame.size());

    // Use mask to blend target person into frame
    for (int y = 0; y < frame.rows; ++y) {
        for (int x = 0; x < frame.cols; ++x) {
            float maskValue = mask.at<float>(y, x);
            cv::Vec3b targetPixel = resizedTarget.at<cv::Vec3b>(y, x);
            cv::Vec3b framePixel = frame.at<cv::Vec3b>(y, x);

            // Blend based on mask and blend strength
            float alpha = maskValue * m_blendStrength;
            result.at<cv::Vec3b>(y, x) = cv::Vec3b(
                static_cast<uchar>(framePixel[0] * (1 - alpha) + targetPixel[0] * alpha),
                static_cast<uchar>(framePixel[1] * (1 - alpha) + targetPixel[1] * alpha),
                static_cast<uchar>(framePixel[2] * (1 - alpha) + targetPixel[2] * alpha)
            );
        }
    }

    return result;
}

cv::Mat PersonReplacementProcessor::EnhanceFaceInFrame(const cv::Mat& frame)
{
    cv::Mat result = frame.clone();

    // Detect faces
    std::vector<cv::Rect> faces = DetectFaces(frame);

    if (faces.empty()) {
        return result;
    }

    // Enhance each detected face
    for (const auto& faceRect : faces) {
        cv::Mat face = frame(faceRect);
        cv::Mat enhancedFace = EnhanceFace(face);

        if (!enhancedFace.empty()) {
            enhancedFace.copyTo(result(faceRect));
        }
    }

    return result;
}

cv::Mat PersonReplacementProcessor::EnhanceFace(const cv::Mat& face)
{
#ifdef HAVE_ONNX
    if (m_faceEnhanceLoaded) {
        return RunFaceEnhancementInference(face);
    }
#endif

    // Fallback: simple enhancement using OpenCV
    cv::Mat enhanced;
    
    // Apply bilateral filter for noise reduction while preserving edges
    cv::bilateralFilter(face, enhanced, 9, 75, 75);
    
    // Increase sharpness
    cv::Mat kernel = (cv::Mat_<float>(3, 3) << 
        0, -1, 0,
        -1, 5, -1,
        0, -1, 0);
    cv::filter2D(enhanced, enhanced, -1, kernel);
    
    return enhanced;
}

cv::Mat PersonReplacementProcessor::SuperResolve(const cv::Mat& image)
{
#ifdef HAVE_ONNX
    if (m_superResLoaded) {
        return RunSuperResolutionInference(image);
    }
#endif

    // Fallback: use OpenCV's resize with INTER_CUBIC
    cv::Mat upscaled;
    cv::resize(image, upscaled, cv::Size(image.cols * 2, image.rows * 2), 0, 0, cv::INTER_CUBIC);
    return upscaled;
}

cv::Mat PersonReplacementProcessor::ApplyStyleTransfer(const cv::Mat& image)
{
    // Placeholder for style transfer
    // This would use a neural style transfer model (ONNX)
    std::cerr << "Style transfer not yet implemented" << std::endl;
    return image.clone();
}

std::vector<cv::Rect> PersonReplacementProcessor::DetectFaces(const cv::Mat& frame)
{
    std::vector<cv::Rect> faces;

    if (m_faceCascade.empty()) {
        // Cascade not loaded - can't detect faces
        return faces;
    }

    // Convert to grayscale for Haar cascade
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    // Detect faces using Haar cascade with BALANCED parameters for accuracy + detection
    // scaleFactor: 1.15 (balanced between speed and accuracy)
    // minNeighbors: 6 (balanced - good detection with reasonable false positive rejection)
    // minSize: 80x80 (reasonable minimum face size)
    // maxSize: frame.cols/2 (prevent detecting entire image as face)
    std::vector<cv::Rect> detectedFaces;
    cv::Size maxSize(frame.cols / 2, frame.rows / 2);
    m_faceCascade.detectMultiScale(gray, detectedFaces, 1.15, 6, 0, cv::Size(80, 80), maxSize);

    // Filter faces: Only keep faces in CENTER 80% of frame (reject far edges/background)
    int centerMarginX = frame.cols * 0.1;  // 10% margin on each side
    int centerMarginY = frame.rows * 0.05;  // 5% margin top/bottom
    cv::Rect centerRegion(centerMarginX, centerMarginY, 
                          frame.cols - 2 * centerMarginX, 
                          frame.rows - 2 * centerMarginY);

    for (const auto& face : detectedFaces) {
        // Calculate center of detected face
        cv::Point faceCenter(face.x + face.width / 2, face.y + face.height / 2);
        
        // Only accept faces with center in the central region
        if (centerRegion.contains(faceCenter)) {
            // Additional check: aspect ratio should be reasonable (0.7 to 1.3)
            float aspectRatio = static_cast<float>(face.width) / face.height;
            if (aspectRatio > 0.7f && aspectRatio < 1.3f) {
                faces.push_back(face);
            }
        }
    }

    // If multiple faces detected, choose the largest one (closest to camera)
    if (faces.size() > 1) {
        auto largestFace = std::max_element(faces.begin(), faces.end(),
            [](const cv::Rect& a, const cv::Rect& b) {
                return (a.width * a.height) < (b.width * b.height);
            });
        faces = { *largestFace };
    }

    // Face tracking: Match detected faces with previous frame
    if (!m_previousFaces.empty() && !faces.empty()) {
        std::vector<cv::Rect> stabilizedFaces;
        
        for (auto& face : faces) {
            // Find best matching face from previous frame
            cv::Rect bestMatch = face;
            float bestOverlap = 0.0f;
            
            for (const auto& prevFace : m_previousFaces) {
                float overlap = CalculateFaceOverlap(face, prevFace);
                if (overlap > bestOverlap) {
                    bestOverlap = overlap;
                    
                    // Smooth transition: blend current and previous position
                    if (overlap > FACE_OVERLAP_THRESHOLD) {
                        // 80% previous position, 20% new detection (very smooth)
                        bestMatch.x = static_cast<int>(prevFace.x * 0.8 + face.x * 0.2);
                        bestMatch.y = static_cast<int>(prevFace.y * 0.8 + face.y * 0.2);
                        bestMatch.width = static_cast<int>(prevFace.width * 0.8 + face.width * 0.2);
                        bestMatch.height = static_cast<int>(prevFace.height * 0.8 + face.height * 0.2);
                    }
                }
            }
            
            stabilizedFaces.push_back(bestMatch);
        }
        
        faces = stabilizedFaces;
        m_framesWithoutDetection = 0;
    }
    else if (faces.empty() && !m_previousFaces.empty()) {
        // No detection this frame - use previous faces for a few frames (persistence)
        if (m_framesWithoutDetection < MAX_FRAMES_WITHOUT_DETECTION) {
            faces = m_previousFaces;
            m_framesWithoutDetection++;
        }
    }
    else {
        m_framesWithoutDetection = 0;
    }

    // Update previous faces
    if (!faces.empty()) {
        m_previousFaces = faces;
    }

    return faces;
}

// Helper function to calculate overlap between two rectangles
float PersonReplacementProcessor::CalculateFaceOverlap(const cv::Rect& rect1, const cv::Rect& rect2)
{
    int x1 = std::max(rect1.x, rect2.x);
    int y1 = std::max(rect1.y, rect2.y);
    int x2 = std::min(rect1.x + rect1.width, rect2.x + rect2.width);
    int y2 = std::min(rect1.y + rect1.height, rect2.y + rect2.height);
    
    if (x2 < x1 || y2 < y1) {
        return 0.0f;  // No overlap
    }
    
    int intersectionArea = (x2 - x1) * (y2 - y1);
    int rect1Area = rect1.width * rect1.height;
    int rect2Area = rect2.width * rect2.height;
    int unionArea = rect1Area + rect2Area - intersectionArea;
    
    return static_cast<float>(intersectionArea) / static_cast<float>(unionArea);
}

cv::Mat PersonReplacementProcessor::MatchColorHistogram(const cv::Mat& source, const cv::Mat& target)
{
    // Match color distribution of source to target for natural lighting
    cv::Mat result = source.clone();
    
    if (source.empty() || target.empty() || source.size() != target.size()) {
        return result;
    }

    // Split into channels
    std::vector<cv::Mat> sourceChannels, targetChannels, resultChannels;
    cv::split(source, sourceChannels);
    cv::split(target, targetChannels);
    
    // Match histogram for each channel (B, G, R)
    for (int i = 0; i < 3; i++) {
        // Calculate histograms
        cv::Mat sourceHist, targetHist;
        int histSize = 256;
        float range[] = {0, 256};
        const float* histRange = {range};
        
        cv::calcHist(&sourceChannels[i], 1, 0, cv::Mat(), sourceHist, 1, &histSize, &histRange);
        cv::calcHist(&targetChannels[i], 1, 0, cv::Mat(), targetHist, 1, &histSize, &histRange);
        
        // Calculate cumulative distribution
        cv::Mat sourceCDF = sourceHist.clone();
        cv::Mat targetCDF = targetHist.clone();
        
        for (int j = 1; j < histSize; j++) {
            sourceCDF.at<float>(j) += sourceCDF.at<float>(j - 1);
            targetCDF.at<float>(j) += targetCDF.at<float>(j - 1);
        }
        
        // Normalize CDFs
        sourceCDF /= sourceCDF.at<float>(histSize - 1);
        targetCDF /= targetCDF.at<float>(histSize - 1);
        
        // Create lookup table for histogram matching
        cv::Mat lookupTable(1, 256, CV_8U);
        for (int j = 0; j < 256; j++) {
            float sourceVal = sourceCDF.at<float>(j);
            int k = 0;
            while (k < 256 && targetCDF.at<float>(k) < sourceVal) {
                k++;
            }
            lookupTable.at<uchar>(j) = std::min(k, 255);
        }
        
        // Apply lookup table
        cv::LUT(sourceChannels[i], lookupTable, sourceChannels[i]);
    }
    
    // Merge channels back
    cv::merge(sourceChannels, result);
    
    return result;
}

cv::Mat PersonReplacementProcessor::CreateFeatheredMask(const cv::Size& size)
{
    // Create elliptical mask with feathered edges for smooth blending
    cv::Mat mask = cv::Mat::zeros(size, CV_32FC1);
    
    cv::Point center(size.width / 2, size.height / 2);
    int radiusX = size.width / 2;
    int radiusY = size.height / 2;
    
    // Create elliptical gradient mask
    for (int y = 0; y < size.height; y++) {
        for (int x = 0; x < size.width; x++) {
            // Calculate normalized distance from center (elliptical)
            float dx = (float)(x - center.x) / radiusX;
            float dy = (float)(y - center.y) / radiusY;
            float dist = std::sqrt(dx * dx + dy * dy);
            
            // Create smooth falloff
            float value = 1.0f;
            if (dist > 0.7f) {
                // Feather the edges (30% outer region)
                value = std::max(0.0f, (1.0f - dist) / 0.3f);
                value = value * value; // Squared for smoother falloff
            }
            
            mask.at<float>(y, x) = value;
        }
    }
    
    // Apply Gaussian blur for even smoother edges
    cv::GaussianBlur(mask, mask, cv::Size(0, 0), size.width * 0.05);
    
    return mask;
}

cv::Mat PersonReplacementProcessor::AlphaBlendWithMask(const cv::Mat& background, 
                                                       const cv::Mat& foreground, 
                                                       const cv::Mat& mask, 
                                                       float blendStrength)
{
    // Alpha blend two images using a mask
    cv::Mat result = background.clone();
    
    if (background.size() != foreground.size() || background.size() != mask.size()) {
        return result;
    }
    
    // Ensure mask is 32F
    cv::Mat maskF;
    if (mask.type() != CV_32FC1) {
        mask.convertTo(maskF, CV_32FC1, 1.0 / 255.0);
    } else {
        maskF = mask;
    }
    
    // Apply blend strength to mask
    maskF = maskF * blendStrength;
    
    // Convert images to float for blending
    cv::Mat bgFloat, fgFloat;
    background.convertTo(bgFloat, CV_32FC3);
    foreground.convertTo(fgFloat, CV_32FC3);
    
    // Blend each pixel
    for (int y = 0; y < result.rows; y++) {
        for (int x = 0; x < result.cols; x++) {
            float alpha = maskF.at<float>(y, x);
            cv::Vec3f bg = bgFloat.at<cv::Vec3f>(y, x);
            cv::Vec3f fg = fgFloat.at<cv::Vec3f>(y, x);
            
            cv::Vec3f blended = bg * (1.0f - alpha) + fg * alpha;
            result.at<cv::Vec3b>(y, x) = cv::Vec3b(
                cv::saturate_cast<uchar>(blended[0]),
                cv::saturate_cast<uchar>(blended[1]),
                cv::saturate_cast<uchar>(blended[2])
            );
        }
    }
    
    return result;
}

cv::Mat PersonReplacementProcessor::AlignFace(const cv::Mat& face, const cv::Rect& faceRect)
{
    // Simple alignment: just return the face region
    // More advanced: detect landmarks and apply affine transform
    return face.clone();
}

std::vector<cv::Point2f> PersonReplacementProcessor::DetectFaceLandmarks(const cv::Mat& face)
{
    // Placeholder for landmark detection
    // Would use dlib or OpenCV face landmark detector
    std::vector<cv::Point2f> landmarks;
    return landmarks;
}

cv::Mat PersonReplacementProcessor::SegmentPerson(const cv::Mat& frame)
{
#ifdef HAVE_ONNX
    if (m_segmentationLoaded) {
        return RunSegmentationInference(frame);
    }
#endif

    // Fallback: simple background subtraction or GrabCut
    cv::Mat mask = cv::Mat::zeros(frame.size(), CV_32FC1);
    
    // Simple person detection: assume center region contains person
    cv::Rect personROI(frame.cols / 4, frame.rows / 4, frame.cols / 2, frame.rows / 2);
    mask(personROI).setTo(1.0f);
    
    return mask;
}

cv::Mat PersonReplacementProcessor::SeamlessBlend(const cv::Mat& source, const cv::Mat& target, const cv::Mat& mask)
{
    cv::Mat result;
    
    // Use Poisson blending if available
    try {
        cv::Point center(target.cols / 2, target.rows / 2);
        cv::seamlessClone(source, target, mask, center, result, cv::NORMAL_CLONE);
    }
    catch (const std::exception& e) {
        std::cerr << "Seamless blend failed: " << e.what() << std::endl;
        result = target.clone();
    }
    
    return result;
}

cv::Mat PersonReplacementProcessor::PoissonBlend(const cv::Mat& source, const cv::Mat& target, const cv::Point& center)
{
    cv::Mat result;
    cv::Mat mask = cv::Mat::ones(source.size(), CV_8UC1) * 255;
    
    try {
        cv::seamlessClone(source, target, mask, center, result, cv::NORMAL_CLONE);
    }
    catch (const std::exception& e) {
        std::cerr << "Poisson blend failed: " << e.what() << std::endl;
        result = target.clone();
    }
    
    return result;
}

cv::Mat PersonReplacementProcessor::GetNextVideoFrame()
{
    if (!m_targetPersonVideo.isOpened()) {
        return cv::Mat();
    }

    cv::Mat frame;
    if (!m_targetPersonVideo.read(frame)) {
        // Loop video
        m_targetPersonVideo.set(cv::CAP_PROP_POS_FRAMES, 0);
        m_targetPersonVideo.read(frame);
    }

    return frame;
}

#ifdef HAVE_ONNX

cv::Mat PersonReplacementProcessor::RunFaceSwapInference(const cv::Mat& sourceFace, const cv::Mat& targetFace)
{
    if (!m_faceSwapLoaded) {
        return cv::Mat();
    }

    try {
        // simswap.onnx requires:
        //   1. target: [1, 3, 224, 224] - target face image
        //   2. source_embedding: [1, 512] - source face embedding from ArcFace
        int inputSize = 224;
        
        // Step 1: Extract source face embedding using ArcFace model
        std::vector<float> sourceEmbedding(512);
        if (m_faceEmbeddingLoaded) {
            // Preprocess source face for ArcFace (typically 112x112 for ArcFace)
            cv::Mat arcfaceInput;
            cv::resize(sourceFace, arcfaceInput, cv::Size(112, 112), 0, 0, cv::INTER_CUBIC);
            cv::cvtColor(arcfaceInput, arcfaceInput, cv::COLOR_BGR2RGB);
            arcfaceInput.convertTo(arcfaceInput, CV_32FC3, 1.0 / 127.5, -1.0);  // Normalize to [-1, 1]

            // Convert HWC to CHW
            std::vector<int64_t> arcfaceShape = {1, 3, 112, 112};
            std::vector<float> arcfaceValues(1 * 3 * 112 * 112);
            for (int c = 0; c < 3; ++c) {
                for (int h = 0; h < 112; ++h) {
                    for (int w = 0; w < 112; ++w) {
                        int idx = c * 112 * 112 + h * 112 + w;
                        arcfaceValues[idx] = arcfaceInput.at<cv::Vec3f>(h, w)[c];
                    }
                }
            }

            // Run ArcFace inference to get embedding
            Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
            Ort::Value arcfaceTensor = Ort::Value::CreateTensor<float>(
                memoryInfo, arcfaceValues.data(), arcfaceValues.size(), arcfaceShape.data(), arcfaceShape.size());

            const char* arcfaceInputName = "input";  // Common ArcFace input name
            const char* arcfaceOutputName = "output"; // Common ArcFace output name
            
            auto arcfaceOutput = m_faceEmbeddingSession->Run(
                Ort::RunOptions{nullptr},
                &arcfaceInputName, &arcfaceTensor, 1,
                &arcfaceOutputName, 1);

            // Extract embedding [1, 512]
            float* embeddingData = arcfaceOutput[0].GetTensorMutableData<float>();
            std::copy(embeddingData, embeddingData + 512, sourceEmbedding.begin());
            
            std::cout << "✅ Extracted face embedding (512D)" << std::endl;
        } else {
            // No embedding model - cannot proceed
            std::cerr << "❌ ArcFace embedding model not loaded, cannot run face swap" << std::endl;
            return cv::Mat();
        }

        // Step 2: Preprocess target face for face swap model (224x224)
        cv::Mat targetPreprocessed;
        cv::resize(targetFace, targetPreprocessed, cv::Size(inputSize, inputSize), 0, 0, cv::INTER_CUBIC);
        cv::cvtColor(targetPreprocessed, targetPreprocessed, cv::COLOR_BGR2RGB);
        targetPreprocessed.convertTo(targetPreprocessed, CV_32FC3, 2.0 / 255.0, -1.0);  // [-1, 1]

        // Convert HWC to CHW format
        std::vector<int64_t> targetShape = {1, 3, inputSize, inputSize};
        size_t targetTensorSize = 1 * 3 * inputSize * inputSize;
        std::vector<float> targetValues(targetTensorSize);

        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    int idx = c * inputSize * inputSize + h * inputSize + w;
                    targetValues[idx] = targetPreprocessed.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        // Step 3: Create input tensors for face swap model
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        
        std::vector<int64_t> embeddingShape = {1, 512};
        Ort::Value embeddingTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, sourceEmbedding.data(), sourceEmbedding.size(), embeddingShape.data(), embeddingShape.size());
        
        Ort::Value targetTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, targetValues.data(), targetTensorSize, targetShape.data(), targetShape.size());

        // Input names: "target" and "source_embedding"
        const char* inputNames[] = {"target", "source_embedding"};
        const char* outputNames[] = {"output"};
        
        std::vector<Ort::Value> inputTensors;
        inputTensors.push_back(std::move(targetTensor));
        inputTensors.push_back(std::move(embeddingTensor));

        // Step 4: Run face swap inference
        auto outputTensors = m_faceSwapSession->Run(
            Ort::RunOptions{nullptr}, 
            inputNames, inputTensors.data(), inputTensors.size(),
            outputNames, 1);

        // Get output tensor
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
        
        int outputHeight = static_cast<int>(outputShape[2]);
        int outputWidth = static_cast<int>(outputShape[3]);

        // Convert back to cv::Mat (denormalize from [-1, 1] to [0, 255])
        cv::Mat output(outputHeight, outputWidth, CV_32FC3);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < outputHeight; ++h) {
                for (int w = 0; w < outputWidth; ++w) {
                    int idx = c * outputHeight * outputWidth + h * outputWidth + w;
                    // Denormalize: from [-1, 1] to [0, 1]
                    float value = (outputData[idx] + 1.0f) / 2.0f;
                    output.at<cv::Vec3f>(h, w)[c] = std::max(0.0f, std::min(1.0f, value));
                }
            }
        }

        // Convert back to BGR and 8-bit
        output.convertTo(output, CV_8UC3, 255.0);
        cv::cvtColor(output, output, cv::COLOR_RGB2BGR);
        
        // Resize to original face size
        cv::resize(output, output, targetFace.size(), 0, 0, cv::INTER_CUBIC);

        std::cout << "✅ AI face swap successful (simswap.onnx)" << std::endl;
        return output;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Face swap inference failed: " << e.what() << std::endl;
        std::cerr << "   Falling back to OpenCV histogram matching" << std::endl;
        return cv::Mat();
    }
}

cv::Mat PersonReplacementProcessor::RunSuperResolutionInference(const cv::Mat& lowRes)
{
    if (!m_superResLoaded) {
        return cv::Mat();
    }

    try {
        // Prepare input (example: variable size input)
        cv::Mat preprocessed;
        lowRes.convertTo(preprocessed, CV_32FC3, 1.0 / 255.0);
        cv::cvtColor(preprocessed, preprocessed, cv::COLOR_BGR2RGB);

        int h = preprocessed.rows;
        int w = preprocessed.cols;
        
        std::vector<int64_t> inputShape = {1, 3, h, w};
        size_t inputTensorSize = 1 * 3 * h * w;
        std::vector<float> inputTensorValues(inputTensorSize);

        // Convert HWC to CHW
        for (int c = 0; c < 3; ++c) {
            for (int y = 0; y < h; ++y) {
                for (int x = 0; x < w; ++x) {
                    inputTensorValues[c * h * w + y * w + x] =
                        preprocessed.at<cv::Vec3f>(y, x)[c];
                }
            }
        }

        // Run inference
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, inputTensorValues.data(), inputTensorSize, inputShape.data(), inputShape.size());

        const char* inputNames[] = {m_superResInputName.c_str()};
        const char* outputNames[] = {m_superResOutputName.c_str()};
        
        auto outputTensors = m_superResSession->Run(
            Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);

        // Get output
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

        int outH = static_cast<int>(outputShape[2]);
        int outW = static_cast<int>(outputShape[3]);

        cv::Mat output(outH, outW, CV_32FC3);
        for (int c = 0; c < 3; ++c) {
            for (int y = 0; y < outH; ++y) {
                for (int x = 0; x < outW; ++x) {
                    output.at<cv::Vec3f>(y, x)[c] = 
                        outputData[c * outH * outW + y * outW + x];
                }
            }
        }

        output.convertTo(output, CV_8UC3, 255.0);
        cv::cvtColor(output, output, cv::COLOR_RGB2BGR);

        return output;
    }
    catch (const std::exception& e) {
        std::cerr << "Super-resolution inference failed: " << e.what() << std::endl;
        return cv::Mat();
    }
}

cv::Mat PersonReplacementProcessor::RunFaceEnhancementInference(const cv::Mat& face)
{
    if (!m_faceEnhanceLoaded) {
        return cv::Mat();
    }

    try {
        // Similar to super-resolution but with face-specific model
        int inputSize = 512;  // Typical for GFPGAN
        cv::Mat preprocessed;
        cv::resize(face, preprocessed, cv::Size(inputSize, inputSize));
        cv::cvtColor(preprocessed, preprocessed, cv::COLOR_BGR2RGB);
        preprocessed.convertTo(preprocessed, CV_32FC3, 1.0 / 255.0);

        std::vector<int64_t> inputShape = {1, 3, inputSize, inputSize};
        size_t inputTensorSize = 1 * 3 * inputSize * inputSize;
        std::vector<float> inputTensorValues(inputTensorSize);

        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    inputTensorValues[c * inputSize * inputSize + h * inputSize + w] =
                        preprocessed.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, inputTensorValues.data(), inputTensorSize, inputShape.data(), inputShape.size());

        const char* inputNames[] = {m_enhanceInputName.c_str()};
        const char* outputNames[] = {m_enhanceOutputName.c_str()};
        
        auto outputTensors = m_faceEnhanceSession->Run(
            Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);

        float* outputData = outputTensors[0].GetTensorMutableData<float>();

        cv::Mat output(inputSize, inputSize, CV_32FC3);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    output.at<cv::Vec3f>(h, w)[c] = 
                        outputData[c * inputSize * inputSize + h * inputSize + w];
                }
            }
        }

        output.convertTo(output, CV_8UC3, 255.0);
        cv::cvtColor(output, output, cv::COLOR_RGB2BGR);
        cv::resize(output, output, face.size());

        return output;
    }
    catch (const std::exception& e) {
        std::cerr << "Face enhancement inference failed: " << e.what() << std::endl;
        return cv::Mat();
    }
}

cv::Mat PersonReplacementProcessor::RunSegmentationInference(const cv::Mat& frame)
{
    if (!m_segmentationLoaded) {
        return cv::Mat();
    }

    try {
        // Use MediaPipe or similar person segmentation model
        int inputSize = 256;
        cv::Mat preprocessed;
        cv::resize(frame, preprocessed, cv::Size(inputSize, inputSize));
        cv::cvtColor(preprocessed, preprocessed, cv::COLOR_BGR2RGB);
        preprocessed.convertTo(preprocessed, CV_32FC3, 1.0 / 255.0);

        std::vector<int64_t> inputShape = {1, 3, inputSize, inputSize};
        size_t inputTensorSize = 1 * 3 * inputSize * inputSize;
        std::vector<float> inputTensorValues(inputTensorSize);

        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    inputTensorValues[c * inputSize * inputSize + h * inputSize + w] =
                        preprocessed.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, inputTensorValues.data(), inputTensorSize, inputShape.data(), inputShape.size());

        Ort::AllocatorWithDefaultOptions allocator;
        auto inputName = m_segmentationSession->GetInputNameAllocated(0, allocator);
        auto outputName = m_segmentationSession->GetOutputNameAllocated(0, allocator);
        
        const char* inputNames[] = {inputName.get()};
        const char* outputNames[] = {outputName.get()};
        
        auto outputTensors = m_segmentationSession->Run(
            Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);

        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

        // Create mask from segmentation output
        cv::Mat mask(inputSize, inputSize, CV_32FC1, outputData);
        
        // Resize to original size
        cv::resize(mask, mask, frame.size());

        return mask;
    }
    catch (const std::exception& e) {
        std::cerr << "Segmentation inference failed: " << e.what() << std::endl;
        return cv::Mat();
    }
}

#endif // HAVE_ONNX

#endif // HAVE_OPENCV
