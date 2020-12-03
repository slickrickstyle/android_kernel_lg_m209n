/* Copyright (c) 2011-2014, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */
#ifndef MSM_EEPROM_H
#define MSM_EEPROM_H

#include <linux/i2c.h>
#include <linux/gpio.h>
#include <soc/qcom/camera2.h>
#include <media/v4l2-subdev.h>
#include <media/msmb_camera.h>
#include "msm_camera_i2c.h"
#include "msm_camera_spi.h"
#include "msm_camera_io_util.h"
#include "msm_camera_dt_util.h"

#if defined(CONFIG_MSM8909_E1Q) || defined(CONFIG_MSM8909_M1) || defined(CONFIG_MSM8909_M1V) || defined(CONFIG_MSM8909_M209) || defined(CONFIG_MSM8909_ME0) || defined(CONFIG_MSM8909_LV1)
#define HI553_LGIT_MODULE
#endif

struct msm_eeprom_ctrl_t;

#define DEFINE_MSM_MUTEX(mutexname) \
	static struct mutex mutexname = __MUTEX_INITIALIZER(mutexname)

#define PROPERTY_MAXSIZE 32

struct msm_eeprom_ctrl_t {
	struct platform_device *pdev;
	struct mutex *eeprom_mutex;

	struct v4l2_subdev sdev;
	struct v4l2_subdev_ops *eeprom_v4l2_subdev_ops;
	enum msm_camera_device_type_t eeprom_device_type;
	struct msm_sd_subdev msm_sd;
	enum cci_i2c_master_t cci_master;

	struct msm_camera_i2c_client i2c_client;
	struct msm_eeprom_memory_block_t cal_data;
	uint8_t is_supported;
	struct msm_eeprom_board_info *eboard_info;
	uint32_t subdev_id;
	enum camb_position_t position;
	uint8_t AAT_Checksum;
};


//EEPROM Code Refinement, Camera-Driver@lge.com, 2015-06-11
typedef enum {
	BigEndian,
	LittleEndian,
} Endian;

#if defined(CONFIG_MACH_MSM8909_K6B_TRF_US) || defined(CONFIG_MACH_MSM8909_K6B_TRF_US_VZW)
#define MODULE_VENDOR_ID 0x3A0
#else
#define MODULE_VENDOR_ID 0x700
#endif

//Module Selector
int32_t msm_eeprom_checksum_imtech(struct msm_eeprom_ctrl_t *e_ctrl);
int32_t msm_eeprom_checksum_lgit(struct msm_eeprom_ctrl_t *e_ctrl);
int32_t msm_eeprom_checksum_cowell(struct msm_eeprom_ctrl_t *e_ctrl);

//Module CheckSum routine
//Cowell
int32_t msm_eeprom_checksum_cowell_hi842(struct msm_eeprom_ctrl_t *e_ctrl);
int32_t msm_eeprom_checksum_s5k5e2(struct msm_eeprom_ctrl_t *e_ctrl);
int32_t msm_eeprom_checksum_cowell_zc533(struct msm_eeprom_ctrl_t *e_ctrl);

//Imtech
int32_t msm_eeprom_checksum_imtech_ov8858(struct msm_eeprom_ctrl_t *e_ctrl);

//LGIT
int32_t msm_eeprom_checksum_lgit_hi842(struct msm_eeprom_ctrl_t *e_ctrl);
int32_t msm_eeprom_checksum_lgit_hi553(struct msm_eeprom_ctrl_t *e_ctrl);
int32_t msm_eeprom_checksum_lgit_at24c16d(struct msm_eeprom_ctrl_t *e_ctrl);

//Helper function for arithmetic shifted addition / just accumulation
uint32_t shiftedSum (struct msm_eeprom_ctrl_t *e_ctrl, uint32_t startAddr, uint32_t endAddr, Endian endian);
uint32_t accumulation (struct msm_eeprom_ctrl_t *e_ctrl, uint32_t startAddr, uint32_t endAddr);
#endif
