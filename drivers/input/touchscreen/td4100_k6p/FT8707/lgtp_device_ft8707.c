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
 *    File  	: lgtp_device_dummy.c
 *    Author(s)   : D3 BSP Touch Team < d3-bsp-touch@lge.com >
 *    Description :
 *
 ***************************************************************************/
#define LGTP_MODULE "[FT8707]"

/****************************************************************************
* Include Files
****************************************************************************/
#include <linux/input/unified_driver_4/lgtp_common.h>

#include <linux/input/unified_driver_4/lgtp_common_driver.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_i2c.h>
#include <linux/input/unified_driver_4/lgtp_platform_api_misc.h>
#include <linux/input/unified_driver_4/lgtp_device_ft8707.h>


/****************************************************************************
* Manifest Constants / Defines
****************************************************************************/
/*******************************************************************************
* 2.Private constant and macro definitions using #define
*******************************************************************************/
/*register define*/
#define FTS_RESET_PIN	GPIO_CTP_RST_PIN
#define TPD_OK	0
#define DEVICE_MODE	0x00
#define GEST_ID	    0x01
#define TD_STATUS   0x02
#define TOUCH1_XH   0x03
#define TOUCH1_XL   0x04
#define TOUCH1_YH   0x05
#define TOUCH1_YL   0x06
#define TOUCH2_XH   0x09
#define TOUCH2_XL   0x0A
#define TOUCH2_YH   0x0B
#define TOUCH2_YL   0x0C
#define TOUCH3_XH   0x0F
#define TOUCH3_XL   0x10
#define TOUCH3_YH   0x11
#define TOUCH3_YL   0x12
#define TPD_MAX_RESET_COUNT 3


/*
#define TPD_PROXIMITY
*/

#define FTS_CTL_IIC
#define FTS_APK_DEBUG

/*
for tp esd check
*/

#if FT_ESD_PROTECT
	#define TPD_ESD_CHECK_CIRCLE	200
	static struct delayed_work gtp_esd_check_work;
	static struct workqueue_struct *gtp_esd_check_workqueue = NULL;
	static int count_irq = 0;
	static u8 run_check_91_register = 0;
	static unsigned long esd_check_circle = TPD_ESD_CHECK_CIRCLE;
	static void gtp_esd_check_func(struct work_struct *);
#endif


/****************************************************************************
 * Macros
 ****************************************************************************/


/****************************************************************************
* Type Definitions
****************************************************************************/


/****************************************************************************
* Variables
****************************************************************************/
#if defined(TOUCH_MODEL_K7)
static int ft8707_chip_rev = FT_CHIP_CUT_1;
static const char defaultFirmware[] = "focaltech/K7/FT8707/LGE_K7_FT8707_0x96_V0x81_D0x00_20160203_all.bin";
static const char defaultBootFirmware[] = "focaltech/K7/FT8707/FT8707_Pramboot_V0.2_20151215_Final.bin";
static const char defaultFirmware_cut2[] = "focaltech/K7/FT8707/LGE_K7_FT87070B_0x96_V0x9A_D0x00_20160227_all.bin";
static const char defaultBootFirmware_cut2[] = "focaltech/K7/FT8707/FT8707_Pramboot_V0.2_20151215_Final.bin";

#define PALM_REJECTION_ENABLE   1
#define CORNER_COVER_USE    1
#endif
static struct fts_ts_data *ts = NULL;
struct delayed_work work_cover;
int cover_status = 0;
extern int Touch_Quick_Cover_Closed;

#if defined(ENABLE_SWIPE_MODE)
static int get_swipe_mode = 1;
/*
static int wakeup_by_swipe = 0;
*/
extern int lockscreen_stat;
#endif

/* path of touch sef test result */
static const char normal_sd_path[] = "/sdcard/touch_self_test.txt";
static const char factory_sd_path[] = "/data/touch/touch_self_test.txt";
static const char mfts_sd_path[] = "/data/touch/touch_self_mfts.txt";
extern int lge_get_factory_boot(void);
extern bool lge_get_mfts_mode(void);
extern struct workqueue_struct* touch_wq;
extern int mfts_lpwg;

static u8 lpwg_fail_reason_onoff = FT_LPWG_FAIL_REASON_NONREAL;
static int lpwg_i2c_debug_enable = 0;

/****************************************************************************
* Extern Function Prototypes
****************************************************************************/
extern int set_deepsleep_state_ft8707(int device, int state);
extern void lcm_firmware_upgrade_recovery(void);
extern void SetDriverState(TouchDriverData *pDriverData, TouchState newState);
extern void reset_lcd_module(unsigned char reset);
extern int lge_get_lcm_revision(void);
extern void lcm_firmware_upgrade_recovery(void);

/****************************************************************************
* Local Function Prototypes
****************************************************************************/
void FT8707_Reset(int status, int delay);
static void FT8707_Reset_Dummy(void);
static int FT8707_InitRegister(void);
static void FT8707_ClearInterrupt(void);

/****************************************************************************
* Local Functions
****************************************************************************/
static int firmwareUpgrade(const struct firmware *fw_img, const struct firmware *bootfw_img)
{
    u8 * pbt_buf = NULL;
	u8 * boot_pbt_buf = NULL;
    int ret;
    int fw_len = fw_img->size;
	int boot_fw_len = bootfw_img->size;
	TOUCH_LOG("firmwareUpgrade.\n");
    /*FW upgrade*/
    pbt_buf = (u8*)fw_img->data;
	boot_pbt_buf = (u8*)bootfw_img->data;

    /*call the upgrade function*/
	ret = fts_ctpm_auto_upgrade(ts->client, pbt_buf, fw_len, boot_pbt_buf, boot_fw_len);
    if (ret != 0) {
       TOUCH_ERR("Firwmare upgrade failed. err=%d.\n", ret);
    } else {
        #ifdef AUTO_CLB
        fts_ctpm_auto_clb(ts->client);  /*start auto CLB*/
        #endif
    }
    return ret;
}

static void change_cover_func(struct work_struct *work_cover)
{
    u8 wbuf[3];
    struct i2c_client *client = Touch_Get_I2C_Handle();

    wbuf[0] = FT_VIEW_COVER;
    wbuf[1] = cover_status;

    if( FT8707_I2C_Write(client, wbuf, 2) )
    {
       TOUCH_ERR("ft8707_Set_CoverMode failed\n");
    }else{
       TOUCH_LOG("FT8707_Set_CoverMode status=%d\n",cover_status);
    }
}

void FT8707_Set_BootCoverMode(int status)
{
    INIT_DELAYED_WORK(&work_cover, change_cover_func);
	if(lge_get_boot_mode_with_cable() == LGE_BOOT_MODE_QEM_130K
        ||lge_get_boot_mode_with_cable() == LGE_BOOT_MODE_QEM_56K) {
        TOUCH_LOG("FT8707_Set_BootCoverMode Factory Mode \n");
    }
	else
	{
		cover_status = status;
        TOUCH_LOG("FT8707_Set_BootCoverMode Normal Mode \n");

	}
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

void FT8707_WriteFile(char *filename, char *data, int time)
{
	int fd = 0;
	char time_string[64] = {0};
	struct timespec my_time;
	struct tm my_date;
	mm_segment_t old_fs = get_fs();

	set_fs(KERNEL_DS);
	if (lge_get_mfts_mode() == 1) {
		filename = (char *) mfts_sd_path;
    }
	else if(lge_get_factory_boot() == 1){
		filename = (char *) factory_sd_path;
	}
	else{
		filename = (char *) normal_sd_path;
	}
	fd = sys_open(filename, O_WRONLY|O_CREAT|O_APPEND, 0666);
	if (fd >= 0) {
		if (time > 0) {
			my_time = __current_kernel_time();
			time_to_tm(my_time.tv_sec, sys_tz.tz_minuteswest * 60 * (-1), &my_date);
			snprintf(time_string, 64, "\n%02d-%02d %02d:%02d:%02d.%03lu \n\n\n", my_date.tm_mon + 1,my_date.tm_mday, my_date.tm_hour, my_date.tm_min, my_date.tm_sec, (unsigned long) my_time.tv_nsec / 1000000);
			sys_write(fd, time_string, strlen(time_string));
		}
		sys_write(fd, data, strlen(data));
		sys_close(fd);
	}
	set_fs(old_fs);
}


/****************************************************************************
* Device Specific Functions
****************************************************************************/
int ft8707_lpwg_config(struct i2c_client* client)
{
	u8 wbuf[32];
	u8 rbuf = 0;
	int ret = 0;

	ret |= fts_write_reg(client, FT_LPWG_SENSITIVITY, 0x06);		// LPWG_SENSITIVITY
	ret |= fts_write_reg(client, FT_LPWG_HZ, 0x32);				// LPWG_HZ
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_X1_L, 0x00);		// LPWG_ACTIVE_AREA (horizontal start low byte)
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_X1_H, 0x00);		// LPWG_ACTIVE_AREA (horizontal start high byte)
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_X2_L, 0x38);		// LPWG_ACTIVE_AREA (horizontal end low byte)
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_X2_H, 0x04);		// LPWG_ACTIVE_AREA (horizontal end high byte)
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_Y1_L, 0x00);		// LPWG_ACTIVE_AREA (vertical start low byte)
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_Y1_H, 0x00);		// LPWG_ACTIVE_AREA (vertical start high byte)
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_Y2_L, 0x80);		// LPWG_ACTIVE_AREA (vertical end low byte)
	ret |= fts_write_reg(client, FT_ACTIVE_AREA_Y2_H, 0x07);		// LPWG_ACTIVE_AREA (vertical end high byte)

	if(ret < 0) {
		TOUCH_ERR("Global LPWG Setting failed\n");
		return TOUCH_FAIL;
	}

	return 0;
}

int ft8707_lpwg_config_knock_on(struct i2c_client* client)
{
	u8 wbuf[32];
	u8 rbuf = 0;
	int ret = 0;

	ret |= fts_write_reg(client, FT_TCI1_SLOP, 0x0A);				// LPWG_TOUCH_SLOP
	ret |= fts_write_reg(client, FT_TCI1_DISTANCE, 0x0A);			// LPWG_DISTANCE
	ret |= fts_write_reg(client, FT_TCI1_INTER_TAP_TIME, 0x46);	// LPWG_INTERTAP_TIME
	ret |= fts_write_reg(client, FT_TCI1_TOTAL_COUNT, 0x02);		// LPWG_TAP_COUNT
	ret |= fts_write_reg(client, FT_TCI1_INT_DELAY_TIME, (ts->lpwgSetting.isFirstTwoTapSame ? (KNOCKON_DELAY/10) : 0)); // LPWG_INTERTAP_DELAY

	if(ret < 0) {
		TOUCH_ERR("Knock On Setting failed\n");
		return TOUCH_FAIL;
	}

	return 0;
}

int ft8707_lpwg_config_knock_code(struct i2c_client* client)
{
	int ret = 0;
	u8 wbuf[32];
	u8 rbuf = 0;

	ret |= fts_write_reg(client, FT_TCI2_SLOP, 0x0A);									// LPWG_TOUCH_SLOP
	ret |= fts_write_reg(client, FT_TCI2_DISTANCE, 0xFF);								// LPWG_DISTANCE 0x0A
	ret |= fts_write_reg(client, FT_TCI2_INTER_TAP_TIME, 0x46); 						// LPWG_INTERTAP_TIME 0x32
	ret |= fts_write_reg(client, FT_TCI2_TOTAL_COUNT, ts->lpwgSetting.tapCount);		// LPWG_TAP_COUNT
	ret |= fts_write_reg(client, FT_TCI2_INT_DELAY_TIME, 0x25); 						// LPWG_INTERTAP_DELAY
	if(ret < 0) {
		TOUCH_ERR("Knock On Setting failed\n");
		return TOUCH_FAIL;
	}

	return 0;
}

int ft8707_lpwg_debug_enable(struct i2c_client* client, u8 enable)
{
	u8 wbuf[4];

	wbuf[0] = FT_LPWG_FAIL_REASON_ENABLE;
	wbuf[1] = enable;
	TOUCH_LOG("Set LPWG Fail Reason = 0x%02x \n", enable);
	if( FT8707_I2C_Write(client, wbuf, 3) )
	{
		TOUCH_ERR("LPWG debug Setting failed\n");
		return TOUCH_FAIL;
	}

	return 0;
}

int ft8707_lpwg_start(struct i2c_client* client, u8 mode)
{
	u8 wbuf[3];

	wbuf[0] = FT_LPWG_START;
	wbuf[1] = mode;
	TOUCH_LOG("start lpwg_mode = 0x%02x \n", mode);
	if( FT8707_I2C_Write(client, wbuf, 2) )
	{
		TOUCH_ERR("ft8707_lpwg_start failed\n");
		return TOUCH_FAIL;
	}

	return 0;
}

static int lpwg_control(struct i2c_client *client, TouchState newState)
{
	TOUCH_FUNC();

	switch( newState )
	{
		case STATE_NORMAL:
			TOUCH_LOG("Normal.\n");
			set_deepsleep_state_ft8707(1,0); /*LCD Deep Sleep OUT*/
			break;

		case STATE_KNOCK_ON_ONLY:
			if(cover_status){
				ft8707_lpwg_start(client, FT_DOUBLE_TAP_ONLY);
				TOUCH_LOG("cover_status is closed, sensing disable\n");
			}else{
				if(ts->currState == STATE_OFF) {
					if(ft8707_chip_rev == FT_CHIP_CUT_1) {
						TOUCH_LOG("Cut 1 Deep -> LPWG\n");
						set_deepsleep_state_ft8707(1,0);
						mdelay(5);
					reset_lcd_module(0);
					mdelay(2);
					reset_lcd_module(1);
					mdelay(90);
					} else {
						TOUCH_LOG("Cut 2 Deep -> LPWG\n");
						set_deepsleep_state_ft8707(1,0);
											FT8707_Reset(0, 2);
											FT8707_Reset(1, 90);
					}
				}

				if(ft8707_chip_rev == FT_CHIP_CUT_1){
					TOUCH_LOG("CUT 1 SKIP RESET for LPWG \n");
				} else {
					TOUCH_LOG("CUT 2 RESET for LPWG \n");
					//FT8707_Reset(0, 2);
					//FT8707_Reset(1, 200);
				}

				#if 0		/* V1.0 work around S */
				if(ts->currState == STATE_NORMAL) {
					reset_lcd_module(0);
					mdelay(2);
					reset_lcd_module(1);
					mdelay(100);
				}
				#endif		/* V1.0 work around E */

				if(lpwg_i2c_debug_enable)
					FT8707_set_i2c_mode(1);
				ft8707_lpwg_config(client);
				ft8707_lpwg_config_knock_on(client);
				ft8707_lpwg_debug_enable(client, lpwg_fail_reason_onoff);
				ft8707_lpwg_start(client, FT_DOUBLE_TAP_ONLY);
				TOUCH_LOG("Knock On \n");
			}
			break;

		case STATE_KNOCK_ON_CODE:
			if(cover_status){
				ft8707_lpwg_start(client, FT_LPWG_ON);
				TOUCH_LOG("cover_status is closed, sensing disable\n");
			}else{
				if(ts->currState == STATE_OFF) {
					TOUCH_LOG("Before State is LPWG OFF \n");
					if(ft8707_chip_rev == FT_CHIP_CUT_1) {
						TOUCH_LOG("Cut 1 Deep -> LPWG\n");
					 set_deepsleep_state_ft8707(1,0);
					mdelay(5);
					reset_lcd_module(0);
					mdelay(2);
					reset_lcd_module(1);
					mdelay(90);
					} else {
						set_deepsleep_state_ft8707(1,0);
						TOUCH_LOG("Cut 2 Deep -> LPWG\n");
											FT8707_Reset(0, 2);
											FT8707_Reset(1, 90);
					}
				}

				if(ft8707_chip_rev == FT_CHIP_CUT_1){
					TOUCH_LOG("CUT 1 SKIP RESET for LPWG \n");
				} else {
					TOUCH_LOG("CUT 2 RESET for LPWG \n");
//					FT8707_Reset(0, 2);
//					FT8707_Reset(1, 200);
				}

				#if 0		/* V1.0 work around S */
				if(ts->currState == STATE_NORMAL) {
					reset_lcd_module(0);
					mdelay(2);
					reset_lcd_module(1);
					mdelay(100);
				}
				#endif		/* V1.0 work around E */

				if(lpwg_i2c_debug_enable)
					FT8707_set_i2c_mode(1);

				ft8707_lpwg_config(client);
				ft8707_lpwg_config_knock_on(client);
				ft8707_lpwg_config_knock_code(client);
				ft8707_lpwg_debug_enable(client, lpwg_fail_reason_onoff);
				ft8707_lpwg_start(client, FT_LPWG_ON);
				TOUCH_LOG("Knock On / Code \n");
			}
			break;

		case STATE_OFF:
			FT8707_Reset(0,1);
			FT8707_Reset(1, 100);
			fts_write_reg(client, FT_DEEP_SLEEP, 0x03);
			mdelay(5);
			set_deepsleep_state_ft8707(1,1);
			TOUCH_LOG("STATE OFF\n");
			break;

		default:
		TOUCH_ERR("invalid touch state ( %d )\n", newState);
		break;
	}

	return TOUCH_SUCCESS;
}

int ft8707_switch_cal(struct i2c_client *client, u8 cal_en)
{
	u8 ret;
	u8 data;
	u8 addr;

	TOUCH_LOG("%s : cal_en = 0x%02x\n", __func__, cal_en);

	addr = 0xEE;
	data = 0x00;
	ret = fts_read_reg(client, addr, &data);
	if(ret < 0) {
		TOUCH_ERR("i2c error\n");
		return ret;
	}

	if (data == cal_en) {
		TOUCH_LOG("Already switch_cal changed\n");
		return 0;
	}

	data = cal_en;
	ret = fts_write_reg(client, addr, data);
	if(ret < 0) {
		TOUCH_ERR("i2c error\n");
		return ret;
	}

	mdelay(10);

	return 0;
}


static ssize_t show_rawdata(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int rawdataStatus = 0;
	int result = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData for rawdata test\n");
		return ret;
	}

	mutex_lock(&pDriverData->thread_lock);
	TouchDisableIrq();

#if 1
	// Change clb switch
	ft8707_switch_cal(client, 0);
#endif

	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to Change mode [factory]\n");
		goto ERROR;
	}

	// rawdata check
	ret = FT8707_GetTestResult(client, buf, &rawdataStatus, RAW_DATA_SHOW);
	if (ret < 0) {
		TOUCH_ERR("failed to get raw data\n");
		ret += sprintf(buf, "failed to get raw data\n");
		goto ERROR;
	}

	result = FT8707_change_mode(client, FTS_WORK_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [work]\n");
		goto ERROR;
	}

ERROR:
	TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}

static ssize_t show_noise(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int noiseStatus = 0;
	int result = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData for noise test\n");
		return ret;
	}

	mutex_lock(&pDriverData->thread_lock);
	TouchDisableIrq();

#if 1
	// Change clb switch
	ft8707_switch_cal(client, 0);
#endif

	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [factory] \n");
		goto ERROR;
	}

	// noise check
	ret = FT8707_GetTestResult(client, buf, &noiseStatus, NOISE_SHOW);
	if (ret < 0) {
		TOUCH_ERR("failed to get noise data\n");
		ret += sprintf(buf, "failed to get noise data\n");
		goto ERROR;
	}

	result = FT8707_change_mode(client, FTS_WORK_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [work] \n");
		goto ERROR;
	}

ERROR:
	TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}

static ssize_t show_p2p_noise(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int p2p_NoiseStatus = 0;
	int result = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData for P2P noise test\n");
		return ret;
	}

if (ft8707_chip_rev == FT_CHIP_CUT_1){
		TOUCH_ERR("failed to get pDriverData for P2P noise test\n");
		return ret;
	}

	mutex_lock(&pDriverData->thread_lock);
	TouchDisableIrq();

#if 1
	// Change clb switch
	ft8707_switch_cal(client, 0);
#endif

	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR(" Fail to Change mode [factory] \n");
		goto ERROR;
	}

	// noise check
	ret = FT8707_GetTestResult(client, buf, &p2p_NoiseStatus, P2P_NOISE_SHOW);
	if (ret < 0) {
		TOUCH_ERR("failed to get P2P noise data\n");
		ret += sprintf(buf, "failed to get P2P noise data\n");
		goto ERROR;
	}

	result = FT8707_change_mode(client, FTS_WORK_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR(" Fail to Change mode [work] \n");
		goto ERROR;
	}

ERROR:
	TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}

static ssize_t show_short(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int shortStatus = 0;
	int result = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData for short test\n");
		return ret;
	}

	if(ft8707_chip_rev != FT_CHIP_CUT_2){
		TOUCH_ERR("Can't excute short test in CUT 1\n");
		return ret;
	}

	if( ts->currState == STATE_NORMAL ) {
		TOUCH_ERR("Can't Excute short Test in LCD On \n");
		return ret;
	}

	mutex_lock(&pDriverData->thread_lock);
	TouchDisableIrq();

#if 1
	// Change clb switch
	ft8707_switch_cal(client, 0);
#endif

	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [factory] \n");
		goto ERROR;
	}

	// short check
	ret = FT8707_GetTestResult(client, buf, &shortStatus, SHORT_SHOW);
	if (ret < 0) {
		TOUCH_ERR("failed to get short data\n");
		ret += sprintf(buf, "failed to get short data\n");
		goto ERROR;
	}

	result = FT8707_change_mode(client, FTS_WORK_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [work] \n");
		goto ERROR;
	}

ERROR:
	TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}

static ssize_t show_ib(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int ibStatus = 0;
	int result = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData for ib test\n");
		return ret;
	}

	mutex_lock(&pDriverData->thread_lock);
	TouchDisableIrq();
#if 1
	// Change clb switch
	ft8707_switch_cal(client, 0);
#endif
	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR(" Fail to Change mode [factory]\n");
		goto ERROR;
	}

	// ib check
	ret = FT8707_GetTestResult(client, buf, &ibStatus, IB_SHOW);
	if (ret < 0) {
		TOUCH_ERR("failed to get ib data\n");
		ret += sprintf(buf, "failed to get ib data\n");
		goto ERROR;
	}

	result = FT8707_change_mode(client, FTS_WORK_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to mode Change [work]\n");
		goto ERROR;
	}

ERROR:
	TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}

static ssize_t show_delta(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int deltaStatus = 0;
	int result = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData for delta test\n");
		return ret;
	}

	mutex_lock(&pDriverData->thread_lock);
	TouchDisableIrq();

#if 1
	// Change clb switch
	ft8707_switch_cal(client, 0);
#endif

	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [factory] \n");
		goto ERROR;
	}

	// delta check
	ret = FT8707_GetTestResult(client, buf, &deltaStatus, DELTA_SHOW);
	if (ret < 0) {
		TOUCH_ERR("failed to get delta data\n");
		ret += sprintf(buf, "failed to get delta data\n");
		goto ERROR;
	}

	result = FT8707_change_mode(client, FTS_WORK_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [work] \n");
		goto ERROR;
	}

ERROR:
	TouchEnableIrq();
	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}

static ssize_t store_reg_control(TouchDriverData *pDriverData, const char *buf, size_t count)
{
    struct i2c_client *client = Touch_Get_I2C_Handle();
    char cmd[10] = {0};
    int reg_addr = {0};
	int	offset = 0;
    int value = 0;
    u8 r_buf[50] = {0};
	u8 w_buf[51] = {0,};

    if ( sscanf(buf, "%s %d %d %d", cmd, &reg_addr, &offset, &value) != 4) {
        TOUCH_LOG("data parsing fail.\n");
		TOUCH_LOG("Usage : \n");
		TOUCH_LOG("read reg offset value \n");
		TOUCH_LOG("write reg offset value \n");
        return count;
    }

    TOUCH_LOG("reg ctrl : %s, 0x%02x, %d, %d\n", cmd, reg_addr, offset, value);

	mutex_lock(&pDriverData->thread_lock);
	if(!strcmp(cmd, "read")){
		FT8707_I2C_Read(client, &reg_addr, 1, r_buf, offset+1);
		TOUCH_LOG("Read : reg_addr[0x%02x] offset[%d] value[0x%02x]\n", reg_addr, offset, r_buf[offset]);
	} else if(!strcmp(cmd, "write")){
		FT8707_I2C_Read(client, &reg_addr, 1, r_buf, offset+1);
		r_buf[offset] = (u8)value;
		w_buf[0] = (u8)reg_addr;
		memcpy(&w_buf[1], r_buf, offset+1);
		FT8707_I2C_Write(client, w_buf, offset+2);
		TOUCH_LOG("Write : reg_addr[0x%02x] offset[%d] value[0x%02x]\n", reg_addr, offset, w_buf[offset+1]);
	} else {
		TOUCH_LOG("Usage : \n");
		TOUCH_LOG("read reg value offset \n");
		TOUCH_LOG("write reg value offset \n");
	}
	mutex_unlock(&pDriverData->thread_lock);
    return count;
}

static ssize_t store_ftk_fw_upgrade(TouchDriverData *pDriverData, const char *buf, size_t count)
{
	char boot_name[MAX_FILENAME];
	char app_name[MAX_FILENAME];
	const struct firrmware *boot_fw = NULL;
	const struct firrmware *app_fw = NULL;
	int ret = 0;

	if (count > MAX_FILENAME)
		return count;

	if ( sscanf(buf, "%s %s", boot_name, app_name) != 2) {
       	TOUCH_LOG("f/w name parsing fail.\n");
        return count;
    }

	TOUCH_LOG("ftk_fw_upgrade : boot = %s, app = %s \n", boot_name, app_name);

	mutex_lock(&pDriverData->thread_lock);
	if(boot_name != NULL && app_name != NULL) {
		/* Update Firmware */
		ret = FT8707_RequestFirmware(app_name, boot_name, &app_fw, &boot_fw);
		if(ret < 0) {
			TOUCH_LOG("Fail to request F/W. \n");
			goto EXIT;
		}
		ret = firmwareUpgrade(app_fw, boot_fw);
		if(ret < 0) {
			TOUCH_LOG("Firmware Upgrade Fail. \n");
			release_firmware(boot_fw);
			release_firmware(app_fw);
			goto EXIT;
		}

		release_firmware(boot_fw);
		release_firmware(app_fw);

	} else {
		TOUCH_LOG("Error : can't find fw, boot = %s, app = %s \n", boot_name, app_name);
		goto EXIT;
	}
	mutex_unlock(&pDriverData->thread_lock);
	return count;

EXIT:
	mutex_unlock(&pDriverData->thread_lock);
	return count;
}

static ssize_t store_ftk_reset(TouchDriverData *pDriverData, const char *buf, size_t count)
{
	int ret = 0;

	if (count > MAX_FILENAME)
		return count;

	TOUCH_LOG("ftk_reset .\n");

	mutex_lock(&pDriverData->thread_lock);
	FT8707_Reset(0,5);
	FT8707_Reset(1, 450);
	mutex_unlock(&pDriverData->thread_lock);
	return count;
}

static ssize_t store_lpwg_fail_reason(TouchDriverData *pDriverData, const char *buf, size_t count)
{
	int ret = 0;
	int cmd = 0;

	if (count > MAX_FILENAME)
		return count;

	mutex_lock(&pDriverData->thread_lock);

	sscanf(buf, "%d", &cmd);
	TOUCH_LOG(" set LPWG Fail Reason [%d] to [%d].\n", lpwg_fail_reason_onoff, cmd);

	switch(cmd){
		case LPWG_FR_OFF:
			TOUCH_LOG("[SUCESS] set LPWG Fail Reason to FT_LPWG_FAIL_REASON_OFF\n");
			lpwg_fail_reason_onoff = FT_LPWG_FAIL_REASON_OFF;
			break;

		case LPWG_FR_NONRT:
			TOUCH_LOG("[SUCESS] set LPWG Fail Reason to FT_LPWG_FAIL_REASON_NONREAL\n");
			lpwg_fail_reason_onoff = FT_LPWG_FAIL_REASON_NONREAL;
			break;

		case LPWG_FR_RT:
			TOUCH_LOG("[SUCESS] set LPWG Fail Reason to FT_LPWG_FAIL_REASON_REAL\n");
			lpwg_fail_reason_onoff = FT_LPWG_FAIL_REASON_REAL;
			break;

		case LPWG_FR_ALL:
			TOUCH_LOG("[SUCESS] set LPWG Fail Reason to FT_LPWG_FAIL_REASON_ALL\n");
			lpwg_fail_reason_onoff = FT_LPWG_FAIL_REASON_ALL;
			break;

		default:
			TOUCH_LOG("[Error] unknown LPWG Fail Reason = %d\n", cmd);
			goto ERROR;
			break;
	}

	mutex_unlock(&pDriverData->thread_lock);
	return count;

ERROR:
	mutex_unlock(&pDriverData->thread_lock);
	return count;
}

static ssize_t show_lpwg_fail_reason(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int intensityStatus = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData. \n");
	}
	mutex_lock(&pDriverData->thread_lock);

	TOUCH_FUNC();
	ret += sprintf(buf, "lpwg_fail_reason_onoff = 0x%02x \n", lpwg_fail_reason_onoff);
	TOUCH_LOG("lpwg_fail_reason_onoff = 0x%02x \n", lpwg_fail_reason_onoff);

	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}

static ssize_t show_register_dump(TouchDriverData *pDriverData, char *buf)
{
	int ret = 0;
	int intensityStatus = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();
	struct register_dump r_dump;
	TOUCH_FUNC();

	if (pDriverData == NULL){
		TOUCH_ERR("failed to get pDriverData. \n");
	}
	mutex_lock(&pDriverData->thread_lock);
	memset(&r_dump, 0x0, sizeof(r_dump));

	/* lpwg global configuration */
	TOUCH_LOG("lPWG Global CONFIG =====================\n");
	ret += sprintf(buf+ret, "lPWG Global CONFIG =====================\n");

	fts_read_reg(client, FT_LPWG_SENSITIVITY, &r_dump.lpwg_global_dump.read_buf[0]);
	fts_read_reg(client, FT_LPWG_HZ, &r_dump.lpwg_global_dump.read_buf[1]);
	fts_read_reg(client, FT_ACTIVE_AREA_X1_L, &r_dump.lpwg_global_dump.read_buf[2]);
	fts_read_reg(client, FT_ACTIVE_AREA_X1_H, &r_dump.lpwg_global_dump.read_buf[3]);
	fts_read_reg(client, FT_ACTIVE_AREA_X2_L, &r_dump.lpwg_global_dump.read_buf[4]);
	fts_read_reg(client, FT_ACTIVE_AREA_X2_H, &r_dump.lpwg_global_dump.read_buf[5]);
	fts_read_reg(client, FT_ACTIVE_AREA_Y1_L, &r_dump.lpwg_global_dump.read_buf[6]);
	fts_read_reg(client, FT_ACTIVE_AREA_Y1_H, &r_dump.lpwg_global_dump.read_buf[7]);
	fts_read_reg(client, FT_ACTIVE_AREA_Y2_L, &r_dump.lpwg_global_dump.read_buf[8]);
	fts_read_reg(client, FT_ACTIVE_AREA_Y2_H, &r_dump.lpwg_global_dump.read_buf[9]);

	TOUCH_LOG("FT_LPWG_SENSITIVITY val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[0]);
	TOUCH_LOG("FT_LPWG_HZ val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[1]);
	TOUCH_LOG("FT_ACTIVE_AREA_X1_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[2]);
	TOUCH_LOG("FT_ACTIVE_AREA_X1_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[3]);
	TOUCH_LOG("FT_ACTIVE_AREA_X2_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[4]);
	TOUCH_LOG("FT_ACTIVE_AREA_X2_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[5]);
	TOUCH_LOG("FT_ACTIVE_AREA_Y1_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[6]);
	TOUCH_LOG("FT_ACTIVE_AREA_Y1_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[7]);
	TOUCH_LOG("FT_ACTIVE_AREA_Y2_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[8]);
	TOUCH_LOG("FT_ACTIVE_AREA_Y2_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[9]);

	ret += sprintf(buf+ret, "FT_LPWG_SENSITIVITY val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[0]);
	ret += sprintf(buf+ret, "FT_LPWG_HZ val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[1]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_X1_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[2]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_X1_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[3]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_X2_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[4]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_X2_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[5]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_Y1_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[6]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_Y1_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[7]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_Y2_L val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[8]);
	ret += sprintf(buf+ret, "FT_ACTIVE_AREA_Y2_H val = 0x%02x \n", r_dump.lpwg_global_dump.read_buf[9]);

	/* lpwg tci_1 configuration */
	TOUCH_LOG("lPWG TCI_1 CONFIG =====================\n");
	ret += sprintf(buf+ret , "lPWG TCI_1 CONFIG =====================\n");

	fts_read_reg(client, FT_TCI1_SLOP, &r_dump.lpwg_tci_1_dump.read_buf[0]);
	fts_read_reg(client, FT_TCI1_DISTANCE, &r_dump.lpwg_tci_1_dump.read_buf[1]);
	fts_read_reg(client, FT_TCI1_INTER_TAP_TIME, &r_dump.lpwg_tci_1_dump.read_buf[2]);
	fts_read_reg(client, FT_TCI1_TOTAL_COUNT, &r_dump.lpwg_tci_1_dump.read_buf[3]);
	fts_read_reg(client, FT_TCI1_INT_DELAY_TIME, &r_dump.lpwg_tci_1_dump.read_buf[4]);

	TOUCH_LOG("FT_TCI1_SLOP val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[0]);
	TOUCH_LOG("FT_TCI1_DISTANCE val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[1]);
	TOUCH_LOG("FT_TCI1_INTER_TAP_TIME val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[2]);
	TOUCH_LOG("FT_TCI1_TOTAL_COUNT val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[3]);
	TOUCH_LOG("FT_TCI1_INT_DELAY_TIME val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[4]);

	ret += sprintf(buf+ret, "FT_TCI1_SLOP val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[0]);
	ret += sprintf(buf+ret, "FT_TCI1_DISTANCE val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[1]);
	ret += sprintf(buf+ret, "FT_TCI1_INTER_TAP_TIME val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[2]);
	ret += sprintf(buf+ret, "FT_TCI1_TOTAL_COUNT val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[3]);
	ret += sprintf(buf+ret, "FT_TCI1_INT_DELAY_TIME val = 0x%02x \n", r_dump.lpwg_tci_1_dump.read_buf[4]);

	/* lpwg tci_2 configuration */
	TOUCH_LOG("lPWG TCI_2 CONFIG =====================\n");
	ret += sprintf(buf+ret, "lPWG TCI_2 CONFIG =====================\n");

	fts_read_reg(client, FT_TCI2_SLOP, &r_dump.lpwg_tci_2_dump.read_buf[0]);
	fts_read_reg(client, FT_TCI2_DISTANCE, &r_dump.lpwg_tci_2_dump.read_buf[1]);
	fts_read_reg(client, FT_TCI2_INTER_TAP_TIME, &r_dump.lpwg_tci_2_dump.read_buf[2]);
	fts_read_reg(client, FT_TCI2_TOTAL_COUNT, &r_dump.lpwg_tci_2_dump.read_buf[3]);
	fts_read_reg(client, FT_TCI2_INT_DELAY_TIME, &r_dump.lpwg_tci_2_dump.read_buf[4]);

	TOUCH_LOG("FT_TCI2_SLOP val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[0]);
	TOUCH_LOG("FT_TCI2_DISTANCE val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[1]);
	TOUCH_LOG("FT_TCI2_INTER_TAP_TIME val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[2]);
	TOUCH_LOG("FT_TCI2_TOTAL_COUNT val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[3]);
	TOUCH_LOG("FT_TCI2_INT_DELAY_TIME val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[4]);

	ret += sprintf(buf+ret, "FT_TCI2_SLOP val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[0]);
	ret += sprintf(buf+ret, "FT_TCI2_DISTANCE val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[1]);
	ret += sprintf(buf+ret, "FT_TCI2_INTER_TAP_TIME val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[2]);
	ret += sprintf(buf+ret, "FT_TCI2_TOTAL_COUNT val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[3]);
	ret += sprintf(buf+ret, "FT_TCI2_INT_DELAY_TIME val = 0x%02x \n", r_dump.lpwg_tci_2_dump.read_buf[4]);

	ret += sprintf(buf+ret, "lpwg_fail_reason_onoff = 0x%02x \n", lpwg_fail_reason_onoff);
	TOUCH_LOG("lpwg_fail_reason_onoff = 0x%02x \n", lpwg_fail_reason_onoff);

	TOUCH_LOG("Done ================================\n");
	ret += sprintf(buf+ret, "Done ================================\n");

	mutex_unlock(&pDriverData->thread_lock);

	return ret;
}


static LGE_TOUCH_ATTR(rawdata, S_IRUGO | S_IWUSR, show_rawdata, NULL);
static LGE_TOUCH_ATTR(ib, S_IRUGO | S_IWUSR, show_ib, NULL);
static LGE_TOUCH_ATTR(jitter, S_IRUGO | S_IWUSR, show_noise, NULL);
static LGE_TOUCH_ATTR(p2p_noise, S_IRUGO | S_IWUSR, show_p2p_noise, NULL);
static LGE_TOUCH_ATTR(short, S_IRUGO | S_IWUSR, show_short, NULL);
static LGE_TOUCH_ATTR(delta, S_IRUGO | S_IWUSR, show_delta, NULL);
static LGE_TOUCH_ATTR(reg_control,  S_IRUGO | S_IWUSR, NULL, store_reg_control);
static LGE_TOUCH_ATTR(ftk_fw_upgrade,  S_IRUGO | S_IWUSR, NULL, store_ftk_fw_upgrade);
static LGE_TOUCH_ATTR(reset,  S_IRUGO | S_IWUSR, NULL, store_ftk_reset);
static LGE_TOUCH_ATTR(lpwg_fail_reason,  S_IRUGO | S_IWUSR, show_lpwg_fail_reason, store_lpwg_fail_reason);
static LGE_TOUCH_ATTR(reg_dump,  S_IRUGO | S_IWUSR, show_register_dump, NULL);


static struct attribute *FT8707_attribute_list[] = {
	&lge_touch_attr_rawdata.attr,
	&lge_touch_attr_ib.attr,
	&lge_touch_attr_jitter.attr,
	&lge_touch_attr_p2p_noise.attr,
	&lge_touch_attr_short.attr,
	&lge_touch_attr_delta.attr,
	&lge_touch_attr_reg_control.attr,
	&lge_touch_attr_ftk_fw_upgrade.attr,
	&lge_touch_attr_reset.attr,
	&lge_touch_attr_lpwg_fail_reason.attr,
	&lge_touch_attr_reg_dump.attr,
	NULL,
};


static int FT8707_Initialize(TouchDriverData *pDriverData)
{
	struct i2c_client *client = Touch_Get_I2C_Handle();
	int err = 0;
	u8 data = 0;
	u8 read_addr = 0;
	int reset_count = 0;
	const char *pFwFilename = NULL;
	const char *pBootFwFilename = NULL;
	const struct firmware *fw = NULL;
	const struct firmware *bootFw = NULL;
    TOUCH_FUNC();

	/* IMPLEMENT : Device initialization at Booting */
	ts = devm_kzalloc(&client->dev, sizeof(struct fts_ts_data), GFP_KERNEL);
	if (ts == NULL) {
		TOUCH_ERR("failed to allocate memory for device driver data\n");
		return TOUCH_FAIL;
	}

	ts->client = client;

	/* Need to Implement add misc driver register */
	/* temp chip rev */
	ft8707_chip_rev = lge_get_lcm_revision();
	TOUCH_LOG("ret = %d , FT8707 CHIP Rev = %s \n",ft8707_chip_rev, (ft8707_chip_rev==FT_CHIP_CUT_1)?"CUT 1 !!":"CUT 2 !!");

	/* F/W Update Check */
RE_TRY:
	read_addr = 0x00;
	err = FT8707_I2C_Read(client, &read_addr, 1, &data, 1);// if auto upgrade fail, it will not read right value next upgrade.

	TOUCH_LOG("[%s] %d,data:%d\n", __func__, err, data);

	if(err< 0 || data!=0) {	// reg0 data running state is 0; other state is not 0
		TOUCH_LOG("I2C transfer error, line: %d\n", __LINE__);

        if ( ++reset_count < TPD_MAX_RESET_COUNT ) {
        	TOUCH_LOG("Reset Process \n");
			FT8707_Reset(0, 80);
			FT8707_Reset(1, 130);
			goto RE_TRY;
        }
		TOUCH_LOG(" Touch Working Mode Check Fail. \n");
		return TOUCH_FAIL;
	}

	fts_get_upgrade_array(client); /* flash */

#ifdef FTS_CTL_IIC
		 if (fts_rw_iic_drv_init(client) < 0)
			 TOUCH_LOG("%s:[FTS] create fts control iic driver failed\n", __func__);
#endif

#ifdef FTS_APK_DEBUG
		fts_create_apk_debug_channel(client);
#endif

	return TOUCH_SUCCESS;
}

void FT8707_Reset(int status, int delay)
{
    if (!status) TouchDisableIrq();
	TouchSetGpioReset(status);
	if ( delay <= 0 || delay > 1000 )
	{
		TOUCH_LOG("%s exeeds limit %d\n", __func__, delay);
		if(status) TouchEnableIrq();
		return;
	}

	if (delay<=20)
		mdelay(delay);
	else
		msleep(delay);
    if(status){
        TouchEnableIrq();
        if(cover_status)queue_delayed_work(touch_wq, &work_cover, msecs_to_jiffies(10));
    }
}

static void FT8707_Reset_Dummy(void)
{
	TOUCH_LOG("Reset Dummy \n");
}

static int FT8707_InitRegister(void)
{

	/* IMPLEMENT : Register initialization after reset */

	return TOUCH_SUCCESS;
}

void FT8707_display_NRT_fail_reason(void) {
	struct i2c_client *client = Touch_Get_I2C_Handle();
	u8 w_buf[2] = {0, };
	u8 r_buf[11] = {0, };
	int i = 0;
	int ret = 0;

	if(lge_get_mfts_mode()) {
		TOUCH_LOG("MFTS Mode : Can't Read NRT Fail Reason \n");
		return;
	}

	if(lpwg_fail_reason_onoff == FT_LPWG_FAIL_REASON_OFF || lpwg_fail_reason_onoff == FT_LPWG_FAIL_REASON_REAL){
		TOUCH_LOG("Disabled Non RT Fail Reason \n");
		return;
	}

	/* TCI 1 Fail Reason */
	w_buf[0] = FT_LPWG_NON_FAIL_REASON_TCI_1;
	ret = FT8707_I2C_Read(client, w_buf, 1, r_buf, 11);
	TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON CNT = 0x%02x \n", r_buf[0]);
	if (ret < 0 || r_buf[0] > 10) {
		TOUCH_LOG("Fail to Read NRT CNT \n");
		return;
	}
	for(i = 1; i <= r_buf[0];i++) {
		switch(r_buf[i]){
			case LPWG_NO_FAIL:
				break;
			case DISTANCE_INTER_TAP:
				TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON = DISTANCE_INTER_TAP\n");
				break;
			case TOUCH_SLOP:
				TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON = TOUCH_SLOP\n");
				break;
			case TIME_OUT_INTER_TAP:
				TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON = TIME_OUT_INTER_TAP\n");
				break;
			case MULTI_FINGER:
				TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON = MULTI_FINGER\n");
				break;
			case INTERRUPT_DELAY_TIME_FAIL:
				TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON = INTERRUPT_DELAY_TIME_FAIL\n");
				break;
			case PALM_DETECT:
				TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON = PALM_DETECT\n");
				break;
			default:
				TOUCH_LOG("LPWG[TCI_1] NonRT FAIL REASON = Unknown Reason = 0x%02x\n", r_buf[i]);
				break;
		}
	}


	/* TCI 2 Fail Reason */
	w_buf[0] = FT_LPWG_NON_FAIL_REASON_TCI_2;
	ret = FT8707_I2C_Read(client, w_buf, 1, r_buf, 11);
	TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON CNT = 0x%02x \n", r_buf[0]);
	if (ret < 0 || r_buf[0] > 10) {
		TOUCH_LOG("Fail to Read NRT CNT \n");
		return;
	}
	for(i = 1; i <= r_buf[0];i++) {
		switch(r_buf[i]){
			case LPWG_NO_FAIL:
				break;
			case DISTANCE_INTER_TAP:
				TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON = DISTANCE_INTER_TAP\n");
				break;
			case TOUCH_SLOP:
				TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON = TOUCH_SLOP\n");
				break;
			case TIME_OUT_INTER_TAP:
				TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON = TIME_OUT_INTER_TAP\n");
				break;
			case MULTI_FINGER:
				TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON = MULTI_FINGER\n");
				break;
			case INTERRUPT_DELAY_TIME_FAIL:
				TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON = INTERRUPT_DELAY_TIME_FAIL\n");
				break;
			case PALM_DETECT:
				TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON = PALM_DETECT\n");
				break;
			default:
				TOUCH_LOG("LPWG[TCI_2] NonRT FAIL REASON = Unknown Reason = 0x%02x\n", r_buf[i]);
				break;
		}
	}

}

int FT8707_set_i2c_mode(int mode){

	int ret;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_LOG("After LPWG Mode, CPU set to Standby Mode.");

	ret = fts_write_reg(client, FTS_LPWG_I2C_EN, mode);
	if(ret < 0) {
		TOUCH_ERR("set i2c mode failed\n");
		return TOUCH_FAIL;
	}
	return TOUCH_SUCCESS;
}

static void FT8707_display_RT_fail_reason(u8 f_reason) {

		/* TCI 1 Fail Reason */
		if(f_reason & 0x0F) {
			switch((f_reason & 0x0F)) {
				case LPWG_NO_FAIL:
					break;
				case DISTANCE_INTER_TAP:
					TOUCH_LOG("LPWG[TCI_1] FAIL REASON = DISTANCE_INTER_TAP\n");
					break;
				case TOUCH_SLOP:
					TOUCH_LOG("LPWG[TCI_1] FAIL REASON = TOUCH_SLOP\n");
					break;
				case TIME_OUT_INTER_TAP:
					TOUCH_LOG("LPWG[TCI_1] FAIL REASON = TIME_OUT_INTER_TAP\n");
					break;
				case MULTI_FINGER:
					TOUCH_LOG("LPWG[TCI_1] FAIL REASON = MULTI_FINGER\n");
					break;
				case INTERRUPT_DELAY_TIME_FAIL:
					TOUCH_LOG("LPWG[TCI_1] FAIL REASON = INTERRUPT_DELAY_TIME_FAIL\n");
					break;
				case PALM_DETECT:
					TOUCH_LOG("LPWG[TCI_1] FAIL REASON = PALM_DETECT\n");
					break;
				default:
					TOUCH_LOG("LPWG[TCI_1] FAIL REASON = Unknown Reason = 0x%02x\n", (f_reason & 0x0F));
					break;
			}
		}

		/* TCI 2 Fail Reason */
		if(f_reason & 0xF0) {
			switch((f_reason & 0xF0) >> 4) {
				case LPWG_NO_FAIL:
					break;
				case DISTANCE_INTER_TAP:
					TOUCH_LOG("LPWG[TCI_2] FAIL REASON = DISTANCE_INTER_TAP\n");
					break;
				case TOUCH_SLOP:
					TOUCH_LOG("LPWG[TCI_2] FAIL REASON = TOUCH_SLOP\n");
					break;
				case TIME_OUT_INTER_TAP:
					TOUCH_LOG("LPWG[TCI_2] FAIL REASON = TIME_OUT_INTER_TAP\n");
					break;
				case MULTI_FINGER:
					TOUCH_LOG("LPWG[TCI_2] FAIL REASON = MULTI_FINGER\n");
					break;
				case INTERRUPT_DELAY_TIME_FAIL:
					TOUCH_LOG("LPWG[TCI_2] FAIL REASON = INTERRUPT_DELAY_TIME_FAIL\n");
					break;
				case PALM_DETECT:
					TOUCH_LOG("LPWG[TCI_2] FAIL REASON = PALM_DETECT\n");
					break;
				default:
					TOUCH_LOG("LPWG[TCI_2] FAIL REASON = Unknown Reason = 0x%02x\n", (f_reason & 0xF0) >> 4);
					break;
			}
		}

}

static int FT8707_InterruptHandler(TouchReadData *pData)
{
	TouchFingerData *pFingerData = NULL;
	u8 i = 0;
	u8 k = 0;
	u8 wbuf[8] = {0};
	u8 rbuf[256] = {0};
	u8 num_finger = 0;
	u8 buf[POINT_READ_BUF] = {0,};
	u8 pointid = FTS_MAX_ID;

	u32 packet_size = 0;
	u8 packet_type = 0;
	u8 alert_type = 0;
	u8 state = 0;
	u8 tap_cnt = 0;
	u8 real_lpwg_fail_reason = 0;
	int ret = 0;
	struct i2c_client *client = Touch_Get_I2C_Handle();

	pData->type = DATA_UNKNOWN;
	pData->count = 0;

	/* ftk read Interrupt Status */
	fts_read_reg(client, FT_INT_STATUS, &packet_type);

	/* Event handler */
	if( packet_type == 0x01 )   /* Touch event */
	{
		FT8707_I2C_Read(client, buf, 1, buf, POINT_READ_BUF);

	    num_finger = buf[FT_TOUCH_POINT_NUM];
	    if(num_finger > 10 || num_finger < 0) {
		TOUCH_LOG("num_finger under min or over max : %d \n", num_finger);
		goto ERROR;
	    }

		for( i = 0 ; i < fts_updateinfo_curr.TPD_MAX_POINTS ; i++ )
		{
			pointid = (buf[FTS_TOUCH_ID_POS + (FTS_TOUCH_STEP * i)]) >> 4;

			if (pointid >= FTS_MAX_ID){
				break;
			}

			state = (buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] & 0xC0) >> 6;

			if(num_finger = 0 && (state == FTK_TOUCH_DOWN || state == FTK_TOUCH_CONTACT)) {
				TOUCH_LOG("Finger Number 0 but Event is Contact \n");
				goto ERROR;
			}

			pData->type = DATA_FINGER;

			if( state == FTK_TOUCH_DOWN || state == FTK_TOUCH_CONTACT) {
				pFingerData = &pData->fingerData[pointid];
				pFingerData->id = pointid;
				pFingerData->x  = (u16) (buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] & 0x0F) << 8 | (u16) buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
				pFingerData->y = (u16) (buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] & 0x0F) << 8 | (u16) buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
				pFingerData->width_major = (buf[FTS_TOUCH_MISC + FTS_TOUCH_STEP * i]) >> 4;
				pFingerData->width_minor = 0;
				pFingerData->orientation = 0;
				pFingerData->pressure = (buf[FTS_TOUCH_XY_POS + FTS_TOUCH_STEP * i]);//cannot constant value

				if(pFingerData->width_major <= 0) {
					pFingerData->width_major = 1;
				}
#if 0 /* for debug */
				TOUCH_LOG("state = 0x%02x id = %d, x = %d, y = %d, width_major = %d, pressure = %d \n", state, pFingerData->id, pFingerData->x, pFingerData->y, pFingerData->width_major, pFingerData->pressure);
#endif
				if( pFingerData->pressure < 1 )
					pFingerData->pressure = 1;
				else if( pFingerData->pressure > 255 - 1 )
					pFingerData->pressure = 255 - 1;

				if((buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] & 0x30) >> 4 == 0x01) { /* Palm Detect */
					TOUCH_LOG("Palm = %d \n", pFingerData->id);
					pFingerData->pressure = 255;
				}

				pData->count++;
				pFingerData->status = FINGER_PRESSED;

				if(Touch_Quick_Cover_Closed){
					if(pFingerData->x<790){
						pFingerData->status = FINGER_UNUSED;
					}
				}

			} else if( state == FTK_TOUCH_UP || state == FTK_TOUCH_NO_EVENT){
				if((buf[FTS_TOUCH_EVENT_POS + FTS_TOUCH_STEP * i] & 0x30) >> 4 == 0x01) { /* Release Palm */
						TOUCH_LOG("Palm Release= %d \n", i);
						memset(pData, 0x0, sizeof(TouchReadData));
						pData->type = DATA_FINGER;
						pData->count = 0;
						for(k = 0; k < 10; k++) { /* for clear reported data */
							pFingerData = &pData->fingerData[k];
					 	pFingerData->id = k;
						}
						break;
				} else {
					pFingerData = &pData->fingerData[pointid];
					pFingerData->id = pointid;
					pFingerData->status = FINGER_RELEASED;
				}
#if 0
				pFingerData->x  = (u16) (buf[FTS_TOUCH_X_H_POS + FTS_TOUCH_STEP * i] & 0x0F) << 8 | (u16) buf[FTS_TOUCH_X_L_POS + FTS_TOUCH_STEP * i];
				pFingerData->y = (u16) (buf[FTS_TOUCH_Y_H_POS + FTS_TOUCH_STEP * i] & 0x0F) << 8 | (u16) buf[FTS_TOUCH_Y_L_POS + FTS_TOUCH_STEP * i];
				TOUCH_LOG("releas= 0x%02x, id = %d, x = %d , y = %d \n", state, pFingerData->id, pFingerData->x, pFingerData->y);
#endif
			}

		}
	}
	else if( packet_type == 0x02 ) {  /* Knock On event */
		TOUCH_LOG("Double TAP \n");

		/* Read Tap Count */
		wbuf[0] = FT_LPWG_COORDINATE;
		ret = FT8707_I2C_Read(client, wbuf, 1, rbuf, 2);
		TOUCH_LOG("TAP CNT = %d, %d \n", rbuf[1], rbuf[0]);
		tap_cnt = rbuf[1];

		/* Read X/Y COORDINATE */
		ret = FT8707_I2C_Read(client, wbuf, 1, rbuf, 2 + (tap_cnt * FT_LPWG_COORDINATE_STEP));
		TOUCH_LOG("2nd TAP CNT = %d, %d \n", rbuf[1], rbuf[0]);


		for( i = 0 ; i < tap_cnt ; i++ ) {
			u8 *tmp = &rbuf[i];
			pData->knockData[i].x = (u16) (rbuf[FT_LPWG_COORDINATE_X_H + FT_LPWG_COORDINATE_STEP * i] & 0xFF) << 8 | (u16) rbuf[FT_LPWG_COORDINATE_X_L + FT_LPWG_COORDINATE_STEP * i];
			pData->knockData[i].y = (u16) (rbuf[FT_LPWG_COORDINATE_Y_H + FT_LPWG_COORDINATE_STEP * i] & 0xFF) << 8 | (u16) rbuf[FT_LPWG_COORDINATE_Y_L + FT_LPWG_COORDINATE_STEP * i];
			pData->count++;
			TOUCH_LOG("TCI1[%d] x = %d, y = %d \n", i, pData->knockData[i].x, pData->knockData[i].y);
		}

		TOUCH_LOG("Knock-on Detected\n");
		pData->type = DATA_KNOCK_ON;

	} else if( packet_type == 0x03 ) {  /* Knock Code event */
		TOUCH_LOG("Multi TAP. \n");

		/* Read Tap Count */
		wbuf[0] = FT_LPWG_COORDINATE;
		ret = FT8707_I2C_Read(client, wbuf, 1, rbuf, 2);
		TOUCH_LOG("TAP CNT = %d, %d \n", rbuf[1], rbuf[0]);
		tap_cnt = rbuf[1];

		/* Read X/Y COORDINATE */
		ret = FT8707_I2C_Read(client, wbuf, 1, rbuf, 2 + (tap_cnt * FT_LPWG_COORDINATE_STEP));
		TOUCH_LOG("2nd TAP CNT = %d, %d \n", rbuf[1], rbuf[0]);

		for( i = 0 ; i < tap_cnt ; i++ ) {
			u8 *tmp = &rbuf[i];
			pData->knockData[i].x = (u16) (rbuf[FT_LPWG_COORDINATE_X_H + FT_LPWG_COORDINATE_STEP * i] & 0xFF) << 8 | (u16) rbuf[FT_LPWG_COORDINATE_X_L + FT_LPWG_COORDINATE_STEP * i];
			pData->knockData[i].y = (u16) (rbuf[FT_LPWG_COORDINATE_Y_H + FT_LPWG_COORDINATE_STEP * i] & 0xFF) << 8 | (u16) rbuf[FT_LPWG_COORDINATE_Y_L + FT_LPWG_COORDINATE_STEP * i];
			pData->count++;
			TOUCH_LOG("TCI2[%d] x = %d, y = %d \n", i, pData->knockData[i].x, pData->knockData[i].y);
		}

		TOUCH_LOG("Knock-code Detected\n");
		pData->type = DATA_KNOCK_CODE;
	} else if( packet_type == 0x04 ) {  /*  LPWG Fail Reason */

		TOUCH_LOG("Real-Time Fail Reason. \n");

		/* Read RT Fail Reason */
		fts_read_reg(client, FT_LPWG_FAIL_REASON_REAL_TIME, &real_lpwg_fail_reason);
		if(ret < 0) {
			TOUCH_ERR("Fail to read lpwg_RT_fail_reason \n");
			goto ERROR;
		}

		/* Logging RT Fail Reason */
		FT8707_display_RT_fail_reason(real_lpwg_fail_reason);

	} else if( packet_type == 0x05 ) {  /* ESD */
		TOUCH_LOG("ESD Detected. \n");
		lcm_firmware_upgrade_recovery();
	} else {
		TOUCH_LOG("Unknown INT_STATUS = %d. \n", packet_type);
		goto ERROR;
	}

	return TOUCH_SUCCESS;

ERROR :
    FT8707_Reset(0, 10);
    FT8707_Reset(1, 150);
    return TOUCH_FAIL;
}

static int FT8707_ReadIcFirmwareInfo( TouchFirmwareInfo *pFwInfo)
{
	u8 wbuf[2] = {0, };
	u8 version[2] = {0, };
	u8 version_buf = 0;
	int ret = 0;
	u8 version_temp[2] = {0,}; /* [0] = 10pos, [1] = 1 pos*/

    struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	/* IMPLEMENT : read IC firmware information function */
	ret = fts_read_reg(client, FTS_REG_FW_VER, &version_buf);
	version[0] = (version_buf & 0x80) >> 7;
	version[1] = version_buf & 0x7F;

	if( ret == TOUCH_FAIL )
		return TOUCH_FAIL;

	/* Re Parse version for Unified 4 */
	version_temp[0] = (version[1] / 10) << 4;
	version_temp[1] = version[1] % 10;

	version[1] = version_temp[0] | version_temp[1];

	pFwInfo->moduleMakerID = 0;
	pFwInfo->moduleVersion = 0;
	pFwInfo->modelID = ft8707_chip_rev;
	pFwInfo->isOfficial = version[0];
	pFwInfo->version = version[1];

	ts->fw_ver[0] = version[0];
	ts->fw_ver[1] = version[1];

	TOUCH_LOG("FT8707 CHIP Rev = %s \n", (ft8707_chip_rev==FT_CHIP_CUT_1)?"CUT 1 !!":"CUT 2 !!");
	TOUCH_LOG("IC F/W Version = v%X.%02X ( %s )\n", version[0], version[1], pFwInfo->isOfficial ? "Official Release" : "Test Release");

	return TOUCH_SUCCESS;
}

static int FT8707_GetBinFirmwareInfo( char *pFilename, TouchFirmwareInfo *pFwInfo)
{
	int ret = 0;
	const struct firmware *fw = NULL;
	u8 version[2] = {0, };
	u8 *pFwFilename = NULL;
    struct i2c_client *client = Touch_Get_I2C_Handle();
	u8 version_temp[2] = {0,}; /* [0] = 10pos, [1] = 1 pos*/

	TOUCH_FUNC();

	if( pFilename == NULL ) {
		if( ft8707_chip_rev == FT_CHIP_CUT_1 )
			pFwFilename = (char *)defaultFirmware;
		else
			pFwFilename = (char *)defaultFirmware_cut2;
	} else {
		pFwFilename = pFilename;
	}

	TOUCH_LOG("Firmware filename = %s\n", pFwFilename);
	TOUCH_LOG("FT8707 CHIP Rev = %s \n", (ft8707_chip_rev==FT_CHIP_CUT_1)?"CUT 1 !!":"CUT 2 !!");

	/* Get firmware image buffer pointer from file */
	ret = request_firmware(&fw, pFwFilename, &client->dev);
	if( ret ) {
		TOUCH_ERR("failed at request_firmware() ( error = %d )\n", ret);
		return TOUCH_FAIL;
	}

	/* Firmware Bin Version */
	version[0] = (fw->data[0x2000+0x10e] & 0x80) >> 7;
	version[1] = fw->data[0x2000+0x10e] & 0x7F;

	/* Re Parse version for Unified 4 */
	version_temp[0] = (version[1] / 10) << 4;
	version_temp[1] = version[1] % 10;

	version[1] = version_temp[0] | version_temp[1];

	pFwInfo->moduleMakerID = 0;
	pFwInfo->moduleVersion = 0;
	pFwInfo->modelID = ft8707_chip_rev;
	pFwInfo->isOfficial = version[0] ;
	pFwInfo->version = version[1];

	/* Free firmware image buffer */
	release_firmware(fw);

	TOUCH_LOG("BIN F/W Version = v%X.%02X ( %s )\n", version[0], version[1], pFwInfo->isOfficial ? "Official Release" : "Test Release");

	return TOUCH_SUCCESS;
}

void FT8707_Get_DefaultFWName(char **app_name, char **boot_name)
{
	if( ft8707_chip_rev == FT_CHIP_CUT_1) {
		*app_name = defaultFirmware;
		*boot_name = defaultBootFirmware;
	} else {
		*app_name = defaultFirmware_cut2;
		*boot_name = defaultBootFirmware_cut2;
	}

	TOUCH_LOG("Default F/W Name Get Done. \n");
}

int FT8707_RequestFirmware(char *name_app_fw, char *name_boot_fw, const struct firmware **fw_app, const struct firmware **fw_boot)
{
	int ret = 0;
    struct i2c_client *client = Touch_Get_I2C_Handle();

	/* Get firmware image buffer pointer from file */
	if(name_app_fw != NULL)
			ret = request_firmware(fw_app, name_app_fw, &client->dev);

	if(ret < 0) {
		TOUCH_LOG("Request App F/W Fail \n");
		goto FW_RQ_FAIL;
	}

	/* Get Boot firmware image buffer pointer from file */
	if(name_boot_fw != NULL)
		ret = request_firmware(fw_boot, name_boot_fw, &client->dev);

	if(ret < 0) {
		TOUCH_LOG("Request Boot F/W Fail \n");
		goto FW_APP_FAIL;
	}

	return TOUCH_SUCCESS;

FW_APP_FAIL:
	release_firmware(fw_app);
FW_RQ_FAIL:
	return TOUCH_FAIL;

}

static int FT8707_UpdateFirmware( char *pFilename)
{
	int ret = 0;
	char *pFwFilename = NULL;
	char *pBootFwFilename = NULL;
	const struct firmware *fw = NULL;
	const struct firmware *bootFw = NULL;
    struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	if( pFilename == NULL ) {
		if( ft8707_chip_rev == FT_CHIP_CUT_1)
			pFwFilename = (char *)defaultFirmware;
		else
			pFwFilename = (char *)defaultFirmware_cut2;
	} else {
		pFwFilename = pFilename;
	}

	mdelay(500);

	if( pBootFwFilename == NULL ) {
		if( ft8707_chip_rev == FT_CHIP_CUT_1)
			pBootFwFilename = (char *)defaultBootFirmware;
		else
			pBootFwFilename = (char *)defaultBootFirmware_cut2;
	} else {
		TOUCH_LOG("Only Use Default Boot F/W \n");
	}
	TOUCH_LOG("FT8707 CHIP Rev = %s \n", (ft8707_chip_rev==FT_CHIP_CUT_1)?"CUT 1 !!":"CUT 2 !!");
	TOUCH_LOG("F/W Firmware = %s \n",pFwFilename);
	TOUCH_LOG("Boot F/W Firmware = %s\n", pBootFwFilename);

	/* request app, boot F/W */
	ret = FT8707_RequestFirmware(pFwFilename, pBootFwFilename, &fw, &bootFw);
	if(ret == TOUCH_FAIL) {
		TOUCH_ERR("Fail to Request App, Boot F/W Fail \n");
		return TOUCH_FAIL;
	}

	/* Do firmware upgrade */
	ret = firmwareUpgrade(fw,bootFw);
	if(ret < 0) {
		TOUCH_LOG("F/W Upgrade Fail. \n ");
		goto FW_UPDATE_FAIL;
	}

	/* Free firmware image buffer */
	release_firmware(bootFw);
	release_firmware(fw);

	return TOUCH_SUCCESS;

FW_UPDATE_FAIL:
	release_firmware(bootFw);
	release_firmware(fw);
	return TOUCH_FAIL;

}

static int FT8707_SetLpwgMode( TouchState newState, LpwgSetting  *pLpwgSetting)
{
	int ret = TOUCH_SUCCESS;
    struct i2c_client *client = Touch_Get_I2C_Handle();

	TOUCH_FUNC();

	memcpy(&ts->lpwgSetting, pLpwgSetting, sizeof(LpwgSetting));

	if( ts->currState == newState ) {
		TOUCH_LOG("device state is same as driver requested\n");
		return TOUCH_SUCCESS;
	}

	if( ( newState < STATE_NORMAL ) && ( newState > STATE_KNOCK_ON_CODE ) ) {
		TOUCH_LOG("invalid request state ( state = %d )\n", newState);
		return TOUCH_FAIL;
	}

	if(lge_get_mfts_mode()) { /*for mfts power off control*/
			TOUCH_LOG("set lpwg mode(mfts mode = %d) \n", mfts_lpwg);

	} else {
			TOUCH_LOG("set lpwg mode \n");
			ret = lpwg_control(client, newState);
	}
	if( ret == TOUCH_FAIL ) {
		TOUCH_ERR("failed to set lpwg mode in device\n");
		return TOUCH_FAIL;
	}

	if( ret == TOUCH_SUCCESS ) {
		ts->currState = newState;
	}

	switch( newState )
	{
		case STATE_NORMAL:
			TOUCH_LOG("device was set to NORMAL\n");
			lpwg_i2c_debug_enable = 0;
			break;
		case STATE_OFF:
			TOUCH_LOG("device was set to OFF\n");
			break;
		case STATE_KNOCK_ON_ONLY:
			TOUCH_LOG("device was set to KNOCK_ON_ONLY\n");
			break;
		case STATE_KNOCK_ON_CODE:
			TOUCH_LOG("device was set to KNOCK_ON_CODE\n");
			break;
		default:
			TOUCH_LOG("impossilbe state ( state = %d )\n", newState);
			ret = TOUCH_FAIL;
			break;
	}
	return TOUCH_SUCCESS;
}


static int FT8707_DoSelfDiagnosis(int* pRawStatus, int* pChannelStatus, char* pBuf, int bufSize, int* pDataLen)
{
	/* CAUTION : be careful not to exceed buffer size */
	char *sd_path = NULL;
	int ret = 0;
	int result = 0;
	int dataLen = 0;

	struct i2c_client *client = Touch_Get_I2C_Handle();
	memset(pBuf, 0, bufSize);
	*pDataLen = 0;
	*pRawStatus = TOUCH_SUCCESS;
	*pChannelStatus = TOUCH_SUCCESS;
	TOUCH_FUNC();
	/* CAUTION : be careful not to exceed buffer size */

	if ( ts->currState != STATE_NORMAL) {
		TOUCH_ERR("don't excute SD when LCD is off! \n");
		*pRawStatus = TOUCH_FAIL;
		return TOUCH_FAIL;
    }

	TOUCH_LOG("lge_get_mfts_mode() = %d\n", lge_get_mfts_mode());

	TOUCH_LOG("IC F/W Version = v%X.%02X\n", ts->fw_ver[0],ts->fw_ver[1]);
	ret = sprintf(pBuf + ret,"IC F/W Version = v%X.%02X\n", ts->fw_ver[0], ts->fw_ver[1]);

	FT8707_WriteFile(sd_path, pBuf, 1);

#if 1
	// Change clb switch
 ft8707_switch_cal(client, 1);
#endif

	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to Change Factory Mode. \n");
		return TOUCH_FAIL;
	}

	msleep(300);
	// raw data check
	ret = FT8707_GetTestResult(client, pBuf, pRawStatus, RAW_DATA_SHOW);
	if( ret < 0 ) {
		TOUCH_ERR("failed to get raw data\n");
		memset(pBuf, 0, bufSize);
		sprintf(pBuf,"Fail to Test RawData\n");
		*pRawStatus |= TOUCH_FAIL;
	}
	FT8707_WriteFile(sd_path, pBuf, 0);
	msleep(30);
	memset(pBuf, 0, bufSize);

	// ib check
	ret = FT8707_GetTestResult(client, pBuf, pRawStatus, IB_SHOW);
	if( ret < 0 ) {
		TOUCH_ERR("failed to get ib data\n");
		memset(pBuf, 0, bufSize);
		sprintf(pBuf,"Fail to Test IB\n");
		*pRawStatus |= TOUCH_FAIL;
	}

	FT8707_WriteFile(sd_path, pBuf, 0);
	msleep(30);
	memset(pBuf, 0, bufSize);

	// p2p noise check
	if(ft8707_chip_rev != FT_CHIP_CUT_2){
		TOUCH_ERR("Can't excute p2p noise test in CUT 1\n");
	}
	else{
		ret = FT8707_GetTestResult(client, pBuf, pRawStatus, P2P_NOISE_SHOW);
		if( ret < 0 ) {
			TOUCH_ERR("failed to get p2p noise data\n");
			memset(pBuf, 0, bufSize);
			sprintf(pBuf,"Fail to Test P2P Noise\n");
			*pRawStatus |= TOUCH_FAIL;
		}
		FT8707_WriteFile(sd_path, pBuf, 0);
		msleep(30);
		memset(pBuf, 0, bufSize);
	}

	// RMS noise check
	ret = FT8707_GetTestResult(client, pBuf, pRawStatus, NOISE_SHOW);
	if( ret < 0 ) {
		TOUCH_ERR("failed to get noise data\n");
		memset(pBuf, 0, bufSize);
		sprintf(pBuf,"Fail to Test RMS Noise\n");
		*pRawStatus |= TOUCH_FAIL;
	}

	FT8707_WriteFile(sd_path, pBuf, 0);
	msleep(30);
	memset(pBuf, 0, bufSize);

  FT8707_WriteFile(sd_path, pBuf, 1);
	log_file_size_check(&client->dev);

	*pDataLen = dataLen;

	result = FT8707_change_mode(client, FTS_WORK_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_LOG(" Fail to change work mode\n");
		return TOUCH_FAIL;
	}

	lpwg_i2c_debug_enable = 1;

	return TOUCH_SUCCESS;
}
static int FT8707_DoSelfDiagnosis_Lpwg(int* pLpwgStatus, char* pBuf, int bufSize, int* pDataLen)
{
	char *sd_path = NULL;
	int ret = 0;
	int result = 0;
	int dataLen = 0;

	struct i2c_client *client = Touch_Get_I2C_Handle();
	memset(pBuf, 0, bufSize);
	*pDataLen = 0;
	*pLpwgStatus = TOUCH_SUCCESS;
	TOUCH_FUNC();

	TOUCH_LOG("lge_get_mfts_mode() = %d\n", lge_get_mfts_mode());

	TOUCH_LOG("IC F/W Version = v%X.%02X\n", ts->fw_ver[0],ts->fw_ver[1]);
	ret = sprintf(pBuf + ret,"IC F/W Version = v%X.%02X\n", ts->fw_ver[0], ts->fw_ver[1]);

	FT8707_WriteFile(sd_path, pBuf, 1);

	if(ft8707_chip_rev == FT_CHIP_CUT_1) {
		TOUCH_LOG("CUT 1: Don't need to run TP reset");
	}
	else{
		TOUCH_LOG("CUT 2: TP reset SKIP");
		//FT8707_Reset(0,2);
		//FT8707_Reset(1,200);
	}

	#if 1
	// Change clb switch
	ft8707_switch_cal(client, 1);
	#endif

	result = FT8707_change_mode(client, FTS_FACTORY_MODE);
	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR("Fail to change mode [factory]\n");
		return TOUCH_FAIL;
}

	// lpwg raw data check
	ret = FT8707_GetTestResult(client, pBuf, pLpwgStatus, LPWG_RAW_DATA_SHOW);
	if( ret < 0 ) {
		TOUCH_ERR("failed to get lpwg raw data\n");
		memset(pBuf, 0, bufSize);
		sprintf(pBuf,"Fail to Test LPWG RAWDATA \n");
		*pLpwgStatus |= TOUCH_FAIL;
	}
	FT8707_WriteFile(sd_path, pBuf, 0);
	msleep(30);
	memset(pBuf, 0, bufSize);

	// lpwg ib check
	ret = FT8707_GetTestResult(client, pBuf, pLpwgStatus, LPWG_IB_SHOW);
	if( ret < 0 ) {
		TOUCH_ERR("failed to get lpwg ib data\n");
		memset(pBuf, 0, bufSize);
		sprintf(pBuf,"Fail to Test LPWG IB \n");
		*pLpwgStatus |= TOUCH_FAIL;
	}
	FT8707_WriteFile(sd_path, pBuf, 0);
	msleep(30);
	memset(pBuf, 0, bufSize);

	// short check
	if(ft8707_chip_rev != FT_CHIP_CUT_2){
		TOUCH_ERR("Can't excute short test in CUT 1\n");
	}
	else{
		ret = FT8707_GetTestResult(client, pBuf, pLpwgStatus, SHORT_SHOW);
		if( ret < 0 ) {
			TOUCH_ERR("failed to get short data\n");
			memset(pBuf, 0, bufSize);
			sprintf(pBuf,"Fail to Test LPWG SHORT \n");
			*pLpwgStatus |= TOUCH_FAIL;
		}
		FT8707_WriteFile(sd_path, pBuf, 0);
		msleep(30);
		memset(pBuf, 0, bufSize);
	}

	// lpwg p2p noise check
	if(ft8707_chip_rev != FT_CHIP_CUT_2){
		TOUCH_ERR("Can't excute p2p noise test in CUT 1\n");
	}
	else{
		ret = FT8707_GetTestResult(client, pBuf, pLpwgStatus, LPWG_P2P_NOISE_SHOW);
		if( ret < 0 ) {
			TOUCH_ERR("failed to get p2p noise data\n");
			memset(pBuf, 0, bufSize);
			sprintf(pBuf,"Fail to Test LPWG P2P NOISE \n");
			*pLpwgStatus |= TOUCH_FAIL;
		}
		FT8707_WriteFile(sd_path, pBuf, 0);
		msleep(30);
		memset(pBuf, 0, bufSize);
	}

	// lpwg noise check
	ret = FT8707_GetTestResult(client, pBuf, pLpwgStatus, LPWG_NOISE_SHOW);
	if( ret < 0 ) {
		TOUCH_ERR("failed to get lpwg noise data\n");
		memset(pBuf, 0, bufSize);
		sprintf(pBuf,"Fail to Test LPWG RMS NOISE \n");
		*pLpwgStatus |= TOUCH_FAIL;
	}

	FT8707_WriteFile(sd_path, pBuf, 0);
	msleep(30);
	memset(pBuf, 0, bufSize);

	FT8707_WriteFile(sd_path, pBuf, 1);
	log_file_size_check(&client->dev);

	*pDataLen = dataLen;

	result = FT8707_change_mode(client, FTS_WORK_MODE);

	if(result != TOUCH_SUCCESS) {
		TOUCH_ERR(" Fail to change working mode \n");
		return TOUCH_FAIL;
	}

	return TOUCH_SUCCESS;
}

static void FT8707_PowerOn(int isOn)
{

}

static void FT8707_ClearInterrupt(void)
{

}

static void FT8707_NotifyHandler(TouchNotify notify, int data)
{

    if(CORNER_COVER_USE){
      if(notify==NOTIFY_Q_COVER){
        cover_status = data;
        if(ts!=NULL&&ts->currState!=STATE_BOOT)
			TOUCH_LOG("[%s] Q Cover Mode start \n", __func__);
			set_deepsleep_state_ft8707(1,0); /*LCD Deep Sleep OUT*/
            queue_delayed_work(touch_wq, &work_cover, msecs_to_jiffies(1));
        }
    }

}

TouchDeviceControlFunction FT8707_Func = {
    .Power                  = FT8707_PowerOn,
	.Initialize 			= FT8707_Initialize,
	.Reset 					= FT8707_Reset_Dummy,
	.InitRegister 			= FT8707_InitRegister,
	.ClearInterrupt         = FT8707_ClearInterrupt,
	.InterruptHandler 		= FT8707_InterruptHandler,
	.ReadIcFirmwareInfo 	= FT8707_ReadIcFirmwareInfo,
	.GetBinFirmwareInfo 	= FT8707_GetBinFirmwareInfo,
	.UpdateFirmware 		= FT8707_UpdateFirmware,
	.SetLpwgMode 			= FT8707_SetLpwgMode,
	.DoSelfDiagnosis 		= FT8707_DoSelfDiagnosis,
	.DoSelfDiagnosis_Lpwg	= FT8707_DoSelfDiagnosis_Lpwg,
	.device_attribute_list 	= FT8707_attribute_list,
	.NotifyHandler          = FT8707_NotifyHandler,
};

/* End Of File */
