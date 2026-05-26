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
    // GPU on by default — Initialize() falls back to CPU cleanly if no provider is available.
    // Mirrors VirtualBackgroundProcessor's default so face-swap doesn't silently stay on CPU
    // when the user's machine has CUDA/DirectML.
    , m_useGPU(true)
    , m_backend("CPU")
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

    // Initialize ONNX Runtime — mirrors VirtualBackgroundProcessor::LoadSegmentationModelONNX
    // (src/ai/virtual_background_processor.cpp:667-728) so the GPU fallback chain stays
    // consistent across processors.
#ifdef HAVE_ONNX
    try {
        m_onnxEnv = std::make_unique<Ort::Env>(ORT_LOGGING_LEVEL_WARNING, "PersonReplacementProcessor");
        m_sessionOptions = std::make_unique<Ort::SessionOptions>();

        if (m_useGPU) {
            try {
                #ifdef USE_CUDA
                OrtCUDAProviderOptions cuda_options;
                m_sessionOptions->AppendExecutionProvider_CUDA(cuda_options);
                m_backend = "CUDA";
                std::cout << "[PersonReplacement] GPU acceleration enabled (CUDA)" << std::endl;
                #elif defined(USE_DIRECTML)
                m_sessionOptions->AppendExecutionProvider_DML(0);
                m_backend = "DirectML";
                std::cout << "[PersonReplacement] GPU acceleration enabled (DirectML)" << std::endl;
                #else
                m_backend = "CPU";
                std::cout << "[PersonReplacement] GPU requested but no provider compiled in, using CPU" << std::endl;
                #endif
            } catch (...) {
                m_backend = "CPU";
                std::cout << "[PersonReplacement] GPU initialization failed, using CPU" << std::endl;
            }
        } else {
            m_backend = "CPU";
        }

        m_sessionOptions->SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_ALL);
        m_sessionOptions->SetIntraOpNumThreads(4);

        std::cout << "[PersonReplacement] ONNX Runtime initialized (backend=" << m_backend << ")" << std::endl;
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
        std::cout << "   For better quality, place simswap.onnx in models/ folder" << std::endl;
    }

    // Auto-load CodeFormer for face quality enhancement after swap. Soft-fail.
    std::vector<std::string> enhancePaths = {
        "models/codeformer.onnx",
        "../../../models/codeformer.onnx"
    };
    for (const auto& p : enhancePaths) {
        if (LoadFaceEnhancementModel(p)) {
            m_enableEnhancement = true;
            std::cout << "✅ Auto-loaded face enhancement: " << p << std::endl;
            break;
        }
    }
    if (!m_faceEnhanceLoaded) {
        std::cout << "ℹ️ codeformer.onnx not found — swapped face will skip post-enhancement" << std::endl;
    }

    // Auto-load MediaPipe selfie segmentation for FULL_BODY_REPLACE.
    // Same file VirtualBackgroundProcessor uses; soft-fail keeps face-swap usable without it.
    std::vector<std::string> segPaths = {
        "models/MediaPipe-Selfie-Segmentation.onnx",
        "../../../models/MediaPipe-Selfie-Segmentation.onnx"
    };
    for (const auto& p : segPaths) {
        if (LoadSegmentationModel(p)) {
            std::cout << "✅ Auto-loaded segmentation model: " << p << std::endl;
            break;
        }
    }
    if (!m_segmentationLoaded) {
        std::cout << "ℹ️ MediaPipe-Selfie-Segmentation.onnx not found — full-body replace will use fallback" << std::endl;
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
    m_targetEmbedding.clear();
    m_targetEmbeddingReady = false;

    if (m_targetPersonImage.empty()) {
        std::cerr << "Failed to load target person image: " << imagePath << std::endl;
        return;
    }

    std::cout << "Target person image loaded: " << imagePath
              << " (" << m_targetPersonImage.cols << "x" << m_targetPersonImage.rows << ")" << std::endl;
    m_useVideoTarget = false;

#ifdef HAVE_ONNX
    // Cache the ArcFace embedding of the target face NOW so we don't redo this every frame.
    // The target image is static — this is the single biggest perf win over the old design
    // and is also what makes the swap direction correct: this embedding becomes the
    // "source_embedding" (identity to inject) at swap time, while the per-frame webcam crop
    // becomes the "target" (canvas to paint onto).
    if (!m_faceEmbeddingLoaded) {
        std::cerr << "[Target] ArcFace model not loaded — embedding cannot be cached. "
                     "Face swap will not work until simswap_arcface_backbone.onnx is present." << std::endl;
        return;
    }

    // Save and restore the per-frame face-tracking state so the one-shot detection on
    // the target photo doesn't bleed into webcam tracking.
    auto savedPrevFaces = m_previousFaces;
    auto savedFramesWithoutDet = m_framesWithoutDetection;
    m_previousFaces.clear();
    m_framesWithoutDetection = 0;

    std::vector<cv::Rect> targetFaces = DetectFaces(m_targetPersonImage);

    m_previousFaces = savedPrevFaces;
    m_framesWithoutDetection = savedFramesWithoutDet;

    if (targetFaces.empty()) {
        std::cerr << "[Target] No face detected in target image — use a clear frontal photo." << std::endl;
        return;
    }

    const cv::Rect& tFace = targetFaces[0];
    if (!IsValidROI(tFace, m_targetPersonImage)) {
        std::cerr << "[Target] Detected face ROI invalid, skipping embedding." << std::endl;
        return;
    }

    cv::Mat targetFaceCrop = m_targetPersonImage(tFace);
    m_targetEmbedding = ExtractFaceEmbedding(targetFaceCrop);
    if (m_targetEmbedding.size() == 512) {
        m_targetEmbeddingReady = true;
        std::cout << "[Target] ✅ ArcFace embedding cached (512-D) from face at "
                  << tFace.x << "," << tFace.y << " " << tFace.width << "x" << tFace.height << std::endl;
    } else {
        std::cerr << "[Target] Embedding extraction returned size=" << m_targetEmbedding.size()
                  << ", expected 512." << std::endl;
        m_targetEmbedding.clear();
    }
#endif // HAVE_ONNX
#endif // HAVE_OPENCV
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

        // SimSwap has two inputs: target image (4D) + source embedding (2D).
        // Introspect both — the order in the ONNX graph is not guaranteed across exports.
        Ort::AllocatorWithDefaultOptions allocator;
        size_t numInputs = m_faceSwapSession->GetInputCount();
        m_faceSwapInputName0 = m_faceSwapSession->GetInputNameAllocated(0, allocator).get();
        if (numInputs > 1) {
            m_faceSwapInputName1 = m_faceSwapSession->GetInputNameAllocated(1, allocator).get();
        } else {
            m_faceSwapInputName1.clear();
        }
        m_faceSwapOutputName = m_faceSwapSession->GetOutputNameAllocated(0, allocator).get();

        // Log the input shapes so we can verify which one is the embedding vs the image.
        std::cout << "Face swap model loaded: " << modelPath << std::endl;
        for (size_t i = 0; i < numInputs; ++i) {
            auto info = m_faceSwapSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
            auto shape = info.GetShape();
            std::cout << "  Input[" << i << "] name=" << (i == 0 ? m_faceSwapInputName0 : m_faceSwapInputName1)
                      << " shape=[";
            for (size_t d = 0; d < shape.size(); ++d) {
                std::cout << shape[d] << (d + 1 < shape.size() ? "," : "");
            }
            std::cout << "]" << std::endl;
        }
        std::cout << "  Output: " << m_faceSwapOutputName << std::endl;

        m_faceSwapLoaded = true;
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

        // Introspect input/output names — different ArcFace exports use different conventions
        // (input/output, input.1/683, data/fc1, etc.). Don't guess.
        Ort::AllocatorWithDefaultOptions allocator;
        m_arcfaceInputName = m_faceEmbeddingSession->GetInputNameAllocated(0, allocator).get();
        m_arcfaceOutputName = m_faceEmbeddingSession->GetOutputNameAllocated(0, allocator).get();

        m_faceEmbeddingLoaded = true;
        std::cout << "Face embedding model (ArcFace) loaded: " << modelPath << std::endl;
        std::cout << "  Input: " << m_arcfaceInputName << ", Output: " << m_arcfaceOutputName << std::endl;
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

cv::Mat PersonReplacementProcessor::ReplaceFace(const cv::Mat& frame, const cv::Mat& /*targetImage*/)
{
    cv::Mat result = frame.clone();

    // The target identity is cached as an ArcFace embedding inside SetTargetPersonImage.
    // We only detect faces in the WEBCAM frame now — target detection is one-shot at load.
    std::vector<cv::Rect> sourceFaces = DetectFaces(frame);

    if (sourceFaces.empty()) {
        static int framesSinceLastWarning = 0;
        if (framesSinceLastWarning == 0) {
            std::cout << "[Face Swap] No faces detected. Try: face camera directly, better lighting, remove glasses/mask" << std::endl;
        }
        framesSinceLastWarning = (framesSinceLastWarning + 1) % 60;

        cv::putText(result, "No face detected - face camera directly", cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
        cv::putText(result, "Try: better lighting, remove glasses/mask", cv::Point(10, 50),
                   cv::FONT_HERSHEY_SIMPLEX, 0.4, cv::Scalar(0, 255, 255), 1, cv::LINE_AA);
        return result;
    }

#ifdef HAVE_ONNX
    if (!m_targetEmbeddingReady) {
        static bool warned = false;
        if (!warned) {
            std::cerr << "[Face Swap] Target embedding not cached — re-load the target image." << std::endl;
            warned = true;
        }
        cv::putText(result, "Target identity not loaded - check assets/default_face.jpg", cv::Point(10, 30),
                   cv::FONT_HERSHEY_SIMPLEX, 0.5, cv::Scalar(0, 0, 255), 1, cv::LINE_AA);
        return result;
    }
#endif

    for (const cv::Rect& sourceFaceRect : sourceFaces) {
        // Expand to give the model context (forehead/chin/ears) and let the seamless clone
        // feather smoothly into the surrounding skin.
        int expandX = static_cast<int>(sourceFaceRect.width * 0.2);
        int expandY = static_cast<int>(sourceFaceRect.height * 0.3);

        cv::Rect expandedSourceRect = sourceFaceRect;
        expandedSourceRect.x = std::max(0, sourceFaceRect.x - expandX);
        expandedSourceRect.y = std::max(0, sourceFaceRect.y - expandY);
        expandedSourceRect.width = std::min(frame.cols - expandedSourceRect.x,
                                            sourceFaceRect.width + 2 * expandX);
        expandedSourceRect.height = std::min(frame.rows - expandedSourceRect.y,
                                             sourceFaceRect.height + 2 * expandY);

        if (!IsValidROI(expandedSourceRect, frame)) {
            std::cerr << "[Face Swap] Invalid expanded rect, skipping face" << std::endl;
            continue;
        }

        cv::Mat sourceFace = frame(expandedSourceRect);

        // The webcam crop IS the canvas the AI paints onto. The cached target embedding
        // provides the identity. Output: identity of the photo, expression/pose of the webcam.
        cv::Mat swappedFace;
#ifdef HAVE_ONNX
        if (m_faceSwapLoaded) {
            swappedFace = RunFaceSwapInference(sourceFace);
        }
#endif

        // If AI was unavailable or failed, fall back to a soft alpha blend onto the webcam
        // (keeps the camera frame intact rather than blanking the face).
        if (swappedFace.empty()) {
            swappedFace = sourceFace.clone();
        }

#ifdef HAVE_ONNX
        // Optional post-step: CodeFormer face quality enhancement. Only runs if the model
        // was auto-loaded in Initialize().
        if (m_enableEnhancement && m_faceEnhanceLoaded) {
            cv::Mat enhanced = RunFaceEnhancementInference(swappedFace);
            if (!enhanced.empty()) {
                swappedFace = enhanced;
            }
        }
#endif

        // Single-strategy blend: seamlessClone(MIXED_CLONE) handles lighting transfer via
        // gradient mixing. Doing histogram matching FIRST and then seamlessClone would
        // double-normalize and wash out the result — we picked one.
        cv::Mat mask = CreateFeatheredMask(swappedFace.size());
        cv::Mat blended;
        try {
            cv::Point center(expandedSourceRect.width / 2, expandedSourceRect.height / 2);
            cv::Mat mask8bit;
            mask.convertTo(mask8bit, CV_8U, 255.0);
            cv::seamlessClone(swappedFace, sourceFace, mask8bit, center, blended, cv::MIXED_CLONE);
        }
        catch (const cv::Exception& e) {
            // Fallback when seamlessClone trips on a degenerate mask/ROI.
            blended = AlphaBlendWithMask(sourceFace, swappedFace, mask, m_blendStrength);
        }

        blended.copyTo(result(expandedSourceRect));
    }

    return result;
}

cv::Mat PersonReplacementProcessor::ReplaceFullBody(const cv::Mat& frame, const cv::Mat& targetPerson)
{
    // Segment the person in the frame (CV_32FC1 in [0, 1]).
    cv::Mat mask = SegmentPerson(frame);
    if (mask.empty()) {
        std::cerr << "Person segmentation failed" << std::endl;
        return frame.clone();
    }

    cv::Mat resizedTarget;
    cv::resize(targetPerson, resizedTarget, frame.size());

    // Vectorized blend: result = frame * (1 - alpha) + target * alpha, where alpha = mask * strength.
    // The old per-pixel loop dropped FPS into single digits at 640x480; OpenCV ops handle this in
    // ~1-2ms per frame.
    cv::Mat alpha1;
    if (mask.type() != CV_32FC1) {
        mask.convertTo(alpha1, CV_32FC1, 1.0 / 255.0);
    } else {
        alpha1 = mask;
    }
    alpha1 = alpha1 * m_blendStrength;

    // Replicate single-channel alpha to 3 channels so we can multiply per-pixel with BGR frames.
    cv::Mat alpha3;
    cv::Mat planes[3] = {alpha1, alpha1, alpha1};
    cv::merge(planes, 3, alpha3);

    cv::Mat frameF, targetF;
    frame.convertTo(frameF, CV_32FC3);
    resizedTarget.convertTo(targetF, CV_32FC3);

    cv::Mat ones3 = cv::Mat::ones(alpha3.size(), CV_32FC3);
    cv::Mat blendedF = frameF.mul(ones3 - alpha3) + targetF.mul(alpha3);

    cv::Mat result;
    blendedF.convertTo(result, CV_8UC3);
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
        // CRITICAL: Validate face rect is within bounds
        if (!IsValidROI(faceRect, frame)) {
            std::cerr << "[Face Enhance] Invalid face rect, skipping" << std::endl;
            continue;
        }
        
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

    // Detect faces using Haar cascade - OPTIMIZED FOR MEETING VIDEO
    // scaleFactor: 1.08 (more sensitive - better detection)
    // minNeighbors: 4 (lower threshold - easier to detect)
    // minSize: 40x40 (smaller minimum - detect faces at various distances)
    // maxSize: frame.cols (allow larger detections for close-up)
    std::vector<cv::Rect> detectedFaces;
    cv::Size maxSize(frame.cols, frame.rows);
    m_faceCascade.detectMultiScale(gray, detectedFaces, 1.08, 4, 0, cv::Size(40, 40), maxSize);

    // Filter faces: Accept faces in CENTER 95% of frame (meeting videos typically centered)
    int centerMarginX = frame.cols * 0.025;  // Only 2.5% margin each side
    int centerMarginY = frame.rows * 0.025;  // Only 2.5% margin top/bottom
    cv::Rect centerRegion(centerMarginX, centerMarginY, 
                          frame.cols - 2 * centerMarginX, 
                          frame.rows - 2 * centerMarginY);

    for (const auto& face : detectedFaces) {
        // Calculate center of detected face
        cv::Point faceCenter(face.x + face.width / 2, face.y + face.height / 2);
        
        // Accept faces in the very wide central region (meeting scenario)
        if (centerRegion.contains(faceCenter)) {
            // Relaxed aspect ratio check (0.6 to 1.5 - handles slight angles)
            float aspectRatio = static_cast<float>(face.width) / face.height;
            if (aspectRatio > 0.6f && aspectRatio < 1.5f) {
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
                        
                        // CRITICAL: Clamp to frame boundaries to prevent ROI errors
                        bestMatch.x = std::max(0, std::min(bestMatch.x, frame.cols - bestMatch.width));
                        bestMatch.y = std::max(0, std::min(bestMatch.y, frame.rows - bestMatch.height));
                        bestMatch.width = std::min(bestMatch.width, frame.cols - bestMatch.x);
                        bestMatch.height = std::min(bestMatch.height, frame.rows - bestMatch.y);
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

// Helper function to validate ROI is within image bounds
bool PersonReplacementProcessor::IsValidROI(const cv::Rect& rect, const cv::Mat& image) const
{
    return rect.x >= 0 && rect.y >= 0 && 
           rect.width > 0 && rect.height > 0 &&
           rect.x + rect.width <= image.cols &&
           rect.y + rect.height <= image.rows;
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

// Extract a 512-D ArcFace embedding from a face crop. Called once on the target photo
// (cached in m_targetEmbedding) and never per-frame.
std::vector<float> PersonReplacementProcessor::ExtractFaceEmbedding(const cv::Mat& face)
{
    std::vector<float> embedding;
    if (!m_faceEmbeddingLoaded || face.empty()) {
        return embedding;
    }

    try {
        // ArcFace: 112x112 RGB, normalized to [-1, 1].
        cv::Mat arcInput;
        cv::resize(face, arcInput, cv::Size(112, 112), 0, 0, cv::INTER_CUBIC);
        cv::cvtColor(arcInput, arcInput, cv::COLOR_BGR2RGB);
        arcInput.convertTo(arcInput, CV_32FC3, 1.0 / 127.5, -1.0);

        std::vector<int64_t> shape = {1, 3, 112, 112};
        std::vector<float> values(1 * 3 * 112 * 112);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < 112; ++h) {
                for (int w = 0; w < 112; ++w) {
                    values[c * 112 * 112 + h * 112 + w] = arcInput.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value tensor = Ort::Value::CreateTensor<float>(
            memInfo, values.data(), values.size(), shape.data(), shape.size());

        const char* inputs[] = {m_arcfaceInputName.c_str()};
        const char* outputs[] = {m_arcfaceOutputName.c_str()};

        auto out = m_faceEmbeddingSession->Run(
            Ort::RunOptions{nullptr},
            inputs, &tensor, 1,
            outputs, 1);

        float* data = out[0].GetTensorMutableData<float>();
        auto outShape = out[0].GetTensorTypeAndShapeInfo().GetShape();
        size_t total = 1;
        for (auto d : outShape) total *= static_cast<size_t>(d);
        embedding.assign(data, data + total);

        // L2-normalize (SimSwap expects unit-length identity vectors).
        double norm = 0.0;
        for (float v : embedding) norm += v * v;
        norm = std::sqrt(norm);
        if (norm > 1e-6) {
            for (float& v : embedding) v = static_cast<float>(v / norm);
        }
    }
    catch (const std::exception& e) {
        std::cerr << "[ExtractFaceEmbedding] ArcFace inference failed: " << e.what() << std::endl;
        embedding.clear();
    }
    return embedding;
}

// SimSwap: paint the cached target identity (m_targetEmbedding) onto the live webcam face.
// Inputs (introspected at load time):
//   - target image (4D NCHW float32, normalized [-1, 1]) = the WEBCAM face crop. This is
//     the canvas being modified; its pose/expression/lighting is preserved.
//   - source embedding (2D [1,512] float32) = the cached identity from SetTargetPersonImage.
// Output: NCHW float32 swapped face, denormalized to BGR uint8.
cv::Mat PersonReplacementProcessor::RunFaceSwapInference(const cv::Mat& webcamFace)
{
    if (!m_faceSwapLoaded || !m_targetEmbeddingReady) {
        return cv::Mat();
    }

    try {
        const int inputSize = 224;

        // Webcam face → 224x224 RGB, [-1, 1], CHW.
        cv::Mat preprocessed;
        cv::resize(webcamFace, preprocessed, cv::Size(inputSize, inputSize), 0, 0, cv::INTER_CUBIC);
        cv::cvtColor(preprocessed, preprocessed, cv::COLOR_BGR2RGB);
        preprocessed.convertTo(preprocessed, CV_32FC3, 2.0 / 255.0, -1.0);

        std::vector<int64_t> targetShape = {1, 3, inputSize, inputSize};
        size_t targetTensorSize = 1 * 3 * inputSize * inputSize;
        std::vector<float> targetValues(targetTensorSize);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < inputSize; ++h) {
                for (int w = 0; w < inputSize; ++w) {
                    targetValues[c * inputSize * inputSize + h * inputSize + w] =
                        preprocessed.at<cv::Vec3f>(h, w)[c];
                }
            }
        }

        // Use the cached embedding directly — copy into a mutable buffer because
        // Ort::Value::CreateTensor takes a non-const pointer.
        std::vector<float> embeddingBuf = m_targetEmbedding;
        std::vector<int64_t> embeddingShape = {1, 512};

        Ort::MemoryInfo memInfo = Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault);
        Ort::Value targetTensor = Ort::Value::CreateTensor<float>(
            memInfo, targetValues.data(), targetTensorSize, targetShape.data(), targetShape.size());
        Ort::Value embeddingTensor = Ort::Value::CreateTensor<float>(
            memInfo, embeddingBuf.data(), embeddingBuf.size(),
            embeddingShape.data(), embeddingShape.size());

        // Match each input by introspected SHAPE: 4D = image (= target/canvas),
        // 2D = embedding (= source identity). Don't assume positional order.
        size_t numInputs = m_faceSwapSession->GetInputCount();
        std::vector<Ort::Value> inputTensors;
        std::vector<const char*> inputNames;
        inputTensors.reserve(numInputs);
        inputNames.reserve(numInputs);

        for (size_t i = 0; i < numInputs; ++i) {
            auto info = m_faceSwapSession->GetInputTypeInfo(i).GetTensorTypeAndShapeInfo();
            auto shp = info.GetShape();
            const std::string& name = (i == 0) ? m_faceSwapInputName0 : m_faceSwapInputName1;
            if (shp.size() == 4) {
                inputTensors.push_back(std::move(targetTensor));
            } else if (shp.size() == 2) {
                inputTensors.push_back(std::move(embeddingTensor));
            } else {
                std::cerr << "[FaceSwap] Unexpected input rank " << shp.size()
                          << " on input '" << name << "' — aborting swap" << std::endl;
                return cv::Mat();
            }
            inputNames.push_back(name.c_str());
        }

        const char* outputNames[] = {m_faceSwapOutputName.c_str()};

        auto outputTensors = m_faceSwapSession->Run(
            Ort::RunOptions{nullptr},
            inputNames.data(), inputTensors.data(), inputTensors.size(),
            outputNames, 1);

        // Output: NCHW float32, range [-1, 1], RGB.
        float* outputData = outputTensors[0].GetTensorMutableData<float>();
        auto outputShape = outputTensors[0].GetTensorTypeAndShapeInfo().GetShape();
        int outputHeight = static_cast<int>(outputShape[2]);
        int outputWidth = static_cast<int>(outputShape[3]);

        cv::Mat output(outputHeight, outputWidth, CV_32FC3);
        for (int c = 0; c < 3; ++c) {
            for (int h = 0; h < outputHeight; ++h) {
                for (int w = 0; w < outputWidth; ++w) {
                    float v = (outputData[c * outputHeight * outputWidth + h * outputWidth + w] + 1.0f) / 2.0f;
                    output.at<cv::Vec3f>(h, w)[c] = std::max(0.0f, std::min(1.0f, v));
                }
            }
        }

        output.convertTo(output, CV_8UC3, 255.0);
        cv::cvtColor(output, output, cv::COLOR_RGB2BGR);
        cv::resize(output, output, webcamFace.size(), 0, 0, cv::INTER_CUBIC);

        // Throttled log — every 30 frames.
        static int swapLogCounter = 0;
        if ((swapLogCounter++ % 30) == 0) {
            std::cout << "✅ AI face swap successful (simswap.onnx, backend=" << m_backend << ")" << std::endl;
        }
        return output;
    }
    catch (const std::exception& e) {
        std::cerr << "❌ Face swap inference failed: " << e.what() << std::endl;
        std::cerr << "   Falling back to OpenCV alpha blend" << std::endl;
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
