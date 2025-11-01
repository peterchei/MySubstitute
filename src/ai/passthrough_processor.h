#pragma once

#include "ai_processor.h"

/**
 * Simple passthrough processor that doesn't modify the frame
 * Useful for testing and as a template for other processors
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
    
private:
    bool m_addTimestamp;
    bool m_addWatermark;
};