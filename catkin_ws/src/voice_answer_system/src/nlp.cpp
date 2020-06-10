#include "ros/ros.h"
#include "std_msgs/String.h"

ros::Publisher tts_pub;
ros::Publisher tuling_pub;
ros::Publisher logout_pub;

// general type:1:Turing2:Command3:Speakout"OK,I'm not doing
// anything."4:Speakout"Sorry,I don't know how to do that."5:Logout
// command type:1:get object
// get object type:1:cup
struct msg_command {
  short a, b, c;
};
msg_command recognize(std::string A) {
  msg_command result;
  result.a = 3;
  result.b = 0;
  result.c = 0;
  int max = A.size();
  bool find_bye = (A.find("再见") < max);
  if (find_bye) {
    result.a = 5;
  } else {
    bool find_no = (A.find("不") < max) || (A.find("别") < max);
    if (!find_no) {
      bool command1 = (A.find("给") < max) || (A.find("拿") < max) ||
                      (A.find("递") < max) || (A.find("送") < max) ||
                      (A.find("取") < max);
      if (command1) {
        bool get_object_type_1 = (A.find("杯") < max) || (A.find("瓶") < max) ||
                                 (A.find("筒") < max) || (A.find("罐") < max);
        if (get_object_type_1) {
          result.a = 2;
          result.b = 1;
          result.c = 1;
        } else
          result.a = 4;
      } else
        result.a = 1;
    } else
      result.a = 3;
  }
  return result;
}

void chatterCallback(const std_msgs::String::ConstPtr &rmsg) {
  msg_command result = recognize(rmsg->data);
  if (result.a == 1) {
    std_msgs::String msg;
    std::stringstream ss;
    msg.data = rmsg->data;
    tuling_pub.publish(msg);
    ros::spinOnce();
    ros::spinOnce();
    ros::spinOnce();
    sleep(6);
    system("rosrun voice_answer_system voice_record_pub");
  } else if (result.a == 2 && result.b == 1) {
    std_msgs::String msg;
    msg.data = "好的";
    tts_pub.publish(msg);
    printf("发送取物指令\n");
    system("/home/mustar/do_catch.sh");
  } else if (result.a == 3) {
    std_msgs::String msg;
    msg.data = "好的，我将不做任何事情";
    tts_pub.publish(msg);
  } else if (result.a == 4) {
    std_msgs::String msg;
    msg.data = "对不起，我不知道怎样去做";
    tts_pub.publish(msg);
  } else {
    std_msgs::String msg;
    msg.data = "再见";
    tts_pub.publish(msg);
    std_msgs::String logoutmsg;
    logoutmsg.data = "";
    logout_pub.publish(logoutmsg);
  }
}

int main(int argc, char *argv[]) {
  ros::init(argc, argv, "nlp");
  ros::NodeHandle n;
  ros::Subscriber sub = n.subscribe("nlp", 1000, chatterCallback);
  tts_pub = n.advertise<std_msgs::String>("tts", 1000);
  logout_pub = n.advertise<std_msgs::String>("face_logout", 1000);
  tuling_pub = n.advertise<std_msgs::String>("tuling", 1000);
  ros::spin();
  return 0;
}