#pragma once

#include "ai_processor.h"
#include <string>
#include <memory>
#include <map>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#endif

#ifdef HAVE_ONNX
#include <onnxruntime_cxx_api.h>
#endif

/**
 * Person Replacement Processor
 * 
 * Advanced AI processor for replacing persons in video with:
 * - Full body replacement
 * - Face swapping (DeepSeek/DeepFake-style)
 * - Super-resolution enhancement
 * - Face restoration and enhancement
 * - Style transfer
 * 
 * Uses ONNX Runtime with models like:
 * - SimSwap, FaceSwap for face replacement
 * - Real-ESRGAN for super-resolution
 * - GFPGAN, CodeFormer for face restoration
 */
class PersonReplacementProcessor : public AIProcessor
{
public:
    enum ReplacementMode {
        FACE_SWAP,           // Replace only the face (DeepFake style)
        FULL_BODY_REPLACE,   // Replace entire person
        FACE_ENHANCE,        // Enhance face quality (GFPGAN/CodeFormer)
        SUPER_RESOLUTION,    // Upscale resolution (Real-ESRGAN)
        STYLE_TRANSFER       // Apply artistic style
    };

    PersonReplacementProcessor();
    virtual ~PersonReplacementProcessor();

    // AIProcessor interface
    virtual bool Initialize() override;
    virtual void Cleanup() override;
    virtual Frame ProcessFrame(const Frame& input) override;
    virtual std::string GetName() const override { return "Person Replacement Processor"; }
    virtual std::string GetVersion() const override { return "1.0.0"; }
    virtual bool SupportsRealTime() const override { return false; } // Heavy processing

    // Configuration
    void SetReplacementMode(ReplacementMode mode);
    void SetTargetPersonImage(const std::string& imagePath);
    void SetTargetPersonVideo(const std::string& videoPath);
    void SetBlendStrength(float strength);  // 0.0 to 1.0
    void SetEnableEnhancement(bool enable);
    void SetUseGPU(bool useGPU);

    // Model management
    bool LoadFaceSwapModel(const std::string& modelPath);
    bool LoadFaceEmbeddingModel(const std::string& modelPath);  // ArcFace backbone
    bool LoadSuperResolutionModel(const std::string& modelPath);
    bool LoadFaceEnhancementModel(const std::string& modelPath);
    bool LoadSegmentationModel(const std::string& modelPath);

    // Information
    std::string GetReplacementInfo() const;
    ReplacementMode GetMode() const { return m_mode; }
    float GetBlendStrength() const { return m_blendStrength; }

    // Parameters
    virtual bool SetParameter(const std::string& name, const std::string& value) override;
    virtual std::map<std::string, std::string> GetParameters() const override;
    virtual double GetExpectedProcessingTime() const override { return m_processingTime; }

private:
#ifdef HAVE_OPENCV
    // Core processing methods
    cv::Mat ReplaceFace(const cv::Mat& frame, const cv::Mat& targetImage);
    cv::Mat ReplaceFullBody(const cv::Mat& frame, const cv::Mat& targetPerson);
    cv::Mat EnhanceFaceInFrame(const cv::Mat& frame);
    cv::Mat EnhanceFace(const cv::Mat& face);
    cv::Mat SuperResolve(const cv::Mat& image);
    cv::Mat ApplyStyleTransfer(const cv::Mat& image);

    // Face detection and alignment
    std::vector<cv::Rect> DetectFaces(const cv::Mat& frame);
    cv::Mat AlignFace(const cv::Mat& face, const cv::Rect& faceRect);
    std::vector<cv::Point2f> DetectFaceLandmarks(const cv::Mat& face);

    // Person segmentation
    cv::Mat SegmentPerson(const cv::Mat& frame);

    // Color correction and blending helpers
    cv::Mat MatchColorHistogram(const cv::Mat& source, const cv::Mat& target);
    cv::Mat CreateFeatheredMask(const cv::Size& size);
    cv::Mat AlphaBlendWithMask(const cv::Mat& background, const cv::Mat& foreground, 
                              const cv::Mat& mask, float blendStrength);

    // Face tracking helper
    float CalculateFaceOverlap(const cv::Rect& rect1, const cv::Rect& rect2);

    // Blending and compositing
    cv::Mat SeamlessBlend(const cv::Mat& source, const cv::Mat& target, const cv::Mat& mask);
    cv::Mat PoissonBlend(const cv::Mat& source, const cv::Mat& target, const cv::Point& center);
    
    // Video handling
    cv::Mat GetNextVideoFrame();

    // Model inference
#ifdef HAVE_ONNX
    cv::Mat RunFaceSwapInference(const cv::Mat& sourceFace, const cv::Mat& targetFace);
    cv::Mat RunSuperResolutionInference(const cv::Mat& lowRes);
    cv::Mat RunFaceEnhancementInference(const cv::Mat& face);
    cv::Mat RunSegmentationInference(const cv::Mat& frame);
#endif

    // Target person data
    cv::Mat m_targetPersonImage;
    cv::VideoCapture m_targetPersonVideo;
    cv::Mat m_currentTargetFrame;
    bool m_useVideoTarget;

    // Face detection
    cv::CascadeClassifier m_faceCascade;
    cv::dnn::Net m_faceDetectionNet;  // DNN-based face detection
    bool m_useDNNFaceDetection;

    // ONNX Runtime sessions
#ifdef HAVE_ONNX
    std::unique_ptr<Ort::Env> m_onnxEnv;
    std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
    
    // Multiple models for different tasks
    std::unique_ptr<Ort::Session> m_faceSwapSession;
    std::unique_ptr<Ort::Session> m_faceEmbeddingSession;  // ArcFace for face embeddings
    std::unique_ptr<Ort::Session> m_superResSession;
    std::unique_ptr<Ort::Session> m_faceEnhanceSession;
    std::unique_ptr<Ort::Session> m_segmentationSession;
    
    std::string m_faceSwapInputName;
    std::string m_faceSwapOutputName;
    std::string m_superResInputName;
    std::string m_superResOutputName;
    std::string m_enhanceInputName;
    std::string m_enhanceOutputName;
    
    bool m_faceSwapLoaded;
    bool m_faceEmbeddingLoaded;  // ArcFace embedding model
    bool m_superResLoaded;
    bool m_faceEnhanceLoaded;
    bool m_segmentationLoaded;
#endif

    // Configuration
    ReplacementMode m_mode;
    float m_blendStrength;
    bool m_enableEnhancement;
    bool m_useGPU;
    std::string m_backend;

    // Performance tracking
    double m_processingTime;
    int m_frameCounter;

    // Face tracking for stability (prevent blinking)
    std::vector<cv::Rect> m_previousFaces;
    int m_framesWithoutDetection;
    const int MAX_FRAMES_WITHOUT_DETECTION = 5;
    const float FACE_OVERLAP_THRESHOLD = 0.5f;

    // Parameters storage
    std::map<std::string, std::string> m_parameters;

    // OpenCV DNN models (fallback)
    cv::dnn::Net m_segmentationNet;
    bool m_modelLoaded;
#endif
};
