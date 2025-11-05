#ifndef PERSON_TRACKER_PROCESSOR_H
#define PERSON_TRACKER_PROCESSOR_H

#include "ai_processor.h"
#include <vector>
#include <deque>
#include <string>

#ifdef HAVE_OPENCV
#include <opencv2/opencv.hpp>
#include <opencv2/dnn.hpp>
#endif

// Person detection and motion tracking processor
class PersonTrackerProcessor : public AIProcessor {
public:
    PersonTrackerProcessor();
    virtual ~PersonTrackerProcessor();

    // AIProcessor interface
    virtual bool Initialize() override;
    virtual void Cleanup() override;
    virtual Frame ProcessFrame(const Frame& input) override;
    virtual std::string GetName() const override;
    virtual std::string GetVersion() const override;
    virtual bool SupportsRealTime() const override;
    virtual bool SetParameter(const std::string& name, const std::string& value) override;
    virtual std::map<std::string, std::string> GetParameters() const override;
    virtual double GetExpectedProcessingTime() const override;

    // Configuration
    void SetConfidenceThreshold(float threshold);
    void SetTrailLength(int length);
    void SetShowBoundingBox(bool show);
    void SetShowTrail(bool show);
    void SetShowSkeleton(bool show);

private:
#ifdef HAVE_OPENCV
    struct DetectedPerson {
        cv::Rect bbox;           // Bounding box
        cv::Point center;        // Center point
        float confidence;        // Detection confidence
        int trackId;            // Tracking ID
        cv::Scalar color;       // Color for this person
    };

    struct TrackPoint {
        cv::Point position;
        int64_t timestamp;
        int trackId;
    };

    // Detection
    cv::dnn::Net m_net;
    std::string m_modelPath;
    std::string m_configPath;
    bool m_modelLoaded;
    float m_confidenceThreshold;
    cv::Size m_inputSize;

    // Tracking
    std::vector<DetectedPerson> m_currentPersons;
    std::vector<DetectedPerson> m_previousPersons;
    std::deque<TrackPoint> m_motionTrail;
    int m_nextTrackId;
    int m_maxTrailLength;

    // Visualization settings
    bool m_showBoundingBox;
    bool m_showTrail;
    bool m_showSkeleton;

    // Performance
    double m_processingTime;
    int m_frameCounter;

    // Helper methods
    std::vector<DetectedPerson> DetectPersons(const cv::Mat& frame);
    void TrackPersons(std::vector<DetectedPerson>& persons);
    void UpdateMotionTrail();
    void DrawVisualization(cv::Mat& frame);
    void DrawBoundingBoxes(cv::Mat& frame);
    void DrawMotionTrail(cv::Mat& frame);
    void DrawSkeleton(cv::Mat& frame, const DetectedPerson& person);
    cv::Scalar GetTrackColor(int trackId);
    float CalculateIoU(const cv::Rect& box1, const cv::Rect& box2);
#endif
};

#endif // PERSON_TRACKER_PROCESSOR_H
