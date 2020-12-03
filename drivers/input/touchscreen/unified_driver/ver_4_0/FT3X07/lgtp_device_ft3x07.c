/*
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
 *    File      : lgtp_device_dummy.c
 *    Author(s)   : D3 BSP Touch Team < d3-bsp-touch@lge.com >
 *    Description :
 *
 ***************************************************************************/
#define LGTP_MODULE "[FT3x07]"

/****************************************************************************
* Include Files
****************************************************************************/
#include <linux/input/unified_driver_4/lgtp_common.h>

#include <linux/input/unified_driver_4/lgtp_common_driver.h>
#include <linux/input/unified_driver_4/lgtp_model_config_misc.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_i2c.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_misc.h>
#include <linux/input/unified_driver_4/lgtp_device_ft3x07.h>
//#include "focaltech_core.h"
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
#if defined( TOUCH_MODEL_LV0 )
static const char defaultFirmware[] = "focaltech/lv0/0_LG_LV0_NA_3407_Ver0x01_20160823_app.bin";
#else
static const char defaultFirmware[] = "focaltech/me0/0_LG_E0_3407_Ver0x12_20160428_app.img";
#endif

static struct focaltech_ts_data *ts = NULL;

static int temp_hover_status = 0;
static TouchDriverData *gTouchDriverData;
static int tap_count = 0;
int enable_iic_dev = 0;
static int is_power_enabled = 0;
static u8 prev_touch_count = 0;
static u8 prev_touch_status[2] = {FTS_TOUCH_NONE, FTS_TOUCH_NONE};
static u8 prev_touch_id[2] = {0,};
static int sd_finger_check = 0;
static int skip_knock = 0;

#if defined(ENABLE_SWIPE_MODE)
static int get_swipe_mode = 1;
/*
static int wakeup_by_swipe = 0;
*/
extern int lockscreen_stat;
#endif

atomic_t incoming_call;
atomic_t ime_state;

static const char normal_sd_path[] = "/sdcard/touch_self_test.txt";
static const char factory_sd_path[] = "/data/touch/touch_self_test.txt";
static const char mfts_sd_path[] = "/data/touch/touch_self_mfts.txt";

static char incoming_call_str[3][8] = {"IDLE", "RINGING", "OFFHOOK"};
static u8 buffer_data[64] = {0,};
atomic_t DeepSleep_flag;
atomic_t Lpwg_flag;

/****************************************************************************
* Extern Function Prototypes
****************************************************************************/
extern int lge_get_factory_boot(void);
extern bool lge_get_mfts_mode(void);
extern void SetDriverState(TouchDriverData *pDriverData, TouchState newState);


/****************************************************************************
* Local Function Prototypes
****************************************************************************/
static void Ft3x07_Reset(void);
static int Ft3x07_Set_Hover(int on);
static int Ft3x07_Set_KnockCode(LpwgSetting  *pLpwgSetting, u8 tap_count);
static int Ft3x07_SetLpwgMode( TouchState newState, LpwgSetting  *pLpwgSetting);


/****************************************************************************
* Local Functions
****************************************************************************/

static int FirmwareUpgrade(const struct firmware *fw_img)
{
    u8 * pbt_buf = NULL;

    int i_ret = 0;
    int fw_len = fw_img->size;

    /*FW upgrade*/
    pbt_buf = (u8*)fw_img->data;

    /*get ic info in fts_updateinfo_curr*/
    fts_get_upgrade_array();

    /*call the upgrade function*/
    i_ret = fts_3x07_ctpm_fw_upgrade(ts->client, pbt_buf, fw_len);
    if (i_ret != 0)
    {
        TOUCH_ERR("Firwmare upgrade failed. err=%d.\n", i_ret);
    }
    else
    {
        #ifdef AUTO_CLB
        fts_ctpm_auto_clb(ts->client);  /*start auto CLB*/
        #endif
    }
    return i_ret;
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

    if (lge_get_mfts_mode() == 1) {
        fname = (char *) mfts_sd_path;
    }
    else if(lge_get_factory_boot() == 1){
        fname = (char *) factory_sd_path;
    }
    else{
        fname = (char *) normal_sd_path;
    }

    if (fname) {
        file = filp_open(fname, O_RDONLY, 0666);
        sys_chmod(fname, 0666);
    } else {
        TOUCH_ERR("%s : fname is NULL, can not open FILE\n",
                __func__);
        goto error;
    }

    if (IS_ERR(file)) {
        TOUCH_ERR("%s : ERR(%ld) Open file error [%s]\n",
                __func__, PTR_ERR(file), fname);
        goto error;
    }

    file_size = vfs_llseek(file, 0, SEEK_END);
    TOUCH_LOG("%s : [%s] file_size = %lld\n",
            __func__, fname, file_size);

    filp_close(file, 0);

    if (file_size > MAX_LOG_FILE_SIZE) {
        TOUCH_LOG("%s : [%s] file_size(%lld) > MAX_LOG_FILE_SIZE(%d)\n",
                __func__, fname, file_size, MAX_LOG_FILE_SIZE);

        for (i = MAX_LOG_FILE_COUNT - 1; i >= 0; i--) {
            if (i == 0)
                sprintf(buf1, "%s", fname);
            else
                sprintf(buf1, "%s.%d", fname, i);

            ret = sys_access(buf1, 0);

            if (ret == 0) {
                TOUCH_LOG("%s : file [%s] exist\n",
                        __func__, buf1);

                if (i == (MAX_LOG_FILE_COUNT - 1)) {
                    if (sys_unlink(buf1) < 0) {
                        TOUCH_ERR("%s : failed to remove file [%s]\n",
                                __func__, buf1);
                        goto error;
                    }

                    TOUCH_LOG("%s : remove file [%s]\n",
                            __func__, buf1);
                } else {
                    sprintf(buf2, "%s.%d",
                            fname,
                            (i + 1));

                    if (sys_rename(buf1, buf2) < 0) {
                        TOUCH_ERR("%s : failed to rename file [%s] -> [%s]\n",
                                __func__, buf1, buf2);
                        goto error;
                    }

                    TOUCH_LOG("%s : rename file [%s] -> [%s]\n",
                            __func__, buf1, buf2);
                }
            } else {
                TOUCH_LOG("%s : file [%s] does not exist (ret = %d)\n",
                        __func__, buf1, ret);
            }
        }
    }

error:
    set_fs(old_fs);
    return;
}

static int Ft3x07_auto_cal_ctrl(u8 cal_cmd, u8 diff_cmd)
{
    u8 addr = 0;

    /* Disable Auto Calibration */
    addr = 0xEE;
    if(Touch_I2C_Write(addr, &cal_cmd, 1) < 0 )
    {
        TOUCH_LOG("Disable Auto Calibration error (0x01)\n");
        return TOUCH_FAIL;
    }
    msleep(50);

    /* Read Differ Data change */
    addr = 0xBB;
    if(Touch_I2C_Write(addr, &diff_cmd, 1) < 0 )
    {
        TOUCH_LOG("Read Differ Change error (0x00)\n");
        return TOUCH_FAIL;
    }
    msleep(50);

    return TOUCH_SUCCESS;
}

static int Ft3x07_mode_chg(u8 command)
{
    u8 addr = FT3x07_DEVICE_MODE;
    u8 reply_data = 0;

    /* Normal Mode: 0x00,  Factory mode: 0x40*/
    if(Touch_I2C_Write(addr, &command, 1) < 0)
    {
        TOUCH_LOG("Device mode enter error (%X)\n", command);
        return TOUCH_FAIL;
    }
    msleep(50);

    if(command == 0x40) {
        addr =  FT3x07_FACTORY_MODE;
        if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Current factory test mode read error (0xAE)\n");
            return TOUCH_FAIL;
        }
        usleep(10000);
    }

    return TOUCH_SUCCESS;
}

static int Ft3x07_read_buf(u8 addr_read, u8 start_ch, u8 read_buf)
{
    u8 addr = 0;
    u8 reply_data = 0;
    u8 command = 0;
    u8 channel_num = 32;

    memset(buffer_data, 0, sizeof(buffer_data));

    addr = FT3x07_START_SCAN;
    if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
    {
        TOUCH_LOG("Test Mode 2 Start scanf reply data error (0x%X)\n",addr);
        return TOUCH_FAIL;
    }
    else
    {
        if(reply_data == 0x00)
        {
            TOUCH_LOG("Start scanf reply data ok(0x%X)", reply_data);
        }
        else
        {
            TOUCH_LOG("Start scanf reply data error(0x%X)", reply_data);
            return TOUCH_FAIL;
        }
    }

    /* Start scan command */
    command = 0x01;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x%X)\n",addr);
        return TOUCH_FAIL;
    }
    do
    {
        usleep(15000);
        if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Start scanf set reply data error (0x%X)\n",addr);
            return TOUCH_FAIL;
        }
    }while(reply_data != 0x00);

    /* Write Start channel Address */
    if(Touch_I2C_Write(addr_read, &start_ch, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x%X)\n", addr_read);
        return TOUCH_FAIL;
    }
    /* Read Buffer Data */
    if(Touch_I2C_Read(read_buf, buffer_data, channel_num * 2/*channel number(32)*2 = 64 */) < 0)
    {
        TOUCH_LOG("Channel data read fail.(0x%X)\n",read_buf);
        return TOUCH_FAIL;
    }

    return TOUCH_SUCCESS;
}

static ssize_t show_use_iic_dev(TouchDriverData *pDriverData, char *buf)
{
    int ret = 0;

    ret = sprintf(buf, "%u\n", enable_iic_dev);

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
/*to do disable debug func, please reboot the device*/
#ifdef FTS_SYSFS_DEBUG
        fts_create_sysfs(ts->client);
#endif
#ifdef FTS_CTL_IIC
        if (fts_rw_iic_drv_init(ts->client) < 0)
            TOUCH_ERR("[FTS] create fts control iic driver failed\n");
#endif
#ifdef FTS_APK_DEBUG
        fts_create_apk_debug_channel(ts->client);
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

static ssize_t show_Pincheck(TouchDriverData *pDriverData, char *buf)
{
    int ret = 0;
    int val_gpio = 0;
    val_gpio = gpio_get_value(TOUCH_GPIO_INTERRUPT);
    TOUCH_LOG("[TOUCH] get GPIO_TOUCH_INT value  = %d\n", val_gpio);

    val_gpio = gpio_get_value(TOUCH_GPIO_RESET);
    TOUCH_LOG("[TOUCH] get GPIO_TOUCH_RESET value  = %d\n", val_gpio);

    return ret;
}

static ssize_t show_knockon(TouchDriverData *pDriverData, char *buf)
{
    int ret = TOUCH_SUCCESS;
    return ret;
}

static ssize_t show_Touch_Reset(TouchDriverData *pDriverData, char *buf)
{
    int ret = 0;
    return ret;
}

static ssize_t show_Set_vp(TouchDriverData *pDriverData, char *buf)
{
    int ret = 0;
    WRITE_SYSBUF(buf, ret, "[%s] \n", temp_hover_status ? "1" : "0");

    return ret;
}

static ssize_t store_Set_vp(TouchDriverData *pDriverData, const char *buf, size_t count)
{
    int cmd = 0;
    sscanf(buf, "%d", &cmd);
    if(cmd == 1)
    {
        Ft3x07_Set_Hover(1);
    }
    else if(cmd == 0)
    {
        Ft3x07_Set_Hover(0);
    }
    else
    {
        TOUCH_LOG("Invalid HOVER command\n");
    }

    return count;
}

static ssize_t show_delta(TouchDriverData *pDriverData, char *buf)
{
    int ret = 0;
    int dataLen = 0;
    int i ,j = 0;
    int channel_num = 32;
    int buf_data[32] = {0,};

    TOUCH_LOG("===== Delta Data =====\n");
    WRITE_SYSBUF(buf, dataLen, "===== Delta Data =====\n");

    mutex_lock(&pDriverData->thread_lock);
//    TouchDisableIrq();
    SetDriverState(pDriverData, STATE_SELF_DIAGNOSIS);

    /* Disable Auto Cal & Delta mode */
    ret = Ft3x07_auto_cal_ctrl(0x01, 0x01);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* Enter Factory Mode */
    ret = Ft3x07_mode_chg(0x40);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* Read DeltaData Buffer */
    ret = Ft3x07_read_buf(0x34, 0x00, 0x35);
    if (ret == TOUCH_FAIL)
        goto fail;

    for(i=0; i < channel_num; i++)
    {
        buf_data[i] = (buffer_data[i*2] << 8) | buffer_data[i*2+1];
    }

    /* Print Data */
    for (j=0; j < 2; j++) {
        WRITE_SYSBUF(buf, dataLen, "[%2d]", j*2);
        for (i=0; i < 8; i++) {
            WRITE_SYSBUF(buf, dataLen, "%8d", buf_data[j*8 + i] - 10000);
        }
        WRITE_SYSBUF(buf, dataLen, "\n[%2d]", j*2+1);
        for (i=0; i < 8; i++) {
            WRITE_SYSBUF(buf, dataLen, "%8d", buf_data[j*8 + i + 16] - 10000);
        }
        WRITE_SYSBUF(buf, dataLen, "\n");
    }

    /* Return Normal Mode */
    ret = Ft3x07_mode_chg(0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* Enable Auto Cal */
    ret = Ft3x07_auto_cal_ctrl(0x00, 0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    SetDriverState(pDriverData, STATE_NORMAL);
 //   TouchEnableIrq();
    mutex_unlock(&pDriverData->thread_lock);
    return dataLen;

fail:
    TOUCH_LOG("===== Read Delta Fail =====\n");
    SetDriverState(pDriverData, STATE_NORMAL);
    Ft3x07_Reset();
  //  TouchEnableIrq();
    mutex_unlock(&pDriverData->thread_lock);
    return dataLen;
}

static ssize_t show_raw_data(TouchDriverData *pDriverData, char *buf)
{
    int ret = 0;
    int i, j = 0;
    int dataLen = 0;
    int channel_num = 32;
    int buf_data[32] = {0,};

    mutex_lock(&pDriverData->thread_lock);
    TouchDisableIrq();
    SetDriverState(pDriverData, STATE_SELF_DIAGNOSIS);

    TOUCH_LOG("===== Raw Data =====\n");
    WRITE_SYSBUF(buf, dataLen, "===== Raw Data =====\n");

    /* Disable Auto Cal & RawData mode */
    ret = Ft3x07_auto_cal_ctrl(0x01, 0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* Enter Factory Mode */
    ret = Ft3x07_mode_chg(0x40);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* Read RawData Buffer */
    ret = Ft3x07_read_buf(0x34, 0x00, 0x35);
    if (ret == TOUCH_FAIL)
        goto fail;

    for(i=0; i < channel_num; i++)
    {
        buf_data[i] = (buffer_data[i*2] << 8) | buffer_data[i*2+1];
    }

    /* Print Data */
    for (j=0; j < 2; j++) {
        WRITE_SYSBUF(buf, dataLen, "[%2d]", j*2);
        for (i=0; i < 8; i++) {
            WRITE_SYSBUF(buf, dataLen, "%8d", buf_data[j*8 + i]);
        }
        WRITE_SYSBUF(buf, dataLen, "\n[%2d]", j*2+1);
        for (i=0; i < 8; i++) {
            WRITE_SYSBUF(buf, dataLen, "%8d", buf_data[j*8 + i + 16]);
        }
        WRITE_SYSBUF(buf, dataLen, "\n");
    }

    /* Return Normal Mode */
    ret = Ft3x07_mode_chg(0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* Enable Auto Cal */
    ret = Ft3x07_auto_cal_ctrl(0x00, 0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    SetDriverState(pDriverData, STATE_NORMAL);
    TouchEnableIrq();
    mutex_unlock(&pDriverData->thread_lock);
    return dataLen;

fail:
    TOUCH_LOG("===== Read Rawdata Fail =====\n");
    SetDriverState(pDriverData, STATE_NORMAL);
    Ft3x07_Reset();
    TouchEnableIrq();
    mutex_unlock(&pDriverData->thread_lock);
    return dataLen;
}

static ssize_t show_incoming_call_state(TouchDriverData *pDriverData, char *buf)
{
    int value = 0;
    int ret = 0;

    value = atomic_read(&incoming_call);

    ret = TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_str[value], value);

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
        TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_str[value], value);
    } else if (value == INCOMING_CALL_IDLE) {
        atomic_set(&incoming_call, value);
        txbuf = 0x00;
        TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_str[value], value);
    } else {
        TOUCH_LOG("Unknown %d\n", value);
    }

    /* Control RF Noise Filter */
    if (Touch_I2C_Write(0xAD, &txbuf, 1) < 0)
    {
        TOUCH_ERR("Write Call State Fail\n");
        return count;
    }
    return count;
}

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

	if (sscanf(buf, "%d", &value) <= 0)
        return count;

	if (value >= IME_OFF && value <= IME_SWYPE) {
        if (atomic_read(&ime_state) == value)
            return count;
        txbuf = value;
        atomic_set(&ime_state, value);
        TOUCH_LOG("%s : %d\n", __func__, value);
    } else {
        TOUCH_LOG("%s : Unknown %d\n", __func__, value);
    }

    if (Touch_I2C_Write(0xBA, &txbuf, 1) < 0)
    {
        TOUCH_ERR("Write Call State Fail\n");
        return count;
    }

    return count;
}

static LGE_TOUCH_ATTR(enable_iic_dev, S_IRUGO | S_IWUSR, show_use_iic_dev, store_use_iic_dev);
static LGE_TOUCH_ATTR(Pincheck, S_IRUGO | S_IWUSR, show_Pincheck, NULL);
static LGE_TOUCH_ATTR(knockon, S_IRUGO | S_IWUSR, show_knockon, NULL);
static LGE_TOUCH_ATTR(Touch_Reset, S_IRUGO | S_IWUSR, show_Touch_Reset, NULL);
static LGE_TOUCH_ATTR(Set_vp, S_IRUGO | S_IWUSR, show_Set_vp, store_Set_vp);
static LGE_TOUCH_ATTR(delta, S_IRUGO | S_IWUSR, show_delta, NULL);
static LGE_TOUCH_ATTR(rawdata, S_IRUGO | S_IWUSR, show_raw_data, NULL);
static LGE_TOUCH_ATTR(incoming_call, S_IRUGO | S_IWUSR, show_incoming_call_state, store_incoming_call_state);
static LGE_TOUCH_ATTR(ime_status, S_IRUGO | S_IWUSR,show_ime_state, store_ime_state);

static struct attribute *Ft3x07_attribute_list[] = {
    &lge_touch_attr_Pincheck.attr,
    &lge_touch_attr_Touch_Reset.attr,
    &lge_touch_attr_Set_vp.attr,
    &lge_touch_attr_delta.attr,
    &lge_touch_attr_rawdata.attr,
    &lge_touch_attr_knockon.attr,
    &lge_touch_attr_enable_iic_dev.attr,
    &lge_touch_attr_incoming_call.attr,
    &lge_touch_attr_ime_status.attr,
    NULL,
};

static int Ft3x07_Initialize(TouchDriverData *pDriverData)
{
    struct i2c_client *client = Touch_Get_I2C_Handle();
    TOUCH_FUNC();
    ts = devm_kzalloc(&client->dev, sizeof(struct focaltech_ts_data), GFP_KERNEL);
    if (ts == NULL)
    {
        TOUCH_ERR("failed to allocate memory for device driver data\n");
        return TOUCH_FAIL;
    }
    gTouchDriverData = pDriverData;
    ts->client = client;
    return TOUCH_SUCCESS;
}

static void Ft3x07_Reset_IC(void)
{
    TOUCH_FUNC();
    TouchDisableIrq();
    TouchSetGpioReset(0);
    msleep(5);
    TouchSetGpioReset(1);
    msleep(300);
    TOUCH_LOG("Device was reset\n");
    TouchEnableIrq();
}

static void Ft3x07_Reset(void)
{
    TOUCH_FUNC();
    TouchDisableIrq();
    TouchSetGpioReset(0);
    msleep(5);
    TouchSetGpioReset(1);
    msleep(300);
    temp_hover_status = 0;
    prev_touch_count = 0;
    memset(prev_touch_status, FTS_TOUCH_NONE,sizeof(prev_touch_status));
//    ts->currState = STATE_NORMAL;
    TOUCH_LOG("Device was reset\n");
    TouchEnableIrq();
    atomic_set(&DeepSleep_flag, 0);
    atomic_set(&Lpwg_flag, 0);
}

static int Ft3x07_InitRegister(void)
{
    /* IMPLEMENT : Register initialization after reset */
    int value = 0;
    u8 txbuf = 0x00;
    TouchState newState = STATE_UNKNOWN;
    TOUCH_FUNC();

    value = atomic_read(&incoming_call);

    if (value ==  INCOMING_CALL_OFFHOOK|| value == INCOMING_CALL_RINGING) {
        txbuf = 0x01;
        TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_str[value], value);
    } else if (value == INCOMING_CALL_IDLE) {
        txbuf = 0x00;
        TOUCH_LOG("%s %s(%d)\n", __func__, incoming_call_str[value], value);
    } else {
        TOUCH_LOG("Unknown %d\n", value);
    }

    /* Control RF Noise Filter */
    if (Touch_I2C_Write(0xAD, &txbuf, 1) < 0)
    {
        TOUCH_ERR("Write Call State Fail\n");
        return TOUCH_FAIL;
    }

    txbuf = atomic_read(&ime_state);
    /* Control IME */
    if (txbuf > 0)
        TOUCH_LOG("Ime Enabled\n");
    else
        TOUCH_LOG("Ime Disabled\n");

    if (Touch_I2C_Write(0xBA, &txbuf, 1) < 0)
    {
        TOUCH_ERR("Write Call State Fail\n");
    }

    if(ts->currState != STATE_NORMAL) {
        TOUCH_LOG("LPWG Mode Rewrite after RESET on Sleep Mode\n");
        newState = ts->currState;
        ts->currState = STATE_NORMAL;
        Ft3x07_SetLpwgMode(newState, &(ts->lpwgSetting));
    }

    return TOUCH_SUCCESS;
}

static int get_lpwg_data(TouchReadData *pData, u8 tap_count)
{
    u8 i = 0;
    u8 buffer[50] = {0,};
    if(Touch_I2C_Read(FT_KNOCK_READ_DATA_REG, buffer, tap_count*4 + 2) == 0)
    {
        pData->count = buffer[1];
        TOUCH_LOG("[NSM]TAP COUNT = %d\n", buffer[1]);
    }
    else
    {
        TOUCH_ERR("KNOCK TAP DATA Read Fail.\n");
        goto error;
    }

    if(!buffer[1])
    {
        TOUCH_LOG("TAP COUNT = %d\n", buffer[1]);
        goto error;
    }

    for(i = 0; i< buffer[1]; i++)
    {
        pData->knockData[i].x = GET_X_POSITION(buffer[4*i+2], buffer[4*i+3]);
        pData->knockData[i].y = GET_Y_POSITION(buffer[4*i+4], buffer[4*i+5]);
        TOUCH_LOG("LPWG data [%d, %d]\n", pData->knockData[i].x, pData->knockData[i].y);
    }

    return TOUCH_SUCCESS;
error:
    return TOUCH_FAIL;
}

static int Ft3x07_InterruptHandler(TouchReadData *pData)
{
    TouchFingerData *pFingerData = NULL;

    int i = 0;
    int ret = -1;
    int retry_cnt = 0;
    u8 touch_event=0;
    u8 buf[POINT_READ_BUF] = { 0 };
    u8 pointid = FT_MAX_ID;
    u8 touch_count = 0;
    u8 event_type = 0;
    static u8 pressure_change = 0;
    int value = 0;

    /* read IME status */
    value = atomic_read(&ime_state);

    pData->type = DATA_UNKNOWN;
    pData->count = 0;
    /* read Interrupt status */
    do {
        ret = Touch_I2C_Read(FT_INT_STATUS, &event_type, sizeof(u8));
        if(ret <0) {
            if (ts->currState == STATE_KNOCK_ON_CODE
                || ts->currState == STATE_KNOCK_ON_ONLY
                || ts->currState == STATE_OFF) {
                TOUCH_LOG("Discard Unintended First Int on LPWG Mode.\n");
                return TOUCH_SUCCESS;
            } else {
                TOUCH_ERR("read int status failed.\n");
                return TOUCH_FAIL;
            }
        }

        if (event_type < (EVENT_HOVERING_FAR + 1)) {
            break;
        } else {
            TOUCH_LOG("[Unknown] Event Type = %d, Retry Read Interrupt\n",event_type);
            retry_cnt++;
        }
    } while(retry_cnt < 3);

//    TOUCH_LOG("[INTERRUPT] Event Type = %d\n", event_type);
    if (lge_get_mfts_mode() == 1) {
        event_type = EVENT_ABS;
    }

    switch(event_type)
    {
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
//            TOUCH_LOG("Current Count: %d, Prev Count: %d\n", touch_count, prev_touch_count);
            for (i = 0; i < FTS_MAX_POINTS; i++)
            {
                pointid = (buf[FT_TOUCH_ID_POS + FT_TOUCH_STEP * i]) >> 4;

                touch_event = buf[FT_TOUCH_EVENT_POS + FT_TOUCH_STEP * i] >> 6;
//                TOUCH_LOG("i = %d, Event = %d, id = %d, touch_count = %d\n", i, touch_event, pointid, touch_count);
                if (pointid >= FT_MAX_ID) {
                    if ((i == 1) && (prev_touch_status[i] == FTS_TOUCH_DOWN || prev_touch_status[i] == FTS_TOUCH_CONTACT)) {
                        if (prev_touch_id[0] == 0)
                            pointid = 1;
                        else if (prev_touch_id[0] == 1)
                            pointid = 0;
                        touch_event = FTS_TOUCH_UP;
                        TOUCH_LOG("Release Missed Multi Touch\n");
                    } else if (i == 0 && (prev_touch_status[i] == FTS_TOUCH_DOWN || prev_touch_status[i] == FTS_TOUCH_CONTACT)) {
                        pointid = 0;
                        touch_event = FTS_TOUCH_UP;
                        TOUCH_LOG("Release Missed Single Touch\n");
                    } else {
                        break;
                    }
                }

                if(touch_event== FTS_TOUCH_DOWN ||touch_event==FTS_TOUCH_CONTACT)
                {
                    pFingerData = &pData->fingerData[pointid];
                    //buf 3+6*i
                    pFingerData->x = (u16) (buf[FT_TOUCH_X_H_POS + FT_TOUCH_STEP * i] & 0x0F) << 8 | (u16) buf[FT_TOUCH_X_L_POS + FT_TOUCH_STEP * i];
                    pFingerData->y = (u16) (buf[FT_TOUCH_Y_H_POS + FT_TOUCH_STEP * i] & 0x0F) << 8 | (u16) buf[FT_TOUCH_Y_L_POS + FT_TOUCH_STEP * i];
                    /* For Improve IME touch Area */
                    if(pFingerData->x > 476 && value > 0)
                        pFingerData->x = 476;
                    else if(pFingerData->x < 5 && value > 0)
                        pFingerData->x = 5;

                    pFingerData->id = pointid;
                    pFingerData->width_major = 15;
                    pFingerData->width_minor = 10;
                    pFingerData->orientation = 1;
                    /* to prevent Hovering detected in CFW.*/
                    pFingerData->pressure =100 + pressure_change;
                    pData->count++;
                    pFingerData->status = FINGER_PRESSED;
#if 0 /* for debug */
                    TOUCH_LOG("Event = %d, id = %d, x = %d, y = %d, width_major = %d, pressure = %d \n", touch_event, pFingerData->id, pFingerData->x, pFingerData->y, pFingerData->width_major, pFingerData->pressure);
#endif
                    prev_touch_status[i] = touch_event;
                    prev_touch_id[i] = pointid;
                } else if(touch_event== FTS_TOUCH_UP || touch_event== FTS_TOUCH_NONE) {
                    pFingerData = &pData->fingerData[pointid];
                    pFingerData->id = pointid;
                    pFingerData->status = FINGER_RELEASED;
#if 0    /*for debug*/
                    TOUCH_LOG("Event = %d, point_id = %d \n", touch_event, pointid);
#endif
                    prev_touch_status[i] = touch_event;
                    prev_touch_id[i] = pointid;
                }
            }
            prev_touch_count = touch_count;
        break;
        case EVENT_KEY :
            break;
        case EVENT_KNOCK_ON :
            pData->type = DATA_KNOCK_ON;
            TOUCH_LOG("[KNOCK ON] Event Type = %d\n", event_type);
            //Ft3x07_Set_KnockOn(client);
            break;

        case EVENT_KNOCK_CODE :
            pData->type = DATA_KNOCK_CODE;
            get_lpwg_data(pData, tap_count);
            TOUCH_LOG("[KNOCK CODE] Event Type = %d\n", event_type);;
            Ft3x07_Set_KnockCode(&(gTouchDriverData->lpwgSetting), tap_count);
            break;

        case EVENT_KNOCK_OVER :
            pData->type = DATA_KNOCK_CODE;
            pData->knockData[0].x = 1;
            pData->knockData[0].y = 1;
            pData->knockData[1].x = -1;
            pData->knockData[1].y = -1;
            TOUCH_LOG("[KNOCK CODE OVER] Event Type = %d\n", event_type);
            break;

        case EVENT_HOVERING_NEAR :
            /*
            pData->type = DATA_HOVER_NEAR;
            pData->hoverState = 0;
            TOUCH_LOG("[HOVERING NEAR] Event Type = %d\n", event_type);
            */
            break;

        case EVENT_HOVERING_FAR :
            /*
            pData->type = DATA_HOVER_FAR;
            pData->hoverState = 1;
            TOUCH_LOG("[HOVERING FAR] Event Type = %d\n", event_type);
            */
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

    if(Touch_I2C_Read(FTS_REG_FW_VER, &readData, 1) != 0)
    {
        TOUCH_ERR("Firmware version read fail (0xA6)\n");
        result = TOUCH_FAIL;
    }
    else
    {
        pFwInfo->version = readData;
        pFwInfo->isOfficial = 1;
        TOUCH_LOG("IC Firmware Official = %d\n", pFwInfo->isOfficial);
        TOUCH_LOG("IC Firmware Version = 0x%02X\n", readData);
    }

    if(Touch_I2C_Read(FTS_REG_FW_VENDOR_ID, &readData, 1) != 0)
    {
        TOUCH_ERR("Touch Panel read fail (0xA8)\n");
        result = TOUCH_FAIL;
    }
    else
    {
        TOUCH_LOG("PANEL ID = %x", readData);
    }

    if(Touch_I2C_Read(FTS_REG_FW_REL_COD, &readData, 1) != 0)
    {
        TOUCH_ERR("Firmware release version read fail. (0xAF)\n");
        result = TOUCH_FAIL;
    }
    else
    {
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

    TOUCH_FUNC();

    if( pFilename == NULL )
    {
        pFwFilename = (char *)defaultFirmware;
    }
    else
    {
        pFwFilename = pFilename;
    }

    TOUCH_LOG("Firmware filename = %s\n", pFwFilename);

    /* Get firmware image buffer pointer from file */
    ret = request_firmware(&fw, pFwFilename, &(ts->client->dev)/*&client->dev*/);
    if(ret < 0)
    {
        TOUCH_ERR("Failed at request_firmware() ( error = %d )\n", ret);
        ret = TOUCH_FAIL;
        // TEMP RETURE VALUE FOR AVOIDING REQUEST_FIRMWARE FAIL
        ret = 0;
        goto earlyReturn;
    }
    pFw = (u8 *)(fw->data);
    pFwInfo->isOfficial = 1;
    pFwInfo->version = pFw[0x10a];

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
    if( pFilename == NULL )
    {
        pFwFilename = (char *)defaultFirmware;
    }
    else
    {
        pFwFilename = pFilename;
    }

    TOUCH_LOG("Firmware file name = %s \n", pFwFilename);

    /* Get firmware image buffer pointer from file*/
    result = request_firmware(&fw, pFwFilename, &(ts->client->dev));
    if( result )
    {
        TOUCH_ERR("Failed at request_firmware() ( error = %d )\n", result);
        result = TOUCH_FAIL;
        return result;
    }
    pBin = (u8 *)(fw->data);

    /* Do firmware upgrade */
    result = FirmwareUpgrade(fw);
    if(result != TOUCH_SUCCESS)
    {
        TOUCH_ERR("Failed at request_firmware() ( error = %d )\n", result);
        result = TOUCH_FAIL;
    }

    /* Free firmware image buffer */
    release_firmware(fw);
    return result;
}

static int Ft3x07_Set_KnockOn(void)
{
    int i = 0;
    int count = 0;
    u8 buf = 0;
    u8 rData = 0;
    int value = 0;

    TOUCH_FUNC();

    value = atomic_read(&DeepSleep_flag);

    /*Only reset in deep sleep status.*/
    if(value== 1) {
        Ft3x07_Reset();
        atomic_set(&DeepSleep_flag, 0);
    }

    buf = 0x00;
    /*Add interrupt delay time set 0*/
    if(Touch_I2C_Write(FT_LPWG_INT_DELAY, &buf, 1) < 0)
    {
        TOUCH_ERR("KNOCK ON INT DELAY TIME write fail\n");
        return TOUCH_FAIL;
    }

    buf = 0x01;
    if (Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, 1) < 0)
    {
        TOUCH_ERR("KNOCK_ON Enable write fail\n");
        return TOUCH_FAIL;
    }
    for(i = 0; i<5;i++)
    {
        if (Touch_I2C_Read(FT_LPWG_CONTROL_REG, &rData, 1) < 0)
        {
            TOUCH_ERR("KNOCK_ON Read Data Fail\n");
            return TOUCH_FAIL;
        }
        else
        {
            if(rData == 0x01)
                break;
            else
            {
                if (Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, 1) < 0)
                {
                    TOUCH_ERR("KNOCK_ON Enable write fail\n");
                    return TOUCH_FAIL;
                }
            }
        }
        count++;
        usleep(5000);
    }

    if (count > 4) {
        TOUCH_ERR("KNOCK_ON Enable write fail, Rerty Count = %d\n", count);
        return TOUCH_FAIL;
    }
    TOUCH_LOG("LPWG Mode Changed to KNOCK_ON_ONLY\n");

    atomic_set(&Lpwg_flag, 1);
    return TOUCH_SUCCESS;
}

static int Ft3x07_Set_KnockCode(LpwgSetting  *pLpwgSetting, u8 tap_count)
{
    u8 buf = 0;
    u8 rData = 0;
    u8 count = 0;
    u8 i = 0;
    int value = 0;

    TOUCH_FUNC();

    value = atomic_read(&DeepSleep_flag);

    /*Only reset in deep sleep status.*/
    if(value== 1) {
        Ft3x07_Reset();
        atomic_set(&DeepSleep_flag, 0);
    }

    if(Touch_I2C_Write(FT_MULTITAP_COUNT_REG, &(tap_count), 1) <0)
    {
        TOUCH_ERR("KNOCK_CODE Tab count write fail\n");
        return TOUCH_FAIL;
    }

    /* 1 = first two code is same, 0 = not same */
    buf = 0x00;
    if(pLpwgSetting->isFirstTwoTapSame == 0)
    {
        TOUCH_LOG("First Two Tap is not same, Knock on delay set 0\n");
        if(Touch_I2C_Write(FT_LPWG_INT_DELAY, &buf, 1) < 0)
        {
            TOUCH_ERR("KNOCK ON INT DELAY TIME write fail\n");
            return TOUCH_FAIL;
        }
    }
    else
    {
        TOUCH_LOG("First Two Tap is same, Knock on delay set default value(500ms)\n");
    }

    buf = 0x3;
    if (Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, 1) < 0)
    {
       TOUCH_ERR("KNOCK_CODE Enable write fail\n");
       return TOUCH_FAIL;
    }
    for(i = 0; i<5;i++)
    {
        if (Touch_I2C_Read(FT_LPWG_CONTROL_REG, &rData, 1) < 0)
        {
            TOUCH_ERR("KNOCK_CODE Read Data Fail\n");
            return TOUCH_FAIL;
        }
        else
        {
            if(rData == 0x3)
                break;
            else
            {
                if (Touch_I2C_Write(FT_LPWG_CONTROL_REG, &buf, 1) < 0)
                {
                    TOUCH_ERR("KNOCK_CODE Enable write fail\n");
                    return TOUCH_FAIL;
                }
            }
        }
        count++;
        usleep(5000);
    }

    if (count > 4) {
        TOUCH_ERR("KNOCK_ON_CODE Enable write fail, Rerty Count = %d\n", count);
        return TOUCH_FAIL;
    }
    TOUCH_LOG("LPWG Mode Changed to KNOCK_ON_CODE\n");

    atomic_set(&Lpwg_flag, 1);
    return TOUCH_SUCCESS;
}


static int Ft3x07_Set_Hover(int on)
{
    u8 buf = 0;
    buf = on ? 0x01 : 0x00;

    if(Touch_I2C_Write(0xB0, &buf, 1)<0)
    {
        TOUCH_LOG("Hover on setting fail.\n");
        return TOUCH_FAIL;
    }
    else
    {
        TOUCH_LOG("Mode Changed to HOVER %s.\n", on ? "On" : "Off");
    }
    return TOUCH_SUCCESS;
}

static int Ft3x07_Set_PDN(void)
{
    /* Only use have proximity sensor */
    u8 cmd = 0x03;
    int value = 0;
    TOUCH_FUNC();
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
    // Remove because Avoid irq pending.
//    TouchDisableIrq();
    return TOUCH_SUCCESS;
}

static int Ft3x07_SetLpwgMode( TouchState newState, LpwgSetting  *pLpwgSetting)
{
    int result = TOUCH_SUCCESS;
    int value = 0;
    int retry_cnt = 3;
    TOUCH_FUNC();
    TOUCH_LOG("ts->currState = %d newState = %d\n",ts->currState, newState);
    value = atomic_read(&incoming_call);

    if ( (value ==  INCOMING_CALL_OFFHOOK || value == INCOMING_CALL_RINGING)
        && (ts->currState == STATE_NORMAL && newState == STATE_OFF)) {
        TOUCH_LOG("Set KnockON/KnockCode Skip flag on CALL\n");
        skip_knock = 1;
    }

    if (ts->currState == newState) {
        TOUCH_LOG("device state is same as driver requested\n");
        return result;
    }
    ts->currState = newState;
    ts->lpwgSetting = *pLpwgSetting;

    switch(ts->currState)
    {
          case STATE_NORMAL :
            //Ft3x07_Reset();
            skip_knock = 0;
            TOUCH_LOG("FT3x07 was changed to NORMAL\n");
        break;

        case STATE_KNOCK_ON_ONLY :
            if(skip_knock) {
                TOUCH_LOG("Skip Knock ON setting after FAR on Call Scene\n");
                break;
            }
            do {
                result = Ft3x07_Set_KnockOn();
                if(result < 0) {
                    TOUCH_LOG("FT3x07 failed changed to STATE_KNOCK_ON_ONLY\n");
                    Ft3x07_Reset_IC();
                    retry_cnt--;
                } else {
                    TOUCH_LOG("FT3x07 was changed to STATE_KNOCK_ON_ONLY\n");
                    retry_cnt = -1;
                }
            } while (retry_cnt > 0);

            if (result < 0)
                TOUCH_LOG("FT3x07 can not change to STATE_KNOCK_ON_ONLY\n");

            break;

        case STATE_KNOCK_ON_CODE :
            tap_count = pLpwgSetting->tapCount;
            if(skip_knock) {
                TOUCH_LOG("Skip Knock Code setting after FAR on Call Scene\n");
                break;
            }
            do {
                result = Ft3x07_Set_KnockCode(pLpwgSetting, tap_count);
                if(result < 0) {
                    TOUCH_LOG("FT3x07 failed changed to STATE_KNOCK_ON_CODE\n");
                    Ft3x07_Reset_IC();
                    retry_cnt--;
                } else {
                    TOUCH_LOG("FT3x07 was changed to STATE_KNOCK_ON_CODE\n");
                    retry_cnt = -1;
                }
            } while (retry_cnt > 0);

            if (result < 0)
                TOUCH_LOG("FT3x07 can not change to STATE_KNOCK_ON_CODE\n");

            break;

        case STATE_NORMAL_HOVER :
            result = Ft3x07_Set_Hover(1);
            break;

        case STATE_HOVER :
            if(gTouchDriverData->reportData.hover == 1)
            {
                result = Ft3x07_Set_Hover(0);
                if( pLpwgSetting->mode == 1 )
                {
                    result = Ft3x07_Set_KnockOn();
                }
                if( pLpwgSetting->mode == 2 )
                {
                    result = Ft3x07_Set_KnockCode(pLpwgSetting, tap_count);
                }
                if( pLpwgSetting->mode == 3 )
                {
                }
            }
            break;
        case STATE_OFF :
            result = Ft3x07_Set_PDN();
            if(result < 0)
                TOUCH_LOG("FT3x07 failed changed to STATE_OFF\n");
            TOUCH_LOG("FT3x07 was changed to STATE_OFF\n");
            break;
        default :
            TOUCH_ERR("Unknown State %d", newState);
            break;
    }

    return result;
}

static void Ft3x07_sd_write(char *filename, char *data, int time)
{
    int fd = 0;
    char time_string[64] = {0};
    struct timespec my_time;
    struct tm my_date;

    mm_segment_t old_fs = get_fs();
    set_fs(KERNEL_DS);

    if (lge_get_mfts_mode() == 1) {
        TOUCH_LOG("boot mfts\n");
        filename = (char *) mfts_sd_path;
    }
    else if(lge_get_factory_boot() == 1){
        TOUCH_LOG("boot minios\n");
        filename = (char *) factory_sd_path;
    }
    else{
        TOUCH_LOG("boot normal\n");
        filename = (char *) normal_sd_path;
    }

    fd = sys_open(filename, O_WRONLY|O_CREAT|O_APPEND|O_SYNC, 0644);

    if (fd >= 0)
    {
        if (time > 0) {
            my_time = __current_kernel_time();
            time_to_tm(my_time.tv_sec, sys_tz.tz_minuteswest * 60 * (-1), &my_date);
            snprintf(time_string, 64, "\n%02d-%02d %02d:%02d:%02d.%03lu \n\n\n",
                my_date.tm_mon + 1,my_date.tm_mday, my_date.tm_hour, my_date.tm_min,
                my_date.tm_sec, (unsigned long) my_time.tv_nsec / 1000000);
            sys_write(fd, time_string, strlen(time_string));
        }
        sys_write(fd, data, strlen(data));
        sys_close(fd);
    }

    set_fs(old_fs);
}

static int Ft3x07_ChCap_Test(char* pBuf ,int* pRawStatus, int* pChannelStatus, int* pDataLen)
{
    u8 addr = FT3x07_DEVICE_MODE;
    u8 command = 0x40;
    u8 reply_data = 0;
    u8 i = 0;
    u8 channel_num = 32;
    int dataLen = 0;
    int ret = 0;
    u8 temp_data[64] = {0,};
    int CB_data[32] = {0,};
    int CB_Differ_data[32] = {0,};

    *pRawStatus = 0;
    *pChannelStatus = 0;
    sd_finger_check = 0;
    dataLen += sprintf(pBuf + dataLen, "=====Test Start=====\n");

    TOUCH_LOG("=====Test Start=====\n");
    TOUCH_LOG("    -CB Value-\n    ");

    /* Disable Auto Cal & Delta mode */
    ret = Ft3x07_auto_cal_ctrl(0x01, 0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* enter factory mode */
    if(Touch_I2C_Write(addr, &command, 1) < 0)
    {
        TOUCH_LOG("Device mode enter error (0x00)\n");
        goto fail;
    }
    else
    {
        msleep(300);
        if(Touch_I2C_Read(FT3x07_DEVICE_MODE, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Device mode reply data read error (0x00)\n");
            goto fail;
        }
        else
        {
            if(reply_data != 0x40)
            {
                TOUCH_LOG("Device mode reply data error(%x)", reply_data);
                goto fail;
            }
        }
    }
    msleep(300);

    /* Read DeltaData Buffer */
    ret = Ft3x07_read_buf(0x34, 0x00, 0x35);
    if (ret == TOUCH_FAIL)
        goto fail;

    for(i=0; i < channel_num; i++)
    {
        CB_data[i] = (buffer_data[i*2] << 8) | buffer_data[i*2+1];
        if (CB_data[i] > 17000) {
            TOUCH_LOG("RawData[%d] = %d\n", i, CB_data[i]);
            *pRawStatus = 1;
            sd_finger_check = 1;
        }
    }

    /* Current factory test mode */
    /* 0x00 : F_NORMAL 0x01 : F_TESTMODE_1 0x02 : F_TESTMODE_2 */
    addr =  FT3x07_FACTORY_MODE;
    if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
    {
        TOUCH_LOG("Current factory test mode read error (0xAE)\n");
        goto fail;
    }
/*
    else
    {
        if(reply_data == 0x00)
        {
            TOUCH_LOG("Current factory test mode reply data ok(%x)", reply_data);
        }
        else
        {
            TOUCH_LOG("Current factory test mode reply data error(%x)", reply_data);
            goto fail;
        }
    }
*/
    /*Test mode 2 enter*/
    command = 0x02;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xAE)\n");
        goto fail;
    }
    msleep(10);
    addr = FT3x07_START_SCAN;
    if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
    {
        TOUCH_LOG("Test Mode 2 Start scanf reply data error (0x08)\n");
        goto fail;
    }
    else
    {
        if(reply_data == 0x00)
        {
            TOUCH_LOG("Start scanf reply data ok(%x)", reply_data);
        }
        else
        {
            TOUCH_LOG("Start scanf reply data error(%x)", reply_data);
            goto fail;
        }
    }

    /* Start scan command */
    command = 0x01;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }
    do
    {
        msleep(15);
        if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Start scanf set reply data error (0x08)\n");
            goto fail;
        }
    }while(reply_data != 0x00);
    TOUCH_LOG(" Test Mode 2 - Data scan complete.\n");

    /*CB Address*/
    addr = 0x33;
    command = 0x00;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }

    addr = 0x39;
    if(Touch_I2C_Read(addr, temp_data, channel_num * 2/*channel number(32)*2 = 64 */) < 0)
    {
        TOUCH_LOG("Channel data read fail.(0x39)\n");
        goto fail;
    }

    for(i=0; i < channel_num; i++)
    {
        CB_data[i] = (temp_data[i*2] << 8) | temp_data[i*2+1];
        TOUCH_LOG("CB Value[%d] = %d\n", i, CB_data[i]);
        if( (CB_data[i] < 0) || (CB_data[i] > 900) )
        {
            dataLen += sprintf(pBuf + dataLen, "[ERROR]CB value  [%d] = %d\n", i, CB_data[i]);
            *pRawStatus = 1;
            *pChannelStatus = 1;
        }
        else
        {
            dataLen += sprintf(pBuf + dataLen, "CB Value [%d] = %d\n", i, CB_data[i]);
        }
    }

    WRITE_SYSBUF(pBuf, dataLen, "    -CB Differ Value-    \n");
    addr = FT3x07_FACTORY_MODE;
    /*Test mode 1 enter*/
    command = 0x01;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xAE)\n");
        goto fail;
    }
    msleep(10);
    addr = FT3x07_START_SCAN;
    if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
    {
        TOUCH_LOG("Test Mode 2 Start scanf reply data error (0x08)\n");
        goto fail;
    }
    else
    {
        if(reply_data == 0x00)
        {
            TOUCH_LOG("Start scanf reply data ok(%x)", reply_data);
        }
        else
        {
            TOUCH_LOG("Start scanf reply data error(%x)", reply_data);
            goto fail;
        }
    }

    /* Start scan command */
    command = 0x01;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }
    do
    {
        msleep(15);
        if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Start scanf set reply data error (0x08)\n");
            goto fail;
        }
    }while(reply_data != 0x00);
    TOUCH_LOG(" Test Mode 1 - Data scan complete.\n");

    /*CB Address*/
    addr = 0x33;
    command = 0x00;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }

    addr = 0x39;
    if(Touch_I2C_Read(addr, temp_data, channel_num * 2/*channel number(32)*2 = 64 */) < 0)
    {
        TOUCH_LOG("Channel data read fail.(0x39)\n");
        goto fail;
    }

    for(i=0; i < channel_num; i++)
    {
        /* F_TestMode_1 - F_TestMode_2 */
        CB_Differ_data[i] = (CB_data[i] - ((temp_data[i*2] << 8) | temp_data[i*2+1])) - Ft3x07_GoldenSample[i];
        TOUCH_LOG("CB Differ Value[%d] = %d\n", i, CB_Differ_data[i]);

        if( (CB_Differ_data[i] < -40) || (CB_Differ_data[i] > 40) )
        {
            dataLen += sprintf(pBuf + dataLen, "[ERROR]CB Differ Value  [%d] = %d\n", i, CB_Differ_data[i]);
            *pRawStatus = 1;
            *pChannelStatus = 1;
        }
        else
        {
            dataLen += sprintf(pBuf + dataLen, "CB Differ Value [%d] = %d\n", i, CB_Differ_data[i]);
        }
    }

    /* Read DeltaData Buffer */
    command = 0x00;
    if(Touch_I2C_Write(FT3x07_FACTORY_MODE, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xAE)\n");
        goto fail;
    }

    ret = Ft3x07_read_buf(0x34, 0x00, 0x35);
    if (ret == TOUCH_FAIL)
        goto fail;

    for(i=0; i < channel_num; i++)
    {
        CB_data[i] = (buffer_data[i*2] << 8) | buffer_data[i*2+1];
        if (CB_data[i] > 17000) {
            TOUCH_LOG("RawData[%d] = %d\n", i, CB_data[i]);
            *pRawStatus = 1;
            sd_finger_check = 1;
        }
    }

    addr = FT3x07_FACTORY_MODE;
    command = 0x00;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xae)\n");
        goto fail;
    }
    *pDataLen += dataLen;

    ret = Ft3x07_auto_cal_ctrl(0x00, 0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    return TOUCH_SUCCESS;
fail:
    *pRawStatus = 1;
    *pChannelStatus = 1;
    dataLen += sprintf(pBuf + dataLen, "Self D Error\n");
    *pDataLen += dataLen;
    return TOUCH_FAIL;
}

static int Ft3x07_Lpwg_ChCap_Test(char* pBuf ,int* pLpwgStatus, int* pDataLen)
{
    u8 addr = FT3x07_DEVICE_MODE;
    u8 command = 0x40;
    u8 reply_data = 0;
    u8 i = 0;
    u8 channel_num = 32;
    int dataLen = 0;
    int ret = 0;
    u8 temp_data[64] = {0,};
    int CB_data[32] = {0,};
    int CB_Differ_data[32] = {0,};

    *pLpwgStatus = 0;
    dataLen += sprintf(pBuf + dataLen, "=====LPWG Test Start=====\n");

    TOUCH_LOG("=====LPWG Test Start=====\n");
    TOUCH_LOG("    -LPWG CB Value-\n");
    /* To avoid H2.0 LPWG Test Error */
    Ft3x07_Reset();

    ret = Ft3x07_auto_cal_ctrl(0x01, 0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    /* enter factory mode */
    if(Touch_I2C_Write(addr, &command, 1) < 0)
    {
        TOUCH_LOG("Device mode enter error (0x00)\n");
        goto fail;
    }
    else
    {
        if(Touch_I2C_Read(FT3x07_DEVICE_MODE, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Device mode reply data read error (0x00)\n");
            goto fail;
        }
        else
        {
            if(reply_data != 0x40)
            {
                TOUCH_LOG("Device mode reply data error(%x)\n", reply_data);
                goto fail;
            }
        }
    }
    msleep(300);

    /* Read DeltaData Buffer */
    ret = Ft3x07_read_buf(0x34, 0x00, 0x35);
    if (ret == TOUCH_FAIL)
        goto fail;

    for(i=0; i < channel_num; i++)
    {
        CB_data[i] = (buffer_data[i*2] << 8) | buffer_data[i*2+1];
        TOUCH_LOG("LPWG Rawdata %d", CB_data[i]);
        if (CB_data[i] > 17000)
            *pLpwgStatus = 1;
        if (sd_finger_check == 1)
            *pLpwgStatus = 1;
    }

    /* Current factory test mode */
    /* 0x00 : F_NORMAL 0x01 : F_TESTMODE_1 0x02 : F_TESTMODE_2 */
    addr =  FT3x07_FACTORY_MODE;
    if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
    {
        TOUCH_LOG("Current factory test mode read error (0xAE)\n");
        goto fail;
    }
/*
    else
    {
        if(reply_data == 0x00)
        {
            TOUCH_LOG("Current factory test mode reply data ok(%x)", reply_data);
        }
        else
        {
            TOUCH_LOG("Current factory test mode reply data error(%x)", reply_data);
            goto fail;
        }
    }
*/
    WRITE_SYSBUF(pBuf, dataLen, "    -LPWG CB Value-    \n");
    /*Test mode 2 enter*/
    command = 0x02;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xAE)\n");
        goto fail;
    }
    msleep(10);
    addr = FT3x07_START_SCAN;
    if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
    {
        TOUCH_LOG("Test Mode 2 Start scanf reply data error (0x08)\n");
        goto fail;
    }
    else
    {
        if(reply_data == 0x00)
        {
            TOUCH_LOG("Start scanf reply data ok(%x)\n", reply_data);
        }
        else
        {
            TOUCH_LOG("Start scanf reply data error(%x)\n", reply_data);
            goto fail;
        }
    }

    /* Start scan command */
    command = 0x01;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }
    do
    {
        msleep(15);
        if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Start scanf set reply data error (0x08)\n");
            goto fail;
        }
    }while(reply_data != 0x00);
    TOUCH_LOG(" Test Mode 2 - Data scan complete.\n");

    /*CB Address*/
    addr = 0x33;
    command = 0x00;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }

    addr = 0x39;
    if(Touch_I2C_Read(addr, temp_data, channel_num * 2/*channel number(32)*2 = 64 */) < 0)
    {
        TOUCH_LOG("Channel data read fail.(0x39)\n");
        goto fail;
    }

    for(i=0; i < channel_num; i++)
    {
        CB_data[i] = (temp_data[i*2] << 8) | temp_data[i*2+1];
        TOUCH_LOG("LPWG CB Value[%d] = %d\n", i, CB_data[i]);
        if( (CB_data[i] < 0) || (CB_data[i] > 900) )
        {
            dataLen += sprintf(pBuf + dataLen, "[ERROR]LPWG CB value  [%d] = %d\n", i, CB_data[i]);
            *pLpwgStatus = 1;
        }
        else
        {
            dataLen += sprintf(pBuf + dataLen, "LPWG CB Value [%d] = %d\n", i, CB_data[i]);
        }
    }
    addr = FT3x07_FACTORY_MODE;
    command = 0x80;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xae)\n");
        goto fail;
    }


    WRITE_SYSBUF(pBuf, dataLen, "    -LPWG CB Differ Value-    \n");
    /*Test mode 1 enter*/
    command = 0x01;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xAE)\n");
        goto fail;
    }
    msleep(10);
    addr = FT3x07_START_SCAN;
    if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
    {
        TOUCH_LOG("Test Mode 2 Start scanf reply data error (0x08)\n");
        goto fail;
    }
    else
    {
        if(reply_data == 0x00)
        {
            TOUCH_LOG("Start scanf reply data ok(%x)", reply_data);
        }
        else
        {
            TOUCH_LOG("Start scanf reply data error(%x)", reply_data);
            goto fail;
        }
    }

    /* Start scan command */
    command = 0x01;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }
    do
    {
        msleep(15);
        if(Touch_I2C_Read(addr, &reply_data, 1) < 0)
        {
            TOUCH_LOG("Start scanf set reply data error (0x08)\n");
            goto fail;
        }
    }while(reply_data != 0x00);
    TOUCH_LOG(" Test Mode 1 - Data scan complete.\n");

    /*CB Address*/
    addr = 0x33;
    command = 0x00;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Start scanf set error (0x08)\n");
        goto fail;
    }

    addr = 0x39;
    if(Touch_I2C_Read(addr, temp_data, channel_num * 2/*channel number(32)*2 = 64 */) < 0)
    {
        TOUCH_LOG("Channel data read fail.(0x39)\n");
        goto fail;
    }

    for(i=0; i < channel_num; i++)
    {
        /* F_TestMode_1 - F_TestMode_2 */
        CB_Differ_data[i] = (CB_data[i] - ((temp_data[i*2] << 8) | temp_data[i*2+1])) - Ft3x07_GoldenSample[i];
        TOUCH_LOG("CB Differ Value[%d] = %d\n", i, CB_Differ_data[i]);

        if( (CB_Differ_data[i] < -40) || (CB_Differ_data[i] > 40) )
        {
            dataLen += sprintf(pBuf + dataLen, "[ERROR]LPWG CB Differ Value  [%d] = %d\n", i, CB_Differ_data[i]);
            *pLpwgStatus = 1;
        }
        else
        {
            dataLen += sprintf(pBuf + dataLen, "LPWG CB Differ Value [%d] = %d\n", i, CB_Differ_data[i]);
        }
    }

    /* Read DeltaData Buffer */
    command = 0x00;
    if(Touch_I2C_Write(FT3x07_FACTORY_MODE, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xAE)\n");
        goto fail;
    }

    ret = Ft3x07_read_buf(0x34, 0x00, 0x35);
    if (ret == TOUCH_FAIL)
        goto fail;

    for(i=0; i < channel_num; i++)
    {
        CB_data[i] = (buffer_data[i*2] << 8) | buffer_data[i*2+1];
        TOUCH_LOG("LPWG Rawdata %d", CB_data[i]);
        if (CB_data[i] > 17000)
            *pLpwgStatus = 1;
        if (sd_finger_check == 1)
            *pLpwgStatus = 1;
    }

    addr = FT3x07_FACTORY_MODE;
    command = 0x00;
    if(Touch_I2C_Write(addr, &command, 1) < 0 )
    {
        TOUCH_LOG("Current factory test mode set error (0xae)\n");
        goto fail;
    }
    *pDataLen += dataLen;

    ret = Ft3x07_auto_cal_ctrl(0x00, 0x00);
    if (ret == TOUCH_FAIL)
        goto fail;

    return TOUCH_SUCCESS;
fail:
    *pLpwgStatus = 1;
    dataLen += sprintf(pBuf + dataLen, "LPWG Self D Error\n");
    *pDataLen += dataLen;
    return TOUCH_FAIL;
}



static int Ft3x07_DoSelfDiagnosis(int* pRawStatus, int* pChannelStatus, char* pBuf, int bufSize, int* pDataLen)
{
    /* CAUTION : be careful not to exceed buffer size */
    char *sd_path = NULL;
    int result = 0;
    int dataLen = 0;
    struct i2c_client *client = Touch_Get_I2C_Handle();
    memset(pBuf, 0, bufSize);
    *pDataLen = 0;
    *pRawStatus = TOUCH_SUCCESS;
    *pChannelStatus = TOUCH_SUCCESS;
    TOUCH_FUNC();

    if ( ts->currState != STATE_NORMAL) {
        TOUCH_ERR("don't excute SD when LCD is off! \n");
        *pRawStatus = TOUCH_FAIL;
        return TOUCH_FAIL;
    }


    /* do implementation for self-diagnosis */
    result = Ft3x07_ChCap_Test(pBuf, pRawStatus, pChannelStatus, &dataLen);
    dataLen += sprintf(pBuf+dataLen,  "======== RESULT File =======\n");
    dataLen += sprintf(pBuf+dataLen, "Channel Status : %s\n", (*pRawStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
    dataLen += sprintf(pBuf+dataLen,  "Raw Data : %s\n", (*pChannelStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");

    TOUCH_LOG("======== RESULT File =======\n");
    TOUCH_LOG("Channel Status : %s\n", (*pRawStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
    TOUCH_LOG("Raw Data : %s\n", (*pChannelStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
    TOUCH_LOG("Channel Status : %d\n", *pRawStatus);
    TOUCH_LOG("Raw Data : %d\n", *pChannelStatus);

    TOUCH_LOG("lge_get_mfts_mode() = %d\n", lge_get_mfts_mode());
    /* IMPLEMENT : self-diagnosis function */
    if (lge_get_mfts_mode() == 1) {
        sd_path = (char *) mfts_sd_path;
    }
    else if(lge_get_factory_boot() == 1){
        sd_path = (char *) factory_sd_path;
    }
    else{
        sd_path = (char *) normal_sd_path;
    }

    Ft3x07_sd_write(sd_path, pBuf, 1);
    memset(pBuf, 0, bufSize);
    log_file_size_check(&client->dev);
    *pDataLen = dataLen;
    if(result != TOUCH_SUCCESS)
        return TOUCH_FAIL;
    return TOUCH_SUCCESS;
}
// sw84.lee: this function will be added after KnockON firmware updated
static int Ft3x07_DoSelfDiagnosis_Lpwg(int *pLpwgStatus, char* pBuf, int bufSize, int* pDataLen)
{
    char *sd_path = NULL;
    int result = 0;
    int dataLen = 0;
    struct i2c_client *client = Touch_Get_I2C_Handle();
    memset(pBuf, 0, bufSize);
    *pDataLen = 0;
    *pLpwgStatus = TOUCH_SUCCESS;
    TOUCH_FUNC();

    if (lge_get_mfts_mode() == 1) {
        sd_path = (char *) mfts_sd_path;
    }
    else if(lge_get_factory_boot() == 1){
        sd_path = (char *) factory_sd_path;
    }
    else{
        sd_path = (char *) normal_sd_path;
    }
    result = Ft3x07_Lpwg_ChCap_Test(pBuf, pLpwgStatus, &dataLen);
    dataLen += sprintf(pBuf+ dataLen, "======== LPWG RESULT File =======\n");
    dataLen += sprintf(pBuf+ dataLen, "LPWG RawData : %s\n", (*pLpwgStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");

    TOUCH_LOG("======== LPWG RESULT File =======\n");
    TOUCH_LOG("LPWG RawData : %s\n", (*pLpwgStatus == TOUCH_SUCCESS) ? "Pass" : "Fail");
    TOUCH_LOG("LPWG RawData : %d\n", *pLpwgStatus);
    Ft3x07_sd_write(sd_path, pBuf, 1);
    log_file_size_check(&client->dev);
    *pDataLen = dataLen;
    if(result != TOUCH_SUCCESS)
        return TOUCH_FAIL;
    return TOUCH_SUCCESS;
}


static void Ft3x07_PowerOn(int isOn)
{
    TOUCH_FUNC();

    if (isOn && !is_power_enabled) {
        TOUCH_LOG ( "turned on the power ( VGP1 )\n" );
        TouchVioPowerModel(isOn);
        TouchVddPowerModel(isOn);
        usleep(1000);
        TouchSetGpioReset(1);
        is_power_enabled = 1;
        msleep(300);
    } else if (!isOn && is_power_enabled) {
        TOUCH_LOG ( "turned off the power ( VGP1 )\n" );
        TouchSetGpioReset(0);
        usleep(1000);
        TouchVddPowerModel(isOn);
        TouchVioPowerModel(isOn);
        is_power_enabled = 0;
    } else {
        TOUCH_LOG ( "Already turned on\n" );
    }
}

static void Ft3x07_ClearInterrupt(void)
{

}

static void Ft3x07_NotifyHandler(TouchNotify notify, int data)
{

}

TouchDeviceControlFunction Ft3x07_Func = {
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
    .DoSelfDiagnosis_Lpwg    = Ft3x07_DoSelfDiagnosis_Lpwg,
    .device_attribute_list  = Ft3x07_attribute_list,
    .NotifyHandler          = Ft3x07_NotifyHandler,
};


/* End Of File */

