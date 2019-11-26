#include "ros/ros.h"
#include "std_msgs/String.h"
#include "std_msgs/UInt8MultiArray.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iostream"
#include "stdlib.h"
#include "string"
#include "sstream"
#include <boost/thread/thread.hpp>
#include <boost/thread/mutex.hpp>
#include <smart_lock.h>


std::string lock_version;

std::vector<int> to_unlock_serials(0);     //boost::mutex::scoped_lock()

std::string super_rfid = "1055";
std::string super_password = "1055";

int SmartLock::param_init(void)
{
    return 0;
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

    mrobot_msgs::vci_can can_msg;
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

int SmartLock::beeper_ctrl(uint8_t times, uint16_t duration, uint16_t interval_time, uint8_t cmd)
{
    int error = -1;
    ROS_WARN("start to ctrl beeper ");

    mrobot_msgs::vci_can can_msg;
    CAN_ID_UNION id;
    memset(&id, 0x0, sizeof(CAN_ID_UNION));
    id.CanID_Struct.SourceID = CAN_SOURCE_ID_BEEPER_TIMES_CTRL;
    id.CanID_Struct.SrcMACID = 0;
    id.CanID_Struct.DestMACID = SMART_LOCK_CAN_SRC_MAC_ID;
    id.CanID_Struct.FUNC_ID = 0x02;
    id.CanID_Struct.ACK = 0;
    id.CanID_Struct.res = 0;

    can_msg.ID = id.CANx_ID;
    can_msg.DataLen = 5;
    can_msg.Data.resize(5);

    can_msg.Data[0] = 0x00;
    can_msg.Data[1]  = times;
    can_msg.Data[2]  = duration / 20;
    can_msg.Data[3]  = interval_time / 20;
    ROS_INFO("duration: %d, interval_time: %d", can_msg.Data[2], can_msg.Data[3]);
    can_msg.Data[4]  = cmd;

    this->pub_to_can_node.publish(can_msg);

    return error;
}

int SmartLock::set_super_pw(std::string super_pw)
{
    int error = -1;
    if(super_pw.size() == SUPER_PASSWORD_LEN)
    {
        ROS_WARN("start to set super password ...");

        mrobot_msgs::vci_can can_msg;
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
        ROS_WARN("start to set super RFID ...");

        mrobot_msgs::vci_can can_msg;
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

int SmartLock::get_doors_state(void)
{
    int error = -1;

    mrobot_msgs::vci_can can_msg;
    CAN_ID_UNION id;
    memset(&id, 0x0, sizeof(CAN_ID_UNION));
    id.CanID_Struct.SourceID = CAN_SOURCE_ID_GET_DOORS_STATE;
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

int SmartLock::get_lock_version(void)
{
    int error = -1;

    mrobot_msgs::vci_can can_msg;
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

void SmartLock::rcv_from_driver_rfid_callback(const std_msgs::UInt16MultiArray::ConstPtr &info)
{
    if(info->data.size() == 2)
    {
        uint8_t dev_id = info->data[0];
        uint16_t rfid_int = info->data[1];
        std_msgs::String rfid;
        rfid.data.resize(RFID_LEN);
        rfid.data.clear();
        rfid.data = this->build_rfid(rfid_int);

        ROS_WARN("from driver rfid: receive RFID: %d, dev_id: %d", rfid_int, dev_id);
        if(dev_id == 0x0e)   //0x0e
        {
            this->report_rfid(rfid);
            if(rfid.data == super_rfid)
            {
                this->unlock();
                to_unlock_serials.clear();
                for (uint8_t i = 0; i < 8; i++)
                {
                    to_unlock_serials.push_back(i + 1);
                }
                this->beeper_ctrl(1, 60, 60, 0);
                this->beeper_ctrl(1, 500, 100, 0);
            }
            else
            {
                this->beeper_ctrl(1, 250, 150, 0);
            }
        }
        else if(dev_id < 0x0e)
        {
            this->report_cabinet_rfid(dev_id, 0, rfid);
            this->beeper_ctrl(1, 250, 150, 0);
        }
    }
    else if(info->data.size() == 1)
    {
        uint8_t dev_id = info->data[0];
        std_msgs::String rfid;
        if(dev_id < 0x0e)
        {
            ROS_INFO("rfid: %d is depart", dev_id);
            this->report_cabinet_rfid(dev_id, 1, rfid);
            this->beeper_ctrl(3, 60, 60, 0);
        }
    }
}



void SmartLock::driver_rfid_info_callback(const std_msgs::String::ConstPtr &msg)
{
    auto j = json::parse(msg->data.c_str());
    std::string j_str = j.dump();
    ROS_INFO("%s",j_str.data());


    if(j.find("rfid_type") == j.end())
    {
        ROS_ERROR("%s:do not find  [rfid_type] in json msg  !", __func__);
        return;
    }

    if(j.find("index") == j.end())
    {
        ROS_ERROR("%s:do not find  [index] in json msg  !", __func__);
        return;
    }

    if(j.find("info") == j.end())
    {
        ROS_ERROR("%s:do not find  [info] in json msg  !", __func__);
        return;
    }

    if(j.find("act") == j.end())
    {
        ROS_ERROR("%s:do not find  [act] in json msg  !", __func__);
        return;
    }


    std::string rfid_type_str = j["rfid_type"];
    std::string act_str = j["act"];
    uint32_t rfid_info = j["info"];
    uint8_t index = j["index"];

    std_msgs::String rfid_info_str;
    rfid_info_str.data = this->build_rfid(rfid_info);

    if(rfid_type_str == "cabinet_detection")
    {
        if(act_str == "depart")
        {
            this->report_cabinet_rfid(index, 1, rfid_info_str);
            this->beeper_ctrl(2, 60, 60, 0);
            return;
        }
        else if(act_str == "arrive")
        {
            this->report_cabinet_rfid(index, 0, rfid_info_str);
            this->beeper_ctrl(1, 250, 150, 0);
            return;
        }
    }
    else if(rfid_type_str == "auth")
    {
        std_msgs::String rfid_tmp;
        rfid_tmp.data = this->build_rfid(rfid_info);
        if(rfid_tmp.data == super_rfid)
        {
            to_unlock_serials.clear();
            for (uint8_t i = 0; i < 8; i++)
            {
                to_unlock_serials.push_back(i + 1);
            }
            this->unlock();
            this->beeper_ctrl(1, 60, 60, 0);
            this->beeper_ctrl(1, 500, 100, 0);
        }
        else
        {
            this->beeper_ctrl(1, 250, 150, 0);
        }
        this->report_rfid(rfid_tmp);
        return;
    }
    else if(rfid_type_str == "dst_src_info")
    {
        if(j.find("dst_info") == j.end())
        {
            ROS_ERROR("%s:do not find  [dst_info] in json msg  !", __func__);
            return;
        }
        if(j.find("src_info") == j.end())
        {
            ROS_ERROR("%s:do not find  [src_info] in json msg  !", __func__);
            return;
        }

        uint32_t dst = j["dst_info"];
        uint32_t src = j["src_info"];
        std::string dst_str = this->build_rfid(dst);
        std::string src_str = this->build_rfid(src);

        if(act_str == "depart")
        {
            this->report_dst_src_info(index, 1, rfid_info_str.data, dst_str, src_str);
            this->beeper_ctrl(2, 60, 60, 0);
            return;
        }
        else if(act_str == "arrive")
        {
            this->report_dst_src_info(index, 0, rfid_info_str.data, dst_str, src_str);
            this->beeper_ctrl(1, 250, 150, 0);
            return;
        }
    }

   ROS_ERROR("%s: ERROR, please check json string msg", __func__);
   return;
}


void SmartLock::report_rfid(std_msgs::String rfid)
{
    json j;
    j.clear();
    j =
    {
        {"rfid", rfid.data.c_str()},
    };
    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data.clear();
    pub_json_msg.data = ss.str();
    report_rfid_pub.publish(pub_json_msg);

}


void SmartLock::report_cabinet_rfid(uint8_t cabinet_num, uint8_t type, std_msgs::String rfid)
{
    json j;
    j.clear();
    if(type == 0)
    {
        j =
        {
            {"pub_name", "rfid_info"},
            {"rfid", rfid.data.c_str()},
            {"cabinet_id", cabinet_num},
        };
    }
    else if(type == 1)
    {
        j =
        {
            {"pub_name", "rfid_depart"},
            {"cabinet_id", cabinet_num},
        };
    }
    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data.clear();
    pub_json_msg.data = ss.str();
    report_cabinet_rfid_pub.publish(pub_json_msg);

}



void SmartLock::report_dst_src_info(uint8_t index, uint8_t type, std::string rfid_info, std::string dst, std::string src)
{
    json j;

    if(type == 0)
    {
        j =
        {
            {"pub_name", "rfid_info"},
            {"index", index},
            {"dst", dst.c_str()},
            {"src", src.c_str()},
        };
    }
    else if(type == 1)
    {
        j =
        {
            {"pub_name", "rfid_depart"},
            {"index", index},
        };
    }

    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data.clear();
    pub_json_msg.data = ss.str();
    report_dst_src_info_pub.publish(pub_json_msg);

}


void SmartLock::report_password(std_msgs::String password)
{
    json j;
    j.clear();
    j =
    {
        {"pwd", password.data.c_str()},
    };

    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data.clear();
    pub_json_msg.data = ss.str();
    report_pwd_pub.publish(pub_json_msg);
}

void SmartLock::report_qr_code(uint8_t index, std_msgs::String qr_code)
{
    json j;
    uint32_t lock_index = 0;
    if(index < 32)
    {
        lock_index = (1 << index);
    }
    else
    {
        return ;
    }

    j.clear();
    j =
    {
        {"lock_index", lock_index},
        {"content", qr_code.data.c_str()},
    };
    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data.clear();
    pub_json_msg.data = ss.str();
    report_qr_code_pub.publish(pub_json_msg);

}


bool SmartLock::service_update_super_admin(mrobot_srvs::JString::Request  &super, mrobot_srvs::JString::Response &status)
{
    auto j = json::parse(super.request.c_str());
    std::string j_str = j.dump();
    ROS_WARN("%s",j_str.data());

    if(j.find("super_pwd") != j.end())
    {
        std::string password;
        password = j["super_pwd"];
        int len = password.size();
        for(int i = 0; i < len; i++)
        {
            if((password[i] < '0') || (password[i] > '9'))
            {
                ROS_ERROR("super password is not pure number !");
                return false;
            }
        }
        super_password = password;
        is_need_update_rfid_pw = true;
        ROS_WARN("%s : get super password %s", __func__, password.c_str());
//        sem_init(&super_admin_sem, 0, 0);
    }


    if(j.find("super_rfid") != j.end())
    {
        std::string rfid;
        rfid = j["super_rfid"];
        int len = rfid.size();
        for(int i = 0; i < len; i++)
        {
            if((rfid[i] < '0') || (rfid[i] > '9'))
            {
                ROS_ERROR("super password is not pure number !");
                return false;
            }
        }
        super_rfid = rfid;
        is_need_update_rfid_pw = true;
        ROS_WARN("%s : get super rfid %s", __func__, rfid.c_str());
//        sem_init(&super_admin_sem, 0, 0);
    }
//    sem_wait(&super_admin_sem);
//    ROS_WARN("wait 1");
//    sem_wait(&super_admin_sem);
//    ROS_WARN("wait 2");
    status.success = true;
    return true;
}

bool SmartLock::service_unlock(mrobot_srvs::JString::Request  &lock_index, mrobot_srvs::JString::Response &status)
{
    auto j = json::parse(lock_index.request.c_str());
    std::string j_str = j.dump();
    ROS_WARN("%s",j_str.data());

    if(j.find("lock_index") != j.end())
    {
        uint32_t lock_index = j["lock_index"];
        if((lock_index == 0) || (lock_index >= (1 << this->door_num)))
        {
            ROS_ERROR("%s: parameter error: lock_index: %d", __func__, lock_index);
            status.success = false;
            return true;
        }
        to_unlock_serials.clear();
        for(uint8_t i = 0; i < 32; i++)
        {
            if(lock_index & (1 << i))
            {
                ROS_INFO("%s: require unlock num: %d", __func__, i + 1);
                to_unlock_serials.push_back(i + 1);
            }
        }
        this->unlock();
        this->beeper_ctrl(1, 500, 100, 0);
    }


//    sem_wait(&super_admin_sem);
//    ROS_WARN("wait 1");
//    sem_wait(&super_admin_sem);
//    ROS_WARN("wait 2");
    status.success = true;
    return true;
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

std::string SmartLock::parse_qr_code(mrobot_msgs::vci_can* msg)
{
    std::string qr_code;
    qr_code.clear();

    for(uint8_t i = 0; i < msg->DataLen - 2; i++)
    {
        qr_code.push_back(msg->Data[i]);
    }

    uint8_t last1 = msg->Data[msg->DataLen - 1];
    uint8_t last2 = msg->Data[msg->DataLen - 2];

    if( last2 != 0x0d)   // CRLF
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


void SmartLock::pub_locks_status(uint8_t *status, uint8_t lock_num)
{
    std_msgs::UInt8MultiArray locks_status;
    locks_status.data.clear();
    for(uint8_t i = 0; i < lock_num; i++)
    {
        locks_status.data.push_back(status[i]);
    }
    this->locks_status_pub.publish(locks_status);
}


void SmartLock::rcv_from_can_node_callback(const mrobot_msgs::vci_can::ConstPtr &c_msg)
{
    mrobot_msgs::vci_can can_msg;
    mrobot_msgs::vci_can long_msg;
    CAN_ID_UNION id;

    long_msg = this->long_frame.frame_construct(c_msg);
    mrobot_msgs::vci_can* msg = &long_msg;
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

    ROS_INFO("source id: 0x%x",source_id);
    ROS_INFO("data length: %d",data_len);

    switch(source_id)
    {
        case CAN_SOURCE_ID_KEY_TEST_UPLOAD:
            {
                ROS_INFO("get key test value upload. data len is %d ", data_len);

                if( 2 == data_len)
                {
                    uint16_t key = *(uint16_t*)&msg->Data[0];
                    ROS_INFO("get key test value:  0x%x", key);
                    char key_value = map_key_value(key);
                    ROS_WARN("get key: %c",key_value);
                }
                break;
            }

        case CAN_SOURCE_ID_UNLOCK:
            ROS_INFO("get unlock ack from mcu");
            break;

        case CAN_SOURCE_ID_RFID_UPLOAD:
            if(data_len == RFID_LEN)
            {
                std_msgs::String rfid;
                uint8_t status = 1;
                int id_type = 0;
                uint16_t rfid_int = 0;
                std::vector<int> to_unlock_serials_tmp;
                to_unlock_serials_tmp.clear();
                rfid.data.resize(RFID_LEN);
                rfid.data.clear();
                rfid_int = msg->Data[2];
                rfid_int = rfid_int << 8;
                rfid_int += msg->Data[3];

                ROS_INFO("rfid int data :%d",rfid_int);
                rfid.data = this->build_rfid(rfid_int);
                ROS_WARN("receive RFID: %s",rfid.data.c_str());
                if(rfid.data.c_str() == super_rfid.c_str())
                {
                    ROS_INFO("get super rfid");
                }
                this->report_rfid(rfid);
            }
            break;

        case CAN_SOURCE_ID_PW_UPLOAD:
            {
                if(data_len == PASSWORD_LEN)
                {
                    std_msgs::String pwd;
                    uint8_t status = 1;
                    int id_type = 0;
                    pwd.data.resize(PASSWORD_LEN);
                    pwd.data.clear();
                    for(uint8_t i = 0; i < PASSWORD_LEN; i++)
                    {
                        pwd.data.push_back(msg->Data[i]);
                    }
                    ROS_WARN("receive pass word: %s",pwd.data.c_str());
                    this->report_password(pwd);
                }
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_1:
            {
                ROS_INFO("get upload QR 1  code info.");
                std_msgs::String qr_code;
                qr_code.data.clear();

                qr_code.data = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.data.c_str());
                this->report_qr_code(0, qr_code);
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_2:
            {
                ROS_INFO("get upload QR 2  code info.");
                std_msgs::String qr_code;
                qr_code.data.clear();

                qr_code.data = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.data.c_str());
                this->report_qr_code(1, qr_code);
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_3:
            {
                ROS_INFO("get upload QR 3  code info.");
                std_msgs::String qr_code;
                qr_code.data.clear();

                qr_code.data = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.data.c_str());
                this->report_qr_code(2, qr_code);
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
                    //sem_post(&super_admin_sem);
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
                    //sem_post(&super_admin_sem);
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

        case CAN_SOURCE_ID_LOCK_STATUS_UPLOAD:
            {
                static uint8_t lock_status_new[10] = {0};
                static uint8_t lock_status_last[10] = {0};
                uint8_t num = data_len;
                if(num > 0)
                {
                    if(num >= this->door_num)
                    {
                        for(uint8_t i = 0; i < num; i++)
                        {
                            lock_status_new[i] = msg->Data[i];
                            if(lock_status_new[i] != lock_status_last[i])
                            {
                                ROS_WARN("lock %d status change to %d", i+1, lock_status_new[i]);
                            }

                            lock_status_last[i] = lock_status_new[i];
                        }
                        this->pub_locks_status(lock_status_new, this->door_num);
                    }
                    else
                    {
                        ROS_ERROR("lock number is incorrect ! !");
                    }
                }
                else
                {
                    ROS_ERROR("CAN_SOURCE_ID_LOCK_STATUS_UPLOAD: CAN data length is incorrect ! !");
                }
                break;
            }

        case CAN_SOURCE_ID_GET_DOORS_STATE:
            {
                ROS_INFO("get doors state");
                for(uint8_t i = 0; i < data_len; i++)
                {
                    ROS_WARN("door %d state is %d", i + 1, msg->Data[i]);
                }
                break;
            }

        default : break;

    }
}

