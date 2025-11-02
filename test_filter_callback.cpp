#include <iostream>
#include <memory>
#include "ai_processor.h"
#include "passthrough_processor.h"

#ifdef HAVE_OPENCV
#include "face_filter_processor.h"
#endif

// Global processor pointer (simulating main.cpp)
std::unique_ptr<AIProcessor> g_processor;

// Filter change callback (from main.cpp)
void OnFilterChanged(const std::string& filterName) {
    std::cout << "OnFilterChanged called with: '" << filterName << "'" << std::endl;

    if (filterName == "none") {
        // Switch to passthrough processor
        g_processor = std::make_unique<PassthroughProcessor>();
        if (g_processor->Initialize()) {
            std::cout << "Switched to: " << g_processor->GetName() << std::endl;
        } else {
            std::cout << "Failed to initialize PassthroughProcessor" << std::endl;
        }
    } else if (filterName == "face_filter") {
#ifdef HAVE_OPENCV
        // Switch to face filter processor
        g_processor = std::make_unique<FaceFilterProcessor>();
        if (g_processor->Initialize()) {
            std::cout << "Switched to: " << g_processor->GetName() << std::endl;
        } else {
            std::cout << "Failed to initialize FaceFilterProcessor (likely missing cascade files)" << std::endl;
            // Fallback to passthrough
            g_processor = std::make_unique<PassthroughProcessor>();
            if (g_processor->Initialize()) {
                std::cout << "Fallback to: " << g_processor->GetName() << std::endl;
            }
        }
#else
        std::cout << "OpenCV not available, cannot use face filter" << std::endl;
        g_processor = std::make_unique<PassthroughProcessor>();
        if (g_processor->Initialize()) {
            std::cout << "Fallback to: " << g_processor->GetName() << std::endl;
        }
#endif
    } else {
        std::cout << "Unknown filter: " << filterName << std::endl;
    }
}

int main() {
    std::cout << "Testing Filter Change Callback System..." << std::endl;

    // Test switching to passthrough
    std::cout << "\n1. Testing switch to 'none' (passthrough):" << std::endl;
    OnFilterChanged("none");

    // Test switching to face filter
    std::cout << "\n2. Testing switch to 'face_filter':" << std::endl;
    OnFilterChanged("face_filter");

    // Test switching back to passthrough
    std::cout << "\n3. Testing switch back to 'none':" << std::endl;
    OnFilterChanged("none");

    // Test unknown filter
    std::cout << "\n4. Testing unknown filter 'invalid':" << std::endl;
    OnFilterChanged("invalid");

    std::cout << "\nFilter change callback test completed!" << std::endl;
    return 0;
}