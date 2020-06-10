#include "ros/ros.h"
#include "std_msgs/Float64.h"
#include "std_msgs/Float64MultiArray.h"
#include <deque>
#include <geometry_msgs/Twist.h>        // for velocity commands
#include <geometry_msgs/TwistStamped.h> // for velocity commands
#include <kobuki_msgs/MotorPower.h>
#include <sstream>

ros::Publisher velocity_publisher_;
ros::Publisher motor_power_publisher_;
ros::Publisher camera_publisher_;

double head = 0;
int head_clock = 0;
bool stop = false;

std::deque<double> dist_array;
std::deque<double> offset_array;

double angular_z = 0, linear_x = 0.25;

double ctrlP(double x, double p) { return -p * x; }
struct ctrl {
  double linear_velocity;
  double angular_velocity;
};

void chatterCallback(const std_msgs::Float64MultiArray &msg) {
  // 640,480
  if (!stop) {
    head_clock++;
    // printf("%lf,%lf\n",msg.data[0],msg.data[1]);
    std_msgs::Float64 msg_camera;
    head += std::min(ctrlP(msg.data[1] - 240, -0.001), 0.03);
    printf("%lf\n", head);
    if (head > 0.63) {
      stop = true;
    }
    if (head > 0.5) {
      linear_x = 0.15;
    }
    msg_camera.data = head;
    camera_publisher_.publish(msg_camera);
    dist_array.push_back(1);
    offset_array.push_back(msg.data[0] - 320.0);
    if (dist_array.size() > 30) {
      dist_array.pop_front();
    }
    if (offset_array.size() > 30) {
      offset_array.pop_front();
    }
    angular_z = ctrlP(msg.data[0] - 320.0, 0.005);
  }
}

int main(int argc, char *argv[]) {
  ros::init(argc, argv, "kobuki_ctrl");
  ros::NodeHandle nh;
  ros::Rate loop_rate(10);
  ros::Subscriber sub = nh.subscribe("centercoords", 1000, chatterCallback);
  velocity_publisher_ =
      nh.advertise<geometry_msgs::Twist>("/mobile_base/commands/velocity", 1);
  ros::Publisher arm_publisher_ =
      nh.advertise<std_msgs::Float64MultiArray>("/arm_sub", 1);
  camera_publisher_ =
      nh.advertise<std_msgs::Float64>("/camera_controller/command", 1);
  ros::Publisher catch_pub =
      nh.advertise<std_msgs::Float64>("/catch_object", 1);
  // motor_power_publisher_ =
  // nh.advertise<kobuki_msgs::MotorPower>("motor_power", 1);
  geometry_msgs::TwistPtr cmd(new geometry_msgs::Twist());
  //初始化
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
  std_msgs::Float64MultiArray arm_msg;
  arm_msg.data.push_back(0);
  arm_msg.data.push_back(-7.5);
  arm_msg.data.push_back(0);
  arm_publisher_.publish(arm_msg);
  std_msgs::Float64 msg_camera;
  msg_camera.data = head;
  camera_publisher_.publish(msg_camera);

  //-------
  sleep(0.5);
  while (ros::ok() && !stop) {
    cmd->linear.x = linear_x;
    // cmd->linear.y = 0.08;
    // cmd->linear.z = 0.08;
    // cmd->angular.x = 1;
    // cmd->angular.y = 0.08;
    cmd->angular.z = angular_z;
    velocity_publisher_.publish(cmd);
    ros::spinOnce();
    loop_rate.sleep();
  }
  std_msgs::Float64 catch_pub_msg;
  catch_pub_msg.data = 81.7647;
  catch_pub.publish(catch_pub_msg);
  printf("发送抓取消息\n");
  sleep(1);
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
  return 0;
}
