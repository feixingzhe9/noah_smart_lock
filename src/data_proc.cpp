#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/UInt8MultiArray.h" 
#include <math.h>
#include <stdio.h>     
#include <stdlib.h>     
#include <unistd.h>     
#include <sys/types.h>  
#include <sys/stat.h>   
#include <fcntl.h>      
#include <termios.h>   
#include <errno.h>     
#include <string.h>
#include <time.h>
#include <signal.h>
#include "iostream"
#include "stdlib.h"
#include "cstdlib"
#include "string"
#include "sstream"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <time.h>
#include <algorithm>
#include <smart_lock.h>
//#include <vector>
#include <sqlite3.h>


//const std::string table_pivas = "PIVAS";
//const std::string table_super_rfid_pw = "SUPER_RFID_PW";
std::string table_pivas = "PIVAS";
std::string table_super_rfid_pw = "SUPER_RFID_PW";
sqlite3*  open_db(void)
{
    sqlite3 *db=NULL;
    char *zErrMsg = 0;
    int rc;

    //打开指定的数据库文件,如果不存在将创建一个同名的数据库文件
    //rc = sqlite3_open("~/catkin_ws/install/lib/smart_lock/pw_rfid.db", &db); 
    rc = sqlite3_open("pw_rfid.db", &db); 
    if( rc )
    {
        fprintf(stderr, "Can't open database: %s/n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return NULL;
    }
    else printf("You have opened a sqlite3 database named pw_rfid.db successfully!/nCongratulations! Have fun !  ^-^ /n");

    return db;

}
int create_table(sqlite3 *db)
{
    std::string sql;
    char *err_msg;
    //sql = "CREATE TABLE COMPANY("  \
    "ID INT PRIMARY KEY     NOT NULL," \
        "NAME           TEXT    NOT NULL," \
        "AGE            INT     NOT NULL," \
        "ADDRESS        CHAR(50)," \
        "SALARY         REAL );";
    //sql = "CREATE TABLE COMPANY(ID INT PRIMARY KEY NOT NULL,   NAME TEXT NOT NULL,  AGE INT  NOT NULL,  ADDRESS CHAR(50),   SALARY REAL);";
    sql = "CREATE TABLE " + table_pivas + "(UID INT PRIMARY KEY NOT NULL,   RFID TEXT NOT NULL,  PASSWORD TEXT NOT NULL,  WORKER_ID INT NOT NULL, DOOR_ID INT NOT NULL);";
    sqlite3_exec(db,sql.data(),NULL,0,&err_msg);

    sql = "CREATE TABLE " + table_super_rfid_pw + " (UID INT PRIMARY KEY NOT NULL,   RFID TEXT NOT NULL,  PASSWORD TEXT NOT NULL);";
    sqlite3_exec(db,sql.data(),NULL,0,&err_msg);
}
int delete_all_db_data(sqlite3 *db, std::string table)
{
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    char *err_msg;

    sql = "DELETE from " + table +";";
    sqlite3_exec(db,sql.data(),NULL,0,&err_msg);
}
static int sqlite_max_uid_callback(void *max_uid, int argc, char **argv, char **azColName)
{
    ROS_WARN("%s",__func__);
    printf("%s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
    if(!argv[0])
    {
        *(int*)max_uid = 0;
        ROS_ERROR("%s: This db is NULL !",__func__);
    }
    else
    {
        *(int*)max_uid = std::atoi(argv[0]);
    }
    return 0;
}

int get_max_uid(sqlite3 *db, std::string table)
{
    //sqlite3 *db=NULL;
    char *zErrMsg = 0;
    int rc;
    int max_uid;
    //char *sql;
    std::string sql;
    char *err_msg;

    sql = "SELECT max(UID) FROM " + table + ";";
    sqlite3_exec(db,sql.data(),sqlite_max_uid_callback,(void*)(&max_uid),&err_msg);

    return max_uid;
}


static int sqlite_get_door_id_by_pw_callback(void *tmp, int argc, char **argv, char **azColName)
{
    //ROS_INFO("%s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
    if(argv[0] != NULL)
    {
        std::vector<int>& door_id = *reinterpret_cast<std::vector<int>*>(tmp);
        door_id.push_back(std::atoi(argv[0]));
    }
    else
    {
        ROS_ERROR("%s: this door_id is NULL !",__func__);
    }
    return 0;
}
std::vector<int> get_door_id_by_pw(sqlite3 *db, std::string input_str)
{
    char *zErrMsg = 0;
    int rc;
    std::vector<int> door_id;
    std::string sql;
    char *err_msg;
    door_id.clear();

    sql = "select DOOR_ID from PIVAS where PASSWORD == \"" + input_str + "\";";
    ROS_INFO("sql: %s",sql.data());
    sqlite3_exec(db,sql.data(),sqlite_get_door_id_by_pw_callback,(void*)(&door_id),&err_msg);

    //for(std::vector<int>::iterator it = door_id.begin(); it != door_id.end(); it++)
    {
        //ROS_INFO("get door id by pass word in the databases: %d",*it);
    }
    return door_id;
}


static int sqlite_get_door_id_by_rfid_callback(void *tmp, int argc, char **argv, char **azColName)
{
    //ROS_INFO("%s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
    ROS_INFO("%s",__func__);
    if(argv[0] != NULL)
    {
        std::vector<int>& door_id = *reinterpret_cast<std::vector<int>*>(tmp);
        door_id.push_back(std::atoi(argv[0]));
    }
    else
    {
        ROS_ERROR("%s: this door_id is NULL !",__func__);
    }
    return 0;
}
std::vector<int> get_door_id_by_rfid(sqlite3 *db, std::string input_str)
{
    char *zErrMsg = 0;
    int rc;
    std::vector<int> door_id;
    std::string sql;
    char *err_msg;
    door_id.clear();

    sql = "select DOOR_ID from PIVAS where RFID == \"" + input_str + "\";";
    ROS_INFO("sql: %s",sql.data());
    sqlite3_exec(db,sql.data(),sqlite_get_door_id_by_rfid_callback,(void*)(&door_id),&err_msg);

    //for(std::vector<int>::iterator it = door_id.begin(); it != door_id.end(); it++)
    {
        //ROS_INFO("get door id by pass word in the databases: %d",*it);
    }
    return door_id;
}


int insert_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw, int work_id, int door_id)
{
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    char *err_msg;
    int uid = get_max_uid(db, table) + 1;
    std::string uid_str = std::to_string(uid);
    sql = "INSERT INTO " + table + " (UID, RFID, PASSWORD, WORKER_ID, DOOR_ID)  VALUES(" + uid_str+ ", \'" + rfid + "\', \'" + pw + "\', " + std::to_string(work_id) + "," + std::to_string(door_id) + ");"; 
    ROS_INFO("%s: %s",__func__, sql.data());
    sqlite3_exec(db,sql.data(),NULL,0,&err_msg);

}



static int sqlite_update_db_by_rfid_callback(void *tmp, int argc, char **argv, char **azColName)
{
    //ROS_INFO("%s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
    ROS_INFO("%s",__func__);
    *(int*)tmp = 1; 
    return 0;
}
int update_db_by_rfid(sqlite3 *db,  std::string table, std::string rfid, std::string pw, int worker_id, int door_id)
{
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    char *err_msg;
    int uid = get_max_uid(db,table) + 1;
    int is_have_the_rfid = 0;
    std::string uid_str = std::to_string(uid);
    //sql = "INSERT INTO PIVAS (UID, RFID, PASSWORD, WORKER_ID, DOOR_ID)  VALUES(" + uid_str+ ", \'" + rfid + "\', \'" + pw + "\', " + std::to_string(work_id) + "," + std::to_string(door_id) + ");"; 
    sql = "SELECT RFID FROM PIVAS WHERE RFID =  \'" + rfid + "\';";
    ROS_INFO("%s: %s",__func__, sql.data());
    sqlite3_exec(db,sql.data(),sqlite_update_db_by_rfid_callback,(void *)&is_have_the_rfid,&err_msg);
    if(is_have_the_rfid == 1)
    {
        
        //sql = "UPDATE PIVAS SET UID = " + std::to_string(uid) + ", SET RFID = \'" + rfid + "\', SET PASSWORD =  \'" + pw + "\', SET WORK_ID =  " + std::to_string(work_id) + ", SET DOOR_ID = " + std::to_string(door_id) + " WHERE RFID = \'" + rfid + "\';"; 
        //sql = "UPDATE PIVAS  SET PASSWORD =  \'" + pw + "\', SET WORK_ID =  " + std::to_string(work_id) + ", SET DOOR_ID = " + std::to_string(door_id) + " WHERE RFID = \'" + rfid + "\';"; 
        sql = "UPDATE PIVAS SET PASSWORD = \'" + pw + "\' WHERE RFID = \'" + rfid + "\';"; 
        sqlite3_exec(db,sql.data(),NULL,0,&err_msg);

        sql = "UPDATE PIVAS SET WORKER_ID = \'" + std::to_string(worker_id) + "\' WHERE RFID = \'" + rfid + "\';"; 
        sqlite3_exec(db,sql.data(),NULL,0,&err_msg);

        sql = "UPDATE PIVAS SET DOOR_ID = \'" + std::to_string(door_id) + "\' WHERE RFID = \'" + rfid + "\';"; 
        sqlite3_exec(db,sql.data(),NULL,0,&err_msg);

        ROS_INFO("%s, sql: %s",__func__, sql.data());
    }
    else
    {
        ROS_INFO("update db by rfid: have no such rfid, will insert this rfid");
        insert_into_db(db,table, rfid, pw, worker_id, door_id);
    }

}



int insert_super_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw)
{
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    char *err_msg;
    int uid = get_max_uid(db, table) + 1;
    std::string uid_str = std::to_string(uid);
    //sql = "INSERT INTO PIVAS (UID, RFID, PASSWORD, WORKER_ID, DOOR_ID)  VALUES(" + uid_str+ ", \'" + rfid + "\', \'" + pw + "\', " + std::to_string(work_id) + "," + std::to_string(door_id) + ");"; 
    sql = "INSERT INTO " + table + " (UID, RFID, PASSWORD)  VALUES(" + uid_str+ ", \'" + rfid + "\', \'" + pw + "\' );"; 
    ROS_INFO("%s: %s",__func__, sql.data());
    sqlite3_exec(db,sql.data(),NULL,0,&err_msg);

}


static int sqlite_get_table_pivas_to_ram_callback(void *tmp, int argc, char **argv, char **azColName)
{
    ROS_WARN("%s",__func__);
    for(int i = 0; i < argc; i++)
    {
        printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    lock_pivas_t pivas_vec_tmp;
    if(!argv[0])
    {
        ROS_ERROR("%s: This db is NULL !",__func__);
    }
    else
    {
        std::vector<lock_pivas_t>& pivas_vec = *reinterpret_cast<std::vector<lock_pivas_t>*>(tmp);
/*"(UID INT PRIMARY KEY NOT NULL,   RFID TEXT NOT NULL,  PASSWORD TEXT NOT NULL,  WORKER_ID INT NOT NULL, DOOR_ID INT NOT NULL);" */
        pivas_vec_tmp.uid = std::atoi(argv[0]);
        //ROS_ERROR("pivas_vec_tmp.uid = %d", pivas_vec_tmp.uid);
        pivas_vec_tmp.rfid = argv[1];
        //ROS_ERROR("pivas_vec_tmp.rfid = %s", pivas_vec_tmp.rfid.data());
        pivas_vec_tmp.password = argv[2];
        //ROS_ERROR("pivas_vec_tmp.password = %s", pivas_vec_tmp.password.data());
        pivas_vec_tmp.worker_id = std::atoi(argv[3]);
        //ROS_ERROR("pivas_vec_tmp.worker_id = %d", pivas_vec_tmp.worker_id);
        pivas_vec_tmp.door_id = std::atoi(argv[3]);
        //ROS_ERROR("pivas_vec_tmp.door_id = %d", pivas_vec_tmp.door_id);
        pivas_vec.push_back(pivas_vec_tmp);
    }
    return 0;
}
std::vector<lock_pivas_t> get_table_pivas_to_ram(sqlite3 *db, std::string table)
{
    char *zErrMsg = 0;
    int rc;
    std::string sql;
    char *err_msg;
    std::vector<lock_pivas_t> pivas;
    pivas.clear();
    sql = "SELECT * FROM " + table + ";";
    ROS_INFO("%s: %s",__func__, sql.data());
    sqlite3_exec(db,sql.data(),sqlite_get_table_pivas_to_ram_callback,(void*)(&pivas), &err_msg);

    return pivas;

}
