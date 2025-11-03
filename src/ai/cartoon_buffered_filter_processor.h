#pragma once

#include "ai_processor.h"
#include <atomic>
#include <deque>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

class CartoonBufferedFilterProcessor : public AIProcessor {
public:
    CartoonBufferedFilterProcessor();
    ~CartoonBufferedFilterProcessor() override;

    bool Initialize() override;
    Frame ProcessFrame(const Frame& input) override;
    void Cleanup() override;

    std::string GetName() const override { return "CartoonBufferedFilterProcessor"; }
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
    void SetBufferSize(int size);  // Default 10 frames

private:
    enum CartoonStyle {
        SIMPLE = 0,
        DETAILED = 1,
        ANIME = 2
    };

#ifdef HAVE_OPENCV
    // Core cartoon processing
    void ApplyCartoonEffect(cv::Mat& frame);
    void ApplyBufferedCartoon(cv::Mat& outputFrame);

    // Helper methods
    cv::Mat DetectEdges(const cv::Mat& src);
    cv::Mat QuantizeColors(const cv::Mat& src, int levels);
    void CombineEdgesWithColors(cv::Mat& frame, const cv::Mat& edges);
    
    // Buffered processing
    cv::Mat ComputeTemporalMedianEdges();
    cv::Mat ComputeTemporalMedianColors();
    void AddFrameToBuffer(const cv::Mat& frame);
    
    // Frame buffers
    std::deque<cv::Mat> m_frameBuffer;         // Raw input frames
    std::deque<cv::Mat> m_edgeBuffer;          // Detected edges
    std::deque<cv::Mat> m_quantizedBuffer;     // Quantized colors
    int m_bufferSize;                          // Number of frames to buffer
#endif

    // Parameters
    CartoonStyle m_style;
    int m_edgeThreshold;
    int m_smoothingLevel;
    int m_colorLevels;

    mutable std::atomic<uint64_t> m_frameCounter;
    double m_processingTime;
};
