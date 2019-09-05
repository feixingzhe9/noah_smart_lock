#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/UInt8MultiArray.h"
#include <signal.h>
#include <stdio.h>
#include <vector>
#include <smart_lock.h>

bool is_need_update_rfid_pw = true;
uint8_t door_num;

class SmartLock;

void sigintHandler(int sig)
{
    ROS_INFO("killing on exit");
    ros::shutdown();
}


const std::string PRODUCT_MODEL_CABINET = "cabinet";
uint8_t get_door_num(void)
{
    if(ros::param::has("/product_model"))
    {
        std::string product_model;
        ros::param::get("/product_model", product_model);
        if(product_model.length() > PRODUCT_MODEL_CABINET.length())
        {
            int ret = product_model.compare(0, PRODUCT_MODEL_CABINET.length(), PRODUCT_MODEL_CABINET, 0, PRODUCT_MODEL_CABINET.length());
            if(ret == 0)
            {
                std::string product_model_tmp = product_model;
                if(product_model.length() - PRODUCT_MODEL_CABINET.length() - 1 > 0)
                {
                    ROS_INFO("from cabinet param: %s", product_model_tmp.substr(PRODUCT_MODEL_CABINET.length() + 1, product_model.length() - PRODUCT_MODEL_CABINET.length() - 1).c_str());
                    uint8_t num = atoi(product_model_tmp.substr(PRODUCT_MODEL_CABINET.length() + 1, product_model.length() - PRODUCT_MODEL_CABINET.length() - 1).c_str());
                    if(num > 0)
                    {
                        return num;
                    }
                    else
                    {
                        ROS_ERROR("parameter error: %s", product_model.c_str());
                        return 0;
                    }
                }
                else
                {
                    ROS_ERROR("parameter error: %s", product_model.c_str());
                    return 0;
                }
            }
            else
            {
                ROS_WARN("do not get PRODUCT_MODEL_CABINET, so cabinet num is 0");
                return 0;
            }
        }
        else
        {
            ROS_ERROR("parameter error: %s", product_model.c_str());
            return 0;
        }
    }
    else
    {
        ROS_ERROR("do not have parameter of product_model");
        return 0;
    }

    return 0;
}


int main(int argc, char **argv)
{
    ros::init(argc, argv, "smart_lock_node");
    door_num = get_door_num();
    ROS_INFO("get door num %d", door_num);

    SmartLock *smart_lock = new SmartLock(door_num);
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

