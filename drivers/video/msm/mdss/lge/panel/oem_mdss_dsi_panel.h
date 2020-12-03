/* Copyright (c) 2012-2015, The Linux Foundation. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

#ifndef OEM_MDSS_PANEL_H
#define OEM_MDSS_PANEL_H

#if defined(CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
/* To Do */
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL)
/* Touch OSC on / off Start */
enum {
	TOUCH_OSC_ON,
	TOUCH_OSC_OFF,
	TOUCH_OSC_CMDS
};
struct touch_osc_cmds_desc {
	struct dsi_panel_cmds touch_osc_cmds[TOUCH_OSC_CMDS];
};
static char *touch_osc_dt[] = {
	"qcom,mdss-dsi-touch-osc-off-command",
	"qcom,mdss-dsi-touch-osc-on-command",
};
static struct touch_osc_cmds_desc *touch_osc_cmds_set;
/* Touch OSC on / off End */

/* LG4894 Panel Revision Start */
#define MAX_LEN_OF_PROPNAME	64
enum lg4894_rev {
	LG4894_REV_EVT6 = 0,
	LG4894_REV_EVT8,
	LG4894_REV_EVT9,
	LG4894_REV_UNKNOWN
};
extern int dic_rev;
/* LG4894 Panel Revision End */
#endif

#endif /* OEM_MDSS_PANEL_H */
