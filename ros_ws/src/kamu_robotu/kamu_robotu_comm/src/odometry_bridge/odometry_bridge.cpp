#include <ros/ros.h>
#include <serial/serial.h>
#include <std_msgs/String.h>
#include <std_msgs/Empty.h>
#include <std_msgs/Float64.h>
#include <string>
#include <tf/transform_broadcaster.h>
#include <nav_msgs/Odometry.h>
#include <geometry_msgs/Twist.h>
#include "okoserial.h"
#include "kamu_robotu_comm/kamu_cmd.h"


double v,w;
serial::Serial ser;
std::string readable;
void twistlistenerCallback(geometry_msgs::Twist cmd){
    v = cmd.linear.x;
    w = cmd.angular.z;
}
bool kamu_command_handler(kamu_robotu_comm::kamu_cmd::Request &req, kamu_robotu_comm::kamu_cmd::Response &res)
{
uint8_t data2send[6];
data2send[0] = 0x55;
data2send[1] = (uint8_t)req.cmd_type;
data2send[2] = (uint8_t)req.cmd_param;
data2send[3] = (uint8_t)req.cmd_enable;
data2send[4] = 0;
data2send[5] = 0;
ser.write((const uint8_t *) data2send, 6);
res.result = true;
return true;
}

unsigned int num_readings = 28;
uint8_t dummy_8 = 0;
int16_t dummy_16 = 0;
int i = 0 ;

int main(int argc, char** argv){
  ros::init(argc, argv, "odometry_bridge");
  ros::NodeHandle n;
  ros::Publisher odom_pub = n.advertise<nav_msgs::Odometry>("odom", 50);
  ros::ServiceServer service = n.advertiseService("/kamu_cmd", kamu_command_handler);
  ros::Subscriber twist_sub = n.subscribe("/cmd_vel",100, twistlistenerCallback);
  tf::TransformBroadcaster odom_broadcaster;


  
double odometry_info[num_readings]; // x,y,th,vx,vy,w
  
  
  try // Connect to the port
    {
        ser.setPort("/dev/rfcomm1"); // miniuart port of the rpi
        ser.setBaudrate(9600);
		serial::stopbits_t  stopbits;
		stopbits = serial::stopbits_one;	
		ser.setStopbits(stopbits);
        serial::Timeout to = serial::Timeout::simpleTimeout(1000);
        ser.setTimeout(to);
        ser.open();
    }
    catch (serial::IOException& e)
    {
        ROS_ERROR_STREAM("Unable to open porttttt ");
        return -1;
    }

    if(ser.isOpen()){
        ROS_INFO_STREAM("Serial Port initialized");
    }else{
        return -1;
    }

  ros::Time current_time;

  ros::Rate r(20);
  while(n.ok()){

    ros::spinOnce();               // check for incoming messages
    current_time = ros::Time::now();
        if(ser.available()>0){
            ROS_INFO_STREAM("Reading from serial port");
            std_msgs::String result;
            readable = ser.readline(28);
		for(i=0;i<num_readings;i++){
			dummy_8 = readable[i];
			getSingleByte(dummy_8);
			ROS_INFO("DATA %d : %x " , i , dummy_8);
			
		}
		ROS_INFO("odom_mess.x :%f " , odom_mess.x);
			ROS_INFO("odom_mess.y :%f " , odom_mess.y);
			ROS_INFO("odom_mess.theta :%f " , odom_mess.theta);
			ROS_INFO("odom_mess.vx :%f " , odom_mess.vx);
			ROS_INFO("odom_mess.w :%f " , odom_mess.w);

        }
    // Send v and w to the robot here with serial
    // and here
    // and here
    
    //since all odometry is 6DOF we'll need a quaternion created from yaw
    geometry_msgs::Quaternion odom_quat = tf::createQuaternionMsgFromYaw(odom_mess.theta);

    //first, we'll publish the transform over tf
    geometry_msgs::TransformStamped odom_trans;
    odom_trans.header.stamp = current_time;
    odom_trans.header.frame_id = "odom";
    odom_trans.child_frame_id = "base_link";

    odom_trans.transform.translation.x = odom_mess.x/1000.0;
    odom_trans.transform.translation.y = odom_mess.y/1000.0;
    odom_trans.transform.translation.z = 0.0;
    odom_trans.transform.rotation = odom_quat;

    //send the transform
    odom_broadcaster.sendTransform(odom_trans);

    //next, we'll publish the odometry message over ROS
    nav_msgs::Odometry odom;
    odom.header.stamp = current_time;
    odom.header.frame_id = "odom";

    //set the position
    odom.pose.pose.position.x = odom_mess.x/1000.0;
    odom.pose.pose.position.y = odom_mess.y/1000.0;
    odom.pose.pose.position.z = 0.0;
    odom.pose.pose.orientation = odom_quat;

    //set the velocity
    odom.child_frame_id = "base_link";
    odom.twist.twist.linear.x = odom_mess.vx/1000.0;
    odom.twist.twist.linear.y = 0.0;
    odom.twist.twist.angular.z = odom_mess.w;

    //publish the message
    odom_pub.publish(odom);
    r.sleep();
  }
}
