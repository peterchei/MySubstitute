#ifndef VIRTUAL_BACKGROUND_PROCESSOR_H
#define VIRTUAL_BACKGROUND_PROCESSOR_H

#include "ai_processor.h"
#include <vector>
#include <string>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
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
        DESKTOP_CAPTURE    // Use Windows desktop as background
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

private:
#ifdef HAVE_OPENCV
    // Background segmentation
    cv::dnn::Net m_segmentationNet;
    bool m_modelLoaded;
    
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
    
    // Cached background (resized to match frame size)
    cv::Mat m_cachedBackground;
    int m_cachedWidth;
    int m_cachedHeight;
    
    // Helper methods
    cv::Mat SegmentPerson(const cv::Mat& frame);
    cv::Mat CreateMask(const cv::Mat& segmentation);
    cv::Mat GetBackgroundFrame(const cv::Mat& frame);
    cv::Mat BlendFrames(const cv::Mat& foreground, const cv::Mat& background, const cv::Mat& mask);
    void CaptureDesktopBackground();
    bool LoadBackgroundImage(const std::string& imagePath);
    cv::Mat ResizeBackgroundToFrame(const cv::Mat& frame);
#endif
};

#endif // VIRTUAL_BACKGROUND_PROCESSOR_H
