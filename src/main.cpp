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
extern std::string table_pivas;
extern std::string table_super_rfid_pw;

extern sqlite3*  open_db(void);
extern int create_table(sqlite3 *db);
extern int delete_all_db_data(sqlite3 *db, std::string table);
extern int get_max_uid(sqlite3 *db, std::string table);
extern std::vector<int> get_door_id_by_pw(sqlite3 *db, std::string input_str);
extern std::vector<int> get_door_id_by_rfid(sqlite3 *db, std::string input_str);
extern int insert_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw, int work_id, int door_id);
extern int update_db_by_rfid(sqlite3 *db,std::string table, std::string rfid, std::string pw, int work_id, int door_id);
extern int insert_super_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw);
std::vector<lock_pivas_t> pivas_db_vector;


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
    ros::Rate loop_rate(1);
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
    pthread_t can_protocol_proc_handle;
    pthread_t agent_protocol_proc_handle;
    pthread_create(&can_protocol_proc_handle, NULL, uart_protocol_process,(void*)powerboard);
    pthread_create(&agent_protocol_proc_handle, NULL, agent_protocol_process,(void*)powerboard);
    signal(SIGINT, sigintHandler);

#if 1
    lock_pivas_t lock_pivas_tmp;

    lock_pivas_tmp.rfid = "1040";
    lock_pivas_tmp.password = "1234";
    lock_pivas_tmp.worker_id = 10;
    lock_pivas_tmp.door_id = 1;
    pivas_db_vector.push_back(lock_pivas_tmp);

    lock_pivas_tmp.rfid = "1041";
    lock_pivas_tmp.password = "1235";
    lock_pivas_tmp.worker_id = 11;
    lock_pivas_tmp.door_id = 2;
    pivas_db_vector.push_back(lock_pivas_tmp);

    lock_pivas_tmp.rfid = "1042";
    lock_pivas_tmp.password = "1236";
    lock_pivas_tmp.worker_id = 12;
    lock_pivas_tmp.door_id = 3;
    pivas_db_vector.push_back(lock_pivas_tmp);

    lock_pivas_tmp.rfid = "1043";
    lock_pivas_tmp.password = "1237";
    lock_pivas_tmp.worker_id = 13;
    lock_pivas_tmp.door_id = 4;
    pivas_db_vector.push_back(lock_pivas_tmp);

    lock_pivas_tmp.rfid = "1044";
    lock_pivas_tmp.password = "1238";
    lock_pivas_tmp.worker_id = 14;
    lock_pivas_tmp.door_id = 5;
    pivas_db_vector.push_back(lock_pivas_tmp);

    lock_pivas_tmp.rfid = "1045";
    lock_pivas_tmp.password = "1239";
    lock_pivas_tmp.worker_id = 15;
    lock_pivas_tmp.door_id = 6;
    pivas_db_vector.push_back(lock_pivas_tmp);

    lock_pivas_tmp.rfid = "1046";
    lock_pivas_tmp.password = "1240";
    lock_pivas_tmp.worker_id = 16;
    lock_pivas_tmp.door_id = 7;
    pivas_db_vector.push_back(lock_pivas_tmp);

    lock_pivas_tmp.rfid = "1047";
    lock_pivas_tmp.password = "1241";
    lock_pivas_tmp.worker_id = 17;
    lock_pivas_tmp.door_id = 8;
    pivas_db_vector.push_back(lock_pivas_tmp);
#endif


#if 1   //sqlite test


    sqlite3 *db= open_db();
    std::string sql;
    char *err_msg;
    create_table(db);

    delete_all_db_data(db, table_pivas); 
    delete_all_db_data(db, table_super_rfid_pw); 

    for(std::vector<lock_pivas_t>::iterator it = pivas_db_vector.begin(); it != pivas_db_vector.end(); it++)
    {
        update_db_by_rfid(db, table_pivas, (*it).rfid,(*it).password,(*it).worker_id,(*it).door_id);
    }

    insert_super_into_db(db, table_super_rfid_pw, "9999", "3333");
    sql = "SELECT * FROM PIVAS";
    sqlite3_exec(db,sql.data(),sqlite_test_callback,0,&err_msg);

    //ROS_INFO("max uid: %d",get_max_uid(db,));
    std::vector<int> door_id_pw_test =  get_door_id_by_pw(db, "1234");
    for(std::vector<int>::iterator it = door_id_pw_test.begin(); it != door_id_pw_test.end(); it++)
    {
        ROS_INFO("get door id by password in databases : %d",*it);
    }


    std::vector<int> door_id_rfid_test =  get_door_id_by_rfid(db, "1055");
    for(std::vector<int>::iterator it = door_id_rfid_test.begin(); it != door_id_rfid_test.end(); it++)
    {
        ROS_INFO("get door id by rfid in databases : %d",*it);
    }

    for(int i = 0; i < 20; i++)
    {
        
        update_db_by_rfid(db, table_pivas, std::to_string(1046 + i), "3333", 10, 100);
    }


    update_db_by_rfid(db, table_pivas, "1059","1911",11,101);


    sqlite3_close(db); //关闭数据库

#endif
    //    powerboard.handle_receive_data(sys_powerboard);
    bool init_flag = false;
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
        cnt++;
        //powerboard->handle_receive_data(sys_powerboard);//test 
        //powerboard->unlock(sys_powerboard);//test 
        //powerboard->get_lock_version(sys_powerboard);//test 
        //powerboard->pub_info_to_agent(1,"test");//test
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
