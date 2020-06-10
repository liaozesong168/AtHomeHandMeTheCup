#include <cv_bridge/cv_bridge.h>
#include <image_transport/image_transport.h>
#include <opencv2/opencv.hpp>
#include <ros/ros.h>

using namespace cv;

int main(int argc, char *argv[]) {
  ros::init(argc, argv, "hand_camera");
  ros::NodeHandle nh;
  ros::Rate loop_rate(10);
  image_transport::ImageTransport it(nh);
  image_transport::Publisher pub =
      it.advertise("/camera_hand/rgb/image_raw", 1);
  VideoCapture cap;
  cap.open(0); //打开摄像头

  if (!cap.isOpened())
    return 1;
  Mat frame;
  Mat result;
  // 640,480
  while (1) {
    Mat result(640, 480, CV_8UC3);
    cap >> frame;

    if (frame.empty())
      break;

    for (int i = 0; i < 480; i++) {
      for (int j = 0; j < 640; j++) {
        cv::Vec3b color = frame.at<cv::Vec3b>(i, j); // BGR
        result.at<cv::Vec3b>(j, 479 - i) = color;
      }
    }
    sensor_msgs::ImagePtr msg =
        cv_bridge::CvImage(std_msgs::Header(), "bgr8", result).toImageMsg();
    pub.publish(msg);
    cv::imshow("handcamera", result);
    if (waitKey(20) > 0)
      break;
  }
  cap.release();
  destroyAllWindows();
  return 0;
}