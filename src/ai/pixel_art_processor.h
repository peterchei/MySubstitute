#pragma once

#include "ai_processor.h"
#include <deque>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

/**
 * Pixel Art Processor
 * 
 * Creates anime-style pixel art effect similar to Minecraft aesthetics:
 * - Pixelation/downsampling for blocky appearance
 * - Bold color quantization with vibrant anime colors
 * - Strong edge outlines for anime style
 * - Optional dithering for retro look
 */
class PixelArtProcessor : public AIProcessor {
public:
    enum Style {
        MINECRAFT,      // Blocky 8x8 pixels, vibrant colors
        ANIME_PIXEL,    // 4x4 pixels with anime color palette
        RETRO_16BIT     // 6x6 pixels with dithering
    };

    PixelArtProcessor();
    virtual ~PixelArtProcessor();

    bool Initialize() override;
    void Cleanup() override;
    Frame ProcessFrame(const Frame& input) override;
    std::string GetName() const override { return "Pixel Art Processor"; }
    std::string GetVersion() const override { return "1.0.0"; }
    bool SupportsRealTime() const override { return true; }
    bool SetParameter(const std::string& name, const std::string& value) override;
    std::map<std::string, std::string> GetParameters() const override;
    double GetExpectedProcessingTime() const override { return m_processingTime; }

    // Style configuration
    void SetStyle(Style style) { m_style = style; }
    Style GetStyle() const { return m_style; }

    // Pixel size configuration (smaller = more blocky)
    void SetPixelSize(int size) { 
        if (size >= 2 && size <= 16) {
            m_pixelSize = size; 
        }
    }
    int GetPixelSize() const { return m_pixelSize; }

    // Color levels (fewer = more pixelated/anime style)
    void SetColorLevels(int levels) {
        if (levels >= 3 && levels <= 16) {
            m_colorLevels = levels;
        }
    }
    int GetColorLevels() const { return m_colorLevels; }

    // Enable/disable edge outlines
    void SetEdgeOutlines(bool enable) { m_enableEdges = enable; }
    bool GetEdgeOutlines() const { return m_enableEdges; }

    // Enable/disable dithering
    void SetDithering(bool enable) { m_enableDithering = enable; }
    bool GetDithering() const { return m_enableDithering; }

private:
#ifdef HAVE_OPENCV
    cv::Mat Pixelate(const cv::Mat& src, int pixelSize);
    cv::Mat QuantizeColors(const cv::Mat& src, int colorLevels);
    cv::Mat ApplyAnimePalette(const cv::Mat& src);
    cv::Mat DetectEdges(const cv::Mat& src);
    cv::Mat ApplyEdgeOutlines(cv::Mat& src, const cv::Mat& edges);
    cv::Mat ApplyDithering(const cv::Mat& src);
    void ApplyMinecraftStyle(cv::Mat& frame);
    void ApplyAnimePixelStyle(cv::Mat& frame);
    void ApplyRetro16BitStyle(cv::Mat& frame);
    
    // Temporal stabilization
    cv::Mat StabilizeFrame(const cv::Mat& currentFrame);
    std::deque<cv::Mat> m_frameBuffer;
    cv::Mat m_previousFrame;
#endif

    Style m_style;
    int m_pixelSize;
    int m_colorLevels;
    bool m_enableEdges;
    bool m_enableDithering;
    int m_frameCounter;
    double m_processingTime;
    int m_bufferSize;
    double m_temporalBlendWeight;
};
