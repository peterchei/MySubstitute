#include <opencv2/opencv.hpp>
#include <iostream>

int main() {
    // Create simple glasses placeholder (semi-transparent cyan rectangle)
    cv::Mat glasses(50, 100, CV_8UC4, cv::Scalar(0, 255, 255, 128)); // Cyan with 50% alpha
    cv::imwrite("assets/glasses.png", glasses);

    // Create simple hat placeholder (semi-transparent magenta rectangle)
    cv::Mat hat(60, 120, CV_8UC4, cv::Scalar(255, 0, 255, 128)); // Magenta with 50% alpha
    cv::imwrite("assets/funny_hat.png", hat);

    std::cout << "Created placeholder accessory images" << std::endl;
    return 0;
}