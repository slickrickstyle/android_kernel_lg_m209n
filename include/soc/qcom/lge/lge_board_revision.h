/* Copyright (c) 2014, The Linux Foundation. All rights reserved.
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

#ifndef _LGE_BOARD_REVISION_H
#define _LGE_BOARD_REVISION_H

#if defined(CONFIG_MSM8909_CF)
enum hw_rev_type {
	HW_REV_EVB1 = 0,
	HW_REV_EVB2,
	HW_REV_A,
	HW_REV_B,
	HW_REV_C,
	HW_REV_D,
	HW_REV_E,
	HW_REV_F,
	HW_REV_G,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_1_2,
	HW_REV_1_3,
	HW_REV_MAX
};
#elif defined(CONFIG_LGE_PM_PCB_REV)
#if defined(CONFIG_MSM8909_M1V) || defined(CONFIG_MSM8909_M1V_VZW) || defined(CONFIG_MSM8909_CLING) || \
	defined(CONFIG_MACH_MSM8909_M209N_TMO_US) || defined(CONFIG_MACH_MSM8909_M209N_MPCS_US) || \
	defined(CONFIG_MACH_MSM8909_M209N_ATT_US) || defined(CONFIG_MSM8909_LEAP) || \
	defined(CONFIG_MACH_MSM8909_M209N_CRK_US) || defined(CONFIG_MSM8909_CF2)
enum hw_rev_type {
	HW_REV_EVB1 = 0,
	HW_REV_EVB2,
	HW_REV_0,
	HW_REV_A,
	HW_REV_B,
	HW_REV_C,
	HW_REV_D,
	HW_REV_E,
	HW_REV_F,
	HW_REV_G,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_MKT,
	HW_REV_MAX
};
#elif defined(CONFIG_MSM8909_K6P)
enum hw_rev_type {
	HW_REV_EVB1 = 0,
	HW_REV_EVB2,
	HW_REV_0s,
	HW_REV_0m,
	HW_REV_A,
	HW_REV_A1,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_MAX
};
#elif defined(CONFIG_MACH_MSM8909_K6B_TRF_US) || defined(CONFIG_MACH_MSM8909_K6B_TRF_US_VZW) || \
	defined(CONFIG_MACH_MSM8909_K6B_SPR_US) || defined(CONFIG_MACH_MSM8909_K6B_TMO_US)
enum hw_rev_type {
        HW_REV_EVB1 = 0,
        HW_REV_EVB2,
        HW_REV_0,
        HW_REV_A,
        HW_REV_B,
        HW_REV_B1,
        HW_REV_D,
        HW_REV_E,
        HW_REV_F,
        HW_REV_G,
        HW_REV_1_0,
        HW_REV_1_1,
        HW_REV_MKT,
        HW_REV_MAX
};
#else
enum hw_rev_type {
	HW_REV_EVB1 = 0,
	HW_REV_EVB2,
	HW_REV_0,
	HW_REV_A,
	HW_REV_B,
	HW_REV_C,
	HW_REV_D,
	HW_REV_E,
	HW_REV_F,
	HW_REV_G,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_1_2,
	HW_REV_MAX
};
#endif
#else
enum hw_rev_type {
	HW_REV_EVB1 = 0,
	HW_REV_EVB2,
	HW_REV_A,
	HW_REV_B,
	HW_REV_C,
	HW_REV_D,
	HW_REV_E,
	HW_REV_F,
	HW_REV_1_0,
	HW_REV_1_1,
	HW_REV_MAX
};
#endif

extern char *rev_str[];

enum hw_rev_type lge_get_board_revno(void);
char *lge_get_board_rev(void);

#endif
