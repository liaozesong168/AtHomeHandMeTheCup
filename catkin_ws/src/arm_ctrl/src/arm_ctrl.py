#!/usr/bin/env python
'''arm01 ROS Node'''
# license removed for brevity
import rospy
from std_msgs.msg import Float64MultiArray,Float64
from math import asin, atan, sqrt, pi, acos, tan
import time
# Unit:Centimeters

global pub,pub1,pub2,pub3

def CalcAngle_new02(x, y, z, l = 10.5):
    
    x = float(x)
    y = float(y)
    z = float(z)
    h = float(z)
    d = float(sqrt(x*x + y*y))
    angle = atan(y/x)
    l = float(l)
    print("debug:"+str((h*h+d*d)/(2*l)))
    beita = pi - 2*asin(sqrt(h*h+d*d)/(2*l))
    alpha = asin(sqrt(h*h+d*d)/(2*l))-atan(h/d)
    gamma = pi/2 - alpha - beita
    return [angle, alpha, beita, gamma]

def callback(msg):
    print(msg.data)
    global pub,pub1,pub2,pub3
    rate = rospy.Rate(10) # 10hz
    x=msg.data[0]
    y=msg.data[1]
    z=msg.data[2]
    if x==0:
        x=0.00001
    if y==0:
        y=0.00001
    if z==0:
        z=0.00001
    a = [x,y,z]
    rslt = CalcAngle_new02(a[0], a[1], a[2])
    rospy.loginfo(rslt)
    #pub.publish(0)
    #time.sleep(0.5)
    pub1.publish(0)
    #time.sleep(0.5)
    #pub2.publish(0)
    #time.sleep(0.5)
    #pub3.publish(0)
    #time.sleep(0.5)
    #pub.publish(0)
    pub.publish(0+rslt[0])
    pub1.publish(rslt[1])
    pub2.publish(rslt[2])
    pub3.publish(rslt[3])
    '''pub.publish(0)
    pub1.publish(0)
    pub2.publish(0)
    pub3.publish(0)'''


if __name__ == '__main__':
    try:
        global pub,pub1,pub2,pub3
        rospy.init_node('arm_ctrl')
        arm_sub = rospy.Subscriber("/arm_sub", Float64MultiArray, callback)
        pub = rospy.Publisher('/waist_controller/command', Float64, queue_size=10)
        pub1 = rospy.Publisher('/shoulder_controller/command', Float64, queue_size=10)
        pub2 = rospy.Publisher('/elbow_controller/command', Float64, queue_size=10)
        pub3 = rospy.Publisher('/wrist_controller/command', Float64, queue_size=10)
        rospy.spin()
    except rospy.ROSInterruptException:
        pass