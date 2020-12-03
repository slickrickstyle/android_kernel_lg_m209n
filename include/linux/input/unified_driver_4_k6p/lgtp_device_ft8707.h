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
 *    File  	: lgtp_device_s3320.h
 *    Author(s)   : D3 BSP Touch Team < d3-bsp-touch@lge.com >
 *    Description :
 *
 ***************************************************************************/

#if !defined ( _LGTP_DEVICE_FT8707_H_ )
#define _LGTP_DEVICE_FT8707_H_

/****************************************************************************
* Nested Include Files
****************************************************************************/
#include <linux/input/unified_driver_4/lgtp_common.h>
#include <linux/syscalls.h>


/****************************************************************************
* Mainfest Constants / Defines
****************************************************************************/


/****************************************************************************
* Type Definitions
****************************************************************************/


/****************************************************************************
* Exported Variables
****************************************************************************/


/****************************************************************************
* Macros
****************************************************************************/
enum {
	IME_OFF = 0,
	IME_ON,
	IME_SWYPE,
};

/****************************************************************************
* Global Function Prototypes
****************************************************************************/

#endif /* _LGTP_DEVICE_MIT200_H_ */

/* End Of File */


/************************ FTK Header ************************/

#if 1  /*FTK Core.h*/

/*
 *
 * FocalTech TouchScreen driver.
 *
 * Copyright (c) 2010-2015, Focaltech Ltd. All rights reserved.
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
 */

#ifndef __LINUX_FTXXXX_H__
#define __LINUX_FTXXXX_H__
 /*******************************************************************************
*
* File Name: focaltech_core.h
*
*    Author: Xu YongFeng
*
*   Created: 2015-01-29
*
*  Abstract:
*
* Reference:
*
*******************************************************************************/

/*******************************************************************************
* 1.Included header files
*******************************************************************************/

/*******************************************************************************
* Private constant and macro definitions using #define
*******************************************************************************/
/*Number of channel*/
#define MAX_ROW								30
#define MAX_COL								18

/**********************Custom define begin**********************************************/


#if (LINUX_VERSION_CODE > KERNEL_VERSION(3, 8, 0))
	#if defined(MODULE) || defined(CONFIG_HOTPLUG)
		#define __devexit_p(x) 				x
	#else
		#define __devexit_p(x) 				NULL
	#endif
	// Used for HOTPLUG
	#define __devinit        					__section(.devinit.text) __cold notrace
	#define __devinitdata    					__section(.devinit.data)
	#define __devinitconst   					__section(.devinit.rodata)
	#define __devexit        					__section(.devexit.text) __exitused __cold notrace
	#define __devexitdata    					__section(.devexit.data)
	#define __devexitconst   					__section(.devexit.rodata)
#endif

#define MAX_NUM_OF_FINGERS					10
#define KNOCKON_DELAY   					700

/* log file size */
#define MAX_LOG_FILE_SIZE	(10 * 1024 * 1024) /* 10 M byte */
#define MAX_LOG_FILE_COUNT	(4)


#define TPD_POWER_SOURCE_CUSTOM         	MT6323_POWER_LDO_VGP1
#define IIC_PORT                   					0				// MT6572: 1  MT6589:0 , Based on the I2C index you choose for TPM
#define TPD_HAVE_BUTTON									// if have virtual key,need define the MACRO
#define TPD_BUTTON_HEIGH        				(40)  			// 100
#define TPD_KEY_COUNT           				3    				// 4
#define TPD_KEYS                					{ KEY_MENU, KEY_HOMEPAGE, KEY_BACK}
#define TPD_KEYS_DIM            					{{80,900,20,TPD_BUTTON_HEIGH}, {240,900,20,TPD_BUTTON_HEIGH}, {400,900,20,TPD_BUTTON_HEIGH}}
#define FT_ESD_PROTECT  									0
/*********************Custom Define end*************************************************/
#define MT_PROTOCOL_B
#define A_TYPE												0
#define TPD_TYPE_CAPACITIVE
#define TPD_TYPE_RESISTIVE
#define TPD_POWER_SOURCE
#define TPD_NAME    							"FTS"
#define TPD_I2C_NUMBER           				0
#define TPD_WAKEUP_TRIAL         				60
#define TPD_WAKEUP_DELAY         				100
#define TPD_VELOCITY_CUSTOM_X 				15
#define TPD_VELOCITY_CUSTOM_Y 				20

#define CFG_MAX_TOUCH_POINTS				10 //5
#define MT_MAX_TOUCH_POINTS				10
#define FTS_MAX_ID							0x0F
#define FTS_TOUCH_STEP						6
#define FTS_FACE_DETECT_POS				1
#define FTS_TOUCH_X_H_POS					3
#define FTS_TOUCH_X_L_POS					4
#define FTS_TOUCH_Y_H_POS					5
#define FTS_TOUCH_Y_L_POS					6
#define FTS_TOUCH_EVENT_POS				3
#define FTS_TOUCH_ID_POS					5
#define FT_TOUCH_POINT_NUM				2
#define FTS_TOUCH_XY_POS					7
#define FTS_TOUCH_MISC						8
#define POINT_READ_BUF						(3 + FTS_TOUCH_STEP * CFG_MAX_TOUCH_POINTS)
#define FT_FW_NAME_MAX_LEN				50
#define TPD_DELAY                					(2*HZ/100)
#define TPD_RES_X                					1080//480
#define TPD_RES_Y                					1280//800
#define TPD_CALIBRATION_MATRIX  			{962,0,0,0,1600,0,0,0};
#define FT_PROXIMITY_ENABLE				0
//#define TPD_AUTO_UPGRADE
//#define TPD_HAVE_CALIBRATION
//#define TPD_HAVE_TREMBLE_ELIMINATION
//#define TPD_CLOSE_POWER_IN_SLEEP
/******************************************************************************/
/* Chip Device Type */
#define IC_FT5X06							0				/* x=2,3,4 */
#define IC_FT5606							1				/* ft5506/FT5606/FT5816 */
#define IC_FT5316							2				/* ft5x16 */
#define IC_FT6208							3	  			/* ft6208 */
#define IC_FT6x06     							4				/* ft6206/FT6306 */
#define IC_FT5x06i     						5				/* ft5306i */
#define IC_FT5x36     							6				/* ft5336/ft5436/FT5436i */



/*register address*/
#define FTS_REG_CHIP_ID						0xA3    			// chip ID
#define FTS_REG_FW_VER						0xA6   			// FW  version
#define FTS_REG_VENDOR_ID					0xA8   			// TP vendor ID
#define FTS_REG_POINT_RATE					0x88   			// report rate
#define TPD_MAX_POINTS_2                        		2
#define TPD_MAX_POINTS_5                        		5
#define TPD_MAX_POINTS_10                        	10
#define AUTO_CLB_NEED                           		1
#define AUTO_CLB_NONEED                         		0
#define LEN_FLASH_ECC_MAX 					0xFFFE
#define FTS_PACKET_LENGTH        				32 //120
#define FTS_GESTRUE_POINTS 				255
#define FTS_GESTRUE_POINTS_ONETIME  		62
#define FTS_GESTRUE_POINTS_HEADER 		8
#define FTS_GESTURE_OUTPUT_ADRESS 		0xD3
#define FTS_GESTURE_OUTPUT_UNIT_LENGTH 	4

#define KEY_GESTURE_U 						KEY_U
#define KEY_GESTURE_UP 						KEY_UP
#define KEY_GESTURE_DOWN 					KEY_DOWN
#define KEY_GESTURE_LEFT 					KEY_LEFT
#define KEY_GESTURE_RIGHT 					KEY_RIGHT
#define KEY_GESTURE_O 						KEY_O
#define KEY_GESTURE_E 						KEY_E
#define KEY_GESTURE_M 						KEY_M
#define KEY_GESTURE_L 						KEY_L
#define KEY_GESTURE_W 						KEY_W
#define KEY_GESTURE_S 						KEY_S
#define KEY_GESTURE_V 						KEY_V
#define KEY_GESTURE_Z 						KEY_Z

#define GESTURE_LEFT						0x20
#define GESTURE_RIGHT						0x21
#define GESTURE_UP		    					0x22
#define GESTURE_DOWN						0x23
#define GESTURE_DOUBLECLICK				0x24
#define GESTURE_O		    					0x30
#define GESTURE_W		    					0x31
#define GESTURE_M		    					0x32
#define GESTURE_E		    					0x33
#define GESTURE_L		    					0x44
#define GESTURE_S		    					0x46
#define GESTURE_V		    					0x54
#define GESTURE_Z		    					0x41


//TOUCH
#define FTK_TOUCH_DOWN	0x00
#define FTK_TOUCH_UP	0x01
#define FTK_TOUCH_CONTACT	0x02
#define FTK_TOUCH_NO_EVENT	0x03

/* LPWG Setting */
/* ADDR */
#define FT_LPWG_COORDINATE_STEP	0x04
#define FT_LPWG_COORDINATE_X_H	0x02
#define FT_LPWG_COORDINATE_X_L	0x03
#define FT_LPWG_COORDINATE_Y_H	0x04
#define FT_LPWG_COORDINATE_Y_L	0x05

#define	FT_VIEW_COVER	0xC0
#define FT_INT_STATUS	0x01
#define FT_ACTIVE_AREA	0xD4
#define FT_LPWG_START	0xD0
#define FT_LPWG_COORDINATE	0xD3
#define FT_LPWG_SENSITIVITY	0x80
#define FT_LPWG_HZ	0x89
#define FT_IME_MODE	0x90
#define FT_ACTIVE_AREA_X1_L	0xB4
#define FT_ACTIVE_AREA_X1_H	0xB5
#define FT_ACTIVE_AREA_X2_L	0xB6
#define FT_ACTIVE_AREA_X2_H	0xB7
#define FT_ACTIVE_AREA_Y1_L	0xB8
#define FT_ACTIVE_AREA_Y1_H	0xB9
#define FT_ACTIVE_AREA_Y2_L	0xBA
#define FT_ACTIVE_AREA_Y2_H	0xBB
#define FT_TCI1_SLOP	0xBC
#define FT_TCI2_SLOP	0xBD
#define FT_TCI1_DISTANCE	0xC4
#define FT_TCI2_DISTANCE	0xC5
#define FT_TCI1_INTER_TAP_TIME	0xC6
#define FT_TCI2_INTER_TAP_TIME	0xC7
#define FT_TCI1_TOTAL_COUNT	0xCA
#define FT_TCI2_TOTAL_COUNT	0xCB
#define FT_TCI1_INT_DELAY_TIME	0xCC
#define FT_TCI2_INT_DELAY_TIME	0xCD
#define FT_LPWG_FAIL_REASON_ENABLE	0xE5
#define FT_LPWG_FAIL_REASON_REAL_TIME	0xE6
#define FT_DEEP_SLEEP	0xA5

#define FT_LPWG_NON_FAIL_REASON_TCI_1	0xE7
#define FT_LPWG_NON_FAIL_REASON_TCI_2	0xE9

/* Need to add NON-Real Time Fail Reason*/

/* VAL */
#define	FT_DOUBLE_TAP_ONLY	0x01
#define	FT_LPWG_ON	0x03
#define	FT_LPWG_OFF	0x00

#define FT_LPWG_FAIL_REASON_ALL	0x11
#define FT_LPWG_FAIL_REASON_REAL	0x01
#define FT_LPWG_FAIL_REASON_NONREAL	0x10
#define FT_LPWG_FAIL_REASON_OFF	0x00

/****************************************************************************
* Diagnostic Macros
****************************************************************************/
#define FTS_MODE_CHANGE_LOOP	20
#define FTS_WORK_MODE		0x00
#define FTS_FACTORY_MODE	0x40
#define TEST_PACKET_LENGTH	255//342
#define FTS_LPWG_I2C_EN 0xF6
/*******************************************************************************
* Private enumerations, structures and unions using typedef
*******************************************************************************/
/* IC info */
struct fts_Upgrade_Info
{
        u8 CHIP_ID;
        u8 TPD_MAX_POINTS;
        u8 AUTO_CLB;
	 u16 delay_aa;			/* delay of write FT_UPGRADE_AA */
	 u16 delay_55;			/* delay of write FT_UPGRADE_55 */
	 u8 upgrade_id_1;		/* upgrade id 1 */
	 u8 upgrade_id_2;		/* upgrade id 2 */
	 u16 delay_readid;		/* delay of read id */
	 u16 delay_earse_flash; 	/* delay of earse flash */
};

/*touch event info*/
struct ts_event
{
	u16 au16_x[CFG_MAX_TOUCH_POINTS];				/* x coordinate */
	u16 au16_y[CFG_MAX_TOUCH_POINTS];				/* y coordinate */
	u8 au8_touch_event[CFG_MAX_TOUCH_POINTS];		/* touch event: 0 -- down; 1-- up; 2 -- contact */
	u8 au8_finger_id[CFG_MAX_TOUCH_POINTS];			/* touch ID */
	u16 pressure[CFG_MAX_TOUCH_POINTS];
	u16 area[CFG_MAX_TOUCH_POINTS];
	u8 touch_point;
	int touchs;
	u8 touch_point_num;
};

struct fts_ts_data {
	struct i2c_client	*client;
	TouchState currState;
	LpwgSetting lpwgSetting;
	struct class *class;
	dev_t mip_dev;
	struct cdev cdev;
	u8 *dev_fs_buf;
	u8 fw_ver[3];
	u8 fw_vendor_id;
};

struct lpwg_global_config_dump {
	u8 read_buf[50];
};

struct lpwg_tci_1_config_dump {
	u8 read_buf[50];
};

struct lpwg_tci_2_dump {
	u8 read_buf[50];
};


struct register_dump {
	struct lpwg_global_config_dump lpwg_global_dump;
	struct lpwg_tci_1_config_dump lpwg_tci_1_dump;
	struct lpwg_tci_2_dump lpwg_tci_2_dump;
};


//Firmware update error code
enum fw_update_errno{
	fw_err_file_read = -4,
	fw_err_file_open = -3,
	fw_err_file_type = -2,
	fw_err_download = -1,
	fw_err_none = 0,
	fw_err_uptodate = 1,
};

enum {
	RAW_DATA_SHOW	= 0,
	IB_SHOW,
	NOISE_SHOW,
	P2P_NOISE_SHOW,
	LPWG_RAW_DATA_SHOW,
	LPWG_IB_SHOW,
	SHORT_SHOW,
	LPWG_P2P_NOISE_SHOW,
	LPWG_NOISE_SHOW,
	DELTA_SHOW,
	INTENSITY_SHOW,
	ABS_SHOW,
	JITTER_SHOW,
	OPENSHORT_SHOW,
	MUXSHORT_SHOW,
	LPWG_JITTER_SHOW,
	LPWG_ABS_SHOW,
};

enum {
	LPWG_NO_FAIL = 0,
	DISTANCE_INTER_TAP,
	TOUCH_SLOP,
	TIME_OUT_INTER_TAP,
	MULTI_FINGER,
	INTERRUPT_DELAY_TIME_FAIL,
	PALM_DETECT
};

enum {
	LPWG_FR_OFF = 0,
	LPWG_FR_NONRT,
	LPWG_FR_RT,
	LPWG_FR_ALL
};

enum {
	FT_CHIP_CUT_1 = 0,
	FT_CHIP_CUT_2
};

enum {
    TA_OUT = 0,
    TA_IN
};

#define CHARGER_UNKNOWN	0
#define STANDARD_HOST	1
#define CHARGING_HOST   2
#define	NONSTANDARD_CHARGER	3
#define STANDARD_CHARGER    4
#define APPLE_2_1A_CHARGER  5
#define APPLE_1_0A_CHARGER  6
#define APPLE_0_5A_CHARGER  7
#define USB_TA_DISCONNECTED 0xFF

/*******************************************************************************
* Static variables
*******************************************************************************/




/*******************************************************************************
* Global variable or extern global variabls/functions
*******************************************************************************/
// Function Switchs: define to open,  comment to close
#define FTS_GESTRUE_EN 							0
#define MTK_EN 									1
#define FTS_APK_DEBUG
#define FT_TP									0

extern struct fts_Upgrade_Info fts_updateinfo_curr;

int fts_rw_iic_drv_init(struct i2c_client *client);
void  fts_rw_iic_drv_exit(void);
void fts_get_upgrade_array(struct i2c_client *client);

#if FTS_GESTRUE_EN
		extern int fts_Gesture_init(struct input_dev *input_dev);
		extern int fts_read_Gestruedata(void);
#endif

extern int fts_ctpm_fw_upgrade_with_app_file(struct i2c_client *client,
				       char *firmware_name);
extern int fts_ctpm_auto_clb(struct i2c_client *client);
extern int fts_ctpm_fw_upgrade_with_i_file(struct i2c_client *client, u8 *fw_buf, int len, u8 *bootfw_buf, int boot_len);
extern int fts_ctpm_get_i_file_ver(void);
extern int fts_remove_sysfs(struct i2c_client *client);
extern void fts_release_apk_debug_channel(void);
extern int fts_ctpm_auto_upgrade(struct i2c_client *client, u8 *fw_buf, int len, u8 *bootfw_buf, int boot_len);
#if FT_ESD_PROTECT
extern void esd_switch(s32 on);
#endif
extern void fts_reset_tp(int HighOrLow);
extern int fts_create_sysfs(struct i2c_client *client, TouchDriverData *pDriverData);
// Apk and ADB functions
extern int fts_create_apk_debug_channel(struct i2c_client * client);

int FT8707_RequestFirmware(char *name_app_fw, char *name_boot_fw, const struct firmware **fw_app, const struct firmware **fw_boot);
void FT8707_Get_DefaultFWName(char **app_name, char **boot_name);
int FT8707_set_i2c_mode(int mode);

extern ssize_t FT8707_GetTestResult(struct i2c_client *client, char *pBuf, int *result, int type);
extern void log_file_size_check(struct device *dev);
extern void FT8707_WriteFile(char *filename, char *data, int time);
extern void FT8707_Reset(int status, int delay);
extern int FT8707_change_mode(struct i2c_client *client, const u8 mode);

/*******************************************************************************
* Static function prototypes
*******************************************************************************/

#define FTS_DBG
#ifdef FTS_DBG
	#define FTS_DBG(fmt, args...) 				printk("[FTS]" fmt, ## args)
#else
	#define FTS_DBG(fmt, args...) 				do{}while(0)
#endif
#endif

#endif /* FTK Core.h End */

