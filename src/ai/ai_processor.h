#pragma once

#include "capture/frame.h"
#include <memory>
#include <string>
#include <map>

/**
 * Base class for AI processing modules
 */
class AIProcessor {
public:
    virtual ~AIProcessor() = default;
    
    /**
     * Initialize the AI processor
     * @return true if successful
     */
    virtual bool Initialize() = 0;
    
    /**
     * Process a single frame
     * @param input Input frame to process
     * @return Processed frame
     */
    virtual Frame ProcessFrame(const Frame& input) = 0;
    
    /**
     * Cleanup resources
     */
    virtual void Cleanup() = 0;
    
    /**
     * Get processor name
     */
    virtual std::string GetName() const = 0;
    
    /**
     * Get processor version
     */
    virtual std::string GetVersion() const = 0;
    
    /**
     * Check if processor supports real-time processing
     */
    virtual bool SupportsRealTime() const = 0;
    
    /**
     * Set processing parameters
     */
    virtual bool SetParameter(const std::string& name, const std::string& value) = 0;
    
    /**
     * Get processing parameters
     */
    virtual std::map<std::string, std::string> GetParameters() const = 0;
    
    /**
     * Get expected processing time per frame (milliseconds)
     */
    virtual double GetExpectedProcessingTime() const = 0;
    
protected:
    bool m_initialized = false;
    std::map<std::string, std::string> m_parameters;
};

/**
 * Factory for creating AI processors
 */
class AIProcessorFactory {
public:
    static std::unique_ptr<AIProcessor> CreateProcessor(const std::string& type);
    static std::vector<std::string> GetAvailableProcessors();
};

/**
 * AI processing pipeline that can chain multiple processors
 */
class AIProcessingPipeline {
public:
    AIProcessingPipeline();
    ~AIProcessingPipeline();
    
    /**
     * Add processor to the pipeline
     */
    void AddProcessor(std::unique_ptr<AIProcessor> processor);
    
    /**
     * Remove processor from pipeline
     */
    void RemoveProcessor(const std::string& name);
    
    /**
     * Process frame through all processors in sequence
     */
    Frame ProcessFrame(const Frame& input);
    
    /**
     * Initialize all processors
     */
    bool Initialize();
    
    /**
     * Cleanup all processors
     */
    void Cleanup();
    
    /**
     * Get total expected processing time
     */
    double GetTotalProcessingTime() const;
    
private:
    std::vector<std::unique_ptr<AIProcessor>> m_processors;
    bool m_initialized;
};