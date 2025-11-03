#include "pixel_art_processor.h"
#include <iostream>
#include <chrono>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#endif

PixelArtProcessor::PixelArtProcessor()
    : m_style(MINECRAFT),
      m_pixelSize(8),
      m_colorLevels(6),
      m_enableEdges(true),
      m_enableDithering(false),
      m_frameCounter(0),
      m_processingTime(0.0),
      m_bufferSize(3),  // Small buffer for stability
      m_temporalBlendWeight(0.7)  // 70% current, 30% previous
{
    std::cout << "[PixelArtProcessor] Initializing with temporal stabilization" << std::endl;
}

PixelArtProcessor::~PixelArtProcessor()
{
    Cleanup();
}

bool PixelArtProcessor::Initialize()
{
    std::cout << "[PixelArtProcessor] Initialize called" << std::endl;
    std::cout << "[PixelArtProcessor] Style: " << static_cast<int>(m_style) << std::endl;
    std::cout << "[PixelArtProcessor] Pixel Size: " << m_pixelSize << std::endl;
    std::cout << "[PixelArtProcessor] Color Levels: " << m_colorLevels << std::endl;
    return true;
}

void PixelArtProcessor::Cleanup()
{
    std::cout << "[PixelArtProcessor] Cleanup called" << std::endl;
#ifdef HAVE_OPENCV
    m_frameBuffer.clear();
    m_previousFrame.release();
#endif
}

bool PixelArtProcessor::SetParameter(const std::string& name, const std::string& value)
{
    if (name == "pixel_size") {
        try {
            int size = std::stoi(value);
            SetPixelSize(size);
            return true;
        } catch (...) {
            return false;
        }
    } else if (name == "color_levels") {
        try {
            int levels = std::stoi(value);
            SetColorLevels(levels);
            return true;
        } catch (...) {
            return false;
        }
    } else if (name == "style") {
        if (value == "minecraft" || value == "0") {
            SetStyle(MINECRAFT);
            return true;
        } else if (value == "anime_pixel" || value == "1") {
            SetStyle(ANIME_PIXEL);
            return true;
        } else if (value == "retro_16bit" || value == "2") {
            SetStyle(RETRO_16BIT);
            return true;
        }
        return false;
    } else if (name == "edge_outlines") {
        SetEdgeOutlines(value == "true" || value == "1");
        return true;
    } else if (name == "dithering") {
        SetDithering(value == "true" || value == "1");
        return true;
    }
    return false;
}

std::map<std::string, std::string> PixelArtProcessor::GetParameters() const
{
    std::map<std::string, std::string> params;
    params["pixel_size"] = std::to_string(m_pixelSize);
    params["color_levels"] = std::to_string(m_colorLevels);
    params["style"] = std::to_string(static_cast<int>(m_style));
    params["edge_outlines"] = m_enableEdges ? "true" : "false";
    params["dithering"] = m_enableDithering ? "true" : "false";
    return params;
}

Frame PixelArtProcessor::ProcessFrame(const Frame& input)
{
    auto startTime = std::chrono::high_resolution_clock::now();

    Frame output = input;

#ifdef HAVE_OPENCV
    if (!input.data.empty()) {
        cv::Mat workingFrame = input.data.clone();
        
        // Apply style-specific processing
        switch (m_style) {
            case MINECRAFT:
                ApplyMinecraftStyle(workingFrame);
                break;
            case ANIME_PIXEL:
                ApplyAnimePixelStyle(workingFrame);
                break;
            case RETRO_16BIT:
                ApplyRetro16BitStyle(workingFrame);
                break;
        }
        
        workingFrame.copyTo(output.data);
    }
#endif

    auto endTime = std::chrono::high_resolution_clock::now();
    m_processingTime = std::chrono::duration<double, std::milli>(endTime - startTime).count();

    m_frameCounter++;
    if (m_frameCounter % 30 == 0) {
        std::cout << "[PixelArtProcessor] Frame " << m_frameCounter 
                  << " processed in " << m_processingTime << "ms" << std::endl;
    }

    return output;
}

#ifdef HAVE_OPENCV

void PixelArtProcessor::ApplyMinecraftStyle(cv::Mat& frame)
{
    // Minecraft style: Large 8x8 blocks, vibrant colors, strong edges
    
    // Step 1: Enhance saturation for vibrant Minecraft-like colors
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    channels[1] = channels[1] * 1.4; // Boost saturation
    cv::merge(channels, hsv);
    cv::cvtColor(hsv, frame, cv::COLOR_HSV2BGR);
    
    // Step 2: Pixelate to create blocky appearance
    frame = Pixelate(frame, m_pixelSize);
    
    // Step 3: Quantize colors to fewer levels (blocky color palette)
    frame = QuantizeColors(frame, m_colorLevels);
    
    // Step 4: Add strong black edge outlines
    if (m_enableEdges) {
        cv::Mat edges = DetectEdges(frame);
        frame = ApplyEdgeOutlines(frame, edges);
    }
    
    // Step 5: Apply temporal stabilization to reduce blinking
    frame = StabilizeFrame(frame);
}

void PixelArtProcessor::ApplyAnimePixelStyle(cv::Mat& frame)
{
    // Anime pixel style: Smaller 4x4 pixels, anime color palette, soft edges
    
    // Step 1: Apply anime-inspired color palette
    frame = ApplyAnimePalette(frame);
    
    // Step 2: Pixelate with smaller blocks for detail
    frame = Pixelate(frame, 4);
    
    // Step 3: Quantize to anime-style color levels
    frame = QuantizeColors(frame, 8);
    
    // Step 4: Add subtle edge outlines
    if (m_enableEdges) {
        cv::Mat edges = DetectEdges(frame);
        frame = ApplyEdgeOutlines(frame, edges);
    }
    
    // Step 5: Apply temporal stabilization to reduce blinking
    frame = StabilizeFrame(frame);
}

void PixelArtProcessor::ApplyRetro16BitStyle(cv::Mat& frame)
{
    // Retro 16-bit style: Medium pixels, dithering, limited colors
    
    // Step 1: Pixelate to medium block size
    frame = Pixelate(frame, 6);
    
    // Step 2: Reduce to limited color palette (16-bit era)
    frame = QuantizeColors(frame, 5);
    
    // Step 3: Apply dithering for retro look
    if (m_enableDithering) {
        frame = ApplyDithering(frame);
    }
    
    // Step 4: Light edge outlines
    if (m_enableEdges) {
        cv::Mat edges = DetectEdges(frame);
        // Lighter edge application for retro look
        cv::Mat softEdges;
        edges.convertTo(softEdges, -1, 0.6, 0);
        frame = ApplyEdgeOutlines(frame, softEdges);
    }
    
    // Step 5: Apply temporal stabilization to reduce blinking
    frame = StabilizeFrame(frame);
}

cv::Mat PixelArtProcessor::Pixelate(const cv::Mat& src, int pixelSize)
{
    if (src.empty() || pixelSize < 1) {
        return src.clone();
    }
    
    // Calculate downsampled size
    int newWidth = std::max(1, src.cols / pixelSize);
    int newHeight = std::max(1, src.rows / pixelSize);
    
    // Downsample
    cv::Mat small;
    cv::resize(src, small, cv::Size(newWidth, newHeight), 0, 0, cv::INTER_LINEAR);
    
    // Upsample back to original size with nearest neighbor for blocky effect
    cv::Mat pixelated;
    cv::resize(small, pixelated, src.size(), 0, 0, cv::INTER_NEAREST);
    
    return pixelated;
}

cv::Mat PixelArtProcessor::QuantizeColors(const cv::Mat& src, int colorLevels)
{
    if (src.empty() || colorLevels < 2) {
        return src.clone();
    }
    
    cv::Mat quantized = src.clone();
    
    // Calculate quantization step
    int step = 256 / colorLevels;
    
    // Quantize each pixel
    for (int y = 0; y < quantized.rows; ++y) {
        cv::Vec3b* row = quantized.ptr<cv::Vec3b>(y);
        for (int x = 0; x < quantized.cols; ++x) {
            for (int c = 0; c < 3; ++c) {
                int val = row[x][c];
                row[x][c] = (val / step) * step + step / 2;
            }
        }
    }
    
    return quantized;
}

cv::Mat PixelArtProcessor::ApplyAnimePalette(const cv::Mat& src)
{
    if (src.empty()) {
        return src.clone();
    }
    
    // Enhance colors to anime-style vibrant palette
    cv::Mat anime = src.clone();
    
    // Convert to HSV for color manipulation
    cv::Mat hsv;
    cv::cvtColor(anime, hsv, cv::COLOR_BGR2HSV);
    
    std::vector<cv::Mat> channels;
    cv::split(hsv, channels);
    
    // Boost saturation and adjust brightness for anime look
    channels[1] = channels[1] * 1.3; // Saturation boost
    channels[2] = channels[2] * 1.1; // Slight brightness boost
    
    // Clamp values
    cv::threshold(channels[1], channels[1], 255, 255, cv::THRESH_TRUNC);
    cv::threshold(channels[2], channels[2], 255, 255, cv::THRESH_TRUNC);
    
    cv::merge(channels, hsv);
    cv::cvtColor(hsv, anime, cv::COLOR_HSV2BGR);
    
    return anime;
}

cv::Mat PixelArtProcessor::DetectEdges(const cv::Mat& src)
{
    if (src.empty()) {
        return cv::Mat();
    }
    
    cv::Mat gray, edges;
    
    // Convert to grayscale
    if (src.channels() == 3) {
        cv::cvtColor(src, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = src.clone();
    }
    
    // Use Canny edge detection for crisp pixel art edges
    cv::Canny(gray, edges, 50, 150);
    
    // Dilate slightly to make edges more visible
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(2, 2));
    cv::dilate(edges, edges, kernel);
    
    return edges;
}

cv::Mat PixelArtProcessor::ApplyEdgeOutlines(cv::Mat& src, const cv::Mat& edges)
{
    if (src.empty() || edges.empty()) {
        return src.clone();
    }
    
    cv::Mat result = src.clone();
    
    // Apply black outlines where edges are detected
    for (int y = 0; y < result.rows; ++y) {
        cv::Vec3b* row = result.ptr<cv::Vec3b>(y);
        const uint8_t* edgeRow = edges.ptr<uint8_t>(y);
        
        for (int x = 0; x < result.cols; ++x) {
            if (edgeRow[x] > 0) {
                // Draw black outline
                row[x] = cv::Vec3b(0, 0, 0);
            }
        }
    }
    
    return result;
}

cv::Mat PixelArtProcessor::ApplyDithering(const cv::Mat& src)
{
    if (src.empty()) {
        return src.clone();
    }
    
    cv::Mat dithered = src.clone();
    
    // Floyd-Steinberg dithering for retro look
    for (int y = 0; y < dithered.rows - 1; ++y) {
        for (int x = 1; x < dithered.cols - 1; ++x) {
            for (int c = 0; c < 3; ++c) {
                int oldPixel = dithered.at<cv::Vec3b>(y, x)[c];
                int newPixel = (oldPixel > 128) ? 255 : 0;
                dithered.at<cv::Vec3b>(y, x)[c] = newPixel;
                
                int error = oldPixel - newPixel;
                
                // Distribute error to neighboring pixels
                dithered.at<cv::Vec3b>(y, x + 1)[c] = 
                    cv::saturate_cast<uint8_t>(dithered.at<cv::Vec3b>(y, x + 1)[c] + error * 7 / 16);
                dithered.at<cv::Vec3b>(y + 1, x - 1)[c] = 
                    cv::saturate_cast<uint8_t>(dithered.at<cv::Vec3b>(y + 1, x - 1)[c] + error * 3 / 16);
                dithered.at<cv::Vec3b>(y + 1, x)[c] = 
                    cv::saturate_cast<uint8_t>(dithered.at<cv::Vec3b>(y + 1, x)[c] + error * 5 / 16);
                dithered.at<cv::Vec3b>(y + 1, x + 1)[c] = 
                    cv::saturate_cast<uint8_t>(dithered.at<cv::Vec3b>(y + 1, x + 1)[c] + error * 1 / 16);
            }
        }
    }
    
    return dithered;
}

cv::Mat PixelArtProcessor::StabilizeFrame(const cv::Mat& currentFrame)
{
    if (currentFrame.empty()) {
        return currentFrame.clone();
    }
    
    // First frame - no stabilization needed
    if (m_previousFrame.empty() || 
        m_previousFrame.size() != currentFrame.size() || 
        m_previousFrame.type() != currentFrame.type()) {
        m_previousFrame = currentFrame.clone();
        return currentFrame.clone();
    }
    
    // Blend current frame with previous frame for temporal stability
    cv::Mat stabilized;
    cv::addWeighted(currentFrame, m_temporalBlendWeight, 
                    m_previousFrame, 1.0 - m_temporalBlendWeight, 
                    0, stabilized);
    
    // Update previous frame for next iteration
    m_previousFrame = stabilized.clone();
    
    return stabilized;
}

#endif // HAVE_OPENCV
