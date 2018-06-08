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


static int sqlite_max_uid_callback(void *max_uid, int argc, char **argv, char **azColName)
{
    ROS_WARN("%s",__func__);
    printf("%s = %s\n", azColName[0], argv[0] ? argv[0] : "NULL");
    *(int*)max_uid = std::atoi(argv[0]);
    return 0;
}

int get_max_uid(sqlite3 *db)
{
    //sqlite3 *db=NULL;
    char *zErrMsg = 0;
    int rc;
    int max_uid;
    char *sql;
    char *err_msg;

    sql = "SELECT max(UID) FROM PIVAS";
    sqlite3_exec(db,sql,sqlite_max_uid_callback,(void*)(&max_uid),&err_msg);

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


