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
    , m_useVideoTarget(false)
    , m_useDNNFaceDetection(false)
    , m_modelLoaded(false)
#ifdef HAVE_ONNX
    , m_faceSwapLoaded(false)
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
    m_superResSession.reset();
    m_faceEnhanceSession.reset();
    m_segmentationSession.reset();
    m_sessionOptions.reset();
    m_onnxEnv.reset();
    
    m_faceSwapLoaded = false;
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

    // Convert Frame to cv::Mat
    cv::Mat frame(input.height, input.width, CV_8UC3, (void*)input.data, input.stride);
    
    // Ensure BGR format
    cv::Mat bgrFrame;
    if (frame.channels() == 4) {
        cv::cvtColor(frame, bgrFrame, cv::COLOR_BGRA2BGR);
    } else {
        bgrFrame = frame.clone();
    }

    cv::Mat result;

    try {
        // Process based on selected mode
        switch (m_mode) {
            case FACE_SWAP:
                if (!m_targetPersonImage.empty()) {
                    result = ReplaceFace(bgrFrame, m_targetPersonImage);
                } else {
                    std::cerr << "No target person image set for face swap!" << std::endl;
                    result = bgrFrame.clone();
                }
                break;

            case FULL_BODY_REPLACE:
                if (!m_targetPersonImage.empty()) {
                    result = ReplaceFullBody(bgrFrame, m_targetPersonImage);
                } else {
                    std::cerr << "No target person image set for full body replacement!" << std::endl;
                    result = bgrFrame.clone();
                }
                break;

            case FACE_ENHANCE:
                result = EnhanceFaceInFrame(bgrFrame);
                break;

            case SUPER_RESOLUTION:
                result = SuperResolve(bgrFrame);
                break;

            case STYLE_TRANSFER:
                result = ApplyStyleTransfer(bgrFrame);
                break;

            default:
                result = bgrFrame.clone();
                break;
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error processing frame: " << e.what() << std::endl;
        result = bgrFrame.clone();
    }

    // Calculate processing time
    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    m_frameCounter++;
    if (m_frameCounter % 30 == 0) {
        std::cout << "Person Replacement Processing Time: " << m_processingTime << " ms" << std::endl;
    }

    // Convert back to Frame
    Frame output;
    output.width = result.cols;
    output.height = result.rows;
    output.stride = result.step;
    output.format = FrameFormat::BGR24;
    output.data = new uint8_t[result.total() * result.elemSize()];
    std::memcpy(output.data, result.data, result.total() * result.elemSize());

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
        std::cerr << "No faces detected in source frame" << std::endl;
        return result;
    }

    if (targetFaces.empty()) {
        std::cerr << "No faces detected in target image" << std::endl;
        return result;
    }

    // Process each detected face
    for (size_t i = 0; i < sourceFaces.size() && i < targetFaces.size(); ++i) {
        const cv::Rect& sourceFaceRect = sourceFaces[i];
        const cv::Rect& targetFaceRect = targetFaces[i];

        // Extract face regions
        cv::Mat sourceFace = frame(sourceFaceRect);
        cv::Mat targetFace = targetImage(targetFaceRect);

        // Resize target face to match source face size
        cv::Mat resizedTarget;
        cv::resize(targetFace, resizedTarget, sourceFace.size());

#ifdef HAVE_ONNX
        // Use ONNX model if loaded
        if (m_faceSwapLoaded) {
            cv::Mat swappedFace = RunFaceSwapInference(sourceFace, resizedTarget);
            if (!swappedFace.empty()) {
                resizedTarget = swappedFace;
            }
        }
#endif

        // Blend the faces
        cv::Mat blended;
        cv::addWeighted(sourceFace, 1.0 - m_blendStrength, resizedTarget, m_blendStrength, 0, blended);

        // Copy blended face back to result
        blended.copyTo(result(sourceFaceRect));
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

    // Convert to grayscale for Haar cascade
    cv::Mat gray;
    cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
    cv::equalizeHist(gray, gray);

    // Detect faces using Haar cascade
    m_faceCascade.detectMultiScale(gray, faces, 1.1, 3, 0, cv::Size(30, 30));

    return faces;
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
        // Prepare input tensor (example: 256x256 RGB normalized)
        int inputSize = 256;
        cv::Mat preprocessed;
        cv::resize(targetFace, preprocessed, cv::Size(inputSize, inputSize));
        cv::cvtColor(preprocessed, preprocessed, cv::COLOR_BGR2RGB);
        preprocessed.convertTo(preprocessed, CV_32FC3, 1.0 / 255.0);

        // Create tensor
        std::vector<int64_t> inputShape = {1, 3, inputSize, inputSize};
        size_t inputTensorSize = 1 * 3 * inputSize * inputSize;
        std::vector<float> inputTensorValues(inputTensorSize);

        // Convert HWC to CHW format
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    inputTensorValues[c * inputSize * inputSize + h * inputSize + w] =
                        preprocessed.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        // Create ONNX tensor
        Ort::MemoryInfo memoryInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value inputTensor = Ort::Value::CreateTensor<float>(
            memoryInfo, inputTensorValues.data(), inputTensorSize, inputShape.data(), inputShape.size());

        // Run inference
        const char* inputNames[] = {m_faceSwapInputName.c_str()};
        const char* outputNames[] = {m_faceSwapOutputName.c_str()};
        
        auto outputTensors = m_faceSwapSession->Run(
            Ort::RunOptions{nullptr}, inputNames, &inputTensor, 1, outputNames, 1);

        // Get output tensor
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();

        // Convert back to cv::Mat
        cv::Mat output(inputSize, inputSize, CV_32FC3);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    output.at<cv::Vec3f>(h, w)[c] = 
                        outputData[c * inputSize * inputSize + h * inputSize + w];
                }
            }
        }

        // Convert back to BGR and 8-bit
        output.convertTo(output, CV_8UC3, 255.0);
        cv::cvtColor(output, output, cv::COLOR_RGB2BGR);
        
        // Resize to original face size
        cv::resize(output, output, sourceFace.size());

        return output;
    }
    catch (const std::exception& e) {
        std::cerr << "Face swap inference failed: " << e.what() << std::endl;
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
