/* Copyright (c) 2012-2014, The Linux Foundation. All rights reserved.
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

#include <linux/kernel.h>
#include <linux/string.h>
#include <soc/qcom/lge/lge_board_revision.h>

static enum hw_rev_type lge_bd_rev = HW_REV_MAX;

#if defined(CONFIG_MSM8909_CF)
/* CAUTION: These strings are come from LK. */
char *rev_str[] = {"evb1", "evb2", "rev_a", "rev_b", "rev_c", "rev_d",
	"rev_e", "rev_f", "rev_g", "rev_10", "rev_11", "rev_12", "rev_13", "revserved"};
#elif defined(CONFIG_LGE_PM_PCB_REV)
#if defined(CONFIG_MSM8909_M1V) || defined(CONFIG_MSM8909_M1V_VZW) || defined(CONFIG_MSM8909_CLING) || \
	defined(CONFIG_MACH_MSM8909_M209N_TMO_US) || defined(CONFIG_MACH_MSM8909_M209N_MPCS_US) || \
	defined(CONFIG_MACH_MSM8909_M209N_ATT_US) || defined(CONFIG_MSM8909_LEAP) || \
	defined(CONFIG_MACH_MSM8909_M209N_CRK_US) || defined(CONFIG_MSM8909_LV1) || \
	defined(CONFIG_MSM8909_CF2)
/* CAUTION: These strings are come from LK. */
char *rev_str[] = {"evb1", "evb2", "rev_0", "rev_a", "rev_b", "rev_c", "rev_d",
	"rev_e", "rev_f", "rev_g", "rev_10", "rev_11", "rev_mkt", "revserved"};
#elif defined(CONFIG_MSM8909_K6P)
char *rev_str[] = {"evb1", "evb2", "rev_0s", "rev_0m", "rev_a", "rev_a1",
	"rev_10", "rev_11", "revserved"};
#elif defined(CONFIG_MACH_MSM8909_K6B_TRF_US) || defined(CONFIG_MACH_MSM8909_K6B_TRF_US_VZW) || \
	defined(CONFIG_MACH_MSM8909_K6B_SPR_US) || defined(CONFIG_MACH_MSM8909_K6B_TMO_US)
/* CAUTION: These strings are come from LK. */
char *rev_str[] = {"evb1", "evb2", "rev_0", "rev_a", "rev_b", "rev_b1", "rev_d",
        "rev_e", "rev_f", "rev_g", "rev_10", "rev_11", "rev_mkt", "revserved"};
#else
/* CAUTION: These strings are come from LK. */
char *rev_str[] = {"evb1", "evb2", "rev_0", "rev_a", "rev_b", "rev_c", "rev_d",
	"rev_e", "rev_f", "rev_g", "rev_10", "rev_11", "rev_12", "revserved"};
#endif
#else
/* CAUTION: These strings are come from LK. */
char *rev_str[] = {"evb1", "evb2", "rev_a", "rev_b", "rev_c", "rev_d",
	"rev_e", "rev_f", "rev_10", "rev_11", "revserved"};
#endif

static int __init board_revno_setup(char *rev_info)
{
	int i;

	for (i = 0; i < HW_REV_MAX; i++) {
		if (!strncmp(rev_info, rev_str[i], 6)) {
			lge_bd_rev = i;
			break;
		}
	}

	pr_info("BOARD : LGE %s\n", rev_str[lge_bd_rev]);
	return 1;
}
__setup("lge.rev=", board_revno_setup);

enum hw_rev_type lge_get_board_revno(void)
{
	return lge_bd_rev;
}

char *lge_get_board_rev(void)
{
	char *name;
	name = rev_str[lge_bd_rev];
	return name;
}
