#!/usr/bin/env python
# -*- coding: utf-8 -*-
from __future__ import print_function

import threading
import time
import os
import roslib
import sys
import rospy
import cv2
from std_msgs.msg import Int64MultiArray
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError
import face_recognition
import PIL.Image as PImage
import numpy as np

rospy.init_node('face_location')
rate = rospy.Rate(10)  # 10hz
pub = rospy.Publisher('face_location', Int64MultiArray, queue_size=10)
global tick
tick=0

class image_converter:
    def __init__(self):
        self.bridge = CvBridge()
        self.image_sub = rospy.Subscriber(
            "/camera_top/rgb/image_raw", Image, self.callback)

    def callback(self, data):
        global tick
        tick=tick+1
        if tick%43==0:
            tick=0
            cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
            
            im = PImage.fromarray(cv2.cvtColor(cv_image, cv2.COLOR_BGR2RGB))
            im = im.convert('RGB')
            results = face_recognition.face_locations(np.array(im),2)
            #print(results)
            if len(results)>0:
                x1=results[0][0]
                y1=results[0][1]
                x2=results[0][2]
                y2=results[0][3]
                msg=Int64MultiArray()
                msg.data=[x1,y1,x2,y2]
                pub.publish(msg)
                cv2.rectangle(cv_image,(y1,x1),(y2,x2),(0,255,0),5)
            cv2.imshow("Locate face", cv_image)
            cv2.waitKey(3)

def main(args):
    ic = image_converter()
    try:
        rospy.spin()
    except KeyboardInterrupt:
        detect_on=False
        print("Shutting down")
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main(sys.argv)
