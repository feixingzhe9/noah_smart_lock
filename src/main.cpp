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
#if 1   //sqlite test

    extern sqlite3*  open_db(void);

    sqlite3 *db= open_db();

    char *sql;
    char *err_msg;
    //sql = "CREATE TABLE COMPANY("  \
           "ID INT PRIMARY KEY     NOT NULL," \
           "NAME           TEXT    NOT NULL," \
           "AGE            INT     NOT NULL," \
           "ADDRESS        CHAR(50)," \
           "SALARY         REAL );";
    //sql = "CREATE TABLE COMPANY(ID INT PRIMARY KEY NOT NULL,   NAME TEXT NOT NULL,  AGE INT  NOT NULL,  ADDRESS CHAR(50),   SALARY REAL);";
    sql = "CREATE TABLE PIVAS(UID INT PRIMARY KEY NOT NULL,   RFID TEXT NOT NULL,  PASSWORD TEXT NOT NULL,  WORKER_ID INT NOT NULL, DOOR_ID INT NOT NULL);";
    sqlite3_exec(db,sql,sqlite_test_callback,0,&err_msg);

    extern int delete_all_db_data(sqlite3 *db);
    //delete_all_db_data(db); 

    sql =   "INSERT INTO PIVAS  (UID,   RFID,   PASSWORD,   WORKER_ID,  DOOR_ID) "  \
                        "VALUES (5,     '1055', '1234',     1023 ,      1   ); "  
            "INSERT INTO PIVAS  (UID,   RFID,   PASSWORD,   WORKER_ID,  DOOR_ID) "  \
                        "VALUES (6,     '1056', '1234',     1024 ,      2   ); " \ 
            "INSERT INTO PIVAS  (UID,   RFID,   PASSWORD,   WORKER_ID,  DOOR_ID) "  \
                        "VALUES (7,     '1055', '1234',     1024 ,      3   ); "  
            "INSERT INTO PIVAS  (UID,   RFID,   PASSWORD,   WORKER_ID,  DOOR_ID) "  \
                        "VALUES (8,     '1055', '1234',     1024 ,      4   ); "; 
          //"INSERT INTO PIVAS    (UID,   RFID,   PASSWORD,   WORD_ID,    DOOR_ID) "  \
                        "VALUES (2,     '1056', '5555',     1024 ,      1   ); ";

    //sql = "DELETE FROM COMPANY WHERE ID = 1";


    sqlite3_exec(db,sql,sqlite_test_callback,0,&err_msg);

    sql = "SELECT * FROM PIVAS";
    sqlite3_exec(db,sql,sqlite_test_callback,0,&err_msg);

    extern int get_max_uid(sqlite3 *db);
    ROS_INFO("max uid: %d",get_max_uid(db));
    extern std::vector<int> get_door_id_by_pw(sqlite3 *db, std::string input_str);
    std::vector<int> door_id_pw_test =  get_door_id_by_pw(db, "1234");
    for(std::vector<int>::iterator it = door_id_pw_test.begin(); it != door_id_pw_test.end(); it++)
    {
        ROS_INFO("get door id by password in databases : %d",*it);
    }


    extern std::vector<int> get_door_id_by_rfid(sqlite3 *db, std::string input_str);
    std::vector<int> door_id_rfid_test =  get_door_id_by_rfid(db, "1055");
    for(std::vector<int>::iterator it = door_id_rfid_test.begin(); it != door_id_rfid_test.end(); it++)
    {
        ROS_INFO("get door id by rfid in databases : %d",*it);
    }

extern int insert_into_db(sqlite3 *db, std::string rfid, std::string pw, int work_id, int door_id);
    for(int i = 0; i < 20; i++)
    {
        
        insert_into_db(db, std::to_string(1041 + i), "3333", 10, 100);
    }


    extern int update_db_by_rfid(sqlite3 *db, std::string rfid, std::string pw, int work_id, int door_id);
    update_db_by_rfid(db, "1059","1111",11,101);



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
