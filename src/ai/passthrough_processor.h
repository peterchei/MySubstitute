#pragma once

#include "ai_processor.h"

/**
 * Passthrough processor with configurable overlay features
 * Passes video through with optional caption, timestamp, and watermark overlays
 */
class PassthroughProcessor : public AIProcessor {
public:
    PassthroughProcessor();
    virtual ~PassthroughProcessor();
    
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
    
    // Caption-specific methods
    void SetCaptionText(const std::string& text);
    void SetCaptionEnabled(bool enabled);
    void SetCaptionPosition(int x, int y);  // Position from bottom-left
    
private:
    // Overlay rendering methods
    void AddTimestamp(cv::Mat& frame);
    void AddWatermark(cv::Mat& frame);
    void AddCaption(cv::Mat& frame);
    
    // Configuration
    bool m_addTimestamp;
    bool m_addWatermark;
    bool m_addCaption;
    
    // Caption properties
    std::string m_captionText;
    int m_captionX;
    int m_captionY;
    cv::Scalar m_captionColor;
    double m_captionScale;
    int m_captionThickness;
};