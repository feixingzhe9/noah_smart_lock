#ifndef _SMART_LOCK_SMART_LOCK_H
#define _SMART_LOCK_SMART_LOCK_H
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "json.hpp"
#include <sqlite3.h>
//#include <time.h>
#include <mrobot_msgs/vci_can.h>
#include <roscan/can_long_frame.h>


using json = nlohmann::json;


#define SMART_LOCK_CAN_SRC_MAC_ID   0xd6

#define PASSWORD_LEN                4
#define RFID_LEN                    4

#define SUPER_PASSWORD_LEN          PASSWORD_LEN
#define SUPER_RFID_LEN              RFID_LEN

#define CAN_SOURCE_ID_GET_VERSION           0x01

#define CAN_SOURCE_ID_UNLOCK                0x80
#define CAN_SOURCE_ID_LOCK_STATUS           0x81
#define CAN_SOURCE_ID_PW_UPLOAD             0x82
#define CAN_SOURCE_ID_RFID_UPLOAD           0x83

#define CAN_SOURCE_ID_SET_SUPER_PW          0x84
#define CAN_SOURCE_ID_SET_SUPER_PW_ACK      0x84

#define CAN_SOURCE_ID_SET_SUPER_RFID        0x85
#define CAN_SOURCE_ID_SET_SUPER_RFID_ACK    0x85


#define CAN_SOURCE_ID_QR_CODE_UPLOAD_1      0x90
#define CAN_SOURCE_ID_QR_CODE_UPLOAD_2      0x91
#define CAN_SOURCE_ID_QR_CODE_UPLOAD_3      0x92

#define CAN_SOURCE_ID_KEY_TEST_UPLOAD       0xa0
#define CAN_SOURCE_ID_LOCK_STATUS_UPLOAD    0xa1

#define CAN_SOURCE_ID_CAN_LOAD_TEST         0xff


#define KEY_VALUE_B        (1<<0)
#define KEY_VALUE_9        (1<<1)
#define KEY_VALUE_8        (1<<2)
#define KEY_VALUE_7        (1<<3)
#define KEY_VALUE_4        (1<<4)
#define KEY_VALUE_5        (1<<5)
#define KEY_VALUE_6        (1<<6)
#define KEY_VALUE_0        (1<<7)
#define KEY_VALUE_1        (1<<8)
#define KEY_VALUE_2        (1<<9)
#define KEY_VALUE_3        (1<<10)
#define KEY_VALUE_A        (1<<11)

#define TYPE_QR_CODE                1
#define TYPE_RFID_CODE              2
#define TYPE_PASSWORD_CODE          3

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
    std::vector<uint8_t> lock_serials;
    std::vector<lock_serials_stauts_t> lock_serials_status;
}smart_lock_t;


extern smart_lock_t    *sys_smart_lock;
class SmartLock
{
    public:
        SmartLock()
        {
            pub_to_agent = n.advertise<std_msgs::String>("agent_sub",1000);
            sub_from_agent = n.subscribe("agent_pub", 1000, &SmartLock::sub_from_agent_callback, this);

            sub_from_can_node = n.subscribe("can_to_smart_lock", 1000, &SmartLock::rcv_from_can_node_callback, this);

            pub_to_can_node = n.advertise<mrobot_msgs::vci_can>("smart_lock_to_can", 1000);

            mcu_version.clear();
            //lock_match_db.clear();

        }
        int param_init(void);

        int unlock(void);
        int get_lock_version(void);
        int set_super_pw(std::string super_pw);
        int set_super_rfid(std::string super_rfid);
        void pub_info_to_agent(long long uuid, uint8_t type, std::string data, uint8_t status, time_t t);

    private:
        ros::NodeHandle n;
        ros::Publisher pub_to_agent;
        ros::Subscriber sub_from_agent;
        ros::Subscriber sub_from_can_node;
        ros::Publisher pub_to_can_node;

        can_long_frame  long_frame;

        std::string mcu_version;
        const std::string mcu_version_param = "mcu_smart_lock_version";

        json j;

        void pub_json_msg_to_app(const nlohmann::json j_msg);
        void sub_from_agent_callback(const std_msgs::String::ConstPtr &msg);

        void rcv_from_can_node_callback(const mrobot_msgs::vci_can::ConstPtr &c_msg);

        std::string build_rfid(int rfid_int);
        std::string parse_qr_code(mrobot_msgs::vci_can* msg);

        void prepare_to_pub_to_agent( std::string code, uint8_t result, uint8_t type);

        std::vector<int> get_door_id_by_rfid_password(std::string data, uint8_t type, uint8_t *match_result);


        uint8_t map_key_value(uint16_t key);

        lock_pivas_t lock_match_tmp;

};

void *agent_protocol_process(void* arg);

extern const std::string TABLE_PIVAS;
extern const std::string TABLE_SUPER_RFID_PW;
extern sqlite3 *db_;
extern bool is_need_update_rfid_pw;

extern sqlite3*  open_db(void);
extern int create_table(sqlite3 *db);
extern int delete_all_db_data(sqlite3 *db, std::string table);
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


