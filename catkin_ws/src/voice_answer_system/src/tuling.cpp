#include <curl/curl.h>
#include <exception>
#include <iostream>
#include <jsoncpp/json/json.h>
#include <ros/ros.h>
#include <sstream>
#include <std_msgs/String.h>
#include <string>

using namespace std;

int flag = 0;
string result;

int writer(char *data, size_t size, size_t nmemb, string *writerData) {
  unsigned long sizes = size * nmemb;
  if (writerData == NULL)
    return -1;

  writerData->append(data, sizes);

  return sizes;
}

int parseJsonResonse(string input) {
  std::cout << input << std::endl;
  Json::Value root;
  Json::Reader reader;
  bool parsingSuccessful = reader.parse(input, root);
  if (!parsingSuccessful) {
    std::cout << "!!! Failed to parse the response data" << std::endl;
    return -1;
  }
  const Json::Value results = root["results"];
  const Json::Value values = results[0]["values"];
  // std::cout<<values.toStyledString()<<std::endl;
  const Json::Value text = values["text"];
  result = text.asString();
  flag = 1;

  // std::cout<< "response code:" << code << std::endl;
  std::cout << "response text:" << result << std::endl;

  return 0;
}

int HttpPostRequest(string input) {
  string buffer;

  std::string strJson =
      "{\"reqType\":0,\"perception\": {\"inputText\": {\"text\": \"";
  strJson += input;
  strJson += "\"}},\"userInfo\": {\"apiKey\": "
             "\"901e43b88dd84655a0bc23b1fc3d0459\",\"userId\": "
             "\"17791373856\"}}";

  std::cout << "post json string: " << strJson << std::endl;

  try {
    CURL *pCurl = NULL;
    CURLcode res;
    pCurl = curl_easy_init();
    if (NULL != pCurl) {
      // 设置超时时间为10秒
      curl_easy_setopt(pCurl, CURLOPT_TIMEOUT, 10);
      curl_easy_setopt(pCurl, CURLOPT_URL,
                       "http://openapi.tuling123.com/openapi/api/v2");

      // 设置http发送的内容类型为JSON
      curl_slist *plist = curl_slist_append(
          NULL, "Content-Type:application/json;charset=UTF-8");
      curl_easy_setopt(pCurl, CURLOPT_HTTPHEADER, plist);

      // 设置要POST的JSON数据
      curl_easy_setopt(pCurl, CURLOPT_POSTFIELDS, strJson.c_str());

      curl_easy_setopt(pCurl, CURLOPT_WRITEFUNCTION, writer);

      curl_easy_setopt(pCurl, CURLOPT_WRITEDATA, &buffer);

      // Perform the request, res will get the return code
      res = curl_easy_perform(pCurl);
      // Check for errors
      if (res != CURLE_OK) {
        printf("curl_easy_perform() failed:%s\n", curl_easy_strerror(res));
      }
      // always cleanup
      curl_easy_cleanup(pCurl);
    }
    curl_global_cleanup();
  } catch (std::exception &ex) {
    printf("curl exception %s.\n", ex.what());
  }
  if (buffer.empty()) {
    std::cout << "!!! ERROR The Tuling sever response NULL" << std::endl;
  } else {
    parseJsonResonse(buffer);
  }

  return 0;
}
void arvCallBack(const std_msgs::String::ConstPtr &msg) {
  std::cout << "your quesion is: " << msg->data << std::endl;
  HttpPostRequest(msg->data);
}

int main(int argc, char **argv) {
  curl_global_init(CURL_GLOBAL_ALL);
  ros::init(argc, argv, "tuling");
  ros::NodeHandle nd;

  ros::Subscriber sub = nd.subscribe("tuling", 10, arvCallBack);
  ros::Publisher pub = nd.advertise<std_msgs::String>("tts", 10);
  ros::Rate loop_rate(10);

  while (ros::ok()) {
    if (flag) {
      std_msgs::String msg;
      msg.data = result;
      pub.publish(msg);
      flag = 0;
    }
    ros::spinOnce();
    loop_rate.sleep();
  }
}
