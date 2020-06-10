#include "msp_cmn.h"
#include "msp_errors.h"
#include "qisr.h"
#include "ros/ros.h"
#include "speech_recognizer.h"
#include "std_msgs/String.h"

#include <sstream>

#define FRAME_LEN 640
#define BUFFER_SIZE 4096

static char *g_result = NULL;
static unsigned int g_buffersize = BUFFER_SIZE;
ros::Publisher chatter_pub;
struct speech_rec iat;
static bool is_ok = false;

static void show_result(char *string, char is_over) {
  if (is_over == 1) {
    std_msgs::String msg;
    std::stringstream ss;
    ss << string;
    msg.data = ss.str();
    ROS_INFO("results:%s", msg.data.c_str());
    chatter_pub.publish(msg);
    is_ok = true;
  }
}

void on_result(const char *result, char is_last) {
  if (result) {
    size_t left = g_buffersize - 1 - strlen(g_result);
    size_t size = strlen(result);
    if (left < size) {
      g_result = (char *)realloc(g_result, g_buffersize + BUFFER_SIZE);
      if (g_result)
        g_buffersize += BUFFER_SIZE;
      else {
        printf("mem alloc failed\n");
        return;
      }
    }
    strncat(g_result, result, size);
    show_result(g_result, is_last);
  }
}
void on_speech_begin() {
  if (g_result) {
    free(g_result);
  }
  g_result = (char *)malloc(BUFFER_SIZE);
  g_buffersize = BUFFER_SIZE;
  memset(g_result, 0, g_buffersize);

  printf("Start Listening...\n");
}
void on_speech_end(int reason) {
  if (reason == END_REASON_VAD_DETECT)
    printf("\nSpeaking done \n");
  else
    printf("\nRecognizer error %d\n", reason);
}

static void demo_mic(const char *session_begin_params) {
  int errcode;
  int i = 0;

  struct speech_rec_notifier recnotifier = {on_result, on_speech_begin,
                                            on_speech_end};

  errcode = sr_init(&iat, session_begin_params, SR_MIC, &recnotifier);
  if (errcode) {
    printf("speech recognizer init failed\n");
    return;
  }
  errcode = sr_start_listening(&iat);
  if (errcode) {
    printf("start listen failed %d\n", errcode);
  }
}

int main(int argc, char *argv[]) {
  std::string target = "nlp";
  if (argc == 2) {
    target = argv[1];
  }
  setlocale(LC_CTYPE, "zh_CN.utf8");
  ros::init(argc, argv, "voice_answer_system");
  ros::NodeHandle n;
  ros::Rate loop_rate(10);
  chatter_pub = n.advertise<std_msgs::String>(target, 1000);
  ros::Publisher tts_pub = n.advertise<std_msgs::String>("tts", 1000);
  ros::Publisher logout_pub =
      n.advertise<std_msgs::String>("face_logout", 1000);
  int n_count = 0;
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
  loop_rate.sleep();
  ros::spinOnce();
  std_msgs::String msg;
  msg.data = "请说话";
  ROS_INFO("%s", msg.data.c_str());
  tts_pub.publish(msg);
  ros::spinOnce();
  loop_rate.sleep();
  sleep(2);
  int ret = MSP_SUCCESS;

  const char *login_params = "appid = 517a72fc, work_dir = .";

  const char *session_begin_params =
      "sub = iat, domain = iat, language = zh_cn, "
      "accent = mandarin, sample_rate = 16000, "
      "result_type = plain, result_encoding = utf8";

  ret = MSPLogin(NULL, NULL, login_params);
  if (MSP_SUCCESS != ret) {
    printf("MSPLogin failed , Error code %d.\n", ret);
    MSPLogout();
    return -1;
  }
  printf("Login successful\n");
  demo_mic(session_begin_params);
  int count = 0;
  while (!is_ok) {
    printf("clock\n");
    sleep(1);
    count++;
    if (count > 10) {
      printf("time out\n");
      std_msgs::String msg_tts_timeout;
      msg_tts_timeout.data = "请求超时，请重新登录";
      tts_pub.publish(msg_tts_timeout);
      std_msgs::String msg_logout;
      msg_logout.data = "";
      logout_pub.publish(msg_logout);
      break;
    }
  }
  int errcode = sr_stop_listening(&iat);
  if (errcode) {
    printf("stop listening failed %d\n", errcode);
  }
  sr_uninit(&iat);
  MSPLogout();
  printf("exiting voice_record\n");
  return 0;
}