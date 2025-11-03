#include "anime_gan_processor.h"
#include <iostream>
#include <chrono>
#include <filesystem>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#endif

AnimeGANProcessor::AnimeGANProcessor()
    : m_modelPath("models/candy.t7"),  // Changed to .t7 format
      m_inputWidth(512),
      m_inputHeight(512),
      m_blendWeight(0.85f),
      m_gpuAvailable(false),
      m_modelLoaded(false),
      m_temporalBlendWeight(0.7f),
      m_processingTime(0.0),
      m_frameCounter(0)
{
    std::cout << "[AnimeGANProcessor] Initializing with Fast Neural Style..." << std::endl;
}

AnimeGANProcessor::~AnimeGANProcessor()
{
    Cleanup();
}

bool AnimeGANProcessor::Initialize()
{
    std::cout << "[AnimeGANProcessor] Initialize called" << std::endl;
    
#ifdef HAVE_OPENCV
    // Check if model file exists
    if (!std::filesystem::exists(m_modelPath)) {
        std::cerr << "[AnimeGANProcessor] ERROR: Model file not found: " << m_modelPath << std::endl;
        std::cerr << "[AnimeGANProcessor] Please download Fast Neural Style .t7 model and place it in models/ folder" << std::endl;
        std::cerr << "[AnimeGANProcessor] Available models: candy.t7, mosaic.t7, starry_night.t7, etc." << std::endl;
        std::cerr << "[AnimeGANProcessor] Download from: https://cs.stanford.edu/people/jcjohns/fast-neural-style/" << std::endl;
        return false;
    }
    
    // Detect GPU support
    m_gpuAvailable = DetectGPUSupport();
    
    try {
        // Load the Torch model (.t7 format)
        std::cout << "[AnimeGANProcessor] Loading Fast Neural Style model from: " << m_modelPath << std::endl;
        m_net = cv::dnn::readNetFromTorch(m_modelPath);  // Changed from readNetFromONNX
        
        if (m_net.empty()) {
            std::cerr << "[AnimeGANProcessor] ERROR: Failed to load model" << std::endl;
            return false;
        }
        
        // Set backend and target
        if (m_gpuAvailable) {
            std::cout << "[AnimeGANProcessor] Using CUDA backend for GPU acceleration" << std::endl;
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_CUDA);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CUDA);
        } else {
            std::cout << "[AnimeGANProcessor] Using CPU backend" << std::endl;
            m_net.setPreferableBackend(cv::dnn::DNN_BACKEND_OPENCV);
            m_net.setPreferableTarget(cv::dnn::DNN_TARGET_CPU);
        }
        
        m_modelLoaded = true;
        std::cout << "[AnimeGANProcessor] Fast Neural Style model loaded successfully" << std::endl;
        std::cout << "[AnimeGANProcessor] Input size: " << m_inputWidth << "x" << m_inputHeight << std::endl;
        std::cout << "[AnimeGANProcessor] Blend weight: " << m_blendWeight << std::endl;
        std::cout << "[AnimeGANProcessor] Temporal blend: " << m_temporalBlendWeight << std::endl;
        
        return true;
        
    } catch (const cv::Exception& e) {
        std::cerr << "[AnimeGANProcessor] OpenCV exception during initialization: " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        std::cerr << "[AnimeGANProcessor] Exception during initialization: " << e.what() << std::endl;
        return false;
    }
#else
    std::cerr << "[AnimeGANProcessor] ERROR: OpenCV not available" << std::endl;
    return false;
#endif
}

void AnimeGANProcessor::Cleanup()
{
    std::cout << "[AnimeGANProcessor] Cleanup called" << std::endl;
    
#ifdef HAVE_OPENCV
    m_previousOutput.release();
    m_modelLoaded = false;
    
    if (!m_net.empty()) {
        // Note: cv::dnn::Net doesn't have explicit cleanup, handled by destructor
        std::cout << "[AnimeGANProcessor] Model released" << std::endl;
    }
#endif
}

Frame AnimeGANProcessor::ProcessFrame(const Frame& input)
{
    auto startTime = std::chrono::high_resolution_clock::now();
    
    Frame output = input;
    
#ifdef HAVE_OPENCV
    if (!m_modelLoaded) {
        std::cerr << "[AnimeGANProcessor] Model not loaded, returning original frame" << std::endl;
        return output;
    }
    
    if (input.data.empty()) {
        return output;
    }
    
    try {
        // Run inference
        cv::Mat animeResult = RunInference(input.data);
        
        if (animeResult.empty()) {
            std::cerr << "[AnimeGANProcessor] Inference failed, returning original frame" << std::endl;
            return output;
        }
        
        // Blend with original if blend weight < 1.0
        if (m_blendWeight < 1.0f) {
            animeResult = BlendWithOriginal(input.data, animeResult);
        }
        
        // Apply temporal stabilization
        animeResult = StabilizeOutput(animeResult);
        
        // Copy to output
        animeResult.copyTo(output.data);
        
    } catch (const cv::Exception& e) {
        std::cerr << "[AnimeGANProcessor] OpenCV exception: " << e.what() << std::endl;
        return input;  // Return original on error
    } catch (const std::exception& e) {
        std::cerr << "[AnimeGANProcessor] Exception: " << e.what() << std::endl;
        return input;
    }
#endif
    
    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();
    
    m_frameCounter++;
    
    // Log performance periodically
    if (m_frameCounter % 100 == 0) {
        std::cout << "[AnimeGANProcessor] Frame " << m_frameCounter 
                  << " | Processing time: " << m_processingTime << "ms" << std::endl;
    }
    
    return output;
}

#ifdef HAVE_OPENCV

bool AnimeGANProcessor::DetectGPUSupport()
{
    try {
        // Check if CUDA is available in OpenCV DNN
        std::vector<cv::dnn::Target> availableTargets = cv::dnn::getAvailableTargets(cv::dnn::DNN_BACKEND_CUDA);
        
        for (const auto& target : availableTargets) {
            if (target == cv::dnn::DNN_TARGET_CUDA || target == cv::dnn::DNN_TARGET_CUDA_FP16) {
                std::cout << "[AnimeGANProcessor] CUDA support detected" << std::endl;
                return true;
            }
        }
        
        std::cout << "[AnimeGANProcessor] CUDA support not detected" << std::endl;
        return false;
        
    } catch (const cv::Exception& e) {
        std::cerr << "[AnimeGANProcessor] Error detecting GPU: " << e.what() << std::endl;
        return false;
    }
}

cv::Mat AnimeGANProcessor::PreprocessFrame(const cv::Mat& input)
{
    if (input.empty()) {
        return cv::Mat();
    }
    
    cv::Mat processed;
    
    // Resize to model input size
    cv::resize(input, processed, cv::Size(m_inputWidth, m_inputHeight), 0, 0, cv::INTER_LINEAR);
    
    // Convert BGR to RGB (most models expect RGB)
    cv::cvtColor(processed, processed, cv::COLOR_BGR2RGB);
    
    // Normalize to [0, 1] or [-1, 1] depending on model
    // Most AnimeGAN models expect [-1, 1]
    processed.convertTo(processed, CV_32F, 1.0 / 127.5, -1.0);
    
    return processed;
}

cv::Mat AnimeGANProcessor::PostprocessFrame(const cv::Mat& output, const cv::Size& targetSize)
{
    if (output.empty()) {
        return cv::Mat();
    }
    
    cv::Mat processed = output.clone();
    
    // Denormalize from [-1, 1] to [0, 255]
    processed = (processed + 1.0) * 127.5;
    
    // Clamp values
    cv::threshold(processed, processed, 255.0, 255.0, cv::THRESH_TRUNC);
    cv::threshold(processed, processed, 0.0, 0.0, cv::THRESH_TOZERO);
    
    // Convert to 8-bit
    processed.convertTo(processed, CV_8UC3);
    
    // Convert RGB back to BGR
    cv::cvtColor(processed, processed, cv::COLOR_RGB2BGR);
    
    // Resize to target size
    if (processed.size() != targetSize) {
        cv::resize(processed, processed, targetSize, 0, 0, cv::INTER_LINEAR);
    }
    
    return processed;
}

cv::Mat AnimeGANProcessor::RunInference(const cv::Mat& input)
{
    if (input.empty() || !m_modelLoaded) {
        return cv::Mat();
    }
    
    // Store original size
    cv::Size originalSize = input.size();
    
    // Preprocess
    cv::Mat preprocessed = PreprocessFrame(input);
    if (preprocessed.empty()) {
        return cv::Mat();
    }
    
    // Create blob from image
    cv::Mat blob = cv::dnn::blobFromImage(preprocessed, 1.0, preprocessed.size(), 
                                          cv::Scalar(), false, false, CV_32F);
    
    // Set input
    m_net.setInput(blob);
    
    // Run forward pass
    cv::Mat output = m_net.forward();
    
    // Extract image from blob (assuming output is [1, C, H, W])
    std::vector<cv::Mat> channels;
    cv::Mat outputImage;
    
    if (output.dims == 4) {
        // Reshape from [1, C, H, W] to [H, W, C]
        int height = output.size[2];
        int width = output.size[3];
        int numChannels = output.size[1];
        
        // Extract each channel
        for (int c = 0; c < numChannels; ++c) {
            cv::Mat channel(height, width, CV_32F, output.ptr<float>(0, c));
            channels.push_back(channel.clone());
        }
        
        // Merge channels
        cv::merge(channels, outputImage);
    } else {
        outputImage = output;
    }
    
    // Postprocess
    cv::Mat result = PostprocessFrame(outputImage, originalSize);
    
    return result;
}

cv::Mat AnimeGANProcessor::StabilizeOutput(const cv::Mat& current)
{
    if (current.empty()) {
        return current.clone();
    }
    
    // First frame - no stabilization
    if (m_previousOutput.empty() || current.size() != m_previousOutput.size()) {
        m_previousOutput = current.clone();
        return current.clone();
    }
    
    // Blend with previous frame for temporal stability
    cv::Mat stabilized;
    cv::addWeighted(current, m_temporalBlendWeight, 
                    m_previousOutput, 1.0f - m_temporalBlendWeight, 
                    0, stabilized);
    
    // Store for next frame
    stabilized.copyTo(m_previousOutput);
    
    return stabilized;
}

cv::Mat AnimeGANProcessor::BlendWithOriginal(const cv::Mat& original, const cv::Mat& anime)
{
    if (original.empty() || anime.empty()) {
        return anime.clone();
    }
    
    if (original.size() != anime.size()) {
        return anime.clone();
    }
    
    cv::Mat blended;
    cv::addWeighted(anime, m_blendWeight, original, 1.0f - m_blendWeight, 0, blended);
    
    return blended;
}

#endif  // HAVE_OPENCV

bool AnimeGANProcessor::SetParameter(const std::string& name, const std::string& value)
{
    try {
        if (name == "model_path") {
            SetModelPath(value);
            return true;
        } else if (name == "input_width") {
            int width = std::stoi(value);
            m_inputWidth = std::max(128, std::min(1024, width));
            return true;
        } else if (name == "input_height") {
            int height = std::stoi(value);
            m_inputHeight = std::max(128, std::min(1024, height));
            return true;
        } else if (name == "blend_weight") {
            float weight = std::stof(value);
            SetBlendWeight(weight);
            return true;
        } else if (name == "temporal_blend") {
            float weight = std::stof(value);
            m_temporalBlendWeight = std::max(0.0f, std::min(1.0f, weight));
            return true;
        }
    } catch (const std::exception& e) {
        std::cerr << "[AnimeGANProcessor] SetParameter error: " << e.what() << std::endl;
        return false;
    }
    return false;
}

std::map<std::string, std::string> AnimeGANProcessor::GetParameters() const
{
    std::map<std::string, std::string> params;
    params["model_path"] = m_modelPath;
    params["input_width"] = std::to_string(m_inputWidth);
    params["input_height"] = std::to_string(m_inputHeight);
    params["blend_weight"] = std::to_string(m_blendWeight);
    params["temporal_blend"] = std::to_string(m_temporalBlendWeight);
    params["gpu_available"] = m_gpuAvailable ? "true" : "false";
    params["model_loaded"] = m_modelLoaded ? "true" : "false";
    return params;
}

double AnimeGANProcessor::GetExpectedProcessingTime() const
{
    return m_processingTime;
}

void AnimeGANProcessor::SetModelPath(const std::string& path)
{
    m_modelPath = path;
    std::cout << "[AnimeGANProcessor] Model path set to: " << m_modelPath << std::endl;
}

void AnimeGANProcessor::SetInputSize(int width, int height)
{
    m_inputWidth = std::max(128, std::min(1024, width));
    m_inputHeight = std::max(128, std::min(1024, height));
    std::cout << "[AnimeGANProcessor] Input size set to: " << m_inputWidth << "x" << m_inputHeight << std::endl;
}

void AnimeGANProcessor::SetBlendWeight(float weight)
{
    m_blendWeight = std::max(0.0f, std::min(1.0f, weight));
    std::cout << "[AnimeGANProcessor] Blend weight set to: " << m_blendWeight << std::endl;
}

bool AnimeGANProcessor::IsGPUAvailable() const
{
    return m_gpuAvailable;
}

std::string AnimeGANProcessor::GetName() const
{
    return "AnimeGAN AI Processor";
}

std::string AnimeGANProcessor::GetVersion() const
{
    return "1.0.0";
}

bool AnimeGANProcessor::SupportsRealTime() const
{
    // Only supports real-time with GPU
    return m_gpuAvailable && m_modelLoaded;
}
