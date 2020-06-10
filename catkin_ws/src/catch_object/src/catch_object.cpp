#include "ros/ros.h"
#include "std_msgs/Float64.h"
#include "std_msgs/Float64MultiArray.h"
#include "std_msgs/Int64.h"
#include "std_msgs/String.h"
#include <cv_bridge/cv_bridge.h>
#include <fstream>
#include <image_transport/image_transport.h>
#include <opencv2/highgui/highgui.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include <sensor_msgs/image_encodings.h>
#include <thread>
#include <vector>

struct status {
  double jd;
  int cnt;
};

static const std::string OPENCV_WINDOW = "Catch Object";
bool isRunning = false;
ros::Publisher arm_xz_publisher; //机械臂旋转
ros::Publisher arm_publisher;
ros::Publisher hand_publisher;
ros::Publisher go_back_publisher;
ros::Subscriber dis_sub;
double PI = 3.1415926536;
bool shot = false;
cv::Mat shoted_image;
double H = 81.7647;
double dataH[640][480];
double dataS[640][480];
double finali = -1, finalj = -1;
status maxstatus;
const double ysyz = 0.1;
int distance;

bool checkp(int x, int y) {
  int cnt = 0;
  for (int m = -8; m < 8; m++) {
    for (int n = -8; n < 8; n++) {
      if (abs(dataH[y + m][x + n] - H) < ysyz && dataS[y + m][x + n] > 0.35) {
        cnt++;
      }
    }
  }
  return cnt > 8;
}

void disCallback(const std_msgs::Int64::ConstPtr &msg) { distance = msg->data; }

void get_object() {
  maxstatus.cnt = -1;
  isRunning = true;
  printf("开始抓物\n");
  std_msgs::Float64MultiArray arm_msg;
  arm_msg.data.push_back(0);
  arm_msg.data.push_back(-7.5);
  arm_msg.data.push_back(-2.5);
  arm_publisher.publish(arm_msg);
  ros::spinOnce();
  std_msgs::Float64 msg_hand;
  msg_hand.data = -0.4;
  hand_publisher.publish(msg_hand);
  ros::spinOnce();
  sleep(1);
  for (double jd = -PI / 6.0; jd < PI / 6.0; jd = jd + 0.12) {
    int cnt = 0;
    std_msgs::Float64 msg_arm_xz;
    msg_arm_xz.data = jd;
    arm_xz_publisher.publish(msg_arm_xz);
    sleep(1);
    shot = true;
    while (shot) {
      sleep(0.001);
    }
    // printf("shot successfully.\n");
    for (int i = 0; i < 640; i++) {
      for (int j = 0; j < 480; j++) {
        cv::Scalar color = shoted_image.at<cv::Vec3b>(i, j); // BGR
        double R = color[2] / 255.0;
        double G = color[1] / 255.0;
        double B = color[0] / 255.0;
        double Cmax = std::max(std::max(R, G), B);
        double Cmin = std::min(std::min(R, G), B);
        double delta = Cmax - Cmin;
        double H = 0;
        if (delta == 0) {
          H = 0;
        } else if (Cmax == R) {
          H = 60 * (G - B) / delta;
        } else if (Cmax == G) {
          H = 60 * ((B - R) / delta + 2);
        } else if (Cmax == B) {
          H = 60 * ((R - G) / delta + 4);
        }
        dataH[i][j] = H;
        if (Cmax == 0) {
          dataS[i][j] = 0;
        } else {
          dataS[i][j] = delta / Cmax;
        }
      }
    }
    // printf("convert successfully.\n");
    for (int j = 0; j < 640; j++) {
      for (int i = 0; i < 480; i++) {
        if (abs(dataH[j][i] - H) < ysyz && dataS[j][i] > 0.35) {
          cnt++;
        }
      }
    }
    printf("clock,jd=%lf,cnt=%d;current best:jd=%lf,cnt=%d\n", jd, cnt,
           maxstatus.jd, maxstatus.cnt);
    status cs;
    cs.cnt = cnt;
    cs.jd = jd;
    if (cs.cnt > maxstatus.cnt) {
      maxstatus.cnt = cs.cnt;
      maxstatus.jd = cs.jd;
    }
  }
  printf("finished\n");
  std_msgs::Float64 msg_arm_xz;
  msg_arm_xz.data = maxstatus.jd;
  arm_xz_publisher.publish(msg_arm_xz);
  ros::spinOnce();
  ros::spinOnce();
  ros::spinOnce();
  sleep(2);
  shot = true;
  while (shot) {
    sleep(0.001);
  }
  int sumi = 0, sumj = 0, ccnt = 0;
  for (int i = 0; i < 640; i++) {
    for (int j = 0; j < 480; j++) {
      cv::Scalar color = shoted_image.at<cv::Vec3b>(i, j); // BGR
      double R = color[2] / 255.0;
      double G = color[1] / 255.0;
      double B = color[0] / 255.0;
      double Cmax = std::max(std::max(R, G), B);
      double Cmin = std::min(std::min(R, G), B);
      double delta = Cmax - Cmin;
      double H = 0;
      if (delta == 0) {
        H = 0;
      } else if (Cmax == R) {
        H = 60 * (G - B) / delta;
      } else if (Cmax == G) {
        H = 60 * ((B - R) / delta + 2);
      } else if (Cmax == B) {
        H = 60 * ((R - G) / delta + 4);
      }
      dataH[i][j] = H;
      if (Cmax == 0) {
        dataS[i][j] = 0;
      } else {
        dataS[i][j] = delta / Cmax;
      }
    }
  }
  // printf("convert successfully.\n");
  for (int j = 10; j < 630; j++) {
    for (int i = 10; i < 470; i++) {
      if (abs(dataH[j][i] - H) < ysyz && dataS[j][i] > 0.35) {
        if (checkp(i, j)) {
          sumi += i;
          sumj += j;
          ccnt++;
        }
      }
    }
  }
  finali = sumi / ((double)ccnt);
  finalj = sumj / ((double)ccnt);
  double dAngle = -0.0015 * (finali - 240) - 0.01;
  double dz = 0.001 * (finalj - 320);
  std_msgs::Float64 msg_arm_xz_test;
  msg_arm_xz_test.data = maxstatus.jd + dAngle;
  arm_xz_publisher.publish(msg_arm_xz_test);
  ros::spinOnce();
  ros::spinOnce();
  ros::spinOnce();
  printf("%lf,%lf,%d\n", finali, finalj, distance);
  sleep(1);
  double a = 7.5, k = 1;
  double x = (a + distance / 10.0 - k) * cos(maxstatus.jd + dAngle);
  double y = (a + distance / 10.0 - k) * sin(maxstatus.jd + dAngle);
  double z = dz;
  std_msgs::Float64MultiArray arm_msg_ctrl;
  arm_msg_ctrl.data.push_back(x);
  arm_msg_ctrl.data.push_back(y);
  arm_msg_ctrl.data.push_back(z);
  arm_publisher.publish(arm_msg_ctrl);
  ros::spinOnce();
  ros::spinOnce();
  ros::spinOnce();
  sleep(1);
  std_msgs::Float64 msg_hand_catch;
  msg_hand_catch.data = 0.6;
  hand_publisher.publish(msg_hand_catch);
  ros::spinOnce();
  ros::spinOnce();
  ros::spinOnce();
  sleep(1);
  std_msgs::Float64MultiArray arm_msg_ctrl_catch;
  arm_msg_ctrl_catch.data.push_back(5);
  arm_msg_ctrl_catch.data.push_back(0);
  arm_msg_ctrl_catch.data.push_back(15);
  arm_publisher.publish(arm_msg_ctrl_catch);
  std_msgs::String go_back_msg;
  go_back_msg.data = "";
  go_back_publisher.publish(go_back_msg);
  ros::spinOnce();
  ros::spinOnce();
  ros::spinOnce();
  sleep(1);
}

void chatterCallback(const std_msgs::Float64::ConstPtr &msg) {
  std::thread t(get_object);
  t.detach();
  H = msg->data;
}

class ImageConverter {
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;

public:
  ImageConverter() : it_(nh_) {
    image_sub_ = it_.subscribe("/camera_hand/rgb/image_raw", 1,
                               &ImageConverter::imageCb, this);

    cv::namedWindow(OPENCV_WINDOW);
  }

  ~ImageConverter() { cv::destroyWindow(OPENCV_WINDOW); }

  void imageCb(const sensor_msgs::ImageConstPtr &msg) {
    cv_bridge::CvImagePtr cv_ptr;
    try {
      cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
    } catch (cv_bridge::Exception &e) {
      ROS_ERROR("cv_bridge exception: %s", e.what());
      return;
    }
    cv::Mat image = cv_ptr->image;
    if (shot == true) {
      shoted_image = image.clone();
      // printf("clone successfully.\n");
      shot = false;
    }
    /*for (int i = 0; i < 640; i++) {
      for (int j = 0; j < 480; j++) {
        cv::Scalar color = image.at<cv::Vec3b>(i, j); // BGR
        double R = color[2] / 255.0;
        double G = color[1] / 255.0;
        double B = color[0] / 255.0;
        double Cmax = std::max(std::max(R, G), B);
        double Cmin = std::min(std::min(R, G), B);
        double delta = Cmax - Cmin;
        double H = 0;
        if (delta == 0) {
          H = 0;
        } else if (Cmax == R) {
          H = 60 * (G - B) / delta;
        } else if (Cmax == G) {
          H = 60 * ((B - R) / delta + 2);
        } else if (Cmax == B) {
          H = 60 * ((R - G) / delta + 4);
        }
        dataH[i][j] = H;
        if (Cmax == 0) {
          dataS[i][j] = 0;
        } else {
          dataS[i][j] = delta / Cmax;
        }
      }
    }
    for (int j = 0; j < 640; j++) {
      for (int i = 0; i < 480; i++) {
        if (abs(dataH[j][i] - H) < ysyz && dataS[j][i] > 0.35) {
          image.at<cv::Vec3b>(j, i) = cv::Vec3b(0, 0, 255);
        }
      }
    }*/
    if (finali != -1 && finalj != -1) {
      cv::circle(image, cv::Point(finali, finalj), 5, cv::Scalar(0, 0, 255),
                 10);
    }
    cv::imshow(OPENCV_WINDOW, image);
    cv::waitKey(3);
  }
};

int main(int argc, char *argv[]) {
  ros::init(argc, argv, "catch_object");
  ros::NodeHandle nh;
  ros::Subscriber sub = nh.subscribe("catch_object", 1000, chatterCallback);
  arm_xz_publisher =
      nh.advertise<std_msgs::Float64>("/waist_controller/command", 1);
  arm_publisher = nh.advertise<std_msgs::Float64MultiArray>("/arm_sub", 1);
  hand_publisher =
      nh.advertise<std_msgs::Float64>("hand_controller/command", 1);
  go_back_publisher = nh.advertise<std_msgs::String>("goback", 1);
  dis_sub = nh.subscribe("distance", 1000, disCallback);

  ImageConverter ic;
  ros::spin();
  return 0;
}