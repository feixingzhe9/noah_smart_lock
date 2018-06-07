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


