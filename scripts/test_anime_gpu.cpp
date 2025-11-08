// Simple test to verify AnimeGAN GPU acceleration
#include <iostream>
#include <memory>
#include "src/ai/anime_gan_processor.h"

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "  AnimeGAN GPU Acceleration Test" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

#ifdef HAVE_OPENCV
    // Create AnimeGAN processor
    auto processor = std::make_unique<AnimeGANProcessor>();
    
    // Set model path to correct location (relative to build/bin/Debug)
    processor->SetModelPath("../../../models/candy.t7");
    
    std::cout << "\n[1] Initializing AnimeGAN processor..." << std::endl;
    std::cout << "    This will detect GPU and load the model" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    bool success = processor->Initialize();
    
    if (!success) {
        std::cerr << "\n❌ Failed to initialize processor!" << std::endl;
        std::cerr << "   Check if model file exists in models/ directory" << std::endl;
        return 1;
    }
    
    std::cout << "\n✅ Processor initialized successfully!" << std::endl;
    
    // Get and display GPU info
    std::cout << "\n[2] GPU Information:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    std::string gpuInfo = processor->GetGPUInfo();
    std::cout << gpuInfo << std::endl;
    
    // Get all parameters
    std::cout << "\n[3] Processor Parameters:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    auto params = processor->GetParameters();
    for (const auto& param : params) {
        std::cout << "  " << param.first << ": " << param.second << std::endl;
    }
    
    // Create a test frame
    std::cout << "\n[4] Processing test frames..." << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    cv::Mat testImage = cv::Mat::zeros(512, 512, CV_8UC3);
    testImage.setTo(cv::Scalar(100, 150, 200));
    
    // Draw some test content
    cv::circle(testImage, cv::Point(256, 256), 100, cv::Scalar(255, 0, 0), -1);
    cv::putText(testImage, "GPU Test", cv::Point(150, 270), 
                cv::FONT_HERSHEY_SIMPLEX, 1.5, cv::Scalar(255, 255, 255), 2);
    
    Frame testFrame(testImage);
    testFrame.timestamp = 0;
    
    // Process multiple frames to get performance metrics
    std::cout << "\nProcessing 10 frames to measure performance..." << std::endl;
    
    for (int i = 0; i < 10; i++) {
        auto start = std::chrono::high_resolution_clock::now();
        Frame result = processor->ProcessFrame(testFrame);
        auto end = std::chrono::high_resolution_clock::now();
        
        double ms = std::chrono::duration<double, std::milli>(end - start).count();
        double fps = 1000.0 / ms;
        
        std::cout << "  Frame " << (i+1) << ": " << ms << " ms (" << fps << " FPS)" << std::endl;
    }
    
    // Test GPU control methods
    std::cout << "\n[5] Testing GPU Control Methods:" << std::endl;
    std::cout << "----------------------------------------" << std::endl;
    
    if (processor->IsGPUAvailable()) {
        std::cout << "\n✅ GPU is available!" << std::endl;
        
        // Test FP16 toggle
        std::cout << "\nTesting FP16 mode toggle..." << std::endl;
        processor->SetUseFP16(false);
        std::cout << "  Set FP16: false" << std::endl;
        
        processor->SetUseFP16(true);
        std::cout << "  Set FP16: true" << std::endl;
        
        // Test GPU toggle
        std::cout << "\nTesting GPU enable/disable..." << std::endl;
        processor->SetUseGPU(false);
        std::cout << "  GPU disabled (using CPU)" << std::endl;
        
        processor->SetUseGPU(true);
        std::cout << "  GPU re-enabled" << std::endl;
        
    } else {
        std::cout << "\n⚠️  No GPU detected - running on CPU" << std::endl;
        std::cout << "\nTo enable GPU acceleration:" << std::endl;
        std::cout << "  1. Install CUDA Toolkit 11.0+ from NVIDIA" << std::endl;
        std::cout << "  2. Install cuDNN library" << std::endl;
        std::cout << "  3. Rebuild OpenCV with CUDA support" << std::endl;
        std::cout << "  4. Rebuild this project" << std::endl;
    }
    
    std::cout << "\n========================================" << std::endl;
    std::cout << "  Test Complete!" << std::endl;
    std::cout << "========================================" << std::endl;
    
#else
    std::cerr << "❌ OpenCV not available - cannot test AnimeGAN processor" << std::endl;
    return 1;
#endif

    return 0;
}
