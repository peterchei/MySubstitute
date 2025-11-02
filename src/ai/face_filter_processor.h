#pragma once

#include "ai_processor.h"
#include <atomic>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

/**
 * Face Filter Processor - adds funny virtual accessories to detected faces
 * Perfect for video calls and meetings!
 */
class FaceFilterProcessor : public AIProcessor {
public:
    FaceFilterProcessor();
    virtual ~FaceFilterProcessor();

    // AIProcessor interface
    bool Initialize() override;
    Frame ProcessFrame(const Frame& input) override;
    void Cleanup() override;
    std::string GetName() const override;
    std::string GetVersion() const override;
    bool SupportsRealTime() const override;
    bool SetParameter(const std::string& name, const std::string& value) override;
    std::map<std::string, std::string> GetParameters() const override;
    double GetExpectedProcessingTime() const override;

    // Face filter specific methods
    void SetGlassesEnabled(bool enabled);
    void SetHatEnabled(bool enabled);
    void SetSpeechBubbleEnabled(bool enabled);
    void SetSpeechBubbleText(const std::string& text);

private:
#ifdef HAVE_OPENCV
    // Face detection and overlay methods
    void DetectFaces(const cv::Mat& frame, std::vector<cv::Rect>& faces);
    void AddVirtualGlasses(cv::Mat& frame, const cv::Rect& face);
    void AddFunnyHat(cv::Mat& frame, const cv::Rect& face);
    void AddSpeechBubble(cv::Mat& frame, const cv::Rect& face, const std::string& text);

    // Helper methods
    cv::Mat LoadAccessoryImage(const std::string& filename);
    void OverlayImage(cv::Mat& background, const cv::Mat& overlay, const cv::Point& position);

    // Face detection cascade
    cv::CascadeClassifier faceCascade;

    // Accessory images
    cv::Mat glassesImage;
    cv::Mat hatImage;

    // Configuration
    bool m_glassesEnabled;
    bool m_hatEnabled;
    bool m_speechBubbleEnabled;
    std::string m_speechBubbleText;

    // Performance tracking
    std::atomic<uint64_t> m_frameCounter;
#endif
};