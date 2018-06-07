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


    sqlite3 *db=NULL;
    char *zErrMsg = 0;
    int rc;

    //打开指定的数据库文件,如果不存在将创建一个同名的数据库文件
    rc = sqlite3_open("/home/kaka/my_ros/src/smart_lock/src/pw_rfid.db", &db); 
    if( rc )
    {
        fprintf(stderr, "Can't open database: %s/n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return 1;
    }
    else printf("You have opened a sqlite3 database named pw_rfid.db successfully!/nCongratulations! Have fun !  ^-^ /n");
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

    sql =   "INSERT INTO PIVAS  (UID,   RFID,   PASSWORD,   WORKER_ID,  DOOR_ID) "  \
                        "VALUES (5,     '1055', '1234',     1023 ,      1   ); "  ;
            "INSERT INTO PIVAS  (UID,   RFID,   PASSWORD,   WORKER_ID,  DOOR_ID) "  \
                        "VALUES (3,     '1055', '1234',     1024 ,      1   ); "  ;
          //"INSERT INTO PIVAS    (UID,   RFID,   PASSWORD,   WORD_ID,    DOOR_ID) "  \
                        "VALUES (2,     '1056', '5555',     1024 ,      1   ); ";

    //sql = "DELETE FROM COMPANY WHERE ID = 1";


    sqlite3_exec(db,sql,sqlite_test_callback,0,&err_msg);

    sql = "SELECT * FROM PIVAS";
    sqlite3_exec(db,sql,sqlite_test_callback,0,&err_msg);

    extern int get_max_uid(sqlite3 *db);
    ROS_INFO("max uid: %d",get_max_uid(db));


    sqlite3_close(db); //关闭数据库

#endif
    //    powerboard.handle_receive_data(sys_powerboard);
    bool init_flag = false;
    while(ros::ok())
    {
        if(init_flag == false)
        {
            sleep(0.1);
            powerboard->get_lock_version(sys_powerboard);//test 
            init_flag = true;

            sleep(1);
            powerboard->set_super_pw(sys_powerboard);//test 
            sleep(1);
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
