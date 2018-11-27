#ifndef _SMART_LOCK_SMART_LOCK_H
#define _SMART_LOCK_SMART_LOCK_H
#include "ros/ros.h"
#include "std_msgs/String.h"
#include "json.hpp"
#include <sqlite3.h>
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
#define CAN_SOURCE_ID_BEEPER_TIMES_CTRL     0xb0

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


#define COM_ERR_REPEAT_TIME             3

typedef struct
{
    uint8_t lock_id;
    bool status;
}lock_serials_stauts_t;


typedef struct
{
    std::vector<uint8_t> lock_serials;
    std::vector<lock_serials_stauts_t> lock_serials_status;
}smart_lock_t;


extern smart_lock_t    *sys_smart_lock;
class SmartLock
{
    public:
        SmartLock(uint8_t num)
        {
            sub_from_can_node = n.subscribe("can_to_smart_lock", 1000, &SmartLock::rcv_from_can_node_callback, this);

            pub_to_can_node = n.advertise<mrobot_msgs::vci_can>("smart_lock_to_can", 1000);
            locks_status_pub = n.advertise<std_msgs::UInt8MultiArray>("smartlock/locks_state", 10);

            mcu_version.clear();
            //lock_match_db.clear();
            door_num = num;

        }
        int param_init(void);

        int unlock(void);
        int get_lock_version(void);
        int set_super_pw(std::string super_pw);
        int set_super_rfid(std::string super_rfid);
        int beeper_ctrl(uint8_t times, uint8_t duration, uint8_t interval_time, uint8_t freqency);

    private:
        ros::NodeHandle n;
        ros::Subscriber sub_from_can_node;

        ros::Publisher pub_to_can_node;

        ros::Publisher locks_status_pub;

        can_long_frame  long_frame;

        std::string mcu_version;
        const std::string mcu_version_param = "mcu_smart_lock_version";

        json j;
        uint8_t door_num;

        void rcv_from_can_node_callback(const mrobot_msgs::vci_can::ConstPtr &c_msg);

        std::string build_rfid(int rfid_int);
        std::string parse_qr_code(mrobot_msgs::vci_can* msg);

        void pub_locks_status(uint8_t *status, uint8_t lock_num);

        uint8_t map_key_value(uint16_t key);
};

extern bool is_need_update_rfid_pw;

extern  std::vector<int> to_unlock_serials;     //boost::mutex::scoped_lock()

extern std::string super_rfid;
extern std::string super_password;

extern int door_num;
extern int door_loading_unlock_exist_time;

#endif


