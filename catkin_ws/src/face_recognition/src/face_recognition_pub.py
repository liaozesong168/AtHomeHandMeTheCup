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
from std_msgs.msg import String
from std_msgs.msg import Float64
from sensor_msgs.msg import Image
from cv_bridge import CvBridge, CvBridgeError
import face_recognition
import PIL.Image as PImage
import numpy as np

known_face_encoding = face_recognition.face_encodings(
    face_recognition.load_image_file("/home/mustar/known_faces/1.jpg"))[0]

rospy.init_node('face_recognition_pub')
pub = rospy.Publisher('tts', String, queue_size=10)
camera_pub = rospy.Publisher('/camera_controller/command', Float64, queue_size=10)
rate = rospy.Rate(10)  # 10hz

def logout(msg):
    t = threading.Thread(target=detect_a, args=())
    t.start()
    print("重新开始识别")

logout_sub = rospy.Subscriber(
            "/face_logout", String, logout)

global cv_image

def talker(success):
    hello_str = ""
    if success :
        hello_str="识别成功"
    else:
        hello_str="识别失败"
    rospy.loginfo(hello_str)
    pub.publish(hello_str)
    rate.sleep()
    data=float(-0.55)
    camera_pub.publish(data)
    time.sleep(0.5)
    if success :
        os.system("rosrun voice_answer_system voice_record_pub") #开始录音


class image_converter:
    def __init__(self):
        self.bridge = CvBridge()
        self.image_sub = rospy.Subscriber(
            "/camera_top/rgb/image_raw", Image, self.callback)

    def callback(self, data):

        global cv_image
        cv_image = self.bridge.imgmsg_to_cv2(data, "bgr8")
        cv2.imshow("Face recognition", cv_image)
        cv2.waitKey(3)

def detect_a():
    print("start thread")
    time.sleep(3)
    rospy.loginfo("thread start")
    global cv_image
    detect_on=True
    while detect_on:
        try:
            
            data=float(-0.55)
            camera_pub.publish(data)
            im = PImage.fromarray(cv2.cvtColor(cv_image, cv2.COLOR_BGR2RGB))
            im = im.convert('RGB')
            unknown_face_encoding = face_recognition.face_encodings(np.array(im))[0]
            known_faces = [
                known_face_encoding
            ]
            results = face_recognition.face_distance(known_faces, unknown_face_encoding)
            if results[0] < 0.55:
                talker(True)
                detect_on=False
            else:
                talker(False)
        except IndexError:
            rospy.loginfo("no face")
        time.sleep(1)
        

def main(args):
    t = threading.Thread(target=detect_a, args=())
    t.start()
    
    ic = image_converter()
    try:
        rospy.spin()
    except KeyboardInterrupt:
        detect_on=False
        print("Shutting down")
    cv2.destroyAllWindows()


if __name__ == '__main__':
    main(sys.argv)
