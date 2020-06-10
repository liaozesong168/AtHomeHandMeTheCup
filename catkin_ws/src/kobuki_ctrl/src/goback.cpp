#include "ros/ros.h"
#include "std_msgs/Float64.h"
#include "std_msgs/Int64MultiArray.h"
#include "std_msgs/String.h"
#include <deque>
#include <geometry_msgs/Twist.h>        // for velocity commands
#include <geometry_msgs/TwistStamped.h> // for velocity commands
#include <kobuki_msgs/MotorPower.h>
#include <sstream>

ros::Publisher velocity_publisher_;
ros::Publisher motor_power_publisher_;
ros::Publisher camera_publisher_;
ros::Publisher tts_pub;
bool stop = true;
double linear = 0.1, angular = 0;
double head = -0.2;

double ctrlP(double x, double p) { return -p * x; }

void face_location(const std_msgs::Int64MultiArray &msg) {
  if (!stop) {
    angular = ctrlP((msg.data[1] + msg.data[3]) / 2.0 - 320, 0.003);
    head += ctrlP((msg.data[0] + msg.data[2]) / 2.0 - 160, -0.002);
    printf("%lf,%lf\n", (msg.data[0] + msg.data[2]) / 2.0, head);
    if (head < -0.5) {
      stop = true;
    }
    if (head < -0.35) {
      linear = 0.05;
    }
    std_msgs::Float64 msg_camera;
    msg_camera.data = head;
    camera_publisher_.publish(msg_camera);
  }
}

void chatterCallback(const std_msgs::String &msg) {
  ros::Rate loop_rate(10);
  geometry_msgs::TwistPtr cmd(new geometry_msgs::Twist());
  int i = 0;
  while (ros::ok() && i < 45) {
    cmd->linear.x = 0;
    cmd->angular.z = 1;
    velocity_publisher_.publish(cmd);
    ros::spinOnce();
    loop_rate.sleep();
    i++;
  }
  cmd->linear.x = 0;
  cmd->angular.z = 0;
  velocity_publisher_.publish(cmd);
  std_msgs::Float64 msg_camera;
  msg_camera.data = -0.2;
  camera_publisher_.publish(msg_camera);
  sleep(1);
  stop = false;
  ros::spinOnce();
  loop_rate.sleep();
  while (ros::ok() && !stop) {
    cmd->linear.x = linear;
    cmd->angular.z = angular;
    velocity_publisher_.publish(cmd);
    ros::spinOnce();
    loop_rate.sleep();
  }
  std_msgs::String tts_msg;
  tts_msg.data = "取物成功";
  tts_pub.publish(tts_msg);
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
}

int main(int argc, char *argv[]) {
  ros::init(argc, argv, "goback");
  ros::NodeHandle nh;
  ros::Subscriber subc = nh.subscribe("goback", 1000, chatterCallback);
  ros::Subscriber subl = nh.subscribe("face_location", 1000, face_location);
  camera_publisher_ =
      nh.advertise<std_msgs::Float64>("/camera_controller/command", 1);
  tts_pub = nh.advertise<std_msgs::String>("tts", 1000);
  velocity_publisher_ =
      nh.advertise<geometry_msgs::Twist>("/mobile_base/commands/velocity", 1);
  ros::spin();
  return 0;
}
