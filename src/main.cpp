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

//#include "../include/smart_lock/sqlite3.h"
//#include <sqlite3.h>

extern  std::vector<int> to_unlock_serials;     //boost::mutex::scoped_lock()
extern std::string get_table_super_pw_to_ram(sqlite3 *db, std::string table);
extern std::string get_table_super_rfid_to_ram(sqlite3 *db, std::string table);
extern int update_super_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw);
sqlite3 *db_;
class NoahPowerboard;
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
    ros::init(argc, argv, "noah_powerboard_node");
    //NoahPowerboard  powerboard;
    NoahPowerboard *powerboard = new NoahPowerboard();
    ros::Rate loop_rate(20);
    uint32_t cnt = 0;
    powerboard->PowerboardParamInit();

#if 1
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
#endif
    //pthread_t can_protocol_proc_handle;
    pthread_t agent_protocol_proc_handle;
    //pthread_create(&can_protocol_proc_handle, NULL, uart_protocol_process,(void*)powerboard);
    pthread_create(&agent_protocol_proc_handle, NULL, agent_protocol_process,(void*)powerboard);
    signal(SIGINT, sigintHandler);



#if 1   //sqlite test


    db_ = open_db();
    std::string sql;
    char *err_msg;
    create_table(db_);


    //update_super_into_db(db_, table_super_rfid_pw, "1050", "3333");

    sql = "SELECT * FROM PIVAS";
    sqlite3_exec(db_,sql.data(),sqlite_test_callback,0,&err_msg);

    //ROS_INFO("max uid: %d",get_max_uid(db_,));
    //std::vector<int> door_id_pw_test =  get_door_id_by_pw(db_, "11");
    //for(std::vector<int>::iterator it = door_id_pw_test.begin(); it != door_id_pw_test.end(); it++)
    {
        //ROS_INFO("get door id by password in databases : %d",*it);
    }


    //std::vector<int> door_id_rfid_test =  get_door_id_by_rfid(db_, "1055");
    //for(std::vector<int>::iterator it = door_id_rfid_test.begin(); it != door_id_rfid_test.end(); it++)
    {
        //ROS_INFO("get door id by rfid in databases : %d",*it);
    }

    //for(int i = 0; i < 7; i++)
    {
        //update_db_by_rfid(db_, table_pivas, std::to_string(1046 + i), "3333", 10, 1);
    }


    //update_db_by_rfid(db_, table_pivas, "1059","1911",11,1);


    lock_match_db_vec = get_table_pivas_to_ram(db_, table_pivas);
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

    super_rfid = get_table_super_rfid_to_ram(db_, table_super_rfid_pw);
    ROS_INFO("super rfid : %s",super_rfid.data());
    super_password = get_table_super_pw_to_ram(db_, table_super_rfid_pw);
    ROS_INFO("super password : %s",super_password.data());
    //sqlite3_close(db_); //关闭数据库

#endif
    bool init_flag = false;
    bool get_agent_info_flag = false;
    while(ros::ok())
    {
        if(init_flag == false)
        {
            usleep(100*1000);
            powerboard->get_lock_version(sys_powerboard);//test 
            init_flag = true;

            sleep(2);
            powerboard->set_super_pw(sys_powerboard);//test 
            sleep(2);
            powerboard->set_super_rfid(sys_powerboard);//test 
        }
        if(get_agent_info_flag == false)
        {
            
        }
        cnt++;
        powerboard->handle_receive_data(sys_powerboard);

        if(!to_unlock_serials.empty())
        {
            usleep(50*1000);
            powerboard->unlock(sys_powerboard);
        }

        ros::spinOnce();
        loop_rate.sleep();
    }
    //close(fd);
    if(close(sys_powerboard->device) > 0)
    {
        // ROS_INFO("Close 
    }
    sqlite3_close(db_); //关闭数据库

}
