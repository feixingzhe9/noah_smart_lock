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
#include <smart_lock.h>
#define TEST_WAIT_TIME     90*1000

#define PowerboardInfo     ROS_INFO

static int led_over_time_flag = 0;
static int last_unread_bytes = 0;
static unsigned char recv_buf_last[BUF_LEN] = {0};

powerboard_t    sys_powerboard_ram; 
powerboard_t    *sys_powerboard = &sys_powerboard_ram;

std::vector<lock_serials_stauts_t> lock_serials_status;

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
    usleep(100*1000);
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
            ROS_ERROR("Set leds effect start to resend");
            goto begin; 
        }
        ROS_ERROR("Set Leds Effecct : com error !");
    }
    else
    {
        err_cnt = 0;
    }
    return error;
}






















int NoahPowerboard::SetLedEffect(powerboard_t *powerboard)     // done
{
begin:
    static uint8_t err_cnt = 0;

    int error = -1;
    powerboard->send_data_buf[0] = PROTOCOL_HEAD;
    powerboard->send_data_buf[1] = 0x0a;
    powerboard->send_data_buf[2] = FRAME_TYPE_LEDS_CONTROL;
    powerboard->send_data_buf[3] = powerboard->led_set.effect;
    memcpy(&powerboard->send_data_buf[4], (uint8_t *)&(powerboard->led_set.color), sizeof(color_t));
    powerboard->send_data_buf[7] = powerboard->led_set.period;
    powerboard->send_data_buf[8] = this->CalCheckSum(powerboard->send_data_buf, 8);
    powerboard->send_data_buf[9] = PROTOCOL_TAIL;
    this->send_serial_data(powerboard);
    usleep(TEST_WAIT_TIME);
    if((error = this->handle_receive_data(powerboard)) < 0)
    {
        
        if(err_cnt++ < COM_ERR_REPEAT_TIME)
        {
            usleep(500*1000); 
            ROS_ERROR("Set leds effect start to resend");
            goto begin; 
        }
        ROS_ERROR("Set Leds Effecct : com error !");
    }
    else
    {
        err_cnt = 0;
    }
    return error;
}
int NoahPowerboard::GetBatteryInfo(powerboard_t *sys)      // done
{
    int error = 0;
    sys->send_data_buf[0] = PROTOCOL_HEAD;
    sys->send_data_buf[1] = 6;
    sys->send_data_buf[2] = FRAME_TYPE_BAT_STATUS;
    sys->send_data_buf[3] = sys->bat_info.cmd;
    sys->send_data_buf[4] = this->CalCheckSum(sys->send_data_buf, 4);
    sys->send_data_buf[5] = PROTOCOL_TAIL;
    this->send_serial_data(sys);
#if 0
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(sys);
#endif
    return error;
}
int NoahPowerboard::SetModulePowerOnOff(powerboard_t *sys)
{
begin:
    static uint8_t err_cnt = 0;
    int error = -1; 
    boost::mutex io_mutex;
    boost::mutex::scoped_lock lock(io_mutex);
    sys->send_data_buf[0] = PROTOCOL_HEAD;
    sys->send_data_buf[1] = 10;
    sys->send_data_buf[2] = FRAME_TYPE_MODULE_CONTROL;
    memcpy(&sys->send_data_buf[3],(uint8_t *)&sys->module_status_set.module, 4 );
    sys->send_data_buf[7] = sys->module_status_set.on_off;
    sys->send_data_buf[8] = this->CalCheckSum(sys->send_data_buf, 8);
    sys->send_data_buf[9] = PROTOCOL_TAIL;
    this->send_serial_data(sys);
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(sys);
    if(error < 0)
    {
        if(err_cnt++ < COM_ERR_REPEAT_TIME)
        {
            usleep(50*1000); 
            goto begin; 
        }
        ROS_ERROR("com error");
        if(sys->module_status_set.module & POWER_VSYS_24V_NV)
        {

            this->j.clear();
            this->j = 
            {
                {"sub_name","set_module_state"},
                {
                    "data",
                    {
                        //{"_xx_xxx_state",!(bool)(sys->module_status.module & POWER_5V_EN)},
                        {"door_ctrl_state",(bool)(sys->module_status.module & POWER_VSYS_24V_NV)},
                        {"error_code", error},
                    } 
                }
            };
            this->pub_json_msg_to_app(this->j);
        }
    }
    else 
    {
        err_cnt = 0;
        
        if(sys->module_status_set.module & POWER_VSYS_24V_NV)
        {

            ROS_INFO("module %d",sys->module_status_set.module);

            this->j.clear();
            this->j = 
            {
                {"sub_name","set_module_state"},
                {
                    "data",
                    {
                        //{"_xx_xxx_state",!(bool)(sys->module_status.module & POWER_5V_EN)},
                        {"door_ctrl_state",(bool)(sys->module_status.module & POWER_VSYS_24V_NV)},
                        {"error_code", error},
                    } 
                }
            };
            this->pub_json_msg_to_app(this->j);
        }
        if(sys->module_status_set.module & POWER_24V_PRINTER)
        {
            
            this->j.clear();
            this->j = 
            {
                {"sub_name","set_module_state"},
                {
                    "data",
                    {
                        //{"_xx_xxx_state",!(bool)(sys->module_status.module & POWER_5V_EN)},
                        {"elevator_led_state",(bool)(sys->module_status.module & POWER_24V_PRINTER)},
                        {"error_code", error},
                    } 
                }
            };
            this->pub_json_msg_to_app(this->j);
        }
        
        if(sys->module_status_set.module & POWER_24V_EXTEND)
        {
            
        }
    }
    return error;
}

int NoahPowerboard::GetModulePowerOnOff(powerboard_t *sys)
{
    int error = -1;
    sys->send_data_buf[0] = PROTOCOL_HEAD;
    sys->send_data_buf[1] = 6;
    sys->send_data_buf[2] = FRAME_TYPE_GET_MODULE_STATE;
    sys->send_data_buf[3] = 1;
    sys->send_data_buf[4] = CalCheckSum(sys->send_data_buf, 4);
    sys->send_data_buf[5] = PROTOCOL_TAIL;
    this->send_serial_data(sys);
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(sys);
    if(error < 0)
    {
        
    }
    return error;
}
int NoahPowerboard::GetAdcData(powerboard_t *sys)      // done
{
    int error = -1;
    sys->send_data_buf[0] = PROTOCOL_HEAD;
    sys->send_data_buf[1] = 8;
    sys->send_data_buf[2] = FRAME_TYPE_GET_CURRENT;
    sys->send_data_buf[3] = 0x01;
    sys->send_data_buf[4] = 0x01;
    sys->send_data_buf[5] = sys->current_cmd_frame.cmd;
    sys->send_data_buf[6] = this->CalCheckSum(sys->send_data_buf, 5);
    sys->send_data_buf[7] = PROTOCOL_TAIL;
    this->send_serial_data(sys);
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(sys);
    if(error < 0)
    {
        
    }
    return error;
}

int NoahPowerboard::GetVersion(powerboard_t *sys)      // done
{
    int error = -1;
    sys->send_data_buf[0] = PROTOCOL_HEAD;
    sys->send_data_buf[1] = 6;
    sys->send_data_buf[2] = FRAME_TYPE_GET_VERSION;
    sys->send_data_buf[3] = sys->get_version_type;
    sys->send_data_buf[4] = this->CalCheckSum(sys->send_data_buf, 4);
    sys->send_data_buf[5] = PROTOCOL_TAIL;
    this->send_serial_data(sys);
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(sys);
    if(error < 0)
    {
        
    }
    return error;
}

int NoahPowerboard::GetSysStatus(powerboard_t *sys)     // done
{
    int error = 0;
    sys->send_data_buf[0] = PROTOCOL_HEAD;
    sys->send_data_buf[1] = 6;
    sys->send_data_buf[2] = FRAME_TYPE_SYS_STATUS;
    sys->send_data_buf[3] = 0x00;
    sys->send_data_buf[4] = this->CalCheckSum(sys->send_data_buf, 4);
    sys->send_data_buf[5] = PROTOCOL_TAIL;
    this->send_serial_data(sys);
#if 0
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(sys);
    if(error < 0)
    {
        
    }
    else
    {
    }
#endif
    return error;
}

int NoahPowerboard::InfraredLedCtrl(powerboard_t *sys)     // done
{
    int error = -1;
    sys->send_data_buf[0] = PROTOCOL_HEAD;
    sys->send_data_buf[1] = 7;
    sys->send_data_buf[2] = FRAME_TYPE_IRLED_CONTROL;
    sys->send_data_buf[3] = sys->ir_cmd.cmd;
    if(sys->send_data_buf[3] == IR_CMD_READ)
    {
        sys->send_data_buf[4] = 0;
    }
    else if(sys->send_data_buf[3] == IR_CMD_WRITE)
    {
        sys->send_data_buf[4] = sys->ir_cmd.set_ir_percent;
    }
    sys->send_data_buf[5] = this->CalCheckSum(sys->send_data_buf, 5);
    sys->send_data_buf[6] = PROTOCOL_TAIL;
    this->send_serial_data(sys);
    usleep(TEST_WAIT_TIME);
    error = this->handle_receive_data(sys);
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
            if(0x5A == recv_buf_complete [i])
            {

                frame_len = recv_buf_complete[i+1]; 
                if(i+frame_len <= data_Len)
                {
                    if(0xA5 == recv_buf_complete[i+frame_len-1])
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
        case DATA_DIRECTION_X86_TO_LOCK:
            break;
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
                        lock_serials_status.push_back(single_status);
                    }
                    for(std::vector<lock_serials_stauts_t>::iterator my_iterator = lock_serials_status.begin() ; my_iterator != lock_serials_status.end(); my_iterator++)
                    {
                        ROS_INFO("lock id:  %d,  status : %d",(*my_iterator).lock_id, (*my_iterator).status);
                    }
                    break;
                }

                default :
                    break;

            }
            
            break; 
        case DATA_DIRECTION_LOCK_TO_X86:
            break;
        case DATA_DIRECTION_X86_ACK:
            break;
        default :
            ROS_ERROR("data direciton ERROR: %d", data_direction);
            break;

    }
    switch(cmd_type)
    {
        case FRAME_TYPE_UNLOCK:
            //rcv_serial_leds_frame_t rcv_serial_led_frame;
            memcpy((uint8_t *)&sys->rcv_serial_leds_frame, &frame_buf[3], sizeof(rcv_serial_leds_frame_t) );
            PowerboardInfo("Get leds mode is %d", sys->rcv_serial_leds_frame.cur_light_mode );
            PowerboardInfo("color.r is %2x",       sys->rcv_serial_leds_frame.color.r);
            PowerboardInfo("color.g is %2x",        sys->rcv_serial_leds_frame.color.g);
            PowerboardInfo("color.b is %2x",        sys->rcv_serial_leds_frame.color.b);
            PowerboardInfo("period is %d",          sys->rcv_serial_leds_frame.period);
            error = FRAME_TYPE_LEDS_CONTROL;
            break;

        case FRAME_TYPE_BAT_STATUS:
            sys->bat_info.cmd = frame_buf[3];
            //PowerboardInfo("sys->bat_info.cmd is %d",sys->bat_info.cmd);
            if(sys->bat_info.cmd == CMD_BAT_VOLTAGE)
            {
                sys->bat_info.bat_info = frame_buf[5]<< 8  | frame_buf[4]; 
                PowerboardInfo("battery voltage is %d",sys->bat_info.bat_info);
            }
            if(sys->bat_info.cmd == CMD_BAT_PERCENT)
            {
                sys->bat_info.bat_info = frame_buf[5] << 8  | frame_buf[4]; 
                if(sys->bat_info.bat_info > 100)
                {
                    sys->bat_info.bat_info = 100;
                }
                PowerboardInfo("battery voltage is %d",sys->bat_info.bat_info);
            }
            error = FRAME_TYPE_BAT_STATUS;
            break;

        case FRAME_TYPE_GET_CURRENT:
            memcpy((uint8_t *)&sys->voltage_info.voltage_data, &frame_buf[4], sizeof(voltage_data_t));
            PowerboardInfo("_12v_voltage     is %5d mv",sys->voltage_info.voltage_data._12V_voltage);
            PowerboardInfo("_24v_voltage     is %5d mv",sys->voltage_info.voltage_data._24V_voltage);
            PowerboardInfo("_5v_voltage      is %5d mv",sys->voltage_info.voltage_data._5V_voltage);
            PowerboardInfo("bat_voltage     is %5d mv",sys->voltage_info.voltage_data.bat_voltage);
            PowerboardInfo("_24V_temp       is %d",sys->voltage_info.voltage_data._24V_temp);
            PowerboardInfo("_12V_temp       is %d",sys->voltage_info.voltage_data._12V_temp);
            PowerboardInfo("_5V_temp        is %d",sys->voltage_info.voltage_data._5V_temp);
            PowerboardInfo("air_temp        is %d",sys->voltage_info.voltage_data.air_temp);
            PowerboardInfo("send_rate       is %d",sys->voltage_info.send_rate);
            error = FRAME_TYPE_GET_CURRENT;
            break;

        case FRAME_TYPE_GET_VERSION:
            sys->get_version_type = frame_buf[3];
            if(sys->get_version_type == VERSION_TYPE_FW)
            {
                memcpy((uint8_t *)&sys->hw_version,&frame_buf[4], HW_VERSION_SIZE);
                memcpy((uint8_t *)&sys->sw_version,&frame_buf[7], SW_VERSION_SIZE);
                PowerboardInfo("hw version: %s",sys->hw_version);
                PowerboardInfo("sw version: %s",sys->sw_version);
            }

            if(sys->get_version_type == VERSION_TYPE_PROTOCOL)
            {
                memcpy((uint8_t *)&sys->protocol_version,&frame_buf[4], PROTOCOL_VERSION_SIZE);
                PowerboardInfo("protocol version: %s",sys->protocol_version);
            }

            error = FRAME_TYPE_GET_VERSION;
            break;

        case FRAME_TYPE_SYS_STATUS:
            sys->sys_status = (frame_buf[4]) | (frame_buf[5] << 8);
            ROS_INFO("sys_status is :%04x",sys->sys_status);
            switch(sys->sys_status & 0x0f)
            {
                case SYS_STATUS_OFF:
                    //PowerboardInfo("system status: off");
                    break;
                case SYS_STATUS_TURNING_ON:
                    //PowerboardInfo("system status: turning on...");
                    break;
                case SYS_STATUS_ON:
                    //PowerboardInfo("system status: on");
                    break;
                case SYS_STATUS_TURNING_OFF:
                    //PowerboardInfo("system status:turning off...");
                    break;
                case SYS_STATUS_ERR:
                    //PowerboardInfo("system status:error");
                    break;
                default:
                    break;
            }
            if(sys->sys_status & STATE_IS_CHARGER_IN )
            {
                ROS_INFO("charger plug in");
            }
            else
            {
                ROS_INFO("charger not plug in");
            }

            if(sys->sys_status & STATE_IS_RECHARGE_IN )
            {
                ROS_INFO("recharger plug in");
                this->PubChargeStatus(1);
            }
            else
            {
                ROS_INFO("recharger not plug in");
                this->PubChargeStatus(0);
            }

#if 0
            this->j.clear();
            this->j = 
            {
                {"pub_name","get_sys_status"},
                {
                    "data",
                    {
                        {"sys_status",sys->sys_status},
                    }
                }
            };
            this->pub_json_msg_to_app(this->j);
#endif 
            error = FRAME_TYPE_SYS_STATUS;
            break;

        case FRAME_TYPE_IRLED_CONTROL:
            sys->ir_cmd.lightness_percent = frame_buf[3];
            PowerboardInfo("ir lightness is %d",sys->ir_cmd.lightness_percent);
#if 0
            this->j.clear();
            this->j = 
            {
                {"sub_name","ir_lightness"},
                {
                    "data",
                    {
                        {"ir_lightness",sys->ir_cmd.lightness_percent},
                    }
                }
            };
            this->pub_json_msg_to_app(this->j);
#endif
            error = FRAME_TYPE_IRLED_CONTROL;
            break;
        case FRAME_TYPE_MODULE_CONTROL:
            {
                uint32_t tmp = 0;
                memcpy((uint8_t *)&tmp, &frame_buf[3], 4);
                if(tmp != HW_NO_SUPPORT)
                {
                    sys->module_status.module = tmp;
                    PowerboardInfo("sys->module_status.module is %08x",sys->module_status.module);
                }
                else
                {
                    PowerboardInfo("hard ware not support !");
                }
#if 0
                this->j.clear();
                this->j = 
                {
                    {"sub_name","set_module_state"},
                    {
                        "data",
                        {
                            {"module_status",sys->module_status.module},
                        } 
                    }
                };
                this->pub_json_msg_to_app(this->j);

                this->j.clear();
                this->j = 
                {
                    {"sub_name","set_module_state"},
                    {
                        "data",
                        {
                            //{"_xx_xxx_state",!(bool)(sys->module_status.module & POWER_5V_EN)},
                            {"door_ctrl_state",!(bool)(sys->module_status.module & POWER_12V_EXTEND)},
                        } 
                    }
                };
                this->pub_json_msg_to_app(this->j);
#endif
                error = FRAME_TYPE_MODULE_CONTROL;
                break; 
            }
        case FRAME_TYPE_GET_MODULE_STATE:
            {
                uint32_t tmp = 0;
                memcpy((uint8_t *)&tmp, &frame_buf[4], 4);
                if(tmp != HW_NO_SUPPORT)
                {
                    sys->module_status.module = tmp;
                    PowerboardInfo("sys->module_status.module is %08x",sys->module_status.module);
                }
                else
                {
                    PowerboardInfo("hardware not support !");
                }
#if 0
                this->j.clear();
                this->j = 
                {
                    {"sub_name","get_module_state"},
                    {
                       "data",
                       {
                           {"module_status",sys->module_status.module},
                       } 
                    }
                };
                this->pub_json_msg_to_app(this->j);
#endif
#if 0
                this->j.clear();
                this->j = 
                {
                    {"sub_name","get_module_state"},
                    {
                       "data",
                       {
                           //{"_xx_xxx_state",!(bool)(sys->module_status.module & POWER_5V_EN)},
                           {"door_ctrl_state",!(bool)(sys->module_status.module & POWER_12V_EXTEND)},
                       } 
                    }
                };
                this->pub_json_msg_to_app(this->j);
#endif
                error = FRAME_TYPE_GET_MODULE_STATE;
                break; 
            }

        default :
            break;
    }
    return error;
}

void NoahPowerboard::from_app_rcv_callback(const std_msgs::String::ConstPtr &msg)
{
    //ROS_INFO("Rcv test data");
    auto j = json::parse(msg->data.c_str());
    if(j.find("pub_name") != j.end())
    {
        //ROS_INFO("find pub_name");
        if(j["pub_name"] == "set_module_state")
        {
            //ROS_INFO("find set_module_state");



            if(j["data"]["dev_name"] == "_24v_printer")
            {
                if(j["data"]["set_state"] == true)
                {
                    ROS_INFO("set 24v printer on");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON;
                    sys_powerboard->module_status_set.module = POWER_24V_PRINTER; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
                else if(j["data"]["set_state"] == false)
                {
                    ROS_INFO("set 24v printer off");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_OFF; 
                    sys_powerboard->module_status_set.module = POWER_24V_PRINTER; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
            }
            if(j["data"]["dev_name"] == "_24v_printer")
            {
                if(j["data"]["set_state"] == true)
                {
                    ROS_INFO("set 24v printer on");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON;
                    sys_powerboard->module_status_set.module = POWER_24V_PRINTER; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
                else if(j["data"]["set_state"] == false)
                {
                    ROS_INFO("set 24v printer off");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_OFF; 
                    sys_powerboard->module_status_set.module = POWER_24V_PRINTER; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
            }



            if(j["data"]["dev_name"] == "_24v_dcdc")
            {
                if(j["data"]["set_state"] == true)
                {
                    ROS_INFO("set 24v dcdc on");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON;
                    sys_powerboard->module_status_set.module = POWER_24V_EN; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
                else if(j["data"]["set_state"] == false)
                {
                    ROS_INFO("set 24v dcdc off");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_OFF; 
                    sys_powerboard->module_status_set.module = POWER_24V_EN; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
            }



            if(j["data"]["dev_name"] == "_5v_dcdc")
            {
                if(j["data"]["set_state"] == true)
                {
                    ROS_INFO("set 5v dcdc on");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON;
                    sys_powerboard->module_status_set.module = POWER_5V_EN; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
                else if(j["data"]["set_state"] == false)
                {
                    ROS_INFO("set 5v dcdc off");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_OFF; 
                    sys_powerboard->module_status_set.module = POWER_5V_EN; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }

            }


            if(j["data"]["dev_name"] == "_12v_dcdc")
            {
                if(j["data"]["set_state"] == true)
                {
                    ROS_INFO("set 12v dcdc on");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON;
                    sys_powerboard->module_status_set.module = POWER_12V_EN; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
                else if(j["data"]["set_state"] == false)
                {
                    ROS_INFO("set 12v dcdc off");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_OFF; 
                    sys_powerboard->module_status_set.module = POWER_12V_EN; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
            }

            if(j["data"]["dev_name"] == "door_ctrl_state")
            {
                if(j["data"]["set_state"] == true)
                {
                    ROS_INFO("set door ctrl  on");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON;
                    sys_powerboard->module_status_set.module = POWER_VSYS_24V_NV; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
                else if(j["data"]["set_state"] == false)
                {
                    ROS_INFO("set door ctrl off");
                    sys_powerboard->module_status_set.on_off = MODULE_CTRL_OFF; 
                    sys_powerboard->module_status_set.module = POWER_VSYS_24V_NV; 
                    this->SetModulePowerOnOff(sys_powerboard);
                }
            }

        }
    }
}





void NoahPowerboard:: from_navigation_rcv_callback(const std_msgs::String::ConstPtr &msg)
{
    int value = atoi(msg->data.c_str());
    ROS_INFO("camera led ctrl:value is %d",value);
    ROS_INFO("%s",msg->data.c_str());
    switch(value)
    {
        case 0:
            ROS_INFO("camera led ctrl:get 00"); 
            sys_powerboard->module_status_set.on_off = MODULE_CTRL_OFF; 
            sys_powerboard->module_status_set.module = POWER_CAMERA_LED; 
            this->SetModulePowerOnOff(sys_powerboard);
            break;
        case 1:
            ROS_INFO("camera led ctrl:get 01"); 
            sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON; 
            sys_powerboard->module_status_set.module = POWER_CAMERA_LED; 
            this->SetModulePowerOnOff(sys_powerboard);
            break;
        case 10:
            ROS_INFO("camera led ctrl:get 10"); 
            sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON; 
            sys_powerboard->module_status_set.module = POWER_CAMERA_LED; 
            this->SetModulePowerOnOff(sys_powerboard);
            break;
        case 11:
            ROS_INFO("camera led ctrl:get 11"); 
            sys_powerboard->module_status_set.on_off = MODULE_CTRL_ON; 
            sys_powerboard->module_status_set.module = POWER_CAMERA_LED; 
            this->SetModulePowerOnOff(sys_powerboard);
            break;
        default :
            break;

    }
}

void NoahPowerboard::PubPower(void)
{
    unsigned char power = 0;
    power = sys_powerboard->bat_info.bat_info;
    unsigned char status = sys_powerboard->sys_status;    //std_msgs::Int8 msg;
    //msg.data=power;
    std_msgs::UInt8MultiArray bytes_msg;

    bytes_msg.data.push_back(power);
    bytes_msg.data.push_back(status);
    power_pub_to_app.publish(bytes_msg);
}
void NoahPowerboard::power_from_app_rcv_callback(std_msgs::UInt8MultiArray data)
{
   uint8_t value = data.data[0]; 
   ROS_INFO("power_from_app_rcv_callback");
   ROS_INFO("data is %d",value);
   if(value == 0)
   {
    this->PubPower();
   }
}

void NoahPowerboard::PubChargeStatus(uint8_t status)
{
    static uint8_t last_status = 0;
    std_msgs::UInt8MultiArray data;
    if(last_status != status)
    {
        data.data.push_back(status);
        pub_charge_status_to_move_base.publish(data);
        last_status = status;
    }

}





