#include "ros/ros.h"
#include "std_msgs/String.h"
#include "json.hpp"
#include <sqlite3.h>
#include <time.h>

using json = nlohmann::json;
#ifndef SMART_LOCK_H
#define SMART_LOCK_H


#define PROTOCOL_HEAD               0xcc
#define PROTOCOL_TAIL               0xa5
#define POWER_CURRENT_LEN           33

#define BUF_LEN                    256


enum
{
    FRAME_TYPE_UNLOCK           =  0x01,
    FRAME_TYPE_LOCK_STATUS      =  0x02,
    FRAME_TYPE_PW_UPLOAD        =  0x03,
    FRAME_TYPE_RFID_UPLOAD      =  0x04,
    FRAME_TYPE_QR_CODE_UPLOAD   =  0x05,
    FRAME_TYPE_SET_SUPER_PW     =  0x06,
    FRAME_TYPE_SET_SUPER_RFID   =  0x07,


    FRAME_TYPE_LOCK_VERSION     =  0x20,

}FRAME_TYPE_E;




enum
{
    DATA_DIRECTION_LOCK_TO_X86 = 1,
    DATA_DIRECTION_LOCK_ACK = 2,
    DATA_DIRECTION_X86_TO_LOCK = 3,
    DATA_DIRECTION_X86_ACK = 4

}DATA_DIRECTION_E;

#define COM_ERR_REPEAT_TIME             3 











typedef struct
{
    uint8_t lock_id;
    bool status;
}lock_serials_stauts_t;


typedef struct
{
    uint8_t lock_id;
    std::string rfid;
    std::string pw;
}lock_match_t;

typedef struct
{
    uint8_t type;
    std::string code;
    uint8_t result;
}pub_to_agent_t;


typedef struct
{
    int uid;
    std::string rfid;
    std::string password;
    int worker_id;
    int door_id;
}lock_pivas_t;


















typedef struct
{
#define DEV_STRING_LEN              50
    char                        dev[DEV_STRING_LEN]; 
    int                         device;

#define SEND_DATA_BUF_LEN           255
    uint8_t                     send_data_buf[SEND_DATA_BUF_LEN];

    std::vector<uint8_t> lock_serials;
    std::vector<lock_serials_stauts_t> lock_serials_status;
}smart_lock_t;

typedef enum 
{
    LIGHTS_MODE_DEFAULT                 = 0,
    LIGHTS_MODE_NOMAL                   = 1,
    LIGHTS_MODE_ERROR                   = 2,
    LIGHTS_MODE_LOW_POWER,
    LIGHTS_MODE_CHARGING,
    LIGHTS_MODE_TURN_LEFT,
    LIGHTS_MODE_TURN_RIGHT,
    LIGHTS_MODE_COM_ERROR,
    LIGHTS_MODE_EMERGENCY_STOP,

    LIGHTS_MODE_SETTING                 = 0xff,
}light_mode_t;

extern smart_lock_t    *sys_smart_lock;
class SmartLock 
{
    public:
        SmartLock()
        {
            pub_to_agent = n.advertise<std_msgs::String>("agent_sub",1000);
            sub_from_agent = n.subscribe("agent_pub", 1000, &SmartLock::sub_from_agent_callback, this);
            //lock_match_db.clear();

        }
        int PowerboardParamInit(void);
        int send_serial_data(smart_lock_t *sys);
        int handle_receive_data(smart_lock_t *sys);


        int unlock(smart_lock_t *smart_lock);  
        int get_lock_version(smart_lock_t *smart_lock);
        int set_super_pw(smart_lock_t *smart_lock);
        int set_super_rfid(smart_lock_t *smart_lock);
        void pub_info_to_agent(long long uuid, uint8_t type, std::string data, uint8_t status, time_t t);

    private:
        uint8_t CalCheckSum(uint8_t *data, uint8_t len);
        int handle_rev_frame(smart_lock_t *sys,unsigned char * frame_buf);
        ros::NodeHandle n;
        ros::Publisher pub_to_agent;
        ros::Subscriber sub_from_agent;
        json j;
        void pub_json_msg_to_app(const nlohmann::json j_msg);
        void sub_from_agent_callback(const std_msgs::String::ConstPtr &msg);

        lock_pivas_t lock_match_tmp;


};
int handle_receive_data(smart_lock_t *sys);
void *uart_protocol_process(void* arg);
void *agent_protocol_process(void* arg);
void set_speed(int fd, int speed);
int set_parity(int fd,int databits,int stopbits,int parity);
int open_com_device(char *dev);


extern std::string table_pivas;
extern std::string table_super_rfid_pw;
extern sqlite3 *db_;
extern bool is_need_update_rfid_pw;

extern sqlite3*  open_db(void);
extern int create_table(sqlite3 *db);
extern int delete_all_db_data(sqlite3 *db, std::string table);
extern int get_max_uid(sqlite3 *db, std::string table);
extern std::vector<int> get_door_id_by_pw(sqlite3 *db, std::string input_str);
extern std::vector<int> get_door_id_by_rfid(sqlite3 *db, std::string input_str);
extern int insert_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw, int work_id, int door_id);
extern int update_db_by_rfid(sqlite3 *db,std::string table, std::string rfid, std::string pw, int work_id, int door_id);
extern int insert_super_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw);
extern std::vector<lock_pivas_t> get_table_pivas_to_ram(sqlite3 *db, std::string table);
extern int update_db_by_door_id(sqlite3 *db,  std::string table, std::string rfid, std::string pw, int worker_id, int door_id);
extern int update_super_into_db(sqlite3 *db, std::string table,std::string rfid, std::string pw);
extern  std::vector<int> to_unlock_serials;     //boost::mutex::scoped_lock()
extern std::string get_table_super_pw_to_ram(sqlite3 *db, std::string table);
extern std::string get_table_super_rfid_to_ram(sqlite3 *db, std::string table);


extern std::vector<lock_pivas_t> lock_match_db_vec;
extern std::string super_rfid;
extern std::string super_password;

#endif


