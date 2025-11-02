#include <iostream>
#include <memory>
#include "face_filter_processor.h"
#include "capture/frame.h"

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

int main() {
    std::cout << "Testing Face Filter Processor..." << std::endl;

#ifdef HAVE_OPENCV
    // Create processor
    auto processor = std::make_unique<FaceFilterProcessor>();

    // Initialize
    if (!processor->Initialize()) {
        std::cerr << "Failed to initialize Face Filter Processor" << std::endl;
        return 1;
    }

    std::cout << "Processor: " << processor->GetName() << " v" << processor->GetVersion() << std::endl;
    std::cout << "Real-time support: " << (processor->SupportsRealTime() ? "Yes" : "No") << std::endl;
    std::cout << "Expected processing time: " << processor->GetExpectedProcessingTime() << "ms" << std::endl;

    // Test parameter setting
    processor->SetSpeechBubbleText("Testing Face Filters!");
    processor->SetGlassesEnabled(true);
    processor->SetHatEnabled(true);

    // Show current parameters
    auto params = processor->GetParameters();
    std::cout << "\nCurrent parameters:" << std::endl;
    for (const auto& param : params) {
        std::cout << "  " << param.first << ": " << param.second << std::endl;
    }

    // Test with a sample image if available
    cv::Mat testImage = cv::Mat(480, 640, CV_8UC3, cv::Scalar(100, 150, 200)); // Blue background

    // Add a simulated face rectangle (for testing overlay)
    cv::rectangle(testImage, cv::Rect(200, 150, 200, 250), cv::Scalar(255, 200, 150), 2);

    Frame testFrame(testImage);

    std::cout << "\nProcessing test frame..." << std::endl;
    Frame resultFrame = processor->ProcessFrame(testFrame);

    if (!resultFrame.data.empty()) {
        std::cout << "Frame processed successfully!" << std::endl;
        std::cout << "Input size: " << testFrame.data.cols << "x" << testFrame.data.rows << std::endl;
        std::cout << "Output size: " << resultFrame.data.cols << "x" << resultFrame.data.rows << std::endl;
    }

    processor->Cleanup();
    std::cout << "\nFace Filter Processor test completed successfully!" << std::endl;

#else
    std::cout << "OpenCV not available - Face Filter Processor test skipped" << std::endl;
#endif

    return 0;
}