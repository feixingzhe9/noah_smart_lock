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
#include <algorithm>
#include <smart_lock.h>

//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>

//#include <uuid/uuid.h>

#define TEST_WAIT_TIME     300*1000

#define PowerboardInfo     ROS_INFO



boost::mutex mtx_agent;
//boost::mutex mtx_smart_lock;
static int led_over_time_flag = 0;
static int last_unread_bytes = 0;

smart_lock_t    sys_smart_lock_ram;
smart_lock_t    *sys_smart_lock = &sys_smart_lock_ram;

std::vector<lock_serials_stauts_t> lock_serials_status;
std::vector<lock_serials_stauts_t> lock_serials_status_ack;
std::vector<lock_serials_stauts_t> lock_serials_status_input;
std::vector<std::string> input_pw;
std::vector<std::string> input_rfid;
std::vector<std::string> input_qr_code;

std::string lock_version;

std::vector<pub_to_agent_t> pub_to_agent_vector(0);	//boost::mutex::scoped_lock()
//std::vector<uint8_t> lock_serials_vector;	//boost::mutex::scoped_lock()
std::vector<int> to_unlock_serials(0);     //boost::mutex::scoped_lock()

std::string to_set_super_pw = "6666";
std::string to_set_super_rfid =  "0000";

std::string set_super_pw_ack;
std::string set_super_rfid_ack;

std::vector<lock_pivas_t> lock_match_db_vec;
std::string super_rfid = "1050";
std::string super_password = "1050";


std::vector<loading_t> to_unlock_load(0);

void SmartLock::pub_info_to_agent(long long uuid, uint8_t type, std::string data, uint8_t status, time_t t)
{
    json j;
    //time_t t;
    struct tm *my_tm;
    struct tm *t2;
    char buf[128] = {0};
#if 0
    time(&t);

    t2 =    localtime(&t);
    sprintf(buf, "%04d%02d%02d  %02d:%02d:%02d", t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday, t2->tm_hour, t2->tm_min, t2->tm_sec);
    ROS_INFO("%s\n", buf);
#else

#endif
    std::string uuid_str = std::to_string(uuid);
    j.clear();
    j =
    {
        {"uuid",uuid_str.c_str()},
        {"sub_name","smart_lock_notice"},
        {
            "data",
            {
                {"type", type},

                {"code",data.c_str()},
                {"time", t * 1000},
                {"result", status},
                //{"array",{1,2,3,4,5,6}},
            }
        }

    };

    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data = ss.str();
    pub_to_agent.publish(pub_json_msg);
}

int SmartLock::param_init(void)
{
    sys_smart_lock->lock_serials.clear();
    sys_smart_lock->lock_serials.push_back(1);
    return 0;
}

#define SEND_TO_AGENT_CNT           3
#define SEND_TO_AGENT_PERIOD        500    //unit: ms
#define AGENT_THREAD_HZ             50    //unit: ms
void *agent_protocol_process(void* arg)
{
    SmartLock *pSmartLock =  (SmartLock*)arg;
    while(ros::ok())
    {
        static uint8_t type;
        static uint8_t result;
        static std::string code;
        static long long uuid = 0;
        static time_t t;
        static bool is_need_to_pub = false;
        static bool is_pub_complete = true;
        do
        {
            boost::mutex::scoped_lock(mtx_agent);
            if(!pub_to_agent_vector.empty())
            {
                if(is_pub_complete == true)
                {
                    auto a = pub_to_agent_vector.begin();
                    type = (*a).type;
                    result =(*a).result;
                    code = (*a).code;
                    time(&t);
                    uuid++;
                    pub_to_agent_vector.erase(a);
                    is_need_to_pub = true;
                    is_pub_complete = false;
                }
            }
        }while(0);

        if(is_need_to_pub == true)
        {
            static int time_cnt = 0;
            static int send_cnt = 0;

            if(time_cnt % ((SEND_TO_AGENT_PERIOD*AGENT_THREAD_HZ) / 1000) == 0)
            {
                pSmartLock->pub_info_to_agent(uuid, type, code, result,t);
                send_cnt++;
                if(send_cnt >= SEND_TO_AGENT_CNT)
                {
                    is_need_to_pub = false;
                    is_pub_complete = true;
                    send_cnt = 0;
                    time_cnt = (SEND_TO_AGENT_PERIOD*AGENT_THREAD_HZ) / 1000 - 1;
                }
            }
            time_cnt++;
        }

        usleep((1000/AGENT_THREAD_HZ) * 1000);
    }
}

void SmartLock::sub_from_agent_callback(const std_msgs::String::ConstPtr &msg)
{
    auto j = json::parse(msg->data.c_str());
    std::string uuid_str;
    if(j.find("uuid") != j.end())
    {
        uuid_str = j["uuid"];
    }
    if(j.find("pub_name") != j.end())
    {
        if(j["pub_name"] == "container_super_password")
        {
            ROS_WARN("get container_super_password");
            if(j.find("passwords") != j.end()/* && j["passwords"].find("passwords") != j["password"].end()*/)
            {
                ROS_WARN("get passwords");
#if 0   //get super password and rfid num: for reserve

                auto array_json = j["passwords"];
                int array_json_size = array_json.size();
                ROS_ERROR("array_json size: %d",array_json_size);
                if(array_json_size > 0)
                {
                    for(int n = 0; n < array_json_size; n++)
                    {
                    }
                }
                else
                {
                }
                auto j_array = j["passwords"];
                std::string j_str = j.dump();
                ROS_ERROR("%s",j_str.data());
#endif
                int box_num = j["passwords"][0]["boxNum"];
                std::string password = j["passwords"][0]["password"];
                std::string rfid = password;

                is_need_update_rfid_pw = true;
                ROS_WARN("get box num : %d, password is %s", box_num, password.data());
                if(update_super_into_db(db_, TABLE_SUPER_RFID_PW, rfid, password) < 0)
                {
                    ROS_ERROR("%s: update_super_into_db ERROR ! !",__func__);
                    is_need_update_rfid_pw = false;
                }

                rfid = get_table_super_rfid_to_ram(db_, TABLE_SUPER_RFID_PW); //need to add mutex
                if(rfid.size() != RFID_LEN)
                {
                    ROS_ERROR("%s: get wrong rfid: %s",__func__, rfid.data());
                    is_need_update_rfid_pw = false;
                }
                else
                {
                    super_rfid = rfid;
                }

                password = get_table_super_pw_to_ram(db_, TABLE_SUPER_RFID_PW); //need to add mutex
                if(password.size() != PASSWORD_LEN)
                {
                    ROS_ERROR("%s: get wrong password: %s",__func__, password.data());
                    is_need_update_rfid_pw = false;
                }
                else
                {
                    super_password = password;
                }
                json j_ack;
                j_ack.clear();
                j_ack =     //ack operation successfull

                {
                    {"uuid",uuid_str.c_str()},
                    {"sub_name","container_super_password"},

                    {
                        "data",
                        {
                            {"result", 1},
                        }
                    }

                };
                std_msgs::String pub_json_msg;
                std::stringstream ss;
                ss.clear();
                ss << j_ack;
                pub_json_msg.data = ss.str();
                pub_to_agent.publish(pub_json_msg);
            }
            else
            {
                std::string  j_str = j.dump();
                ROS_ERROR("Parse Json data ERROR  : %s ",j_str.data());

                json j_ack;
                j_ack.clear();
                j_ack =
                {
                    {"sub_name","container_super_password"},

                    {
                        "data",
                        {
                            {"result", 0},
                        }
                    }

                };

                std_msgs::String pub_json_msg;
                std::stringstream ss;
                ss.clear();
                ss << j_ack;
                pub_json_msg.data = ss.str();
                pub_to_agent.publish(pub_json_msg);
            }

        }
        if(j["pub_name"] == "binding_credit_card_employees")
        {
            std::string j_str = j.dump();
            ROS_ERROR("%s",j_str.data());

            ROS_INFO("get binding_credit_card_employees . . .");
            if(delete_all_db_data(db_, TABLE_PIVAS) < 0)
            {
                ROS_ERROR("delete_all_db_data ERROR ! !");
            }

            bool get_door_id = false;
            int id_type = 0;
            for(int i = 1; i < 33; i++)
            {
                if(j["data_load"].find(std::to_string(i)) != j["data_load"].end())
                {
                    get_door_id = true;
                    std::vector<std::string> worker_id_vec = j["data_load"][std::to_string(i)];
                    std::string worker_id;
                    ROS_INFO(" get door id : %d ",i);
                    for(std::vector<std::string>::iterator it = worker_id_vec.begin(); it != worker_id_vec.end(); it++)
                    {
                        worker_id = (*it);
                        ROS_INFO("worker id : %s",worker_id.c_str());

                        std::string rfid = worker_id;
                        std::string password = worker_id;
                        int int_worker_id = std::atoi(worker_id.c_str());

                        //update_db_by_door_id(db_, table_pivas, rfid, password,  worker_id, i);
                        if(insert_into_db(db_, TABLE_PIVAS,rfid, password, int_worker_id, i, ID_TYPE_LOADING) < 0)
                        {
                            ROS_ERROR("%s: insert_into_db ERROR ! !",__func__);
                        }
                    }
                    lock_match_db_vec.clear();
                    lock_match_db_vec = get_table_pivas_to_ram(db_, TABLE_PIVAS);       //need to add mutex
                }

                if(j["data_unload"].find(std::to_string(i)) != j["data_unload"].end())
                {
                    get_door_id = true;
                    std::vector<std::string> worker_id_vec = j["data_unload"][std::to_string(i)];
                    std::string worker_id;
                    ROS_INFO(" get door id : %d ",i);
                    for(std::vector<std::string>::iterator it = worker_id_vec.begin(); it != worker_id_vec.end(); it++)
                    {
                        worker_id = (*it);
                        ROS_INFO("worker id : %s",worker_id.c_str());

                        std::string rfid = worker_id;
                        std::string password = worker_id;
                        int int_worker_id = std::atoi(worker_id.c_str());

                        //update_db_by_door_id(db_, table_pivas, rfid, password,  worker_id, i);
                        if(insert_into_db(db_, TABLE_PIVAS,rfid, password, int_worker_id, i, ID_TYPE_UNLOADING) < 0)
                        {
                            ROS_ERROR("%s: insert_into_db ERROR ! !",__func__);
                        }
                    }
                    lock_match_db_vec.clear();
                    lock_match_db_vec = get_table_pivas_to_ram(db_, TABLE_PIVAS);       //need to add mutex
                }
            }
            if(get_door_id == true)
            {
                time_t t;
                time(&t);
                json j_ack;
                j_ack.clear();
                j_ack =
                {
                    {"uuid",uuid_str.c_str()},
                    {"sub_name","binding_credit_card_employees"},

                    {
                        "data",
                        {
                            {"result", 1},
                        }
                    }

                };

                std_msgs::String pub_json_msg;
                std::stringstream ss;
                ss.clear();
                ss << j_ack;
                pub_json_msg.data = ss.str();
                pub_to_agent.publish(pub_json_msg);
            }
            else
            {
                json j_ack;
                j_ack.clear();
                j_ack =     // operation failed
                {
                    {"uuid",uuid_str.c_str()},
                    {"sub_name","binding_credit_card_employees"},

                    {
                        "data",
                        {
                            {"result", 0},
                        }
                    }

                };
                std_msgs::String pub_json_msg;
                std::stringstream ss;
                ss.clear();
                ss << j_ack;
                pub_json_msg.data = ss.str();
                pub_to_agent.publish(pub_json_msg);
            }
        }

    }
}


int SmartLock::unlock(void)     // done
{
    int error = 0;
    uint32_t lock_bit = 0;
    for(std::vector<int>::iterator it = to_unlock_serials.begin(); it != to_unlock_serials.end(); it++)
    {
        if((*it) <= 32)
        {
            lock_bit |= 1<<((*it) - 1);
        }
    }
    to_unlock_serials.clear();

    mrobot_driver_msgs::vci_can can_msg;
    CAN_ID_UNION id;
    memset(&id, 0x0, sizeof(CAN_ID_UNION));
    id.CanID_Struct.SourceID = CAN_SOURCE_ID_UNLOCK;
    id.CanID_Struct.SrcMACID = 0;
    id.CanID_Struct.DestMACID = SMART_LOCK_CAN_SRC_MAC_ID;
    id.CanID_Struct.FUNC_ID = 0x02;
    id.CanID_Struct.ACK = 0;
    id.CanID_Struct.res = 0;

    can_msg.ID = id.CANx_ID;
    can_msg.DataLen = 5;
    can_msg.Data.resize(5);
    can_msg.Data[0] = 0x00;

    can_msg.Data[1] = (uint8_t)(lock_bit) & 0xff;
    can_msg.Data[2] = (uint8_t)(lock_bit>>8) & 0xff ;
    can_msg.Data[3] = (uint8_t)(lock_bit>>16) & 0xff;
    can_msg.Data[4] = (uint8_t)(lock_bit>>24) & 0xff;
    //*(uint32_t*)&can_msg.Data[1] = to_unlock;

    this->pub_to_can_node.publish(can_msg);


    ROS_INFO("%s", __func__);
    for(int i = 0; i < 5; i++)
    {
        ROS_INFO("data[%d] : 0x%x", i, can_msg.Data[i]);
    }

    return error;
}


int SmartLock::set_super_pw(std::string super_pw)
{
    int error = -1;
    if(super_pw.size() == SUPER_PASSWORD_LEN)
    {
        ROS_WARN("start to set super pass word ...");

        mrobot_driver_msgs::vci_can can_msg;
        CAN_ID_UNION id;
        memset(&id, 0x0, sizeof(CAN_ID_UNION));
        id.CanID_Struct.SourceID = CAN_SOURCE_ID_SET_SUPER_PW;
        id.CanID_Struct.SrcMACID = 0;
        id.CanID_Struct.DestMACID = SMART_LOCK_CAN_SRC_MAC_ID;
        id.CanID_Struct.FUNC_ID = 0x02;
        id.CanID_Struct.ACK = 0;
        id.CanID_Struct.res = 0;

        can_msg.ID = id.CANx_ID;
        can_msg.DataLen = 5;
        can_msg.Data.resize(5);

        can_msg.Data[0] = 0x00;
        can_msg.Data[1]  = super_pw.c_str()[0];
        can_msg.Data[2]  = super_pw.c_str()[1];
        can_msg.Data[3]  = super_pw.c_str()[2];
        can_msg.Data[4]  = super_pw.c_str()[3];

        this->pub_to_can_node.publish(can_msg);
    }

    return error;
}

int SmartLock::set_super_rfid(std::string super_rfid)
{
    int error = -1;
    if(super_rfid.size() == SUPER_RFID_LEN)
    {
        ROS_WARN("start to set super pass word ...");

        mrobot_driver_msgs::vci_can can_msg;
        CAN_ID_UNION id;
        memset(&id, 0x0, sizeof(CAN_ID_UNION));
        id.CanID_Struct.SourceID = CAN_SOURCE_ID_SET_SUPER_RFID;
        id.CanID_Struct.SrcMACID = 0;
        id.CanID_Struct.DestMACID = SMART_LOCK_CAN_SRC_MAC_ID;
        id.CanID_Struct.FUNC_ID = 0x02;
        id.CanID_Struct.ACK = 0;
        id.CanID_Struct.res = 0;

        can_msg.ID = id.CANx_ID;
        can_msg.DataLen = 5;
        can_msg.Data.resize(5);

        can_msg.Data[0] = 0x00;
        can_msg.Data[1]  = super_rfid.c_str()[0];
        can_msg.Data[2]  = super_rfid.c_str()[1];
        can_msg.Data[3]  = super_rfid.c_str()[2];
        can_msg.Data[4]  = super_rfid.c_str()[3];

        this->pub_to_can_node.publish(can_msg);
    }

    return error;
}

int SmartLock::get_lock_version(void)
{
    int error = -1;

    mrobot_driver_msgs::vci_can can_msg;
    CAN_ID_UNION id;
    memset(&id, 0x0, sizeof(CAN_ID_UNION));
    id.CanID_Struct.SourceID = CAN_SOURCE_ID_GET_VERSION;
    id.CanID_Struct.SrcMACID = 0;
    id.CanID_Struct.DestMACID = SMART_LOCK_CAN_SRC_MAC_ID;
    id.CanID_Struct.FUNC_ID = 0x02;
    id.CanID_Struct.ACK = 0;
    id.CanID_Struct.res = 0;

    can_msg.ID = id.CANx_ID;
    can_msg.DataLen = 1;
    can_msg.Data.resize(1);
    can_msg.Data[0] = 0x00;

    this->pub_to_can_node.publish(can_msg);

    return error;
}


void SmartLock::pub_json_msg_to_app( const nlohmann::json j_msg)
{
    std_msgs::String pub_json_msg;
    std::stringstream ss;

    ss.clear();
    ss << j_msg;
    pub_json_msg.data = ss.str();
    //this->noah_smart_lock_pub.publish(pub_json_msg);
}

uint8_t SmartLock::map_key_value(uint16_t key_value)
{
    uint8_t key_true_value = 0;

    if(key_value & KEY_VALUE_0)
    {
        key_true_value = '0';
    }
    else if(key_value & KEY_VALUE_1)
    {
        key_true_value = '1';
    }
    else if(key_value & KEY_VALUE_2)
    {
        key_true_value = '2';
    }
    else if(key_value & KEY_VALUE_3)
    {
        key_true_value = '3';
    }
    else if(key_value & KEY_VALUE_4)
    {
        key_true_value = '4';
    }
    else if(key_value & KEY_VALUE_5)
    {
        key_true_value = '5';
    }
    else if(key_value & KEY_VALUE_6)
    {
        key_true_value = '6';
    }
    else if(key_value & KEY_VALUE_7)
    {
        key_true_value = '7';
    }
    else if(key_value & KEY_VALUE_8)
    {
        key_true_value = '8';
    }
    else if(key_value & KEY_VALUE_9)
    {
        key_true_value = '9';
    }
    else if(key_value & KEY_VALUE_A)
    {
        key_true_value = 'a';
    }
    else if(key_value & KEY_VALUE_B)
    {
        key_true_value = 'b';
    }
    else
    {
        return 0;
    }

    return key_true_value;
}

std::string SmartLock::build_rfid(int rfid_int)
{
    std::string rfid;
    if(rfid_int >= 1000)
    {
        rfid = std::to_string(rfid_int);
    }
    else if(rfid_int >= 100)
    {
        std::string rfid_1 = "0";
        std::string rfid_2_4 = std::to_string(rfid_int);
        rfid = rfid_1 + rfid_2_4;
    }
    else if(rfid_int >= 10)
    {
        std::string rfid_1_2 = "00";
        std::string rfid_3_4 = std::to_string(rfid_int);
        rfid = rfid_1_2 + rfid_3_4;
    }
    else
    {
        std::string rfid_1_3 = "000";
        std::string rfid_4 = std::to_string(rfid_int);
        rfid = rfid_1_3 + rfid_4;
    }
    return rfid;
}

std::string SmartLock::parse_qr_code(mrobot_driver_msgs::vci_can* msg)
{
    std::string qr_code;
    qr_code.clear();

    for(uint8_t i = 0; i < msg->DataLen - 2; i++)
    {
        qr_code.push_back(msg->Data[i]);
    }

    uint8_t last1 = msg->Data[msg->DataLen - 1];
    uint8_t last2 = msg->Data[msg->DataLen - 2];

    if( last2!= 0x0d)   // CRLF
    {
        qr_code.push_back(last2);
    }
    else
    {
        ROS_ERROR("get CRLF in end of QR code");
        return qr_code;
    }

    if( (last1 != 0x09) && (last1 != 0x0a) && (last1 != 0x0d) ) //TAB CR LF
    {
        qr_code.push_back(last1);
    }
    else
    {
        ROS_ERROR("get %d in end of QR code", last1);
    }

    return qr_code;
}

std::vector<int> SmartLock::get_door_id_by_rfid_password(std::string data, uint8_t type, std::string *code, uint8_t *match_result, int * id_type)
{
    std::vector<int> to_unlock_tmp;
    to_unlock_tmp.clear();

    for(std::vector<lock_pivas_t>::iterator it = lock_match_db_vec.begin(); it != lock_match_db_vec.end(); it++)
    {
        if(type == TYPE_RFID_CODE)
        {
            if((*it).rfid == data)
            {
                ROS_INFO("get right RFID  ID");
                *code = (*it).password;
                *id_type = (*it).id_type;
                to_unlock_tmp.push_back((*it).door_id);
                *match_result = 0;
                break;
            }
        }
        else if(type == TYPE_PASSWORD_CODE)
        {
            if((*it).password == data)
            {
                ROS_INFO("get right password");
                *code = (*it).rfid;
                *id_type = (*it).id_type;
                to_unlock_tmp.push_back((*it).door_id);
                *match_result = 0;
                break;
            }
        }
    }

    std::sort(to_unlock_tmp.begin(),to_unlock_tmp.end());
    to_unlock_tmp.erase(unique(to_unlock_tmp.begin(), to_unlock_tmp.end()), to_unlock_tmp.end());

    return to_unlock_tmp;
}

void SmartLock::start_to_pub_to_agent( std::string code, uint8_t result, uint8_t type)
{
    pub_to_agent_t pub_to_agent;
    pub_to_agent.code = code;
    pub_to_agent.type = type;
    pub_to_agent.result = result;
    do
    {
        boost::mutex::scoped_lock(mtx_agent);
        pub_to_agent_vector.push_back(pub_to_agent);
    }while(0);

}

void SmartLock::rcv_from_can_node_callback(const mrobot_driver_msgs::vci_can::ConstPtr &c_msg)
{
    mrobot_driver_msgs::vci_can can_msg;
    mrobot_driver_msgs::vci_can long_msg;
    CAN_ID_UNION id;

    long_msg = this->long_frame.frame_construct(c_msg);
    mrobot_driver_msgs::vci_can* msg = &long_msg;
    if( msg->ID == 0 )
    {
        return;
    }

    can_msg.ID = msg->ID;
    id.CANx_ID = can_msg.ID;
    //can_msg.DataLen = msg->DataLen;
    if(id.CanID_Struct.SrcMACID != SMART_LOCK_CAN_SRC_MAC_ID)
    {
        return ;
    }

    //can_msg.Data.resize(can_msg.DataLen);

    uint8_t source_id = id.CanID_Struct.SourceID;
    uint8_t data_len = msg->DataLen;

    ROS_INFO("%s", __func__);
    ROS_INFO("source id: 0x%x",source_id);
    ROS_INFO("data length: %d",data_len);

    switch(source_id)
    {
        case CAN_SOURCE_ID_UNLOCK:
            ROS_INFO("get unlock ack from mcu");
            break;

        case CAN_SOURCE_ID_RFID_UPLOAD:
            ROS_INFO("get rfid upload info from mcu");
            if(data_len == RFID_LEN)
            {
                std::string rfid;
                uint8_t status = 1;
                int id_type = 0;
                uint16_t rfid_int = 0;
                std::vector<int> to_unlock_serials_tmp;
                rfid.resize(RFID_LEN);
                rfid.clear();
                rfid_int = msg->Data[2];
                rfid_int = rfid_int << 8;
                rfid_int += msg->Data[3];

                ROS_INFO("rfid int data :%d",rfid_int);
                rfid = this->build_rfid(rfid_int);
                ROS_INFO("receive RFID: %s",rfid.c_str());
                input_rfid.push_back(rfid);
                std::string password;
                password.clear();
                to_unlock_serials_tmp = get_door_id_by_rfid_password(rfid, TYPE_RFID_CODE, &password, &status, &id_type);
                ROS_INFO("get password: %s  by rfid : %s",password.c_str(), rfid.c_str());
                if(id_type == ID_TYPE_UNLOADING)
                {
                    to_unlock_serials = to_unlock_serials_tmp;
                }
                else if(id_type == ID_TYPE_LOADING)
                {
                    bool flag = false;
                    for(std::vector<loading_t>::iterator it = to_unlock_load.begin(); it != to_unlock_load.end(); it++)
                    {
                        if(rfid == (*it).rfid)
                        {
                            (*it).cnt++;
                            to_unlock_serials.clear();
                            to_unlock_serials.push_back((*it).cnt % 3 + 1);
                            ROS_WARN("start to unlock %d", (*it).cnt % 3 + 1);
                            flag = true;
                            break;
                        }
                    }

                    if(flag == false)
                    {
                        loading_t load_tmp;
                        load_tmp.cnt = 0;
                        load_tmp.rfid = rfid;
                        load_tmp.password = password;
                        to_unlock_load.push_back(load_tmp);
                    }

                }

                if(rfid == super_rfid)
                {
                    status = 0;
                    ROS_WARN("get right super RFID");
                }

                start_to_pub_to_agent(rfid, status, TYPE_RFID_CODE);
            }

            break;

        case CAN_SOURCE_ID_PW_UPLOAD:
            {
                if(data_len == PASSWORD_LEN)
                {
                    ROS_INFO("get password upload");
                    std::string pw;
                    uint8_t status = 1;
                    int id_type = 0;
                    pw.resize(PASSWORD_LEN);
                    pw.clear();
                    for(uint8_t i = 0; i < PASSWORD_LEN; i++)
                    {
                        pw.push_back(msg->Data[i]);
                    }
                    ROS_INFO("receive pass word: %s",pw.data());
                    input_pw.push_back(pw);
                    std::string rfid;
                    rfid.clear();

                    std::vector<int> to_unlock_serials_tmp;
                    to_unlock_serials = get_door_id_by_rfid_password(pw, TYPE_PASSWORD_CODE,&rfid,  &status, &id_type);
                    ROS_INFO("id_type : %d",id_type);
                    ROS_INFO("get rfid: %s  by password : %s",pw.c_str(), rfid.c_str());
                    if(id_type == ID_TYPE_UNLOADING)
                    {
                        to_unlock_serials = to_unlock_serials_tmp;
                    }
                    else if(id_type == ID_TYPE_LOADING)
                    {
                        bool flag = false;
                        for(std::vector<loading_t>::iterator it = to_unlock_load.begin(); it != to_unlock_load.end(); it++)
                        {
                            if(rfid == (*it).password)
                            {
                                (*it).cnt++;
                                to_unlock_serials.clear();
                                to_unlock_serials.push_back((*it).cnt % 3 + 1);
                                ROS_WARN("start to unlock %d", (*it).cnt % 3 + 1);
                                flag = true;
                                break;
                            }
                        }

                        if(flag == false)
                        {
                            loading_t load_tmp;
                            load_tmp.cnt = 0;
                            load_tmp.rfid = rfid;
                            load_tmp.password = pw;
                            to_unlock_load.push_back(load_tmp);
                        }

                    }

                    if(pw == super_password)
                    {
                        status = 0;
                        ROS_WARN("get right super password");
                    }

                    start_to_pub_to_agent(pw, status, TYPE_PASSWORD_CODE);
                }
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_1:
            {
                ROS_INFO("get upload QR 1  code info.");
                std::string qr_code;
                qr_code.clear();

                qr_code = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.c_str());
                //input_qr_code.push_back(qr_code);

                start_to_pub_to_agent(qr_code, 0, TYPE_QR_CODE);
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_2:
            {
                ROS_INFO("get upload QR 2  code info.");
                std::string qr_code;
                qr_code.clear();

                qr_code = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.c_str());
                //input_qr_code.push_back(qr_code);

                start_to_pub_to_agent(qr_code, 0, TYPE_QR_CODE);
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_3:
            {
                ROS_INFO("get upload QR 3  code info.");
                std::string qr_code;
                qr_code.clear();

                qr_code = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.c_str());
                //input_qr_code.push_back(qr_code);

                start_to_pub_to_agent(qr_code, 0, TYPE_QR_CODE);
            }
            break;

        case CAN_SOURCE_ID_SET_SUPER_PW_ACK:
            {
                ROS_INFO("get ack of set super password from mcu");
                std::string super_password_ack;
                super_password_ack.clear();
                if(PASSWORD_LEN == data_len)
                {
                    for(int i = 0; i < PASSWORD_LEN; i++)
                    {
                        super_password_ack.push_back(msg->Data[i]);
                    }
                    ROS_INFO("get ack super password: %s", super_password_ack.c_str());
                }
                break;
            }

        case CAN_SOURCE_ID_SET_SUPER_RFID_ACK:
            {
                ROS_INFO("get ack of set super rfid from mcu");
                std::string super_rfid_ack;
                super_rfid_ack.clear();
                if(RFID_LEN == data_len)
                {
                    for(int i = 0; i < RFID_LEN; i++)
                    {
                        super_rfid_ack.push_back(msg->Data[i]);
                    }
                    ROS_INFO("get ack super rfid: %s", super_rfid_ack.c_str());
                }
                break;
            }

        case CAN_SOURCE_ID_GET_VERSION:
            {
                ROS_INFO("get mcu version info. ");
                for(int i = 0; i < data_len; i++)
                {
                    this->mcu_version.push_back(msg->Data[i]);
                }
                n.setParam(mcu_version_param, this->mcu_version.c_str());
                ROS_INFO("get lock mcu version : %s",this->mcu_version.c_str());
                break;
            }

        case CAN_SOURCE_ID_KEY_TEST_UPLOAD:
            {
                ROS_INFO("get key test value upload. ");
                ROS_INFO("get key test value upload. data len is %d ", data_len);

                if( 2 == data_len)
                {
                    uint16_t key = *(uint16_t*)&msg->Data[0];
                    ROS_INFO("get key test value:  0x%x", key);
                    char key_value = map_key_value(key);
                    ROS_INFO("get key: %c",key_value);
                }
                break;
            }

        case CAN_SOURCE_ID_CAN_LOAD_TEST:
           {
               static uint32_t real_cnt = 0;
               static uint32_t last_cnt = 0;
               static uint32_t lost_cnt = 0;
               static ros::Time start_time;
               std::string test_str;
               uint32_t cnt = *(uint32_t*)&msg->Data[0];
               if(0 == cnt)
               {
                   ROS_INFO("start to CAN bus load test . . . ");
                   real_cnt = 0;
                   last_cnt = 0;
                   lost_cnt = 0;
                   start_time = ros::Time::now();
               }
               else
               {
                   ROS_INFO("receive cnt from MCU: %d",cnt);
                   real_cnt++;
                   if(cnt != last_cnt + 1)
                   {
                       ROS_ERROR("CAN  frame lost ! ! ! !");
                       lost_cnt++;
                   }
                   last_cnt = cnt;
                   ROS_INFO("real cnt:       %d",real_cnt);
                   ROS_INFO("lost frame cnt: %d",lost_cnt);
                   for(uint32_t i = 4; i < data_len; i++)
                   {
                       test_str.push_back(msg->Data[i]);
                   }
                   ROS_INFO("get str: %s", test_str.c_str());
                   ROS_INFO("duration time: %f second",ros::Time::now().toSec() - start_time.toSec());
               }
               break;
           }

        case CAN_SOURCE_ID_LOCK_STATUS_UPLOAD:
            {
                static uint8_t lock_status_new[10] = {0};
                static uint8_t lock_status_last[10] = {0};
                uint8_t lock_num = data_len;
                if(lock_num > 0)
                {
                    for(uint8_t i = 0; i < lock_num; i++)
                    {
                        lock_status_new[i] = msg->Data[i];
                        if(lock_status_new[i] != lock_status_last[i])
                        {
                            ROS_INFO("lock %d status change to %d", i+1, lock_status_new[i]);
                        }

                        lock_status_last[i] = lock_status_new[i];
                    }
                }
                break;
            }

        default : break;

    }
}

