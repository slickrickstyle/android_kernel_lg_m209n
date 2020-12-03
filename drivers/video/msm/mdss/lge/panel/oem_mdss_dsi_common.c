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

#include <linux/module.h>
#include <linux/interrupt.h>
#include <linux/spinlock.h>
#include <linux/delay.h>
#include <linux/io.h>
#include <linux/of_device.h>
#include <linux/of_gpio.h>
#include <linux/gpio.h>
#include <linux/err.h>
#include <linux/regulator/consumer.h>
#include <linux/clk.h>

#define XO_CLK_RATE	19200000
#define INIT_FUNC(x, y) ((x->y) = y)

#include "oem_mdss_dsi_common.h"
#include "oem_mdss_dsi.h"

static struct panel_list supp_panels[] = {
	{"TIANMA 320p cmd mode dsi panel", TIANMA_ILI9488_HVGA_CMD_PANEL},
	{"TIANMA 320p video mode dsi panel", TIANMA_ILI9488_HVGA_VIDEO_PANEL},
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	{"LGD FWVGA video mode dsi panel", LGD_SSD2068_FWVGA_VIDEO_PANEL},
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	{"LGD DB7400 HD LH530WX2", LGD_DB7400_HD_LH530WX2_VIDEO_PANEL},
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	{"LGD DB7400 HD 5Inch video mode", LGD_DB7400_HD_5INCH_VIDEO_PANEL},
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	{"tcl fwvga video mode dsi panel",TCL_ILI9806E_FWVGA_VIDEO_PANEL},
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	{"TCL FWVGA video mode dsi panel", TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL},
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	{"tianma ft860x 720p video mode dsi panel", TIANMA_FT860x_HD_VIDEO_PANEL},
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	{"LGD LG4894 HD video mode dsi panel", LGD_LG4894_HD_VIDEO_PANEL},
	{"TVS TD4100 HD video mode dsi panel", TVS_TD4100_HD_VIDEO_PANEL},
#elif defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	{"JDI FWVGA video mode dsi panel", JDI_R69326_FWVGA_VIDEO_PANEL},
#endif
};

static int panel_id;

int panel_name_to_id(struct panel_list supp_panels[],
			  uint32_t supp_panels_size,
			  const char *panel_name)
{
	int i;
	int panel_id = UNKNOWN_PANEL;

	if (!panel_name) {
		pr_err("Invalid panel name\n");
		return panel_id;
	}

	/* Remove any leading whitespaces */
	panel_name += strspn(panel_name, " ");
	for (i = 0; i < supp_panels_size; i++) {
		if (!strncmp(panel_name, supp_panels[i].name,
			MAX_PANEL_ID_LEN)) {
			panel_id = supp_panels[i].id;
			break;
		}
	}

	return panel_id;
}

int pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		rc = tianma_hvga_cmd_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_ili9806e_fwvga_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_pre_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#endif
	default:
		break;
	}

	return rc;

}

int post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		rc = tianma_hvga_cmd_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_ili9806e_fwvga_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_post_mdss_dsi_panel_power_ctrl(pdata, enable);
		break;
#endif
	default:
		break;
	}

	return rc;

}

int pre_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH540WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		break;
#endif
	default:
		break;
	}

	return rc;
}

int lge_mdss_dsi_event_handler(struct mdss_panel_data *pdata, int event, void *arg)
{
	int rc = 0;
	switch (panel_id) {
#if defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		pr_info("%s: event=%d\n", __func__, event);
		rc = tianma_ft860x_hd_video_mdss_dsi_event_handler(pdata, event, arg);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		pr_info("%s: event=%d\n", __func__, event);
		rc = lgd_lg4894_hd_video_mdss_dsi_event_handler(pdata, event, arg);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		pr_info("%s: event=%d\n", __func__, event);
		rc = tvs_td4100_hd_video_mdss_dsi_event_handler(pdata, event, arg);
		break;
#endif
	default:
		break;
	}
	return rc;
}

int lge_msm_dss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_msm_mdss_enable_vreg(ctrl_pdata, enable);
		break;
#endif
	default:
		break;
	}

	return rc;

}

int lge_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		rc = tianma_hvga_cmd_mdss_dsi_panel_reset(pdata, enable);
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_ili9806e_fwvga_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_mdss_dsi_panel_reset(pdata, enable);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_mdss_dsi_panel_reset(pdata, enable);
		break;
#endif
	default:
		break;
	}

	return rc;
}

int lge_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		rc = tianma_hvga_cmd_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_ili9806e_fwvga_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_mdss_dsi_request_gpios(ctrl_pdata);
		break;
#endif
	default:
		break;
	}

	return rc;
}

int lge_mdss_panel_parse_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		rc = tianma_hvga_cmd_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_ili9806e_fwvga_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_mdss_panel_parse_dts(np, ctrl_pdata);
		break;
#endif
	default:
		break;
	}

	return rc;

}

int lge_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		rc = tianma_hvga_cmd_panel_device_create(node, ctrl_pdata);
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_panel_device_create(node, ctrl_pdata);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_panel_device_create(node, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_panel_device_create(node, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_panel_device_create(node, ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_ili9806e_fwvga_video_panel_device_create(node, ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_panel_device_create(node, ctrl_pdata);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_panel_device_create(node, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_panel_device_create(node, ctrl_pdata);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_panel_device_create(node, ctrl_pdata);
		break;
#endif
	default:
		break;
	}

	return rc;

}

int lge_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;

	switch (panel_id) {
	case TIANMA_ILI9488_HVGA_CMD_PANEL:
		rc = tianma_hvga_cmd_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
	case JDI_R69326_FWVGA_VIDEO_PANEL:
		rc = jdi_fwvga_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
	case LGD_SSD2068_FWVGA_VIDEO_PANEL:
		rc = lgd_fwvga_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		rc = lgd_db7400_hd_5inch_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_ili9806e_fwvga_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
		rc = tcl_oncell_ili9806e_fwvga_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
		rc = tianma_ft860x_hd_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
		rc = lgd_lg4894_hd_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
	case TVS_TD4100_HD_VIDEO_PANEL:
		rc = tvs_td4100_hd_video_dsi_panel_device_register(pan_node, ctrl_pdata);
		break;
#endif
	default:
		break;
	}

	return rc;

}

int post_mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	int rc = 0;

	switch (panel_id) {
#if defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
		rc = lgd_db7400_hd_lh530wx2_video_post_mdss_dsi_panel_off(pdata);
		break;
#endif
	default:
		break;
	}

	return rc;
}

int get_lge_panel_id(void)
{
	return panel_id;
}

void lge_mdss_dsi_seperate_panel_api_init(struct lge_mdss_dsi_interface *pdata, struct device_node *dsi_pan_node)
{
	static const char *panel_name;

	panel_name = of_get_property(dsi_pan_node, "qcom,mdss-dsi-panel-name", NULL);

	panel_id = panel_name_to_id(supp_panels,
			ARRAY_SIZE(supp_panels), panel_name);

	switch (panel_id) {
		case TIANMA_ILI9488_HVGA_CMD_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			break;
#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
		case JDI_R69326_FWVGA_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			break;
#endif
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
		case LGD_SSD2068_FWVGA_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			break;
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
	case LGD_DB7400_HD_LH530WX2_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			INIT_FUNC(pdata, post_mdss_dsi_panel_off);
			break;
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
	case LGD_DB7400_HD_5INCH_VIDEO_PANEL:
		printk("###\n");
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			break;
#elif defined (CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ILI9806E_FWVGA_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			break;
#elif defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
	case TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			break;
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
	case TIANMA_FT860x_HD_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_event_handler);
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			break;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
	case LGD_LG4894_HD_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_event_handler);
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			break;
	case TVS_TD4100_HD_VIDEO_PANEL:
			INIT_FUNC(pdata, lge_mdss_dsi_event_handler);
			INIT_FUNC(pdata, lge_mdss_dsi_panel_reset);
			INIT_FUNC(pdata, lge_mdss_dsi_request_gpios);
			INIT_FUNC(pdata, lge_mdss_panel_parse_dt);
			INIT_FUNC(pdata, lge_panel_device_create);
			INIT_FUNC(pdata, lge_dsi_panel_device_register);
			INIT_FUNC(pdata, lge_msm_dss_enable_vreg);
			break;
#endif
		default:
			break;
	}
}
