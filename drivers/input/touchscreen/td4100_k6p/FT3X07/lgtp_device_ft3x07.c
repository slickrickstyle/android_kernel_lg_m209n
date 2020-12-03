/***************************************************************************
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *    File      : lgtp_device_ft3x07.c
 *    Author(s)   : D3 BSP Touch Team < d3-bsp-touch@lge.com >
 *    Description :
 *
 ***************************************************************************/
#define LGTP_MODULE "[FT3X07]"

/****************************************************************************
* Include Files
****************************************************************************/
#include <linux/input/unified_driver_4/lgtp_common.h>
#include <linux/input/unified_driver_4/lgtp_common_driver.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_i2c.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_misc.h>
#include <linux/input/unified_driver_4/lgtp_device_ft3x07.h>
#include <linux/input/unified_driver_4/lgtp_model_config_misc.h>
#include <linux/time.h>
#include "focaltech_ctl.h"
#include "ft6x06_ex_fun.h"
#include "TestLimits_ft3x07.h"

/****************************************************************************
* Manifest Constants / Defines
****************************************************************************/


/****************************************************************************
 * Macros
 ****************************************************************************/


/****************************************************************************
* Type Definitions
****************************************************************************/


/****************************************************************************
* Variables
****************************************************************************/

#if defined( TOUCH_MODEL_E0)
static const char defaultFirmware[] = "focaltech/E0/FT3X07/0_LG_E0_GLOBAL_3407_Ver0x10_2_20160422_app.bin";
#endif
static struct regulator *vdd_vio;
static TouchDriverData *gTouchDriverData;
int enable_iic_dev = 0;
static const char normal_sd_path[] = "/sdcard/touch_self_test.txt";
static const char factory_sd_path[] = "/data/touch/touch_self_test.txt";
static const char mfts_sd_path[] = "/data/touch/touch_self_mfts.txt";
static const char incoming_call_state[3][8] = {"IDLE", "RINGING", "OFFHOOK"};
static u8 buffer_data[64] = {0,};
atomic_t DeepSleep_flag;
atomic_t Lpwg_flag;
atomic_t ime_state;
atomic_t incoming_call;

/****************************************************************************
* Extern Function Prototypes
****************************************************************************/
extern int lge_get_factory_boot(void);
extern bool lge_get_mfts_mode(void);
extern void SetDriverState(TouchDriverData *pDriverData, TouchState newState);


/****************************************************************************
* Local Function Prototypes
****************************************************************************/
static int Ft3x07_Set_KnockOn(void);
static int Ft3x07_Set_KnockCode(LpwgSetting  *pLpwgSetting);
static void get_regulator(void);
static void Ft3x07_Reset(void);
static int Ft3x07_SetLpwgMode( TouchState newState, LpwgSetting  *pLpwgSetting);


/****************************************************************************
* Local Functions
****************************************************************************/

static int FirmwareUpgrade(const struct firmware *fw_img)
{
	u8 * pbt_buf = NULL;

	int i_ret;
	int fw_len = fw_img->size;

	/*FW upgrade*/
	pbt_buf = (u8*)fw_img->data;

	/*call the upgrade function*/
	i_ret =  fts_ctpm_fw_upgrade(gTouchDriverData->client, pbt_buf, fw_len);
	if (i_ret != 0) {
		TOUCH_ERR("Firwmare upgrade failed. err=%d.\n", i_ret);
	}
	else {
#ifdef AUTO_CLB
		fts_ctpm_auto_clb(gTouchDriverData->client);	/*start auto CLB*/
#endif
	}
	return i_ret;
}

static int Ft3x07_Auto_Cal_Ctrl(u8 cal_cmd, u8 read_cmd)
{
	u8 addr = 0;
	TOUCH_FUNC();

	/* Disable Auto Calibration. */
	addr = 0xEE;
	if(Touch_I2C_Write(addr, &cal_cmd, sizeof(cal_cmd)) != TOUCH_SUCCESS) {
		TOUCH_LOG("Disable auto calibration error. \n");
		return TOUCH_FAIL;
	}
	msleep(50);

	/* Select data of touch channel. (Raw/Delta) */
	addr = 0xBB;
	if(Touch_I2C_Write(addr, &read_cmd, sizeof(read_cmd)) != TOUCH_SUCCESS) {
		TOUCH_LOG("Select channel data error.(Raw/Delta) \n");
		return TOUCH_FAIL;
	}
	msleep(50);
	return TOUCH_SUCCESS;
}

static int Ft3x07_Mode_Change(u8 cmd)
{
	u8 addr = FT_DEVICE_MODE;
	u8 reply_data = 0;
	TOUCH_FUNC();

	/* Normal Mode : 0x00, Factory mode : 0x40 */
	if(Touch_I2C_Write(addr, &cmd, sizeof(cmd)) != TOUCH_SUCCESS) {
		return TOUCH_FAIL;
	} else {
		msleep(50);
		if(Touch_I2C_Read(addr, &reply_data, sizeof(reply_data)) != TOUCH_SUCCESS) {
			return TOUCH_FAIL;
		} else {
			/*Check factory mode(0x40)*/
			if(cmd != reply_data) {
				TOUCH_LOG("Device mode reply data error.(reply data = %x)\n", reply_data);
				return TOUCH_FAIL;
			}
		}
	}
	msleep(10);
	return TOUCH_SUCCESS;
}

static int Ft3x07_Read_ChannelData(u8 addr_read, u8 start_ch, u8 read_buf)
{
	u8 addr = 0;
	u8 reply_data = 0;
	u8 command = 0;

	addr = FT_START_SCAN;
	if(Touch_I2C_Read(addr, &reply_data, sizeof(reply_data)) != TOUCH_SUCCESS) {
		return TOUCH_FAIL;
	} else {
		if(reply_data == 0x00) {
			TOUCH_LOG("Start scan reply data OK(0x%X) \n", reply_data);
		} else {
			TOUCH_LOG("Start scan reply data ERROR(0x%X) \n", reply_data);
			return TOUCH_FAIL;
		}
	}
	command = 0x01;
	if(Touch_I2C_Write(addr, &command, sizeof(command)) != TOUCH_SUCCESS) {
		return TOUCH_FAIL;
	} else {
		do {
			msleep(15);
			if(Touch_I2C_Read(addr, &reply_data, sizeof(reply_data)) != TOUCH_SUCCESS) {
				return TOUCH_FAIL;
			}
		}while(reply_data != 0x00);

		if(Touch_I2C_Write(addr_read, &start_ch, sizeof(start_ch)) != TOUCH_SUCCESS) {
			TOUCH_LOG("Start scan set error. (0x%X)\n", addr_read);
			return TOUCH_FAIL;
		}
		if(Touch_I2C_Read(read_buf, buffer_data, MAX_CHANNEL_NUM *2) != TOUCH_SUCCESS) {
			TOUCH_LOG("Fail to read Channel data. \n");
			return TOUCH_FAIL;
		}
	}
	return TOUCH_SUCCESS;
}

static ssize_t show_use_iic_dev(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	WRITE_SYSBUF(buf, ret, "%u\n", enable_iic_dev);
	return ret;
}

static ssize_t store_use_iic_dev(TouchDriverData *pDriverData, const char *buf, size_t count)
{

    int value = 0;

    sscanf(buf, "%d", &value);

    if (value < 0 || value > 1) {
        TOUCH_LOG("Invalid enable_rmi_dev value:%d\n", value);
        return count;
    }

    TOUCH_LOG("enable_iic_dev:%u value: %d \n", enable_iic_dev ,value);

    if (enable_iic_dev==0 && value==1) {

#ifdef SYSFS_DEBUG
        ft6x06_create_sysfs(gTouchDriverData->client);
#endif
#ifdef FTS_CTL_IIC
        if (ft_rw_iic_drv_init(gTouchDriverData->client) < 0)
            TOUCH_ERR("[FTS] create fts control iic driver failed\n");
#endif
#ifdef FTS_APK_DEBUG
        ft6x06_create_apk_debug_channel(gTouchDriverData->client);
#endif
        enable_iic_dev=value;

    }
    else if(enable_iic_dev==1 && value==0){
/*to do disable debug func, please reboot the device*/
#if 0
#ifdef SYSFS_DEBUG
        ft6x06_release_sysfs(client);
#endif
#ifdef FTS_CTL_IIC
        ft_rw_iic_drv_exit();
#endif
#ifdef FTS_APK_DEBUG
        ft6x06_release_apk_debug_channel();
#endif

        enable_iic_dev=value;
#endif
    }
    return count;
}
static LGE_TOUCH_ATTR(enable_iic_dev, S_IRUGO | S_IWUSR, show_use_iic_dev, store_use_iic_dev);

static ssize_t show_version_read(TouchDriverData *pDriverData, char *buf)
{
    int ret = 0;
    return ret;
}
static LGE_TOUCH_ATTR(version_read, S_IRUGO | S_IWUSR, show_version_read, NULL);

static ssize_t show_Pincheck(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int val_gpio = 0;

	val_gpio = mt_get_gpio_dir(GPIO_TOUCH_INT);
	TOUCH_LOG("[DEBUG] get GPIO_TOUCH_INT DIR value  = %d\n", val_gpio);

	val_gpio = mt_get_gpio_in(GPIO_TOUCH_INT);
	TOUCH_LOG("[DEBUG] get GPIO_TOUCH_INT value  = %d\n", val_gpio);

	val_gpio = mt_get_gpio_dir(GPIO_TOUCH_RESET);
	TOUCH_LOG("[DEBUG] get GPIO_TOUCH_RESET DIR value  = %d\n", val_gpio);

	val_gpio = mt_get_gpio_in(GPIO_TOUCH_RESET);
	TOUCH_LOG("[DEBUG] get GPIO_TOUCH_RESET value  = %d\n", val_gpio);

	TOUCH_LOG("[DEBUG]GPIO_TOUCH_INT address = 0x%x\n", GPIO_TOUCH_INT);
	TOUCH_LOG("[DEBUG]GPIO_TOUCH_RESET address = 0x%x\n", GPIO_TOUCH_RESET);
	TOUCH_LOG("[DEBUG]GPIO62 = 0x%x\n", GPIO62);
	TOUCH_LOG("[DEBUG]GPIO10 = 0x%x\n", GPIO10);

    return ret;
}
static LGE_TOUCH_ATTR(Pincheck, S_IRUGO | S_IWUSR, show_Pincheck, NULL);

static ssize_t show_knockon(TouchDriverData *pDriverData, char *buf)
{
    int ret = TOUCH_SUCCESS;
    return ret;
}
static LGE_TOUCH_ATTR(knockon, S_IRUGO | S_IWUSR, show_knockon, NULL);

static ssize_t show_Touch_Reset(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	TouchSetGpioReset(0);
	msleep(10);
	TouchSetGpioReset(1);
	msleep(200);
	TOUCH_LOG("[DEBUG]Device was reset\n");
	return ret;
}
static LGE_TOUCH_ATTR(Touch_Reset, S_IRUGO | S_IWUSR, show_Touch_Reset, NULL);

static ssize_t show_delta(TouchDriverData *pDriverData, char *buf)
{
	int dataLen = 0;
	int i,j = 0;
	int delta_data[MAX_CHANNEL_NUM] = {0,};

	memset(buffer_data, 0, sizeof(buffer_data));

	mutex_lock(&pDriverData->thread_lock);
	//TouchDisableIrq();
	SetDriverState(pDriverData, STATE_SELF_DIAGNOSIS);

	TOUCH_LOG("===== Delta Data =====\n");
	WRITE_SYSBUF(buf, dataLen, "===== Delta Data =====\n");

	/* Set disable auto cal, Read delta mode. */
	if(Ft3x07_Auto_Cal_Ctrl(0x01, 0x01) != TOUCH_SUCCESS)
		goto fail;

	/* Set Factory Mode. */
	if(Ft3x07_Mode_Change(0x40) != TOUCH_SUCCESS)
		goto fail;

	/* Read delta data. */
	if(Ft3x07_Read_ChannelData(0x34, 0x00, 0x35) != TOUCH_SUCCESS)
		goto fail;

	/* Print delta data. */
	for(i = 0; i< MAX_CHANNEL_NUM; i++) {
		delta_data[i] = ((buffer_data[i*2]<<8) | (buffer_data[i*2+1]));
	}

	for(j =0; j < 2; j++) {
		WRITE_SYSBUF(buf, dataLen, "[%2d]", j*2);
		TOUCH_LOG("[%2d]", j*2);
		for (i=0; i < 8; i++) {
			WRITE_SYSBUF(buf, dataLen, "%8d", delta_data[j*8 + i] - 10000);
			TOUCH_LOG("%8d", delta_data[j*8 + i] - 10000);
		}
		WRITE_SYSBUF(buf, dataLen, "\n[%2d]", j*2+1);
		TOUCH_LOG("\n[%2d]", j*2+1);
		for (i=0; i < 8; i++) {
			WRITE_SYSBUF(buf, dataLen, "%8d", delta_data[j*8 + i + 16] - 10000);
			TOUCH_LOG("%8d", delta_data[j*8 + i + 16] - 10000);
		}
		WRITE_SYSBUF(buf, dataLen, "\n");
		TOUCH_LOG("\n");
	}

	/* Set Normal Mode */
	if(Ft3x07_Mode_Change(0x00) != TOUCH_SUCCESS)
		goto fail;

	/* Enable auto cal*/
	if(Ft3x07_Auto_Cal_Ctrl(0x00, 0x00) != TOUCH_SUCCESS)
		goto fail;

	SetDriverState(pDriverData, STATE_NORMAL);
	//TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);
	return dataLen;

fail:
    TOUCH_LOG("===== Read Delta Fail =====\n");
	SetDriverState(pDriverData, STATE_NORMAL);
	Ft3x07_Reset();
	//TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);
	return dataLen;
}
static LGE_TOUCH_ATTR(delta, S_IRUGO | S_IWUSR, show_delta, NULL);

static ssize_t show_raw_data(TouchDriverData *pDriverData, char *buf)
{
	int dataLen = 0;
	int i,j = 0;
	int raw_data[MAX_CHANNEL_NUM] = {0,};

	memset(raw_data, 0, sizeof(raw_data));

	mutex_lock(&pDriverData->thread_lock);
	//TouchDisableIrq();
	SetDriverState(pDriverData, STATE_SELF_DIAGNOSIS);

	TOUCH_LOG("===== Raw Data =====\n");
	WRITE_SYSBUF(buf, dataLen, "===== Raw Data =====\n");

	/* Set disable auto cal, Read delta mode. */
	if(Ft3x07_Auto_Cal_Ctrl(0x01, 0x00) != TOUCH_SUCCESS)
		goto fail;

	/* Set Factory Mode. */
	if(Ft3x07_Mode_Change(0x40) != TOUCH_SUCCESS)
		goto fail;

	/* Read raw data. */
	if(Ft3x07_Read_ChannelData(0x34, 0x00, 0x35) != TOUCH_SUCCESS)
		goto fail;

	/* Print raw data. */
	for(i = 0; i< MAX_CHANNEL_NUM; i++) {
		raw_data[i] = ((buffer_data[i*2]<<8) | (buffer_data[i*2+1]));
	}

	for(j = 0; j < 2; j++) {
		WRITE_SYSBUF(buf, dataLen, "[%2d]", j*2);
		TOUCH_LOG("[%2d]", j*2);
		for (i=0; i < 8; i++) {
			WRITE_SYSBUF(buf, dataLen, "%8d", raw_data[j*8 + i]);
			TOUCH_LOG("%8d", raw_data[j*8 + i]);
		}
		WRITE_SYSBUF(buf, dataLen, "\n[%2d]", j*2+1);
		TOUCH_LOG("\n[%2d]", j*2+1);
		for (i=0; i < 8; i++) {
			WRITE_SYSBUF(buf, dataLen, "%8d", raw_data[j*8 + i + 16]);
			TOUCH_LOG("%8d", raw_data[j*8 + i + 16]);
		}
		WRITE_SYSBUF(buf, dataLen, "\n");
		TOUCH_LOG("\n");
	}

	/* Set Normal Mode */
	if(Ft3x07_Mode_Change(0x00) != TOUCH_SUCCESS)
		goto fail;

	/* Enable auto cal*/
	if(Ft3x07_Auto_Cal_Ctrl(0x00, 0x00) != TOUCH_SUCCESS)
		goto fail;

	SetDriverState(pDriverData, STATE_NORMAL);
	//TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);
	return dataLen;

fail:
    TOUCH_LOG("===== Read Delta Fail =====\n");
	SetDriverState(pDriverData, STATE_NORMAL);
	Ft3x07_Reset();
	//TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);
	return dataLen;
}
static LGE_TOUCH_ATTR(rawdata, S_IRUGO, show_raw_data, NULL);

static ssize_t show_incoming_call_state(TouchDriverData *pDriverData, char *buf)
{
    int value = 0;
    int ret = 0;

    value = atomic_read(&incoming_call);

    ret = TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_state[value], value);

    return ret;
}

static ssize_t store_incoming_call_state(TouchDriverData *pDriverData, const char *buf, size_t count)
{
    int value = 0;
    u8 txbuf = 0;
    if (sscanf(buf, "%d", &value) <= 0)
        return count;

    if (value ==  INCOMING_CALL_OFFHOOK|| value == INCOMING_CALL_RINGING) {
        atomic_set(&incoming_call, value);
        txbuf = 0x01;
        TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_state[value], value);
    } else if (value == INCOMING_CALL_IDLE) {
        atomic_set(&incoming_call, value);
        txbuf = 0x00;
        TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_state[value], value);
    } else {
        TOUCH_LOG("Unknown %d\n", value);
    }


#if 0
    /* Control RF Noise Filter */
    if (Touch_I2C_Write(0xAD, &txbuf, 1) < 0)
    {
        TOUCH_ERR("Write Call State Fail\n");
        return count;
    }
#endif
    return count;
}
static LGE_TOUCH_ATTR(incoming_call, S_IRUGO | S_IWUSR, show_incoming_call_state, store_incoming_call_state);
static ssize_t show_ime_state(TouchDriverData *pDriverData, char *buf)
{
    int value = 0;
    int ret = 0;

    value = atomic_read(&ime_state);

    ret = TOUCH_LOG("%s : %d\n", __func__, value);

    return ret;
}

static ssize_t store_ime_state(TouchDriverData *pDriverData, const char *buf, size_t count)
{
	int value = 0;
	u8 txbuf = 0;

	if(sscanf(buf, "%d", &value) <= 0)
		return count;

	if(value >= IME_OFF && value <= IME_SWYPE) {
		if (atomic_read(&ime_state) == value)
			return count;
		txbuf = value;
		atomic_set(&ime_state, value);
		TOUCH_LOG("%s : %d\n", __func__, value);
	} else {
		TOUCH_LOG("%s : Unknown %d\n", __func__, value);
	}

	if(Touch_I2C_Write(0xBA, &txbuf, 1) < 0) {
		TOUCH_ERR("Write Call State Fail\n");
		return count;
	}
	return count;
}
static LGE_TOUCH_ATTR(ime_status, S_IRUGO | S_IWUSR,show_ime_state, store_ime_state);


static struct attribute *Ft3x07_attribute_list[] = {
    &lge_touch_attr_version_read.attr,
    &lge_touch_attr_Pincheck.attr,
    &lge_touch_attr_Touch_Reset.attr,
    &lge_touch_attr_delta.attr,
    &lge_touch_attr_rawdata.attr,
    &lge_touch_attr_knockon.attr,
    &lge_touch_attr_enable_iic_dev.attr,
	&lge_touch_attr_ime_status.attr,
	&lge_touch_attr_incoming_call.attr,
	NULL,
};

static int Ft3x07_Initialize(TouchDriverData *pDriverData)
{
	struct i2c_client *client = Touch_Get_I2C_Handle();
	TOUCH_FUNC();

	gTouchDriverData = pDriverData;
	gTouchDriverData->client = client;
	return TOUCH_SUCCESS;
}

static void Ft3x07_Reset(void)
{
	TOUCH_FUNC();
	TouchSetGpioReset(0);
	msleep(10);
	TouchSetGpioReset(1);
	msleep(150);
	TOUCH_LOG("Device was reset\n");

	atomic_set(&DeepSleep_flag, 0);
	atomic_set(&Lpwg_flag, 0);
}

static int Ft3x07_InitRegister(void)
{
	TOUCH_FUNC();
	u8 txbuf = 0x00;
	TouchState newState = STATE_UNKNOWN;
	TOUCH_LOG("Device current state = %d\n", gTouchDriverData->currState);
	TOUCH_LOG("Device Next state = %d\n", gTouchDriverData->nextState);


	
    txbuf = atomic_read(&ime_state);
    /* Control IME */
    if (txbuf > 0)
        TOUCH_LOG("Ime Enabled\n");
    else
        TOUCH_LOG("Ime Disabled\n");

	if (Touch_I2C_Write(0xBA, &txbuf, 1) < 0){
		TOUCH_ERR("Write Call State Fail\n");
	}
	if(gTouchDriverData->currState != STATE_NORMAL) {
		TOUCH_LOG("LPWG Mode Rewrite after RESET on Sleep Mode\n");
		newState = gTouchDriverData->currState;
		gTouchDriverData->currState = STATE_NORMAL;
		Ft3x07_SetLpwgMode(newState, &(gTouchDriverData->lpwgSetting));
	}
    return TOUCH_SUCCESS;
}

static int get_lpwg_data(TouchReadData *pData)
{
	u8 i = 0;
    u8 buffer[50] = {0,};
	TOUCH_FUNC();

    if(Touch_I2C_Read(FT_KNOCK_READ_DATA_REG, buffer, gTouchDriverData->lpwgSetting.tapCount*4 + 2) != TOUCH_SUCCESS) {
        return TOUCH_FAIL;
    } else {
		pData->count = buffer[1];
		TOUCH_LOG("[DEBUG]TAP COUNT = %d\n", buffer[1]);
    }

    for(i = 0; i< buffer[1]; i++) {
        pData->knockData[i].x = GET_X_POSITION(buffer[4*i+2], buffer[4*i+3]);
        pData->knockData[i].y = GET_Y_POSITION(buffer[4*i+4], buffer[4*i+5]);
		TOUCH_LOG("[DEBUG] TAP DATA[%d] = %d, %d\n", i, pData->knockData[i].x, pData->knockData[i].y);
	}

    return TOUCH_SUCCESS;
}

static int Ft3x07_InterruptHandler(TouchReadData *pData)
{
	TouchFingerData *pFingerData = NULL;
	int ret = -1;
	int i = 0;
	u8 event_type = 0;
	u8 buf[POINT_READ_BUF] = { 0 };
	u8 touch_event=0;
	u8 point_id = FT_MAX_ID;
	u8 touch_count = 0;
	static u8 pressure_change = 0;

	pData->type = DATA_UNKNOWN;
	pData->count = 0;
	/* read Interrupt status */
	if(Touch_I2C_Read(FT_INT_STATUS, &event_type, sizeof(event_type)) != TOUCH_SUCCESS) {
		TOUCH_LOG("Fail to read FT_INT_STATUS\n");
		return TOUCH_FAIL;
	}

	switch(event_type) {
		case EVENT_ABS :
			ret = Touch_I2C_Read(0x00, buf, POINT_READ_BUF);
		    if (ret < 0) 
	        {
	            TOUCH_ERR("read touchdata failed.\n");
	            return TOUCH_FAIL;
	        }
	        pData->type = DATA_FINGER;
		    touch_count = buf[2]&0xf;
	    	pressure_change ^= 1;
			
		    for (i = 0; i < touch_count; i++)
		    {
	            point_id = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;
				//Ft3x07_InterruptDebuging(buf);
				if (point_id >= FT_MAX_ID)
	    			break;
	    		pFingerData = &pData->fingerData[pData->count];
	    		touch_event = buf[FT_TOUCH_EVENT_POS + FT_TOUCH_STEP * i] >> 6;

	    		pFingerData->x = (s16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) << 8 | (s16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
	    		pFingerData->y = (s16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) << 8 | (s16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];

	            if(touch_event== 0 ||touch_event==2)
	            {
						pFingerData->id  = point_id;
	                    pFingerData->width_major = 15;
	                    pFingerData->width_minor = 10;
	                    pFingerData->orientation = 1;

	                    /* to prevent Hovering detected in CFW.*/
	                    pFingerData->pressure =100 + pressure_change;
	    				pData->count++;
	            }
	        }
			break;
		case EVENT_KEY :
			/*Not use thist case*/
			break;

		case EVENT_KNOCK_ON :
			pData->type = DATA_KNOCK_ON;
			Ft3x07_Set_KnockOn();
			TOUCH_LOG("[KNOCK ON] Event Type = %d\n", event_type);
			break;

		case EVENT_KNOCK_CODE :
			pData->type = DATA_KNOCK_CODE;
            get_lpwg_data(pData);
			Ft3x07_Set_KnockCode(&(gTouchDriverData->lpwgSetting));
			TOUCH_LOG("[KNOCK CODE] Event Type = %d\n", event_type);
			break;

		default:
			TOUCH_LOG("[Unknown] Event Type = %d\n",event_type);
			break;
		}
	return TOUCH_SUCCESS;
}


static int Ft3x07_ReadIcFirmwareInfo( TouchFirmwareInfo *pFwInfo)
{
	int result = TOUCH_SUCCESS;
	u8 readData = 0;
	TOUCH_FUNC();

	/* IMPLEMENT : read IC firmware information function */
	pFwInfo->moduleMakerID = 0;
	pFwInfo->moduleVersion = 0;
	pFwInfo->modelID = 0;
	pFwInfo->isOfficial = 0;
	pFwInfo->version = 0;

	if(Touch_I2C_Read(FT_REG_FW_VER, &readData, sizeof(readData)) != TOUCH_SUCCESS) {
		result = TOUCH_FAIL;
	} else {
		//pFwInfo->isOfficial = readData>>7;
		pFwInfo->isOfficial = 1;
		pFwInfo->version = readData & 0x7f;
		TOUCH_LOG("IC Firmware Official = %d\n", pFwInfo->isOfficial);
		TOUCH_LOG("IC Firmware Version = 0x%02X\n", readData);
	}

	if(Touch_I2C_Read(FT_REG_PANEL_ID, &readData, sizeof(readData)) != TOUCH_SUCCESS) {
		result = TOUCH_FAIL;
	} else {
		TOUCH_LOG("PANEL ID = %x", readData);
	}

	if(Touch_I2C_Read(FT_REG_FW_REL, &readData, sizeof(readData)) != TOUCH_SUCCESS) {
		result = TOUCH_FAIL;
	} else {
		TOUCH_LOG("IC Firmware Release ID = %d", readData);
	}
	return result;
}

static int Ft3x07_GetBinFirmwareInfo(char *pFilename, TouchFirmwareInfo *pFwInfo)
{
	const struct firmware *fw = NULL;
	int ret = TOUCH_SUCCESS;
	char *pFwFilename = NULL;
	u8 *pFw = NULL;
	struct i2c_client *client = Touch_Get_I2C_Handle();
	TOUCH_FUNC();

	if( pFilename == NULL ) {
		pFwFilename = (char *)defaultFirmware;
	} else {
		pFwFilename = pFilename;
	}

	TOUCH_LOG("Firmware filename = %s\n", pFwFilename);

	/* Get firmware image buffer pointer from file */
	ret = request_firmware(&fw, pFwFilename, &client->dev);
	if(ret < 0) {
		TOUCH_ERR("Failed at request_firmware() ( error = %d )\n", ret);
		ret = TOUCH_FAIL;
		goto earlyReturn;
	}
	pFw = (u8 *)(fw->data);
	//pFwInfo->isOfficial = pFw[0x10a] >> 7;
	pFwInfo->isOfficial = 1;
	pFwInfo->version = pFw[0x10a]&0x7F;

	TOUCH_LOG("BIN Firmware Official = %d\n", pFwInfo->isOfficial);
	TOUCH_LOG("BIN Firmware Version = 0x%04X\n", pFwInfo->version);

	/* Free firmware image buffer */
	release_firmware(fw);

	earlyReturn:
	return ret;
}

static int Ft3x07_UpdateFirmware( char *pFilename)
{
	const struct firmware *fw = NULL;
	int result = TOUCH_SUCCESS;
	char *pFwFilename = NULL;
	u8 *pBin = NULL;
	TOUCH_FUNC();

	/* Select firmware */
	if( pFilename == NULL ) {
		pFwFilename = (char *)defaultFirmware;
	} else {
		pFwFilename = pFilename;
	}

	TOUCH_LOG("Firmware file name = %s \n", pFwFilename);

	/* Get firmware image buffer pointer from file*/
	result = request_firmware(&fw, pFwFilename, &(gTouchDriverData->client->dev));
	if( result ) {
		TOUCH_ERR("Failed at request_firmware() ( error = %d )\n", result);
		result = TOUCH_FAIL;
		return result;
	}
	pBin = (u8 *)(fw->data);

	/* Do firmware upgrade */
	result = FirmwareUpgrade(fw);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Failed at request_firmware() ( error = %d )\n", result);
		result = TOUCH_FAIL;
	}

	/* Free firmware image buffer */
	release_firmware(fw);
	return result;
}

static int Ft3x07_Set_KnockOn(void)
{
	u8 buf = 0;
	u8 rData = 0;
	int count = 0;
	int i = 0;
	int value = 0;
	TOUCH_FUNC();
	value = atomic_read(&DeepSleep_flag);
	
	/*Only reset in deep sleep status.*/
	if(value== 1) {
		Ft3x07_Reset();
		atomic_set(&DeepSleep_flag, 0);
	}
	
	buf = 0x00;
	if(Touch_I2C_Write(FT_LPWG_INT_DELAY, &buf, sizeof(buf)) != TOUCH_SUCCESS) {
		TOUCH_ERR("KNOCK ON INT DELAY write fail\n");
		return TOUCH_FAIL;
	}
	buf = 0x01;
	if(Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, sizeof(buf)) != TOUCH_SUCCESS) {
		TOUCH_ERR("KNOCK ON SET fail\n");
		return TOUCH_FAIL;
	} else {
		if(Touch_I2C_Read(FT_LPWG_CONTROL_REG, &buf, sizeof(buf)) != TOUCH_SUCCESS) {
			TOUCH_ERR("[DEBUG]KNOCK ON SET Data Read fail\n");
		} else {
			TOUCH_LOG("[DEBUG][%s]KNOCK ON SET Data = 0x%x\n", (buf == 0x01) ? "SUCCESS":"ERROR", buf);
		}
	}
	
	for(i = 0; i<5;i++) {
		if(Touch_I2C_Read(FT_LPWG_CONTROL_REG, &rData, 1) < 0) {
			TOUCH_ERR("KNOCK_ON Read Data Fail\n");
			return TOUCH_FAIL;
		} else {
			if(rData == 0x01)
				break;
			else {
				/*
				if(Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, 1) < 0) {
					TOUCH_ERR("KNOCK_ON Enable write fail\n");
					return TOUCH_FAIL;
				}
				*/
			}
		}
		count++;
		msleep(10);
	}

    if (count > 4) {
        TOUCH_ERR("KNOCK_ON Enable write fail, Rerty Count = %d\n", count);
        return TOUCH_FAIL;
    }
	
    TOUCH_LOG("LPWG Mode Changed to KNOCK_ON_ONLY\n");
	atomic_set(&Lpwg_flag, 1);
	return TOUCH_SUCCESS;
}

static int Ft3x07_Set_KnockCode(LpwgSetting  *pLpwgSetting)
{
	u8 buf = 0;
	u8 rData = 0;
	int count = 0;
	int i = 0;
	int value = 0;
	TOUCH_FUNC();
	value = atomic_read(&DeepSleep_flag);

	/*Only reset in deep sleep status.*/
	if(value== 1) {
		Ft3x07_Reset();
		atomic_set(&DeepSleep_flag, 0);
	}

	if(Touch_I2C_Write(FT_MULTITAP_COUNT_REG, &(pLpwgSetting->tapCount), sizeof(pLpwgSetting->tapCount)) != TOUCH_SUCCESS) {
		TOUCH_ERR("[DEBUG]KNOCK_CODE Tab count write fail\n");
	} else {
		TOUCH_LOG("[DEBUG]KNOCK_CODE TAB count(%d) write success\n", pLpwgSetting->tapCount);
	}

	/* 1 = first two code is same, 0 = not same */
	buf = 0x00;
	if(pLpwgSetting->isFirstTwoTapSame == 0) {
		TOUCH_LOG("First Two Tap is not same, Knock on delay set 0\n");
		if(Touch_I2C_Write(FT_LPWG_INT_DELAY, &buf, sizeof(buf)) != TOUCH_SUCCESS) {
			TOUCH_ERR("KNOCK ON INT DELAY TIME write fail\n");
		}
	} else {
		TOUCH_LOG("First Two Tap is same, Knock on delay set default value(500ms)\n");
	}
	

	buf = 0x3;
	if (Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, sizeof(buf)) != TOUCH_SUCCESS) {
		TOUCH_ERR("KNOCK CODE SET fail\n");
		return TOUCH_FAIL;
	} else {
		if(Touch_I2C_Read(FT_LPWG_CONTROL_REG, &buf, sizeof(buf)) != TOUCH_SUCCESS) {
			TOUCH_ERR("[DEBUG]KNOCK CODE SET Data Read fail\n");
		} else {
			TOUCH_LOG("[DEBUG][%s]KNOCK CODE SET Data = 0x%x\n", (buf == 0x3) ? "SUCCESS":"ERROR", buf);
		}
	}

	for(i = 0; i<5;i++) {
		if(Touch_I2C_Read(FT_LPWG_CONTROL_REG, &rData, 1) < 0) {
			TOUCH_ERR("KNOCK_ON Read Data Fail\n");
			return TOUCH_FAIL;
		} else {
			if(rData == 0x03)
				break;
			else {
				if(Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, 1) < 0) {
					TOUCH_ERR("KNOCK_ON Enable write fail\n");
					return TOUCH_FAIL;
				}
			}
		}
		count++;
		msleep(10);
	}
	if (count > 4) {
        TOUCH_ERR("KNOCK_ON Enable write fail, Rerty Count = %d\n", count);
        return TOUCH_FAIL;
    }
	
	TOUCH_LOG("LPWG Mode Changed to KNOCK_ON_CODE\n");
	atomic_set(&Lpwg_flag, 1);
	return TOUCH_SUCCESS;
}



static int Ft3x07_Set_PDN(void)
{
	TOUCH_FUNC();
	/* Only use have proximity sensor */
	u8 reg = 0xA5;
	u8 cmd = 0x03;
	int value = 0;
	value = atomic_read(&Lpwg_flag);
	if(value == 1) {
		Ft3x07_Reset();
	}

	if(Touch_I2C_Write(0xA5, &cmd, sizeof(cmd)) != TOUCH_SUCCESS) {
		TOUCH_LOG("Deep Sleep Fail");
		atomic_set(&DeepSleep_flag, 0);
	} else {
		TOUCH_LOG("Deep Sleep Success");
		atomic_set(&DeepSleep_flag, 1);
	}
	return TOUCH_SUCCESS;
}

static int Ft3x07_SetLpwgMode( TouchState newState, LpwgSetting  *pLpwgSetting)
{
	int result = TOUCH_SUCCESS;

	TOUCH_FUNC();
	TOUCH_LOG("LPWG Mode Change [%d] to [%d]\n", gTouchDriverData->currState, newState);

	if(gTouchDriverData->currState == newState) {
		TOUCH_LOG("Device state is same\n");
		return TOUCH_SUCCESS;
	}
	switch(newState) {
		case STATE_NORMAL :
			//Ft3x07_Reset();
			TOUCH_LOG("Device was set to NORMAL\n");
			break;

		case STATE_KNOCK_ON_ONLY :
			result = Ft3x07_Set_KnockOn();
			if(result < 0) {
				TOUCH_LOG("Fail to set KNOCK_ON_ONLY\n");
			} else {
				TOUCH_LOG("Device was set to KNOCK_ON_ONLY\n");
			}
			break;

		case STATE_KNOCK_ON_CODE :
			result = Ft3x07_Set_KnockCode(pLpwgSetting);
			if(result < 0) {
				TOUCH_LOG("Fail to set KNOCK_ON_CODE\n");
			} else {
				TOUCH_LOG("Device was set to KNOCK_ON_CODE\n");
			}
			break;

		case STATE_OFF :
			result = Ft3x07_Set_PDN();
			if(result < 0) {
				TOUCH_LOG("Fail to set STATE_OFF\n");
			} else {
				TOUCH_LOG("Device was set to STATE_OFF\n");
			}
			break;

		default :
			TOUCH_ERR("Unknown State %d", newState);
			break;
	}

	return result;
}

static void Ft3x07_sd_write(char *data,  int time)
{
	int fd = 0;
	char time_string[64];
	char *file_name = NULL;
	struct timespec write_time;
	struct tm write_date; 
	mm_segment_t old_fs = get_fs();
	TOUCH_FUNC();

	set_fs(KERNEL_DS);
	if(lge_get_mfts_mode() == 1) {
		TOUCH_LOG("Boot mfts\n");
		file_name = (char *)mfts_sd_path; 
	} else if(lge_get_factory_boot() == 1) {
		TOUCH_LOG("Boot minios\n");
		file_name = (char *)factory_sd_path;
	} else {
		TOUCH_LOG("Boot normal\n");
		file_name = (char *)normal_sd_path;
	}
	TOUCH_LOG("[DEBUG]Write sd file dir = %s\n", file_name);
	fd = sys_open(file_name, O_WRONLY|O_CREAT|O_APPEND|O_SYNC, 0644);

	if (fd >= 0)
	{
		if (time > 0) {
			write_time = __current_kernel_time();
			time_to_tm(write_time.tv_sec, sys_tz.tz_minuteswest * 60 * (-1), &write_date);
			snprintf(time_string, 64, "\n%02d-%02d %02d:%02d:%02d.%03lu \n\n\n",
				write_date.tm_mon + 1, write_date.tm_mday, write_date.tm_hour, write_date.tm_min,
				write_date.tm_sec, (unsigned long) write_time.tv_nsec / 1000000);
			sys_write(fd, time_string, strlen(time_string));
		}
		sys_write(fd, data, strlen(data));
		sys_close(fd);
	}
	set_fs(old_fs);
}
static int Ft3x07_ChCap_Test(char* pBuf ,int* pRawStatus, int* pChannelStatus, int* pDataLen)
{
	u8 addr = FT_DEVICE_MODE;
	u8 command = 0x40;
	u8 reply_data = 0;
	u8 i = 0;
	int dataLen = 0;
	u8 temp_data[MAX_CHANNEL_NUM*2] = {0,};
	int CB_data[MAX_CHANNEL_NUM] = {0,};
	int CB_Differ_data[MAX_CHANNEL_NUM] = {0,};
	int temp_delta = 0;
	TOUCH_FUNC();

	*pRawStatus = 0;
	*pChannelStatus = 0;

	WRITE_SYSBUF(pBuf, dataLen, "=====Test Start=====\n");
	WRITE_SYSBUF(pBuf, dataLen, "	 -CB Value-    \n");

	TOUCH_LOG("=====Test Start=====\n");
	TOUCH_LOG("    -CB Value-\n    ");

	/* Set disable auto cal, Read delta mode. */
	if(Ft3x07_Auto_Cal_Ctrl(0x01, 0x00) != TOUCH_SUCCESS)
		goto fail;

	/* enter factory mode */
	if(Touch_I2C_Write(addr, &command, 1) < 0) {
		TOUCH_LOG("Device mode enter error (0x00)\n");
		goto fail;
	} else {
		msleep(300);
		if(Touch_I2C_Read(FT_DEVICE_MODE, &reply_data, 1) < 0) {
			TOUCH_LOG("Device mode reply data read error (0x00)\n");
			goto fail;
		} else {
			if(reply_data != 0x40) {
				TOUCH_LOG("Device mode reply data error(%x)", reply_data);
				goto fail;
			}
		}
	}
	msleep(300);

	if(Ft3x07_Read_ChannelData(0x34, 0x00, 0x35) != TOUCH_SUCCESS)
		goto fail;
	for(i = 0; i < MAX_CHANNEL_NUM; i++) {
		temp_delta = ((buffer_data[i*2]<<8) | (buffer_data[i*2]));
		if(temp_delta > 17000) {
			TOUCH_LOG("[DEBUG][ERROR]Delta Data[%d] = %d\n", i, temp_delta);
			*pRawStatus = 1;
		} else {
			TOUCH_LOG("[DEBUG]Delta Data[%d] = %d\n", i, temp_delta);
		}
	}


	/* Current factory test mode */
	/* 0x00 : F_NORMAL 0x01 : F_TESTMODE_1 0x02 : F_TESTMODE_2 */
	addr =  FT_FACTORY_MODE;
	if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
		TOUCH_LOG("Current factory test mode read error (0xAE)\n");
		goto fail;
	} else {
		/*Normal mode*/
		if(reply_data == 0x00) {
			TOUCH_LOG("Current factory test mode reply data ok(%x)", reply_data);
		} else {
			TOUCH_LOG("Current factory test mode reply data error(%x)", reply_data);
			goto fail;
		}
	}
	/*Test mode 2 enter*/
	command = 0x02;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xAE)\n");
		goto fail;
	}
	msleep(10);
	addr = FT_START_SCAN;
	if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
		TOUCH_LOG("Test Mode 2 Start scan reply data error (0x08)\n");
		goto fail;
	} else {
		if(reply_data == 0x00) {
		TOUCH_LOG("Start scan reply data ok(%x)", reply_data);
		} else {
			TOUCH_LOG("Start scan reply data error(%x)", reply_data);
			goto fail;
		}
	}

	/* Start scan command */
	command = 0x01;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}
	do {
		msleep(15);
		if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
			TOUCH_LOG("Start scanf set reply data error (0x08)\n");
			goto fail;
		}
	} while(reply_data != 0x00);
	TOUCH_LOG(" Test Mode 2 - Data scan complete.\n");

	/*CB Address*/
	addr = 0x33;
	command = 0x00;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}

	addr = 0x39;
	if(Touch_I2C_Read(addr, temp_data, MAX_CHANNEL_NUM * 2) < 0) {
		TOUCH_LOG("Channel data read fail.(0x39)\n");
		goto fail;
	}

	for(i=0; i < MAX_CHANNEL_NUM; i++) {
		CB_data[i] = (temp_data[i*2] << 8) | temp_data[i*2+1];
		if( (CB_data[i] < 0) || (CB_data[i] > 900) ) {
			WRITE_SYSBUF(pBuf, dataLen, "[ERROR]CB value  [%d] = %d\n", i, CB_data[i]);
			TOUCH_LOG("[ERROR]CB Value[%d] = %d\n", i, CB_data[i]);
			*pRawStatus = 1;
			*pChannelStatus = 1;
		} else {
			WRITE_SYSBUF(pBuf, dataLen, "CB Value [%d] = %d\n", i, CB_data[i]);
			TOUCH_LOG("CB Value[%d] = %d\n", i, CB_data[i]);
		}
	}

	addr = FT_FACTORY_MODE;
	command = 0x80;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xae)\n");
		goto fail;
	}

/****************************************************************************/
	WRITE_SYSBUF(pBuf, dataLen, "    -CB Differ Value-    \n");
	/*Test mode 1 enter*/
	command = 0x01;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xAE)\n");
		goto fail;
	}
	msleep(10);
	addr = FT_START_SCAN;
	if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
		TOUCH_LOG("Test Mode 2 Start scan reply data error (0x08)\n");
		goto fail;
	} else {
		if(reply_data == 0x00) {
		TOUCH_LOG("Start scan reply data ok(%x)", reply_data);
		} else {
			TOUCH_LOG("Start scan reply data error(%x)", reply_data);
			goto fail;
		}
	}

	/* Start scan command */
	command = 0x01;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}
	do {
		msleep(15);
		if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
			TOUCH_LOG("Start scanf set reply data error (0x08)\n");
			goto fail;
		}
	} while(reply_data != 0x00);
	TOUCH_LOG(" Test Mode 1 - Data scan complete.\n");

	/*CB Address*/
	addr = 0x33;
	command = 0x00;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}

	addr = 0x39;
	if(Touch_I2C_Read(addr, temp_data, MAX_CHANNEL_NUM * 2/*channel number(36)*2 = 72 */) < 0) {
		TOUCH_LOG("Channel data read fail.(0x39)\n");
		goto fail;
	}

	for(i=0; i < MAX_CHANNEL_NUM; i++) {
		CB_Differ_data[i] = (CB_data[i] - ((temp_data[i*2] << 8) | temp_data[i*2+1])) - Ft3x07_GoldenSample[i];
		if( (CB_Differ_data[i] < -40) || (CB_Differ_data[i] > 40) ) {
			WRITE_SYSBUF(pBuf, dataLen, "[ERROR]CB Differ Value  [%d] = %d\n", i, CB_Differ_data[i]);
			TOUCH_LOG("[ERROR]CB Differ Value[%d] = %d\n", i, CB_Differ_data[i]);
			*pRawStatus = 1;
			*pChannelStatus = 1;
		} else {
			WRITE_SYSBUF(pBuf, dataLen, "CB Differ Value [%d] = %d\n", i, CB_Differ_data[i]);
			TOUCH_LOG("CB Differ Value[%d] = %d\n", i, CB_Differ_data[i]);
		}
	}
	addr = FT_FACTORY_MODE;
	command = 0x80;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xae)\n");
		goto fail;
	}
	*pDataLen += dataLen;
	//*pRawStatus = 1;
	//*pChannelStatus = 1;
	//*pRawStatus = 0;
	//*pChannelStatus = 0;

	return TOUCH_SUCCESS;
fail:
	*pRawStatus = 1;
	*pChannelStatus = 1;
	//*pRawStatus = 0;
	//*pChannelStatus = 0;
	WRITE_SYSBUF(pBuf, dataLen, "Self D Error\n");
	*pDataLen += dataLen;
	return TOUCH_FAIL;
}


static int Ft3x07_Lpwg_ChCap_Test(char* pBuf ,int* pLpwgStatus, int* pDataLen)
{
	u8 addr = FT_DEVICE_MODE;
	u8 command = 0x40;
	u8 reply_data = 0;
	u8 i = 0;
	int dataLen = 0;
	u8 temp_data[MAX_CHANNEL_NUM*2] = {0,};
	int CB_data[MAX_CHANNEL_NUM] = {0,};
	int CB_Differ_data[MAX_CHANNEL_NUM] = {0,};
	int temp_delta = 0;
	TOUCH_FUNC();

	*pLpwgStatus = 0;
	WRITE_SYSBUF(pBuf, dataLen, "=====LPWG Test Start=====\n");

	TOUCH_LOG("=====LPWG Test Start=====\n");
	TOUCH_LOG("    -LPWG CB Value-\n	");

	Ft3x07_Reset();
	/* Set disable auto cal, Read delta mode. */
	if(Ft3x07_Auto_Cal_Ctrl(0x01, 0x00) != TOUCH_SUCCESS)
		goto fail;

	/* enter factory mode */

	if(Touch_I2C_Write(addr, &command, 1) < 0) {
		TOUCH_LOG("Device mode enter error (0x00)\n");
		goto fail;
	} else {
		msleep(300);
		if(Touch_I2C_Read(FT_DEVICE_MODE, &reply_data, 1) < 0) {
			TOUCH_LOG("Device mode reply data read error (0x00)\n");
			goto fail;
		} else {
			if(reply_data != 0x40) {
			TOUCH_LOG("Device mode reply data error(%x)", reply_data);
			goto fail;
			}
		}
	}
	msleep(300);

	if(Ft3x07_Read_ChannelData(0x34, 0x00, 0x35) != TOUCH_SUCCESS)
		goto fail;
	for(i = 0; i < MAX_CHANNEL_NUM; i++) {
		temp_delta = ((buffer_data[i*2]<<8) | (buffer_data[i*2]));
		if(temp_delta > 17000) {
			TOUCH_LOG("[DEBUG][ERROR]Delta Data[%d] = %d\n", i, temp_delta);
			*pLpwgStatus = 1;
		} else {
			TOUCH_LOG("[DEBUG]Delta Data[%d] = %d\n", i, temp_delta);
		}
	}

	/* Current factory test mode */
	/* 0x00 : F_NORMAL 0x01 : F_TESTMODE_1 0x02 : F_TESTMODE_2 */
	addr =	FT_FACTORY_MODE;
	if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
		TOUCH_LOG("Current factory test mode read error (0xAE)\n");
		goto fail;
	} else {
	/*Normal mode*/
		if(reply_data == 0x00) {
			TOUCH_LOG("Current factory test mode reply data ok(%x)", reply_data);
		} else {
			TOUCH_LOG("Current factory test mode reply data error(%x)", reply_data);
			goto fail;
		}
	}
	WRITE_SYSBUF(pBuf, dataLen, "	 -LPWG CB Value-	\n");

	/*Test mode 2 enter*/
	command = 0x02;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xAE)\n");
		goto fail;
	}
	msleep(10);

	addr = FT_START_SCAN;
	if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
		TOUCH_LOG("Test Mode 2 Start scan reply data error (0x08)\n");
		goto fail;
	} else {
		if(reply_data == 0x00) {
			TOUCH_LOG("Start scan reply data ok(%x)", reply_data);
		} else {
			TOUCH_LOG("Start scan reply data error(%x)", reply_data);
			goto fail;
		}
	}

	/* Start scan command */
	command = 0x01;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}
	do {
		msleep(15);
		if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
			TOUCH_LOG("Start scanf set reply data error (0x08)\n");
			goto fail;
		}
	} while(reply_data != 0x00);
	TOUCH_LOG(" Test Mode 2 - Data scan complete.\n");

	/*CB Address*/
	addr = 0x33;
	command = 0x00;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}

	addr = 0x39;
	if(Touch_I2C_Read(addr, temp_data, MAX_CHANNEL_NUM * 2) < 0) {
		TOUCH_LOG("Channel data read fail.(0x39)\n");
		goto fail;
	}

	for(i=0; i < MAX_CHANNEL_NUM; i++) {
		CB_data[i] = (temp_data[i*2] << 8) | temp_data[i*2+1];
		if( (CB_data[i] < 0) || (CB_data[i] > 900) ) {
			WRITE_SYSBUF(pBuf, dataLen, "[ERROR]LPWG CB value  [%d] = %d\n", i, CB_data[i]);
			TOUCH_LOG("[ERROR]LPWG CB Value[%d] = %d\n", i, CB_data[i]);
			*pLpwgStatus = 1;
		} else {
			TOUCH_LOG("LPWG CB Value[%d] = %d\n", i, CB_data[i]);
			WRITE_SYSBUF(pBuf, dataLen, "LPWG CB Value [%d] = %d\n", i, CB_data[i]);
		}
	}

	addr = FT_FACTORY_MODE;
	command = 0x80;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xae)\n");
		goto fail;
	}

/****************************************************************************/
	WRITE_SYSBUF(pBuf, dataLen, "	 -LPWG CB Differ Value-    \n");

	/*Test mode 1 enter*/
	command = 0x01;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xAE)\n");
		goto fail;
	}
	msleep(10);

	addr = FT_START_SCAN;
	if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
		TOUCH_LOG("Test Mode 2 Start scan reply data error (0x08)\n");
		goto fail;
	} else {
		if(reply_data == 0x00) {
			TOUCH_LOG("Start scan reply data ok(%x)", reply_data);
		} else {
			TOUCH_LOG("Start scan reply data error(%x)", reply_data);
		goto fail;
		}
	}

	/* Start scan command */
	command = 0x01;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}
	do {
	msleep(15);
		if(Touch_I2C_Read(addr, &reply_data, 1) < 0) {
			TOUCH_LOG("Start scanf set reply data error (0x08)\n");
			goto fail;
		}
	} while(reply_data != 0x00);
	TOUCH_LOG(" Test Mode 1 - Data scan complete.\n");

	/*CB Address*/
	addr = 0x33;
	command = 0x00;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Start scanf set error (0x08)\n");
		goto fail;
	}

	addr = 0x39;
	if(Touch_I2C_Read(addr, temp_data, MAX_CHANNEL_NUM * 2/*channel number(36)*2 = 72 */) < 0) {
		TOUCH_LOG("Channel data read fail.(0x39)\n");
		goto fail;
	}

	for(i=0; i < MAX_CHANNEL_NUM; i++) {
		CB_Differ_data[i] = (CB_data[i] - ((temp_data[i*2] << 8) | temp_data[i*2+1])) - Ft3x07_GoldenSample[i];

		if( (CB_Differ_data[i] < -40) || (CB_Differ_data[i] > 40) ) {
			WRITE_SYSBUF(pBuf, dataLen, "[ERROR]LPWG CB Differ Value  [%d] = %d\n", i, CB_Differ_data[i]);
			TOUCH_LOG("[ERROR]LPWG CB Differ Value[%d] = %d\n", i, CB_Differ_data[i]);
			*pLpwgStatus = 1;
		} else {
			WRITE_SYSBUF(pBuf, dataLen, "LPWG CB Differ Value [%d] = %d\n", i, CB_Differ_data[i]);			
			TOUCH_LOG("LPWG CB Differ Value[%d] = %d\n", i, CB_Differ_data[i]);
		}
	}

	addr = FT_FACTORY_MODE;
	command = 0x80;
	if(Touch_I2C_Write(addr, &command, 1) < 0 ) {
		TOUCH_LOG("Current factory test mode set error (0xae)\n");
		goto fail;
	}
	*pDataLen += dataLen;
	//*pLpwgStatus = 1;
	//*pLpwgStatus = 0;

	return TOUCH_SUCCESS;

fail:
	*pLpwgStatus = 1;
	//*pLpwgStatus = 0;
	WRITE_SYSBUF(pBuf, dataLen, "LPWG Self D Error\n");
	*pDataLen += dataLen;
	return TOUCH_FAIL;
}

void log_file_size_check(struct device *dev)
{
    char *fname = NULL;
    struct file *file;
    loff_t file_size = 0;
    int i = 0;
    char buf1[128] = {0};
    char buf2[128] = {0};
    mm_segment_t old_fs = get_fs();
    int ret = 0;
    set_fs(KERNEL_DS);

    if(lge_get_mfts_mode() == 1) {
        fname = (char *) mfts_sd_path;
    }
    else if(lge_get_factory_boot() == 1){
        fname = (char *) factory_sd_path;
    }
    else{
        fname = (char *) normal_sd_path;
    }

    if(fname) {
        file = filp_open(fname, O_RDONLY, 0666);
        sys_chmod(fname, 0666);
    } else {
        TOUCH_ERR("%s : fname is NULL, can not open FILE\n", __func__);
        goto error;
    }

    if(IS_ERR(file)) {
        TOUCH_ERR("%s : ERR(%ld) Open file error [%s]\n", __func__, PTR_ERR(file), fname);
        goto error;
    }

    file_size = vfs_llseek(file, 0, SEEK_END);
    TOUCH_LOG("%s : [%s] file_size = %lld\n", __func__, fname, file_size);

    filp_close(file, 0);

    if(file_size > MAX_LOG_FILE_SIZE) {
        TOUCH_LOG("%s : [%s] file_size(%lld) > MAX_LOG_FILE_SIZE(%d)\n", __func__, fname, file_size, MAX_LOG_FILE_SIZE);

        for(i = MAX_LOG_FILE_COUNT - 1; i >= 0; i--) {
            if(i == 0)
                sprintf(buf1, "%s", fname);
            else
                sprintf(buf1, "%s.%d", fname, i);

            ret = sys_access(buf1, 0);

            if(ret == 0) {
                TOUCH_LOG("%s : file [%s] exist\n",
                        __func__, buf1);

                if(i == (MAX_LOG_FILE_COUNT - 1)) {
                    if(sys_unlink(buf1) < 0) {
                        TOUCH_ERR("%s : failed to remove file [%s]\n", __func__, buf1);
                        goto error;
                    }

                    TOUCH_LOG("%s : remove file [%s]\n", __func__, buf1);
                } else {
                    sprintf(buf2, "%s.%d", fname, (i + 1));

                    if(sys_rename(buf1, buf2) < 0) {
                        TOUCH_ERR("%s : failed to rename file [%s] -> [%s]\n", __func__, buf1, buf2);
                        goto error;
                    }
                    TOUCH_LOG("%s : rename file [%s] -> [%s]\n", __func__, buf1, buf2);
                }
            } else {
                TOUCH_LOG("%s : file [%s] does not exist (ret = %d)\n", __func__, buf1, ret);
            }
        }
    }

error:
    set_fs(old_fs);
    return;
}

static int Ft3x07_DoSelfDiagnosis(int* pRawStatus, int* pChannelStatus, char* pBuf, int bufSize, int* pDataLen)
{
	int dataLen = 0;
	int result = 0;
	TOUCH_FUNC();
	/* CAUTION : be careful not to exceed buffer size */

	/* do implementation for self-diagnosis */
	result = Ft3x07_ChCap_Test(pBuf, pRawStatus, pChannelStatus, &dataLen);
	WRITE_SYSBUF(pBuf, dataLen, "======== RESULT File =======\n");
	WRITE_SYSBUF(pBuf, dataLen, "Channel Status : %s\n", (*pRawStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
	WRITE_SYSBUF(pBuf, dataLen, "Raw Data : %s\n", (*pChannelStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");

	TOUCH_LOG("======== RESULT File =======\n");
	TOUCH_LOG("Channel Status : %s\n", (*pRawStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
	TOUCH_LOG("Raw Data : %s\n", (*pChannelStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
	TOUCH_LOG("Channel Status : %d\n", *pRawStatus);
	TOUCH_LOG("Raw Data : %d\n", *pChannelStatus);
	Ft3x07_sd_write(pBuf, 1);
	memset(pBuf, 0, bufSize);
	log_file_size_check(&(gTouchDriverData->client->dev));
	*pDataLen = dataLen;
	if(result != TOUCH_SUCCESS)
		return TOUCH_FAIL;
	return TOUCH_SUCCESS;
}

static int Ft3x07_DoSelfDiagnosis_Lpwg(int *pLpwgStatus, char* pBuf, int bufSize, int* pDataLen)
{
	int dataLen = 0;
	int result = 0;
	TOUCH_FUNC();
	Ft3x07_Lpwg_ChCap_Test(pBuf, pLpwgStatus, &dataLen);
	WRITE_SYSBUF(pBuf, dataLen, "======== LPWG RESULT File =======\n");
	WRITE_SYSBUF(pBuf, dataLen, "LPWG RawData : %s\n", (*pLpwgStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");

	TOUCH_LOG("======== LPWG RESULT File =======\n");
	TOUCH_LOG("LPWG RawData : %s\n", (*pLpwgStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
	TOUCH_LOG("LPWG RawData : %d\n", *pLpwgStatus);
	Ft3x07_sd_write(pBuf, 1);
	log_file_size_check(&(gTouchDriverData->client->dev));
	*pDataLen = dataLen;
	if(result != TOUCH_SUCCESS)
		return TOUCH_FAIL;
	return TOUCH_SUCCESS;
}
static void get_regulator(void)
{
	struct i2c_client *client = Touch_Get_I2C_Handle();
	TOUCH_FUNC();

	if(client != NULL) {
		vdd_vio = regulator_get(&client->dev, "vtouch");
	} else {
		TOUCH_ERR("Fail to get i2c client\n");
	}
	if(vdd_vio == NULL) {
		TOUCH_ERR("Fail to get regulator(vgp1 id = vtouch)\n");
	} else {
		regulator_set_voltage(vdd_vio, 3000000, 3000000);
	}
}
static void Ft3x07_PowerOn(int isOn)
{
	int ret = TOUCH_SUCCESS;
	TOUCH_FUNC();

	get_regulator();
	if(isOn == 1) {
		TOUCH_LOG("Turned on the power ( VGP1 )\n");
		ret = regulator_enable(vdd_vio);
		msleep(300);
		TouchSetGpioReset(0);
		msleep(10);
		TouchSetGpioReset(1);
		msleep(300);
		if(ret != TOUCH_SUCCESS)
			TOUCH_ERR("Failed to enable reg-vgp1: %d\n", ret);
	} else {
		/* Only use power off charge case. */
		TOUCH_LOG("Turned off the power ( VGP1 )\n");
		ret = regulator_disable(vdd_vio);
		if( ret != TOUCH_SUCCESS)
			TOUCH_ERR("Failed to disable reg-vgp1: %d\n", ret);
	}
}

static void Ft3x07_ClearInterrupt(void)
{
	/* Not use this function. */
}

static void Ft3x07_NotifyHandler(TouchNotify notify, int data)
{
	/* Not use this function. */

}

TouchDeviceControlFunction FT3X07_Func = {
    .Power                  = Ft3x07_PowerOn,
    .Initialize             = Ft3x07_Initialize,
    .Reset                  = Ft3x07_Reset,
    .InitRegister           = Ft3x07_InitRegister,
    .ClearInterrupt         = Ft3x07_ClearInterrupt,
    .InterruptHandler       = Ft3x07_InterruptHandler,
    .ReadIcFirmwareInfo     = Ft3x07_ReadIcFirmwareInfo,
    .GetBinFirmwareInfo     = Ft3x07_GetBinFirmwareInfo,
    .UpdateFirmware         = Ft3x07_UpdateFirmware,
    .SetLpwgMode            = Ft3x07_SetLpwgMode,
    .DoSelfDiagnosis        = Ft3x07_DoSelfDiagnosis,
    .DoSelfDiagnosis_Lpwg	= Ft3x07_DoSelfDiagnosis_Lpwg,
    .device_attribute_list  = Ft3x07_attribute_list,
    .NotifyHandler          = Ft3x07_NotifyHandler,
};


/* End Of File */


