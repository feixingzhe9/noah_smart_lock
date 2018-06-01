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
#include <smart_lock.h>
#define TEST_WAIT_TIME     90*1000

#define PowerboardInfo     ROS_INFO

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


std::vector<uint8_t> to_unlock_serials;

std::string lock_version;


//extern NoahPowerboard  powerboard;
int NoahPowerboard::PowerboardParamInit(void)
{
    //char dev_path[] = "/dev/ttyUSB0";
    //char dev_path[] = "/dev/ros/powerboard";
    char dev_path[] = "/dev/ttyUSB0";
    memcpy(sys_powerboard->dev,dev_path, sizeof(dev_path));
    sys_powerboard->led_set.effect = LIGHTS_MODE_DEFAULT;
    return 0;
}

void *uart_protocol_process(void* arg)
{
    NoahPowerboard *pNoahPowerboard =  (NoahPowerboard*)arg; 
    pNoahPowerboard->handle_receive_data(sys_powerboard);
    usleep(500*1000);
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
        PowerboardInfo("noah_power send_buf len: %d small 0!",send_buf_len);
        return -1;
    }
    for(int i =0;i<send_buf_len;i++)
    {
        //PowerboardInfo("noah_power send_buf :%02x",sys->send_data_buf[i]);
    }
    len = write(sys->device,sys->send_data_buf,send_buf_len);
    if (len == send_buf_len)
    {
        //PowerboardInfo("noah_powerboard send ok");
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

    uint8_t lock_num = powerboard->lock_serials.size();
    int error = -1;
    powerboard->send_data_buf[0] = PROTOCOL_HEAD;
    powerboard->send_data_buf[1] = lock_num + 6;
    powerboard->send_data_buf[2] = FRAME_TYPE_UNLOCK;
    powerboard->send_data_buf[3] = DATA_DIRECTION_X86_TO_LOCK;

    //for(std::vector<uint8_t>::iterator my_iterator = powerboard->lock_serials.begin() ; my_iterator != powerboard->lock_serials.end(); my_iterator++)
    for(uint8_t i = 0; i < lock_num; i++)
    {
        powerboard->send_data_buf[4 + i]  = powerboard->lock_serials[i];
    }
    powerboard->send_data_buf[lock_num + 4] = this->CalCheckSum(powerboard->send_data_buf, lock_num + 4);
    powerboard->send_data_buf[lock_num + 5] = PROTOCOL_TAIL;
    this->send_serial_data(powerboard);
    usleep(TEST_WAIT_TIME);
    if((error = this->handle_receive_data(powerboard)) < 0)
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
    return error;
}






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
    PowerboardInfo("start read ...");
    PowerboardInfo("rcv device is %d",sys->device);
    if((nread = read(sys->device, recv_buf, BUF_LEN))>0)
    { 
        PowerboardInfo("read complete ... ");
        memcpy(recv_buf_complete+last_unread_bytes,recv_buf,nread);
        data_Len = last_unread_bytes + nread;
        last_unread_bytes = 0;
        for(i = 0; i < data_Len; i++)
        {
            PowerboardInfo("rcv buf %2x ", recv_buf_complete[last_unread_bytes + i]);
        }

        while(i<data_Len)
        {
            if(PROTOCOL_HEAD == recv_buf_complete [i])
            {

                frame_len = recv_buf_complete[i+1]; 
                if(i+frame_len <= data_Len)
                {
                    if(PROTOCOL_TAIL == recv_buf_complete[i+frame_len-1])
                    {
                        for(j=0;j<frame_len;j++)
                        {
                            recv_buf_temp[j] = recv_buf_complete[i+j];
                            PowerboardInfo("rcv buf %d is %2x",j, recv_buf_temp[j]);
                        }

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
    frame_len = frame_buf[1];

    for(i=0;i<frame_len-2;i++)
    {
        check_data += frame_buf[i];
    }

    if(check_data != frame_buf[frame_len-2] || PROTOCOL_TAIL != frame_buf[frame_len -1])
    {
        PowerboardInfo("led receive frame check error");
        return -1;
    }
    PowerboardInfo("Powrboard recieve data check OK.");

    cmd_type = frame_buf[2];
    data_direction = frame_buf[3];
    data_len = frame_buf[1];
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
                        pw.resize(4);
                        pw.clear();
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            pw.push_back(frame_buf[4+i]);
                        }
                        ROS_INFO("receive pass word: %s",pw.data());
                        input_pw.push_back(pw);

                        for(std::vector<lock_match_t>::iterator it = lock_match_db.begin(); it != lock_match_db.end(); it++)
                        {
                            if((*it).pw == pw)
                            {
                                ROS_INFO("get right pass word");
                                to_unlock_serials.clear();
                                to_unlock_serials.push_back((*it).lock_id);
                            }
                        }
                        pub_info_to_agent(3,pw);
                    }
                    break;

                case FRAME_TYPE_RFID_UPLOAD:
                    {
                        std::string rfid;
                        rfid.resize(4);
                        rfid.clear();
                        for(uint8_t i = 0; i < 4; i++)
                        {
                            rfid.push_back(frame_buf[4+i]);
                        }
                        ROS_INFO("receive RFID: %s",rfid.data());
                        input_rfid.push_back(rfid);
                        for(std::vector<lock_match_t>::iterator it = lock_match_db.begin(); it != lock_match_db.end(); it++)
                        {
                            if((*it).rfid == rfid)
                            {
                                ROS_INFO("get right pass word");
                                to_unlock_serials.clear();
                                to_unlock_serials.push_back((*it).lock_id);

                            }
                        }
                        pub_info_to_agent(2,rfid);
                    }
                    break;
                case FRAME_TYPE_QR_CODE_UPLOAD:
                    {
                        std::string qr_code;
                        qr_code.clear();
                        for(uint8_t i = 0; i < data_len - 5; i++)
                        {
                            qr_code.push_back(frame_buf[4+i]);
                        }
                        ROS_INFO("receive QR code: %s",qr_code.data());
                        input_qr_code.push_back(qr_code);
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




void NoahPowerboard::pub_info_to_agent(uint8_t type, std::string data)
{
    json j;
    time_t t;
    static int uuid = 0;
    struct tm *my_tm;
    struct tm *t2;
    char buf[128] = {0};
    my_tm = localtime(&t);
    //sprintf(buf, "%04d-%02d-%02d  %02d:%02d:%02d", my_tm->tm_year + 1900, my_tm->tm_mon + 1, my_tm->tm_mday, my_tm->tm_hour, my_tm->tm_min, my_tm->tm_sec);
    sprintf(buf, "%04d%02d%02d  %02d:%02d:%02d", my_tm->tm_year + 1900, my_tm->tm_mon + 1, my_tm->tm_mday, my_tm->tm_hour, my_tm->tm_min, my_tm->tm_sec);
    ROS_INFO("%s\n", buf);              
    //ROS_INFO("%s\n", asctime(t2));              
    //pub_to_agent
    uuid++;
    j.clear();
    j =
    {
        {"uuid",uuid},
        {"sub_name","smart_lock_notice"},

        {
            "data",
            {
                {"type", type},

                {"data",data.data()},
                {"time", buf},
            }
        }

    };

    std_msgs::String pub_json_msg;
    std::stringstream ss;
    ss.clear();
    ss << j;
    pub_json_msg.data = ss.str();
    for(uint8_t i = 0; i < 10; i++)
    {
        pub_to_agent.publish(pub_json_msg);
        usleep(20*1000);
    }
}















