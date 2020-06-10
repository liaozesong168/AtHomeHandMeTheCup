#include <ros/ros.h>
#include <image_transport/image_transport.h>
#include <cv_bridge/cv_bridge.h>
#include <sensor_msgs/image_encodings.h>
#include <opencv2/imgproc/imgproc.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <std_msgs/Float64MultiArray.h>
#include <cmath>

static const std::string OPENCV_WINDOW = "find_object";
const double PI=3.1415926535;
int timer=0;
double dataH[480][640];
double dataS[480][640];
//const double H=93.1707;
const double H=103.59;
const double ysyz=1.5;
ros::Publisher pub;
int lasti=-1,lastj=-1;

bool checkp(int x,int y)
{
  int cnt=0;
          for(int m=-8;m<8;m++)
          {
            for(int n=-8;n<8;n++)
            {
              if(abs(dataH[y+m][x+n]-H)<ysyz && dataS[y+m][x+n]>0.35)
              {
                cnt++;
              }
            }
          }
  return cnt>8;
}

class ImageConverter
{
  ros::NodeHandle nh_;
  image_transport::ImageTransport it_;
  image_transport::Subscriber image_sub_;

public:
  ImageConverter()
    : it_(nh_)
  {
    image_sub_ = it_.subscribe("/camera_top/rgb/image_raw", 1, &ImageConverter::imageCb, this);

    cv::namedWindow(OPENCV_WINDOW);
  }

  ~ImageConverter()
  {
    cv::destroyWindow(OPENCV_WINDOW);
  }

  void imageCb(const sensor_msgs::ImageConstPtr& msg)
  {
    timer++;
    if(timer%2==0)
    {
      cv_bridge::CvImagePtr cv_ptr;
      try
      {
        cv_ptr = cv_bridge::toCvCopy(msg, sensor_msgs::image_encodings::BGR8);
      }
      catch (cv_bridge::Exception& e)
      {
        ROS_ERROR("cv_bridge exception: %s", e.what());
        return;
      }
      //640x480,480 640
      cv::Mat image;
      cv::Mat image_hd;
      
      image=cv_ptr->image;
      cv::cvtColor(image,image_hd,cv::COLOR_BGR2GRAY);
      for(int i=0;i<480;i++)
      {
        for(int j=0;j<640;j++)
        {
          cv::Scalar color = image.at<cv::Vec3b>(i,j);//BGR
          double R=color[2]/255.0;
          double G=color[1]/255.0;
          double B=color[0]/255.0;
          double Cmax=std::max(std::max(R,G),B);
          double Cmin=std::min(std::min(R,G),B);
          double delta=Cmax-Cmin;
          double H=0;
          if(delta==0)
          {
            H=0;
          }else if(Cmax==R)
          {
            H=60*(G-B)/delta;
          }else if(Cmax==G)
          {
            H=60*((B-R)/delta+2);
          }else if(Cmax==B)
          {
            H=60*((R-G)/delta+4);
          }
          dataH[i][j]=H;
          if(Cmax==0)
          {
            dataS[i][j]=0;
          }else{
            dataS[i][j]=delta/Cmax;
          }
        }
      }
      int c_cnt=0,sumi=0,sumj=0;
      //i为横坐标
      int imin=10,imax=630,jmin=10,jmax=470;
      if(lasti!=-1)
      {
        imin=std::max(imin,lasti-75);
        imax=std::min(imax,lasti+75);
      }
      if(lastj!=-1)
      {
        jmin=std::max(jmin,lastj-75);
        jmax=std::min(jmax,lastj+75);
      }
      for(int j=jmin;j<jmax;j++)
      {
        for(int i=imin;i<imax;i++)
        {
          if(abs(dataH[j][i]-H)<ysyz && dataS[j][i]>0.35)
          {
            if(checkp(i,j))
            {
              image.at<cv::Vec3b>(j, i) = cv::Vec3b(0,255,0);
              c_cnt++;
              sumi+=i;
              sumj+=j;
            }else{
              image.at<cv::Vec3b>(j, i) = cv::Vec3b(0,0,255);
            }
          }
        }
      }
      if(c_cnt>0)
      {
        lasti=sumi/((double)c_cnt);
        lastj=sumj/((double)c_cnt);
        cv::circle(image,cv::Point(lasti,lastj),5,cv::Scalar(0,0,255),10);
        std_msgs::Float64MultiArray msg;
        msg.data.push_back(lasti);
        msg.data.push_back(lastj);
        pub.publish(msg);
      }
      
      cv::imshow(OPENCV_WINDOW,image);
      cv::waitKey(3);
    }
  }
};

int main(int argc, char *argv[])
{
	ros::init(argc, argv, "find_object");
  ros::NodeHandle nd;
  pub = nd.advertise<std_msgs::Float64MultiArray>("centercoords", 10);
	ImageConverter ic;
	ros::spin();
	return 0;
}