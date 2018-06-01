#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/UInt8MultiArray.h"
//#include "geometry_msgs/Twist.h"
//#include "geometry_msgs/PoseStamped.h"
//#include "geometry_msgs/TwistStamped.h"
//#include "tf/transform_broadcaster.h"
#include <signal.h>

//#include <sstream>
//#include <math.h>
#include <stdio.h>
#include <vector>
#include <pthread.h>
#include <smart_lock.h>

class NoahPowerboard;
void sigintHandler(int sig)
{
    ROS_INFO("killing on exit");
    ros::shutdown();
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "noah_powerboard_node");
    //NoahPowerboard  powerboard;
    NoahPowerboard *powerboard = new NoahPowerboard();
    ros::Rate loop_rate(100);
    uint32_t cnt = 0;
    powerboard->PowerboardParamInit();

    pthread_t can_protocol_proc_handle;
    pthread_create(&can_protocol_proc_handle, NULL, uart_protocol_process,(void*)powerboard);

    sys_powerboard->device = open_com_device(sys_powerboard->dev);
    if(sys_powerboard->device < 0 )
    {
        ROS_ERROR("Open %s Failed !",sys_powerboard->dev);
    }
    else
    {
        set_speed(sys_powerboard->device,9600);
        set_parity(sys_powerboard->device,8,1,'N');  
        ROS_INFO("Open %s OK.",sys_powerboard->dev);
    }
    signal(SIGINT, sigintHandler);

//    powerboard.handle_receive_data(sys_powerboard);
    while(ros::ok())
    {
        cnt++;
        powerboard->handle_receive_data(sys_powerboard);
        if(cnt % 200 == 50)
        {
            
#if 1   //Get battery info test function
            sys_powerboard->bat_info.cmd = 2; 
#endif
        }
        if(cnt % 200 == 150)
        {
        }


        ros::spinOnce();
        loop_rate.sleep();
    }
    //close(fd);
    if(close(sys_powerboard->device) > 0)
    {
       // ROS_INFO("Close 
    }

}
