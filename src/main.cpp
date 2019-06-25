#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/UInt8MultiArray.h"
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <smart_lock.h>

bool is_need_update_rfid_pw = true;
int door_num;

class SmartLock;

void sigintHandler(int sig)
{
    ROS_INFO("killing on exit");
    ros::shutdown();
}

void get_param(void)
{
    if(ros::param::has("/door_num"))
    {
        ros::param::get("/door_num",door_num);
        if(door_num == 0)
        {
            ROS_ERROR("get door num: %d, but door number CAN NOT be 0 ! using default value: 1", door_num);
            door_num = 1;
        }
        else
        {
            ROS_INFO("get door num: %d",door_num);
        }
    }
    else
    {
        ROS_ERROR("can not find param: /door_num !  door number using default value: 1");
        door_num = 1;//default value
    }
}

int main(int argc, char **argv)
{
    ros::init(argc, argv, "smart_lock_node");
    get_param();

    SmartLock *smart_lock = new SmartLock((uint8_t)door_num);
    ros::Rate loop_rate(20);
    uint32_t cnt = 0;
    smart_lock->param_init();

    signal(SIGINT, sigintHandler);

    bool init_flag = false;
    bool get_agent_info_flag = false;
    sleep(1);
    while(ros::ok())
    {
        if(init_flag == false)
        {
            usleep(100*1000);
            smart_lock->get_lock_version();
            smart_lock->get_doors_state();
            init_flag = true;
            smart_lock->beeper_ctrl(1, 100, 0, 0);
        }
        if(is_need_update_rfid_pw == true)
        {
            static int time_cnt = 0;
            if(time_cnt == 40)
            {
                smart_lock->set_super_pw(super_password);
            }
            if(time_cnt == 80)
            {
                smart_lock->set_super_rfid(super_rfid);
                is_need_update_rfid_pw = false;
                time_cnt = 0;
            }
            time_cnt++;
        }
        cnt++;

        if(!to_unlock_serials.empty())
        {
            usleep(50*1000);
            smart_lock->unlock();
        }

        ros::spinOnce();
        loop_rate.sleep();
    }
}

