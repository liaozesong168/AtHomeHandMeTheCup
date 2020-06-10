#!/usr/bin/env python
# -*- coding: utf-8 -*-
import rospy
from std_msgs.msg import Int64
import serial


def talker():
    portx = "/dev/ttyUSB0"
    bps = 9600
    timex = 5
    ser = serial.Serial(portx, bps, timeout=timex)
    pub = rospy.Publisher('distance', Int64, queue_size=10)
    rospy.init_node('distance', anonymous=True)
    rate = rospy.Rate(1000)  # 10hz
    while not rospy.is_shutdown():
        rstr = ser.readline().decode("ascii")
        data = int(rstr[0:-4])
        # print(data)
        pub.publish(data)
        rate.sleep()
    ser.close()  # 关闭串口


if __name__ == '__main__':
    try:
        talker()
    except rospy.ROSInterruptException:
        pass
