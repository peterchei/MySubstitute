#ifndef VIRTUAL_BACKGROUND_PROCESSOR_H
#define VIRTUAL_BACKGROUND_PROCESSOR_H

#include "ai_processor.h"
#include <vector>
#include <string>
#include <deque>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#endif

// ONNX Runtime support for better segmentation models
#ifdef HAVE_ONNX
#include <onnxruntime_cxx_api.h>
#endif

/**
 * Virtual Background Processor
 * Removes foreground (person) from video and replaces background with custom image or desktop
 */
class VirtualBackgroundProcessor : public AIProcessor {
public:
    enum BackgroundMode {
        BLUR,              // Blur the background
        SOLID_COLOR,       // Replace with solid color
        CUSTOM_IMAGE,      // Use custom image
        DESKTOP_CAPTURE,   // Use Windows desktop as background
        MINECRAFT_PIXEL    // Minecraft-style pixelated background
    };

    enum SegmentationMethod {
        METHOD_MOTION,           // Motion detection + face detection (fallback)
        METHOD_ONNX_SELFIE,     // Google MediaPipe Selfie Segmentation (best)
        METHOD_OPENCV_DNN        // OpenCV DNN models (DeepLab, etc.)
    };

    VirtualBackgroundProcessor();
    virtual ~VirtualBackgroundProcessor();

    // AIProcessor interface
    virtual bool Initialize() override;
    virtual void Cleanup() override;
    virtual Frame ProcessFrame(const Frame& input) override;
    virtual std::string GetName() const override;
    virtual std::string GetVersion() const override;
    virtual bool SupportsRealTime() const override;
    virtual bool SetParameter(const std::string& name, const std::string& value) override;
    virtual std::map<std::string, std::string> GetParameters() const override;
    virtual double GetExpectedProcessingTime() const override;

    // Configuration methods
    void SetBackgroundMode(BackgroundMode mode);
    void SetBackgroundImage(const std::string& imagePath);
    void SetBlurStrength(int kernelSize);  // 1-100, applies to BLUR mode
#ifdef HAVE_OPENCV
    void SetSolidColor(cv::Scalar color);   // For SOLID_COLOR mode
#endif
    void SetSegmentationThreshold(float threshold);
    void SetBlendAlpha(float alpha);  // 0.0-1.0, for edge smoothing
    void SetSegmentationMethod(SegmentationMethod method);
    void SetUseGPU(bool useGPU);  // Enable GPU acceleration
    bool LoadSegmentationModel(const std::string& modelPath);
    std::string GetSegmentationInfo() const;  // Get current method and performance info

private:
#ifdef HAVE_OPENCV
    // Background segmentation
    cv::dnn::Net m_segmentationNet;
    bool m_modelLoaded;
    SegmentationMethod m_segmentationMethod;
    std::string m_modelPath;
    bool m_useGPU;
    std::string m_backend;
    
#ifdef HAVE_ONNX
    // ONNX Runtime for better models
    std::unique_ptr<Ort::Env> m_onnxEnv;
    std::unique_ptr<Ort::Session> m_onnxSession;
    std::unique_ptr<Ort::SessionOptions> m_sessionOptions;
    std::string m_onnxInputName;
    std::string m_onnxOutputName;
#endif
    
    // Background data
    cv::Mat m_backgroundImage;
    cv::Scalar m_solidColor;
    BackgroundMode m_backgroundMode;
    
    // Processing parameters
    float m_segmentationThreshold;
    float m_blendAlpha;
    int m_blurStrength;
    
    // Performance tracking
    double m_processingTime;
    int m_frameCounter;
    
    // Background subtraction for better segmentation
    cv::Ptr<cv::BackgroundSubtractor> m_bgSubtractor;
    cv::Mat m_previousFrame;
    cv::Mat m_previousMask;  // Store previous mask for temporal smoothing
    bool m_bgSubtractorInitialized;
    int m_stableFrameCount;  // Count frames with stable detection
    
    // Temporal consistency for stable masks
    std::deque<cv::Mat> m_maskHistory;
    static const int MAX_MASK_HISTORY = 5;
    cv::Mat m_temporalMask;
    
    // Edge refinement
    bool m_useGuidedFilter;
    
    // Cached background (resized to match frame size)
    cv::Mat m_cachedBackground;
    int m_cachedWidth;
    int m_cachedHeight;
    
    // Helper methods - Main segmentation
    cv::Mat SegmentPerson(const cv::Mat& frame);
    cv::Mat DetectPersonUsingMotionAndFace(const cv::Mat& frame);
    
    // Improved segmentation methods
    cv::Mat SegmentPersonWithONNX(const cv::Mat& frame);
    cv::Mat SegmentPersonWithOpenCVDNN(const cv::Mat& frame);
    bool LoadSegmentationModelONNX(const std::string& modelPath);
    bool LoadSegmentationModelOpenCVDNN(const std::string& modelPath);
    
    // Post-processing for better quality
    cv::Mat PostProcessMask(const cv::Mat& rawMask, const cv::Mat& frame);
    void TemporalSmoothing(cv::Mat& mask);
    void EdgeRefinement(cv::Mat& mask, const cv::Mat& frame);
    
    // Existing helper methods
    cv::Mat CreateMask(const cv::Mat& segmentation);
    cv::Mat GetBackgroundFrame(const cv::Mat& frame);
    cv::Mat BlendFrames(const cv::Mat& foreground, const cv::Mat& background, const cv::Mat& mask);
    void CaptureDesktopBackground();
    bool LoadBackgroundImage(const std::string& imagePath);
    cv::Mat ResizeBackgroundToFrame(const cv::Mat& frame);
    cv::Mat CreateMinecraftPixelBackground(const cv::Mat& frame);
#endif
};

#endif // VIRTUAL_BACKGROUND_PROCESSOR_H
