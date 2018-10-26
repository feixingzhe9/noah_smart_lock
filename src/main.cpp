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
#include <sqlite3.h>

bool is_need_update_rfid_pw = true;
sqlite3 *db_;

class SmartLock;
void sigintHandler(int sig)
{
    ROS_INFO("killing on exit");
    ros::shutdown();
}

static int sqlite_test_callback(void *NotUsed, int argc, char **argv, char **azColName)
{
    ROS_INFO("%s",__func__);
    int i;
    for(i=0; i<argc; i++){
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    printf("\n");
    return 0;
}
int main(int argc, char **argv)
{

    //system("shutdown now");
    //system("echo \'kaka\' | sudo -S sh -c \' shutdown now\'");
    ros::init(argc, argv, "smart_lock_node");
#if 1
    int lock_num = 1;
    if(ros::param::has("/smart_lock/lock_num"))
    {
        ros::param::get("/smart_lock/lock_num", lock_num);
        if(lock_num == 0)
        {
            ROS_ERROR("get door num: %d, but door number CAN NOT be 0 ! using default value: 1", lock_num);
            lock_num = 1;
        }
        else
        {
            ROS_INFO("get door num: %d", lock_num);
        }
    }
    else
    {
        ROS_ERROR("can not find param: /smart_lock/door_num !  door number using default value: 1");
        lock_num = 1;//default value
    }
#endif
    SmartLock *smart_lock = new SmartLock((uint8_t)lock_num);
    ros::Rate loop_rate(20);
    uint32_t cnt = 0;
    smart_lock->param_init();

    pthread_t agent_protocol_proc_handle;
    pthread_create(&agent_protocol_proc_handle, NULL, agent_protocol_process,(void*)smart_lock);
    signal(SIGINT, sigintHandler);

#if 1   //sqlite test


    db_ = open_db();
    std::string sql;
    char *err_msg;
    if(create_table(db_) < 0)
    {
        //ROS_ERROR("create_table ERROR ! !");
        //sqlite3_close(db_);

    }


    //update_super_into_db(db_, table_super_rfid_pw, "1050", "3333");

    sql = "SELECT * FROM PIVAS";
    sqlite3_exec(db_,sql.data(),sqlite_test_callback,0,&err_msg);



    lock_match_db_vec = get_table_pivas_to_ram(db_, TABLE_PIVAS);
    int lock_match_size = lock_match_db_vec.size();
    ROS_INFO("lock_match_db_vec.size = %d",lock_match_size);
    for(std::vector<lock_pivas_t>::iterator it = lock_match_db_vec.begin(); it != lock_match_db_vec.end(); it++)
    {
        ROS_WARN("lock_match_db_vec.uid = %d",          (*it).uid);
        ROS_INFO("lock_match_db_vec.rfid = %s",         (*it).rfid.data());
        ROS_INFO("lock_match_db_vec.password = %s",     (*it).password.data());
        ROS_INFO("lock_match_db_vec.worker_id = %d",    (*it).worker_id);
        ROS_INFO("lock_match_db_vec.door_id = %d",      (*it).door_id);
    }

    super_rfid = get_table_super_rfid_to_ram(db_, TABLE_SUPER_RFID_PW);
    if(super_rfid.size() != 4)
    {
        super_rfid = "1055";
        ROS_ERROR("wrong data in database, using default super rfid: %s",super_rfid.data());
    }
    else
    {
        ROS_INFO("super rfid : %s",super_rfid.data());
    }
    super_password = get_table_super_pw_to_ram(db_, TABLE_SUPER_RFID_PW);
    if(super_password.size() != 4)
    {
        super_password = "1055";
        ROS_ERROR("wrong data in database, using default super password: %s",super_password.data());
    }
    else
    {
        ROS_INFO("super password : %s",super_password.data());
    }
    //sqlite3_close(db_); //关闭数据库

#endif
    bool init_flag = false;
    bool get_agent_info_flag = false;
    sleep(1);
    while(ros::ok())
    {
        if(init_flag == false)
        {
            usleep(100*1000);
            smart_lock->get_lock_version();
            init_flag = true;
            //smart_lock->unlock(0x11111111);
            //smart_lock->set_super_pw("5678");
            //smart_lock->set_super_rfid("1055");

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
        if(get_agent_info_flag == false)
        {

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
    sqlite3_close(db_); //关闭数据库

}
