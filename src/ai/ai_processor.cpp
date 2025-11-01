#include "ai_processor.h"
#include "passthrough_processor.h"
#include <iostream>

// AIProcessorFactory implementation
std::unique_ptr<AIProcessor> AIProcessorFactory::CreateProcessor(const std::string& type) {
    if (type == "passthrough") {
        return std::make_unique<PassthroughProcessor>();
    }
    
    // Add more processor types here as they are implemented
    // if (type == "background_replacer") {
    //     return std::make_unique<BackgroundReplacer>();
    // }
    
    return nullptr;
}

std::vector<std::string> AIProcessorFactory::GetAvailableProcessors() {
    return {
        "passthrough"
        // Add more processor names here as they are implemented
    };
}

// AIProcessingPipeline implementation
AIProcessingPipeline::AIProcessingPipeline() : m_initialized(false) {
}

AIProcessingPipeline::~AIProcessingPipeline() {
    Cleanup();
}

void AIProcessingPipeline::AddProcessor(std::unique_ptr<AIProcessor> processor) {
    if (processor) {
        m_processors.push_back(std::move(processor));
    }
}

void AIProcessingPipeline::RemoveProcessor(const std::string& name) {
    m_processors.erase(
        std::remove_if(m_processors.begin(), m_processors.end(),
                      [&name](const std::unique_ptr<AIProcessor>& processor) {
                          return processor->GetName() == name;
                      }),
        m_processors.end()
    );
}

Frame AIProcessingPipeline::ProcessFrame(const Frame& input) {
    if (!m_initialized || !input.IsValid()) {
        return input;
    }
    
    Frame current = input;
    
    // Process through each processor in sequence
    for (auto& processor : m_processors) {
        if (processor) {
            current = processor->ProcessFrame(current);
            if (!current.IsValid()) {
                std::cerr << "Processor " << processor->GetName() 
                         << " returned invalid frame" << std::endl;
                return input; // Return original frame on error
            }
        }
    }
    
    return current;
}

bool AIProcessingPipeline::Initialize() {
    if (m_initialized) {
        return true;
    }
    
    bool allSuccess = true;
    
    for (auto& processor : m_processors) {
        if (processor && !processor->Initialize()) {
            std::cerr << "Failed to initialize processor: " 
                     << processor->GetName() << std::endl;
            allSuccess = false;
        }
    }
    
    m_initialized = allSuccess;
    return allSuccess;
}

void AIProcessingPipeline::Cleanup() {
    for (auto& processor : m_processors) {
        if (processor) {
            processor->Cleanup();
        }
    }
    
    m_processors.clear();
    m_initialized = false;
}

double AIProcessingPipeline::GetTotalProcessingTime() const {
    double totalTime = 0.0;
    
    for (const auto& processor : m_processors) {
        if (processor) {
            totalTime += processor->GetExpectedProcessingTime();
        }
    }
    
    return totalTime;
}