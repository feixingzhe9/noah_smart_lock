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
//static unsigned char recv_buf_last[BUF_LEN] = {0};

smart_lock_t    sys_smart_lock_ram;
smart_lock_t    *sys_smart_lock = &sys_smart_lock_ram;




std::vector<lock_serials_stauts_t> lock_serials_status;
std::vector<lock_serials_stauts_t> lock_serials_status_ack;
std::vector<lock_serials_stauts_t> lock_serials_status_input;
std::vector<std::string> input_pw;
std::vector<std::string> input_rfid;
std::vector<std::string> input_qr_code;

std::string lock_version;

std::vector<pub_to_agent_t> pub_to_agent_vector;	//boost::mutex::scoped_lock()
//std::vector<uint8_t> lock_serials_vector;	//boost::mutex::scoped_lock()
std::vector<int> to_unlock_serials;     //boost::mutex::scoped_lock()

std::string to_set_super_pw = "6666";
std::string to_set_super_rfid =  "0000";

std::string set_super_pw_ack;
std::string set_super_rfid_ack;

std::vector<lock_pivas_t> lock_match_db_vec;
std::string super_rfid = "1050";
std::string super_password = "1050";


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
                if(rfid.size() != 4)
                {
                    ROS_ERROR("%s: get wrong rfid: %s",__func__, rfid.data());
                    is_need_update_rfid_pw = false;
                }
                else
                {
                    super_rfid = rfid;
                }

                password = get_table_super_pw_to_ram(db_, TABLE_SUPER_RFID_PW); //need to add mutex
                if(password.size() != 4)
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
            ROS_INFO("get binding_credit_card_employees . . .");
            if(delete_all_db_data(db_, TABLE_PIVAS) < 0)
            {
                ROS_ERROR("delete_all_db_data ERROR ! !");
            }
            bool get_door_id = false;
            for(int i = 1; i < 33; i++)
            {
                if(j["data"].find(std::to_string(i)) != j["data"].end())
                {
                    get_door_id = true;
                    std::vector<std::string> worker_id_vec = j["data"][std::to_string(i)];
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
                        if(insert_into_db(db_, TABLE_PIVAS,rfid, password, int_worker_id, i) < 0)
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
    if(super_pw.size() == 4)
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
    if(super_rfid.size() == 4)
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
#if 0
    if(this->is_log_on == true)
    {
        for(uint8_t i = 0; i < msg->DataLen; i++)
        {
            ROS_INFO("msg->Data[%d] = 0x%x",i,msg->Data[i]);
        }
    }
#endif
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
            if(data_len == 4)
            {
                std::string rfid;
                uint8_t status = 1;
                uint16_t rfid_int = 0;
                rfid.resize(4);
                rfid.clear();
                rfid_int = msg->Data[2];
                rfid_int = rfid_int<<8;
                rfid_int += msg->Data[3];

                ROS_INFO("rfid int data :%d",rfid_int);

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
                ROS_INFO("receive RFID: %s",rfid.c_str());
                input_rfid.push_back(rfid);
                for(std::vector<lock_pivas_t>::iterator it = lock_match_db_vec.begin(); it != lock_match_db_vec.end(); it++)
                {
                    if((*it).rfid == rfid)
                    {
                        ROS_INFO("get right RFID  ID");
                        to_unlock_serials.push_back((*it).door_id);
                        status = 0;
                        do
                        {
                            boost::mutex::scoped_lock(tmx_smart_lock);
                            to_unlock_serials.push_back((*it).door_id);
                        }while(0);

                    }
                }

                std::sort(to_unlock_serials.begin(),to_unlock_serials.end());
                to_unlock_serials.erase(unique(to_unlock_serials.begin(), to_unlock_serials.end()), to_unlock_serials.end());

                if(rfid == super_rfid)
                {
                    status = 0;
                    ROS_WARN("get right super RFID");
                }
                pub_to_agent_t tmp;
                tmp.type = 2;
                tmp.code = rfid;
                tmp.result = status;
                do
                {
                    boost::mutex::scoped_lock(mtx_agent);
                    pub_to_agent_vector.push_back(tmp);
                }while(0);
            }

            break;

        case CAN_SOURCE_ID_PW_UPLOAD:
            {
                if(data_len == 4)
                {
                    ROS_INFO("get password upload");
                    std::string pw;
                    uint8_t status = 1;
                    pw.resize(4);
                    pw.clear();
                    for(uint8_t i = 0; i < 4; i++)
                    {
                        pw.push_back(msg->Data[i]);
                    }
                    ROS_INFO("receive pass word: %s",pw.data());
                    input_pw.push_back(pw);

                    for(std::vector<lock_pivas_t>::iterator it = lock_match_db_vec.begin(); it != lock_match_db_vec.end(); it++)
                    {
                        if((*it).password == pw)
                        {
                            ROS_INFO("get right pass word");
                            do
                            {
                                boost::mutex::scoped_lock(tmx_smart_lock);
                                to_unlock_serials.push_back((*it).door_id);
                            }while(0);
                            status = 0;
                        }
                    }
                    std::sort(to_unlock_serials.begin(),to_unlock_serials.end());
                    to_unlock_serials.erase(unique(to_unlock_serials.begin(), to_unlock_serials.end()), to_unlock_serials.end());
                    if(pw == super_password)
                    {
                        status = 0;
                        ROS_WARN("get right super password");
                    }
                    pub_to_agent_t tmp;
                    tmp.type = 3;
                    tmp.code = pw;
                    tmp.result = status;
                    do
                    {
                        boost::mutex::scoped_lock(mtx_agent);
                        pub_to_agent_vector.push_back(tmp);
                    }while(0);

                }

            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_1:
            {
                ROS_INFO("get upload QR 1  code info."); 
                std::string qr_code;
                qr_code.clear();
                int i = 0;
                for(i = 0; i < data_len; i++)
                {
                    qr_code.push_back(msg->Data[i]);
                }

                ROS_WARN("receive QR code: %s",qr_code.c_str());
                input_qr_code.push_back(qr_code);
                pub_to_agent_t tmp;
                tmp.type = 1;
                tmp.code = qr_code;
                tmp.result = 0;
                do
                {
                    boost::mutex::scoped_lock(mtx_agent);
                    pub_to_agent_vector.push_back(tmp);
                }while(0);
            }
            break;
        case CAN_SOURCE_ID_QR_CODE_UPLOAD_2:
            {
                ROS_INFO("get upload QR 2  code info."); 
                std::string qr_code;
                qr_code.clear();
                int i = 0;
                for(i = 0; i < data_len; i++)
                {
                    qr_code.push_back(msg->Data[i]);
                }

                ROS_WARN("receive QR code: %s",qr_code.c_str());
                input_qr_code.push_back(qr_code);
                pub_to_agent_t tmp;
                tmp.type = 1;
                tmp.code = qr_code;
                tmp.result = 0;
                do
                {
                    boost::mutex::scoped_lock(mtx_agent);
                    pub_to_agent_vector.push_back(tmp);
                }while(0);
            }
            break;
        case CAN_SOURCE_ID_QR_CODE_UPLOAD_3:
            {
                ROS_INFO("get upload QR 3  code info."); 
                std::string qr_code;
                qr_code.clear();
                int i = 0;
                for(i = 0; i < data_len; i++)
                {
                    qr_code.push_back(msg->Data[i]);
                }

                ROS_WARN("receive QR code: %s",qr_code.c_str());
                input_qr_code.push_back(qr_code);
                pub_to_agent_t tmp;
                tmp.type = 1;
                tmp.code = qr_code;
                tmp.result = 0;
                do
                {
                    boost::mutex::scoped_lock(mtx_agent);
                    pub_to_agent_vector.push_back(tmp);
                }while(0);
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
        
        default : break;

    }
}

#if 0
int SmartLock::handle_rev_frame(smart_lock_t *sys,unsigned char * frame_buf)
{
    int frame_len = 0;
    int i = 0;
    int j = 0;
    int command = 0;
    unsigned char check_data = 0;
    uint8_t cmd_type = 0;
    uint8_t data_direction = 0;
    uint8_t data_len = 0;
    int error = -1;
    frame_len = frame_buf[1] - 1;

    for(i=0;i<frame_len-2;i++)
    {
        check_data += frame_buf[i];
    }
    check_data += 0xcc;
    if(check_data != frame_buf[frame_len-2] || PROTOCOL_TAIL != frame_buf[frame_len -1])
    {
        PowerboardInfo("smart lock receive frame check error");
        //return -1;
    }
    PowerboardInfo("smart lock recieve data check OK.");

    cmd_type = frame_buf[2];
    data_direction = frame_buf[3];
    data_len = frame_buf[1];
    ROS_INFO("cmd_type : %d",cmd_type);
    ROS_INFO("direction : %d",data_direction);
    ROS_INFO("data_len : %d",data_len);
    switch(data_direction)
    {
        case DATA_DIRECTION_LOCK_ACK:
            switch(cmd_type)
            {
                case FRAME_TYPE_UNLOCK:
                    {
                        lock_serials_stauts_t single_status;

                        ROS_WARN("UNLOCK : receive ack data from lock.");
                        //lock_serials_status
#if 0
                        for(uint8_t i = 0; i < (data_len -6)/2; i++)
                        {
                            single_status.lock_id = frame_buf[4+i*2];
                            single_status.status = (bool)frame_buf[4+i*2 + 1];
                            std::vector<lock_serials_stauts_t>::iterator it = lock_serials_status.begin();
                            //lock_serials_stauts_t to_find;
                            //to_find.lock_id = 1;
                            //to_find.status = 0;
                            //std::vector<lock_serials_stauts_t>::iterator result = find(lock_serials_status.begin(), lock_serials_status.end(), to_find);
                            for( ; it != lock_serials_status.end(); it++)
                            {
                                if((*it).lock_id == single_status.lock_id)
                                {
                                    (*it).status = single_status.status;
                                    break;
                                }
                            }
                            if(it == lock_serials_status.end())
                            {
                                lock_serials_status.push_back(single_status);
                            }


                            it = lock_serials_status_ack.begin();
                            for( ; it != lock_serials_status_ack.end(); it++)
                            {
                                if((*it).lock_id == single_status.lock_id)
                                {
                                    (*it).status = single_status.status;
                                    break;
                                }
                            }
                            if(it == lock_serials_status_ack.end())
                            {
                                lock_serials_status_ack.push_back(single_status);
                            }


                            lock_serials_status_ack.push_back(single_status);
                        }
#else
                        int lock_status_bit = 0;
                        lock_status_bit += frame_buf[4];
                        lock_status_bit += frame_buf[5] << 8;
                        lock_status_bit += frame_buf[6] << 16;
                        lock_status_bit += frame_buf[7] << 24;
                        ROS_INFO("lock status bit : %d",lock_status_bit);
                        lock_serials_status.clear();
                        for(int j = 0; j < 32; j++)
                        {
                            single_status.lock_id = j + 1;
                            single_status.status = 0;
                            if(lock_status_bit & (1<<j))
                            {
                                ROS_INFO("lock %d status is on", j + 1);
                                single_status.status = 1;
                            }
                            lock_serials_status.push_back(single_status);
                        }

#endif
                        for(std::vector<lock_serials_stauts_t>::iterator my_iterator = lock_serials_status.begin() ;\
                                my_iterator != lock_serials_status.end(); my_iterator++)
                        {
                            ROS_INFO("lock id:  %d,  status : %d",(*my_iterator).lock_id, (*my_iterator).status);
                        }
                    }
                    break;
                case FRAME_TYPE_LOCK_VERSION:
                    {

                        lock_version.clear();
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            lock_version.push_back(frame_buf[4+i]);
                        }
                        ROS_INFO("get lock version : %s",lock_version.data());
                    }
                    break;

                case FRAME_TYPE_SET_SUPER_PW:
                    {
                        uint8_t status = 0;
                        status = frame_buf[4];
                        ROS_WARN("get lock super pass word ack: status is %d",status);
                        set_super_pw_ack.clear();
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            set_super_pw_ack.push_back(frame_buf[5+i]);
                        }
                        ROS_INFO("get lock super pass word ack : %s",set_super_pw_ack.data());
                    }
                    break;

                case FRAME_TYPE_SET_SUPER_RFID:
                    {
                        uint8_t status = 0;
                        status = frame_buf[4];
                        ROS_WARN("get lock super RFID ack: status is %d",status);
                        set_super_rfid_ack.clear();
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            set_super_rfid_ack.push_back(frame_buf[5+i]);
                        }
                        ROS_INFO("get lock super RFID ack : %s",set_super_rfid_ack.data());
                    }
                    break;

                default :
                    break;

            }

            break;
        case DATA_DIRECTION_LOCK_TO_X86:
            switch(cmd_type)
            {
                case FRAME_TYPE_PW_UPLOAD:
                    {
                        std::string pw;
                        uint8_t status = 1;
                        pw.resize(4);
                        pw.clear();
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            pw.push_back(frame_buf[4+i]);
                        }
                        ROS_INFO("receive pass word: %s",pw.data());
                        input_pw.push_back(pw);

                        for(std::vector<lock_pivas_t>::iterator it = lock_match_db_vec.begin(); it != lock_match_db_vec.end(); it++)
                        {
                            if((*it).password == pw)
                            {
                                ROS_INFO("get right pass word");
                                do
                                {
                                    boost::mutex::scoped_lock(tmx_smart_lock);
                                    to_unlock_serials.push_back((*it).door_id);
                                }while(0);
                                status = 0;
                            }
                        }
                        std::sort(to_unlock_serials.begin(),to_unlock_serials.end());
                        to_unlock_serials.erase(unique(to_unlock_serials.begin(), to_unlock_serials.end()), to_unlock_serials.end());
                        if(pw == super_password)
                        {
                            status = 0;
                            ROS_WARN("get right super password");
                        }
                        pub_to_agent_t tmp;
                        tmp.type = 3;
                        tmp.code = pw;
                        tmp.result = status;
                        do
                        {
                            boost::mutex::scoped_lock(mtx_agent);
                            pub_to_agent_vector.push_back(tmp);
                        }while(0);
                    }
                    break;

                case FRAME_TYPE_RFID_UPLOAD:
                    {
                        std::string rfid;
                        uint8_t status = 1;
                        rfid.resize(4);
                        rfid.clear();
                        for(uint8_t i = 0; i < data_len - 7; i++)
                        {
                            rfid.push_back(frame_buf[4+i]);
                        }
                        ROS_INFO("receive RFID: %s",rfid.data());
                        input_rfid.push_back(rfid);
                        for(std::vector<lock_pivas_t>::iterator it = lock_match_db_vec.begin(); it != lock_match_db_vec.end(); it++)
                        {
                            if((*it).rfid == rfid)
                            {
                                ROS_INFO("get right RFID  ID");
                                to_unlock_serials.push_back((*it).door_id);
                                status = 0;
                                do
                                {
                                    boost::mutex::scoped_lock(tmx_smart_lock);
                                    to_unlock_serials.push_back((*it).door_id);
                                }while(0);

                            }
                        }

                        std::sort(to_unlock_serials.begin(),to_unlock_serials.end());
                        to_unlock_serials.erase(unique(to_unlock_serials.begin(), to_unlock_serials.end()), to_unlock_serials.end());

                        if(rfid == super_rfid)
                        {
                            status = 0;
                            ROS_WARN("get right super RFID");
                        }
                        pub_to_agent_t tmp;
                        tmp.type = 2;
                        tmp.code = rfid;
                        tmp.result = status;
                        do
                        {
                            boost::mutex::scoped_lock(mtx_agent);
                            pub_to_agent_vector.push_back(tmp);
                        }while(0);
                    }
                    break;
                case FRAME_TYPE_QR_CODE_UPLOAD:
                    {
                        std::string qr_code;
                        qr_code.clear();
                        int i = 0;
                        for(i = 0; i < data_len - 8; i++)
                        {
                            qr_code.push_back(frame_buf[4+i]);
                        }
                        if(frame_buf[4 + i] != 0x0d)
                        {
                            ROS_WARN("frame_buf[ %d ] = 0x%X", 4 + i, frame_buf[4 + i]);
                            qr_code.push_back(frame_buf[4 + i]);
                        }
                        ROS_WARN("receive QR code: %s",qr_code.c_str());
                        input_qr_code.push_back(qr_code);
                        pub_to_agent_t tmp;
                        tmp.type = 1;
                        tmp.code = qr_code;
                        tmp.result = 0;
                        do
                        {
                            boost::mutex::scoped_lock(mtx_agent);
                            pub_to_agent_vector.push_back(tmp);
                        }while(0);
                    }
                    break;

                default :
                    break;

            }
            break;
        case DATA_DIRECTION_X86_ACK:
            break;
        default :
            ROS_ERROR("data direciton ERROR: %d", data_direction);
            break;

    }
    return error;
}
#endif
