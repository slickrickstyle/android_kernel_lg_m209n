//EEPROM MAP & CheckSum for HI-841, Camera-Driver@lge.com, 2015-06-11
#include <linux/module.h>
#include <linux/of_gpio.h>
#include <linux/delay.h>
#include <linux/crc32.h>
#include "msm_sd.h"
#include "msm_cci.h"
#include "msm_eeprom.h"

typedef enum LGIT_8MP_EEPROM_MAP {
	//T4KA3 (LGIT) EEPROM MAP
	//Little-Endian: LSB[7:0] MSB[15:8]
	AWB_5100K_START_ADDR       = 0x0000,
	AWB_5100K_END_ADDR         = 0x0005,
	AWB_5100K_CHECKSUM_LSB     = 0x0006,
	AWB_5100K_CHECKSUM_MSB     = 0x0007,

	AWB_3000K_START_ADDR       = 0x0382,
	AWB_3000K_END_ADDR         = 0x0387,
	AWB_3000K_CHECKSUM_LSB     = 0x0388,
	AWB_3000K_CHECKSUM_MSB     = 0x0389,

	LSC_5100K_START_ADDR       = 0x000C,
	LSC_5100K_END_ADDR         = 0x037F,
	LSC_5100K_CHECKSUM_LSB     = 0x0380,
	LSC_5100K_CHECKSUM_MSB     = 0x0381,

	LSC_4000K_START_ADDR       = 0x038A,
	LSC_4000K_END_ADDR         = 0x06FD,
	LSC_4000K_CHECKSUM_LSB     = 0x06FE,
	LSC_4000K_CHECKSUM_MSB     = 0x06FF,

	LSC_3000K_START_ADDR       = 0x0A00,
	LSC_3000K_END_ADDR         = 0x0D73,
	LSC_3000K_CHECKSUM_LSB     = 0x0D74,
	LSC_3000K_CHECKSUM_MSB     = 0x0D75,

	//TOTAL CHECKSUM (Little-Endian)
	DATA_START_ADDR            = 0x0000,
	DATA_END_ADDR              = 0x07FB,
	TOTAL_CHECKSUM_START_ADDR  = 0x07FC,
	TOTAL_CHECKSUM_END_ADDR    = 0x07FF,

	//META INFO
	MODULE_VENDOR_ID_ADDR     = 0x0700,
	EEPROM_VERSION_ADDR       = 0x0770,
} MAP_LGIT;




int32_t msm_eeprom_checksum_lgit_hi842(struct msm_eeprom_ctrl_t *e_ctrl) {
	uint16_t awb_5k = 0, awb_5k_checksum = 0;
	uint16_t lsc_5k = 0, lsc_5k_checksum = 0;
	uint16_t sensor_id = 0, sensor_id_checksum = 0;
	int32_t rc = -EFAULT;
	int i;

	for(i = 0x0; i < 0x6; i++) {
		awb_5k += e_ctrl->cal_data.mapdata[i];
	}
	awb_5k_checksum = (e_ctrl->cal_data.mapdata[0x7] << 8) |
		e_ctrl->cal_data.mapdata[0x6];

	for(i = 0xC; i < 0x380; i++) {
		lsc_5k += e_ctrl->cal_data.mapdata[i];
	}
	lsc_5k_checksum = (e_ctrl->cal_data.mapdata[0x381] << 8) |
		e_ctrl->cal_data.mapdata[0x380];

	for(i = 0x710; i < 0x71A; i++) {
		sensor_id += e_ctrl->cal_data.mapdata[i];
	}
	sensor_id_checksum = (e_ctrl->cal_data.mapdata[0x71B] << 8) |
		e_ctrl->cal_data.mapdata[0x71A];

	if( awb_5k == awb_5k_checksum &&
		lsc_5k == lsc_5k_checksum &&
		sensor_id == sensor_id_checksum) {
		pr_info("EEPROM data verified\n");
		rc = 0;
	} else {
		pr_err("awb_5k = 0x%x, awb_5k_checksum = 0x%x, "
			"lsc_5k = 0x%x, lsc_5k_checksum = 0x%x, "
			"sensor_id = 0x%x, sensor_id_checksum = 0x%x\n",
			awb_5k, awb_5k_checksum, lsc_5k, lsc_5k_checksum,
			sensor_id, sensor_id_checksum);
	}
	return rc;
}


int32_t msm_eeprom_checksum_lgit_hi553(struct msm_eeprom_ctrl_t *e_ctrl) {
	int32_t rc = -EFAULT;

	uint32_t dataSum_AWB_5K = 0;
	uint32_t dataSum_LSC_5K = 0;

	uint32_t checkSum_AWB_5K;
	uint32_t checkSum_LSC_5K;

	//Add: DataSum
	dataSum_AWB_5K = accumulation(e_ctrl, AWB_5100K_START_ADDR, AWB_5100K_END_ADDR);
	dataSum_LSC_5K = accumulation(e_ctrl, LSC_5100K_START_ADDR, LSC_5100K_END_ADDR);

	dataSum_AWB_5K &= 0x0000FFFF;
	dataSum_LSC_5K &= 0x0000FFFF;

	//Get: CheckSum
	checkSum_AWB_5K = shiftedSum(e_ctrl, AWB_5100K_CHECKSUM_LSB, AWB_5100K_CHECKSUM_MSB, LittleEndian);
	checkSum_LSC_5K = shiftedSum(e_ctrl, LSC_5100K_CHECKSUM_LSB, LSC_5100K_CHECKSUM_MSB, LittleEndian);

	pr_err("[CHECK][BEFORE] e_ctrl->is_supported: %X\n", e_ctrl->is_supported);

	//Check#1: AWB 5K Part
	if (dataSum_AWB_5K == checkSum_AWB_5K) {
		pr_err("[CHECK] AWB 5K Data Valid\n");
		e_ctrl->is_supported |= 0x10;
	}
	else {
		pr_err("[CHECK][FAIL] dataSum_AWB_5K = %d, checkSum_AWB_5K = %d\n",
				dataSum_AWB_5K, checkSum_AWB_5K);
	}

	//Check#3: LSC 5K Part
	if (dataSum_LSC_5K == checkSum_LSC_5K) {
		pr_err("[CHECK] LSC 5K Data Valid\n");
		e_ctrl->is_supported |= 0x20;
	}
	else {
		pr_err("[CHECK][FAIL] dataSum_LSC_5K = %d, checkSum_LSC_5K = %d\n",
				dataSum_LSC_5K, checkSum_LSC_5K);
	}

	pr_err("[CHECK][AFTER] e_ctrl->is_supported: %X\n", e_ctrl->is_supported);

	if(e_ctrl->is_supported == 0x30) { //All bits are On
		pr_err("%s checksum succeed!\n", __func__);
		rc = 0;
	} else {
		//each bit (in e_ctrl->is_supported) indicates the checksum results.
	}

	return rc;
}

int32_t msm_eeprom_checksum_lgit_at24c16d(struct msm_eeprom_ctrl_t *e_ctrl) {
	uint16_t awb_5k = 0, awb_5k_checksum = 0;
	uint16_t lsc_5k = 0, lsc_5k_checksum = 0;
	uint16_t sensor_id = 0, sensor_id_checksum = 0;
	int32_t rc = -EFAULT;
	int i;

	for(i = 0x0; i < 0x6; i++) {
		awb_5k += e_ctrl->cal_data.mapdata[i];
	}
	awb_5k_checksum = (e_ctrl->cal_data.mapdata[0x7] << 8) |
		e_ctrl->cal_data.mapdata[0x6];

	for(i = 0xC; i < 0x380; i++) {
		lsc_5k += e_ctrl->cal_data.mapdata[i];
	}
	lsc_5k_checksum = (e_ctrl->cal_data.mapdata[0x381] << 8) |
		e_ctrl->cal_data.mapdata[0x380];

	for(i = 0x710; i < 0x71A; i++) {
		sensor_id += e_ctrl->cal_data.mapdata[i];
	}
	sensor_id_checksum = (e_ctrl->cal_data.mapdata[0x71B] << 8) |
		e_ctrl->cal_data.mapdata[0x71A];

	if( awb_5k == awb_5k_checksum &&
		lsc_5k == lsc_5k_checksum &&
		sensor_id == sensor_id_checksum) {
		pr_info("EEPROM data verified\n");
		rc = 0;
	} else {
		pr_err("awb_5k = 0x%x, awb_5k_checksum = 0x%x, "
			"lsc_5k = 0x%x, lsc_5k_checksum = 0x%x, "
			"sensor_id = 0x%x, sensor_id_checksum = 0x%x\n",
			awb_5k, awb_5k_checksum, lsc_5k, lsc_5k_checksum,
			sensor_id, sensor_id_checksum);
	}
	return rc;

}

