#pragma once

#include "ai_processor.h"
#include <atomic>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

class CartoonFilterProcessor : public AIProcessor {
public:
    CartoonFilterProcessor();
    ~CartoonFilterProcessor() override;

    bool Initialize() override;
    Frame ProcessFrame(const Frame& input) override;
    void Cleanup() override;

    std::string GetName() const override { return "CartoonFilterProcessor"; }
    std::string GetVersion() const override { return "1.0.0"; }
    bool SupportsRealTime() const override { return true; }

    bool SetParameter(const std::string& name, const std::string& value) override;
    std::map<std::string, std::string> GetParameters() const override;
    double GetExpectedProcessingTime() const override;

    // Cartoon-specific controls
    void SetCartoonStyle(int style);
    void SetEdgeThreshold(int threshold);
    void SetSmoothingLevel(int level);
    void SetColorLevels(int levels);

private:
    enum CartoonStyle {
        SIMPLE = 0,
        DETAILED = 1,
        ANIME = 2
    };

#ifdef HAVE_OPENCV
    // Core cartoon processing
    void ApplyCartoonEffect(cv::Mat& frame);
    void ApplySimpleCartoon(cv::Mat& frame);
    void ApplyDetailedCartoon(cv::Mat& frame);
    void ApplyAnimeStyle(cv::Mat& frame);

    // Helper methods
    cv::Mat DetectEdges(const cv::Mat& src);
    cv::Mat QuantizeColors(const cv::Mat& src, int levels);
    void CombineEdgesWithColors(cv::Mat& frame, const cv::Mat& edges);
    
    // Temporal coherence for stable edges
    cv::Mat m_previousEdges;
    cv::Mat m_previousFrame;
    cv::Mat StabilizeEdges(const cv::Mat& currentEdges);
    cv::Mat m_previousQuantized;  // Store previous quantized frame for smoother transitions
#endif

    // Parameters
    CartoonStyle m_style;
    int m_edgeThreshold;
    int m_smoothingLevel;
    int m_colorLevels;
    bool m_addOutlineOnly;
    bool m_preserveDetails;

    mutable std::atomic<uint64_t> m_frameCounter;
    double m_processingTime;
};
