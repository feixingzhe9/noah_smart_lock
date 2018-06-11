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

//#include <boost/uuid/uuid.hpp>
//#include <boost/uuid/uuid_generators.hpp>

//#include <uuid/uuid.h>

#define TEST_WAIT_TIME     300*1000

#define PowerboardInfo     ROS_INFO



boost::mutex mtx_agent;
//boost::mutex mtx_smart_lock;
static int led_over_time_flag = 0;
static int last_unread_bytes = 0;
static unsigned char recv_buf_last[BUF_LEN] = {0};

powerboard_t    sys_powerboard_ram; 
powerboard_t    *sys_powerboard = &sys_powerboard_ram;




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

std::string to_set_super_pw = "5555";
std::string to_set_super_rfid =  "0000";

std::string set_super_pw_ack;
std::string set_super_rfid_ack;



std::vector<lock_pivas_t> lock_match_db_vec;
std::string super_rfid;
std::string super_password;


void NoahPowerboard::pub_info_to_agent(uint8_t type, std::string data, uint8_t status)
{
    json j;
    time_t t;
    static int uuid = 0;
    struct tm *my_tm;
    struct tm *t2;
    char buf[128] = {0};
#if 0
    time(&t);

    t2 =    localtime(&t);
    sprintf(buf, "%04d%02d%02d  %02d:%02d:%02d", t2->tm_year + 1900, t2->tm_mon + 1, t2->tm_mday, t2->tm_hour, t2->tm_min, t2->tm_sec);
    ROS_INFO("%s\n", buf);              
#else
    time(&t);

#endif
    uuid++;
    std::string uuid_str = std::to_string(uuid);
    j.clear();
    j =
    {
        {"uuid",uuid_str.data()},
        {"sub_name","smart_lock_notice"},

        {
            "data",
            {
                {"type", type},

                {"code",data.data()},
                {"time", t * 1000},
                {"result", status},
            }
        }

    };

    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data = ss.str();
    for(uint8_t i = 0; i < 3; i++)
    {
        pub_to_agent.publish(pub_json_msg);
        //usleep(2000*1000);
        usleep(500*1000);
    }
}
//extern NoahPowerboard  powerboard;
int NoahPowerboard::PowerboardParamInit(void)
{
    //char dev_path[] = "/dev/ttyUSB0";
    //char dev_path[] = "/dev/ros/powerboard";
    char dev_path[] = "/dev/ttyS2";
    memcpy(sys_powerboard->dev,dev_path, sizeof(dev_path));
    sys_powerboard->led_set.effect = LIGHTS_MODE_DEFAULT;
    sys_powerboard->lock_serials.clear();
    sys_powerboard->lock_serials.push_back(1);
    return 0;
}


void *agent_protocol_process(void* arg)
{
    NoahPowerboard *pNoahPowerboard =  (NoahPowerboard*)arg; 
    while(ros::ok())
    {
        uint8_t type;
        uint8_t result;
        std::string code;
        bool is_need_to_pub = false;
        do
        {
            boost::mutex::scoped_lock(mtx_agent);
            if(!pub_to_agent_vector.empty())
            {
                auto a = pub_to_agent_vector.begin();
                type = (*a).type;
                result =(*a).result;
                code = (*a).code;
                if((type > 0) && (type < 4))
                {
                    pub_to_agent_vector.erase(a);
                    is_need_to_pub = true;
                }
            }
        }
        while(0);
        if(is_need_to_pub == true)
        {
            pNoahPowerboard->pub_info_to_agent(type, code, result);
        }

        usleep(100*1000);
    }
}

int NoahPowerboard::send_serial_data(powerboard_t *sys)
{
    boost::mutex io_mutex;
    boost::mutex::scoped_lock lock(io_mutex);
    int len = 0;

    int send_buf_len = 0;

    if((sys->device < 0) || (NULL == sys->send_data_buf))
    {
        ROS_INFO("dev or send_buf NULL!");
        return -1;
    }

    send_buf_len = sys->send_data_buf[1];

    if(send_buf_len <= 0 )
    {
        PowerboardInfo("smart_lock_node send_buf len: %d small 0!",send_buf_len);
        return -1;
    }
    for(int i =0;i<send_buf_len;i++)
    {
        //PowerboardInfo("noah_power send_buf :%02x",sys->send_data_buf[i]);
        ROS_INFO("smart_lock_node send_buf :%02x",sys->send_data_buf[i]);
    }
    len = write(sys->device,sys->send_data_buf,send_buf_len);
    if (len == send_buf_len)
    {
        PowerboardInfo("smart_lock_node send ok");
        return 0;
    }     
    else   
    {               
        tcflush(sys->device,TCOFLUSH);
        if(-1 == len)
        {
            //sys->com_state = COM_CLOSING;
        }
        return -1;
    }
}

uint8_t NoahPowerboard::CalCheckSum(uint8_t *data, uint8_t len)
{
    uint8_t sum = 0;
    for(uint8_t i = 0; i < len; i++)
    {
        sum += data[i];
    }
    return sum;  
}




int NoahPowerboard::unlock(powerboard_t *powerboard)     // done
{
begin:
    static uint8_t err_cnt = 0;

    uint8_t lock_num = 1;
    int error = -1;
    ROS_WARN("start to unlock ...");
#if 0
    do
    {
        boost::mutex::scoped_lock(tmx_smart_lock);
        lock_num = to_unlock_serials.size();
        powerboard->send_data_buf[0] = PROTOCOL_HEAD;
        powerboard->send_data_buf[1] = lock_num + 6;
        powerboard->send_data_buf[2] = FRAME_TYPE_UNLOCK;
        powerboard->send_data_buf[3] = DATA_DIRECTION_X86_TO_LOCK;
        for(uint8_t i = 0; i < lock_num; i++)
        {
            powerboard->send_data_buf[4 + i]  = to_unlock_serials[i];
        }
        to_unlock_serials.clear();
    }while(0);
#else

    powerboard->send_data_buf[0] = 0x5A;
    powerboard->send_data_buf[1] = lock_num + 6;
    powerboard->send_data_buf[2] = FRAME_TYPE_UNLOCK;
    powerboard->send_data_buf[3] = DATA_DIRECTION_X86_TO_LOCK;
    powerboard->send_data_buf[4]  = 1;

#endif
    powerboard->send_data_buf[lock_num + 4] = this->CalCheckSum(powerboard->send_data_buf, lock_num + 4);
    powerboard->send_data_buf[lock_num + 5] = PROTOCOL_TAIL;
    to_unlock_serials.clear();
    this->send_serial_data(powerboard);
    usleep(TEST_WAIT_TIME);
#if 0
    boost::mutex::scoped_lock(mtx_smart_lock);

    if(error  < 0)
    {

        if(err_cnt++ < COM_ERR_REPEAT_TIME)
        {
            usleep(500*1000); 
            ROS_ERROR("unlock opertion start to resend");
            goto begin; 
        }
        ROS_ERROR("unlock operation : FAILED !");
    }
    else
    {
        err_cnt = 0;
    }
#endif
    return error;
}




int NoahPowerboard::set_super_pw(powerboard_t *powerboard)    
{
begin:
    static uint8_t err_cnt = 0;

    int error = -1;
    ROS_WARN("start to set super pass word ...");

    powerboard->send_data_buf[0] = 0x5A;
    powerboard->send_data_buf[1] = 10;
    powerboard->send_data_buf[2] = FRAME_TYPE_SET_SUPER_PW;
    powerboard->send_data_buf[3] = DATA_DIRECTION_X86_TO_LOCK;
    to_set_super_pw = super_password;
    for(int i = 0; i < 4; i++)
    {
        if(to_set_super_pw.size() == 4)
        {
            powerboard->send_data_buf[4 + i]  = to_set_super_pw[i];
        }
        else
        {
            ROS_ERROR("to_set_super_pw.size() is not 4 ! !");
        }
    }
    //powerboard->send_data_buf[4]  = '5';
    //powerboard->send_data_buf[5]  = '5';
    //powerboard->send_data_buf[6]  = '5';
    //powerboard->send_data_buf[7]  = '5';

    powerboard->send_data_buf[8] = this->CalCheckSum(powerboard->send_data_buf, 8);
    powerboard->send_data_buf[9] = PROTOCOL_TAIL;

    this->send_serial_data(powerboard);
    usleep(TEST_WAIT_TIME);
    return error;
}

int NoahPowerboard::set_super_rfid(powerboard_t *powerboard)     
{
begin:
    static uint8_t err_cnt = 0;

    int error = -1;
    ROS_WARN("start to set super pass word ...");

    powerboard->send_data_buf[0] = 0x5A;
    powerboard->send_data_buf[1] = 10;
    powerboard->send_data_buf[2] = FRAME_TYPE_SET_SUPER_RFID;
    powerboard->send_data_buf[3] = DATA_DIRECTION_X86_TO_LOCK;
    to_set_super_rfid = super_rfid;
    for(int i = 0; i < 4; i++)
    {
        if(to_set_super_rfid.size() == 4)
        {
            powerboard->send_data_buf[4 + i]  = to_set_super_rfid[i];
        }
        else
        {
            ROS_ERROR("to_set_super_rfid.size() is not 4 ! !");
        }
    }
    //powerboard->send_data_buf[4]  = '1';
    //powerboard->send_data_buf[5]  = '0';
    //powerboard->send_data_buf[6]  = '5';
    //powerboard->send_data_buf[7]  = '5';

    powerboard->send_data_buf[8] = this->CalCheckSum(powerboard->send_data_buf, 8);
    powerboard->send_data_buf[9] = PROTOCOL_TAIL;

    this->send_serial_data(powerboard);
    usleep(TEST_WAIT_TIME);
    return error;
}

#if 0
int NoahPowerboard::get_lock_version(powerboard_t *powerboard)
{
    int error = -1;
    powerboard->send_data_buf[0] = PROTOCOL_HEAD;
    powerboard->send_data_buf[1] = 6;
    powerboard->send_data_buf[2] = FRAME_TYPE_LOCK_VERSION;
    powerboard->send_data_buf[3] = DATA_DIRECTION_X86_TO_LOCK;
    powerboard->send_data_buf[4] = this->CalCheckSum(powerboard->send_data_buf, 4);
    powerboard->send_data_buf[5] = PROTOCOL_TAIL;
    this->send_serial_data(powerboard);
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(powerboard);
    if(error < 0)
    {

    }
    return error;
}
#else
int NoahPowerboard::get_lock_version(powerboard_t *powerboard)
{
    int error = -1;
    powerboard->send_data_buf[0] = 0x5a;
    powerboard->send_data_buf[1] = 6;
    powerboard->send_data_buf[2] = 0x20;
    powerboard->send_data_buf[3] = 3;
    powerboard->send_data_buf[4] = 0x69;
    powerboard->send_data_buf[5] = 0xa5;
    this->send_serial_data(powerboard);
    usleep(TEST_WAIT_TIME);
#if 0
    error = this->handle_receive_data(powerboard);
    if(error < 0)
    {

    }
#endif
    return error;
}

#endif


int NoahPowerboard::handle_receive_data(powerboard_t *sys)
{
    int nread = 0;
    int i = 0;
    int j = 0;
    int data_Len = 0;
    int frame_len = 0;
    unsigned char recv_buf[BUF_LEN] = {0};
    unsigned char recv_buf_complete[BUF_LEN] = {0};
    unsigned char recv_buf_temp[BUF_LEN] = {0};

    struct stat file_info;
    int error = -1;
    if(0 != last_unread_bytes)
    {
        for(j=0;j<last_unread_bytes;j++)
        {
            recv_buf_complete [j] = recv_buf_last[j];
        }
    }
    //PowerboardInfo("start read ...");
    //PowerboardInfo("rcv device is %d",sys->device);
    if((nread = read(sys->device, recv_buf, BUF_LEN))>0)
    { 
        PowerboardInfo("read complete ... ");
        memcpy(recv_buf_complete+last_unread_bytes,recv_buf,nread);
        data_Len = last_unread_bytes + nread;
        last_unread_bytes = 0;
        for(uint8_t i = 0; i < data_Len; i++)
        {
            PowerboardInfo("rcv buf[%d]: %2x ",last_unread_bytes + i, recv_buf_complete[last_unread_bytes + i]);
        }

        while(i<data_Len)
        {
            if(0xcc == recv_buf_complete [i])
            {

                //frame_len = recv_buf_complete[i+1]; 
                //ROS_WARN("recv_buf_complete[%d]: is %d",i,recv_buf_complete[i]);
                //ROS_WARN("recv_buf_complete[%d]: is %d",i+1,recv_buf_complete[i+1]);
                if(recv_buf_complete[i+1] != 0xcc)
                {
                    frame_len = recv_buf_complete[i+1] - 1; 
                }
                else
                {
                    i++;
                    continue;
                }
                ROS_WARN("frame len is : %d",frame_len);
                if(i+frame_len <= data_Len)
                {
                    if((0xA5 == recv_buf_complete[i+frame_len-1]) && (i + frame_len > 0))
                        //if(0xA5 == recv_buf_complete[i+frame_len-2])
                    {
                        for(j=0;j<frame_len;j++)
                        {
                            recv_buf_temp[j] = recv_buf_complete[i+j];
                            PowerboardInfo("rcv buf %d is %2x",j, recv_buf_temp[j]);
                        }
                        ROS_INFO("get one frame");
                        error = this->handle_rev_frame(sys,recv_buf_temp);
                        i = i+ frame_len;
                    }
                    else
                    {
                        i++;
                    }
                }
                else
                {
                    last_unread_bytes = data_Len - i;
                    for(j=0;j<last_unread_bytes;j++)
                    {
                        recv_buf_last[j] = recv_buf_complete[i+j];
                    }
                    break;
                }
            }
            else 
            {
                i++;
            }
        }
    }
    else 
    {
        i = stat(sys->dev,&file_info);
        if(-1 == i)
        {
            //sys->com_state = COM_CLOSING;
        }
    }
    return error;
}


void NoahPowerboard::pub_json_msg_to_app( const nlohmann::json j_msg)
{
    std_msgs::String pub_json_msg;
    std::stringstream ss;

    ss.clear();
    ss << j_msg;
    pub_json_msg.data = ss.str();
    this->noah_powerboard_pub.publish(pub_json_msg);
}

int NoahPowerboard::handle_rev_frame(powerboard_t *sys,unsigned char * frame_buf)
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
        PowerboardInfo("led receive frame check error");
        //return -1;
    }
    PowerboardInfo("Powrboard recieve data check OK.");

    cmd_type = frame_buf[2];
    data_direction = frame_buf[3];
    data_len = frame_buf[1];
    ROS_INFO("cmd_type : %d",cmd_type);
    ROS_INFO("direction : %d",data_direction);
    ROS_INFO("data_len : %d",data_len);
    switch(data_direction)
    {
        //case DATA_DIRECTION_X86_TO_LOCK:
        //break;
        case DATA_DIRECTION_LOCK_ACK:
            switch(cmd_type)
            {
                case FRAME_TYPE_UNLOCK:
                    {
                        lock_serials_stauts_t single_status;

                        ROS_WARN("UNLOCK : receive ack data from lock."); 
                        //lock_serials_status
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
                        //ROS_ERROR("receive pass word: %s",pw.data());
                        input_pw.push_back(pw);

                        for(std::vector<lock_pivas_t>::iterator it = lock_match_db_vec.begin(); it != lock_match_db_vec.end(); it++)
                        {
                            if((*it).password == pw)
                            {
                                ROS_INFO("get right pass word");
                                //ROS_ERROR("get right pass word");
                                do
                                {
                                    boost::mutex::scoped_lock(tmx_smart_lock);
                                    to_unlock_serials.clear();
                                    //to_unlock_serials.push_back((*it).lock_id);
                                    to_unlock_serials.push_back((*it).door_id);
                                }while(0);
                                status = 0;
                                //unlock(sys_powerboard);                    
                            }
                        }
                        if(pw == super_password)
                        {
                            status = 0;
                            ROS_WARN("get right super password");
                        }
                        //pub_info_to_agent(3,pw ,status);
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
                        //for(uint8_t i = 0; i < 4; i++)
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
                                to_unlock_serials.clear();
                                to_unlock_serials.push_back((*it).door_id);
                                status = 0;
                                do
                                {
                                    boost::mutex::scoped_lock(tmx_smart_lock);
                                    to_unlock_serials.clear();
                                    //to_unlock_serials.push_back((*it).lock_id);
                                    to_unlock_serials.push_back(1);
                                }while(0);
                                //unlock(sys_powerboard);                    

                            }
                        }
                        if(rfid == super_rfid)
                        {
                            status = 0;
                            ROS_WARN("get right super RFID");
                        }
                        //pub_info_to_agent(2,rfid, status);
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
                        for( int i = 0; i < data_len - 7; i++)
                        {
                            qr_code.push_back(frame_buf[4+i]);
                        }
                        //ROS_INFO("receive QR code: %s",qr_code.data());
                        ROS_ERROR("receive QR code: %s",qr_code.data());
                        input_qr_code.push_back(qr_code);
                        //pub_info_to_agent(1,qr_code,0);
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



















