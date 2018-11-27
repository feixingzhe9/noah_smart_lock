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

std::string super_rfid = "1050";
std::string super_password = "1050";

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

int SmartLock::beeper_ctrl(uint8_t times, uint8_t duration, uint8_t interval_time, uint8_t frequency)
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
    can_msg.Data[2]  = duration;
    can_msg.Data[3]  = interval_time;
    can_msg.Data[4]  = frequency;

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
                std::string rfid;
                uint8_t status = 1;
                int id_type = 0;
                uint16_t rfid_int = 0;
                std::vector<int> to_unlock_serials_tmp;
                to_unlock_serials_tmp.clear();
                rfid.resize(RFID_LEN);
                rfid.clear();
                rfid_int = msg->Data[2];
                rfid_int = rfid_int << 8;
                rfid_int += msg->Data[3];

                ROS_INFO("rfid int data :%d",rfid_int);
                rfid = this->build_rfid(rfid_int);
                ROS_WARN("receive RFID: %s",rfid.c_str());
            }
            break;

        case CAN_SOURCE_ID_PW_UPLOAD:
            {
                if(data_len == PASSWORD_LEN)
                {
                    std::string pw;
                    uint8_t status = 1;
                    int id_type = 0;
                    pw.resize(PASSWORD_LEN);
                    pw.clear();
                    for(uint8_t i = 0; i < PASSWORD_LEN; i++)
                    {
                        pw.push_back(msg->Data[i]);
                    }
                    ROS_WARN("receive pass word: %s",pw.data());
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
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_2:
            {
                ROS_INFO("get upload QR 2  code info.");
                std::string qr_code;
                qr_code.clear();

                qr_code = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.c_str());
            }
            break;

        case CAN_SOURCE_ID_QR_CODE_UPLOAD_3:
            {
                ROS_INFO("get upload QR 3  code info.");
                std::string qr_code;
                qr_code.clear();

                qr_code = parse_qr_code(msg);

                ROS_WARN("receive QR code: %s",qr_code.c_str());
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

