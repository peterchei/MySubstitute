#pragma once

#include "ai_processor.h"
#include <string>
#include <memory>

#ifdef HAVE_OPENCV
#include <opencv2/dnn.hpp>
#include <opencv2/opencv.hpp>
#endif

/**
 * @brief AnimeGAN-based AI processor for real-time anime style transfer
 * 
 * This processor uses a pre-trained AnimeGAN ONNX model to convert real-world
 * images into anime-style images. It requires GPU support for real-time performance.
 * 
 * Model Requirements:
 * - AnimeGAN ONNX model (e.g., AnimeGANv2, AnimeGANv3)
 * - Model should be placed in: models/anime_gan.onnx
 * - Expected input: RGB image, 256x256 or 512x512 (configurable)
 * - Expected output: RGB image, same size as input
 * 
 * Performance:
 * - GPU (CUDA): ~30-100ms per frame (real-time capable)
 * - CPU: ~500-2000ms per frame (too slow for real-time)
 */
class AnimeGANProcessor : public AIProcessor
{
public:
    AnimeGANProcessor();
    ~AnimeGANProcessor() override;

    bool Initialize() override;
    void Cleanup() override;
    Frame ProcessFrame(const Frame& input) override;

    bool SetParameter(const std::string& name, const std::string& value) override;
    std::map<std::string, std::string> GetParameters() const override;
    double GetExpectedProcessingTime() const override;
    
    // Required AIProcessor interface methods
    std::string GetName() const override;
    std::string GetVersion() const override;
    bool SupportsRealTime() const override;

    // Specific methods
    void SetModelPath(const std::string& path);
    void SetInputSize(int width, int height);
    void SetBlendWeight(float weight);  // 0.0 = original, 1.0 = full anime
    bool IsGPUAvailable() const;

private:
#ifdef HAVE_OPENCV
    // Model inference
    cv::dnn::Net m_net;
    
    // Model configuration
    std::string m_modelPath;
    int m_inputWidth;
    int m_inputHeight;
    float m_blendWeight;  // Blend between original and anime (0.0 to 1.0)
    bool m_gpuAvailable;
    bool m_modelLoaded;
    
    // Temporal stabilization
    cv::Mat m_previousOutput;
    float m_temporalBlendWeight;
    
    // Performance tracking
    double m_processingTime;
    uint64_t m_frameCounter;
    
    // Helper methods
    cv::Mat PreprocessFrame(const cv::Mat& input);
    cv::Mat PostprocessFrame(const cv::Mat& output, const cv::Size& targetSize);
    cv::Mat RunInference(const cv::Mat& input);
    cv::Mat StabilizeOutput(const cv::Mat& current);
    cv::Mat BlendWithOriginal(const cv::Mat& original, const cv::Mat& anime);
    
    // GPU detection
    bool DetectGPUSupport();
#else
    // Dummy members when OpenCV is not available
    std::string m_modelPath;
    int m_inputWidth;
    int m_inputHeight;
    float m_blendWeight;
    bool m_gpuAvailable;
    bool m_modelLoaded;
    double m_processingTime;
    uint64_t m_frameCounter;
#endif
};
