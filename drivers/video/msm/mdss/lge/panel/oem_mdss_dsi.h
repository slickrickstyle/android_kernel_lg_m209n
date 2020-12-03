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

#ifndef OEM_MDSS_DSI_H
#define OEM_MDSS_DSI_H

#include <linux/list.h>
#include <linux/mdss_io_util.h>
#include <linux/irqreturn.h>
#include <linux/pinctrl/consumer.h>
#include <linux/gpio.h>

int tianma_hvga_cmd_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tianma_hvga_cmd_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tianma_hvga_cmd_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_hvga_cmd_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_hvga_cmd_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_hvga_cmd_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_hvga_cmd_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int tianma_hvga_cmd_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);

#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
int jdi_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int jdi_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int jdi_fwvga_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int jdi_fwvga_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int jdi_fwvga_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int jdi_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int jdi_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int jdi_fwvga_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
#endif

#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
int lgd_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_fwvga_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_fwvga_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_fwvga_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int lgd_fwvga_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
extern int get_mfts_lpwg(void);
extern bool lge_get_mfts_mode(void);
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
int lgd_db7400_hd_lh530wx2_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_db7400_hd_lh530wx2_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_db7400_hd_lh530wx2_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_lh530wx2_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_lh530wx2_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_lh530wx2_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_lh530wx2_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int lgd_db7400_hd_lh530wx2_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
int lgd_db7400_hd_lh530wx2_video_post_mdss_dsi_panel_off(struct mdss_panel_data *pdata);
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
int lgd_db7400_hd_5inch_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_db7400_hd_5inch_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_db7400_hd_5inch_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_5inch_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_5inch_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_5inch_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_db7400_hd_5inch_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int lgd_db7400_hd_5inch_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
#elif defined(CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
int tcl_ili9806e_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tcl_ili9806e_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tcl_ili9806e_fwvga_video_dsi_panel_device_register(struct device_node *pan_node, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_ili9806e_fwvga_video_mdss_panel_parse_dts(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_ili9806e_fwvga_video_panel_device_create(struct device_node *node, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_ili9806e_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_ili9806e_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
#elif defined(CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
int tcl_oncell_ili9806e_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tcl_oncell_ili9806e_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tcl_oncell_ili9806e_fwvga_video_dsi_panel_device_register(struct device_node *pan_node, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_oncell_ili9806e_fwvga_video_mdss_panel_parse_dts(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_oncell_ili9806e_fwvga_video_panel_device_create(struct device_node *node, struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_oncell_ili9806e_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tcl_oncell_ili9806e_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int tcl_oncell_ili9806e_fwvga_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
#elif defined(CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
int tianma_ft860x_hd_video_mdss_dsi_event_handler(struct mdss_panel_data *pdata, int event, void *arg);
int tianma_ft860x_hd_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tianma_ft860x_hd_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tianma_ft860x_hd_video_dsi_panel_device_register(struct device_node *pan_node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_ft860x_hd_video_mdss_panel_parse_dts(struct device_node *np,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_ft860x_hd_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_ft860x_hd_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tianma_ft860x_hd_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int tianma_ft860x_hd_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
int tianma_ft860x_hd_video_panel_extenal_api(int type, int enable);
enum external_api_type {
	VDDI_CTRL,
	AVDD_AVEE_CTRL,
	LCD_RESET_CTRL,
	LCD_INIT_CMD_TRANSFER,
	NONE,
};
int get_boot_fw_recovery(void);
void set_boot_fw_recovery(int recovery);
int lge_mdss_report_firmware_recovery(void);
extern int rt4832_lcd_backlight_get_level(void);
extern int get_mfts_lpwg(void);
extern bool lge_get_mfts_mode(void);
extern bool lcd_connection_boot;
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
int lgd_lg4894_hd_video_mdss_dsi_event_handler(struct mdss_panel_data *pdata, int event, void *arg);
int lgd_lg4894_hd_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_lg4894_hd_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int lgd_lg4894_hd_video_dsi_panel_device_register(struct device_node *pan_node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_lg4894_hd_video_mdss_panel_parse_dts(struct device_node *np,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_lg4894_hd_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_lg4894_hd_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int lgd_lg4894_hd_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int lgd_lg4894_hd_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
extern bool lcd_connection_boot;

int tvs_td4100_hd_video_mdss_dsi_event_handler(struct mdss_panel_data *pdata, int event, void *arg);
int tvs_td4100_hd_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tvs_td4100_hd_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable);
int tvs_td4100_hd_video_dsi_panel_device_register(struct device_node *pan_node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tvs_td4100_hd_video_mdss_panel_parse_dts(struct device_node *np,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tvs_td4100_hd_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tvs_td4100_hd_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata);
int tvs_td4100_hd_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable);
int tvs_td4100_hd_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable);
#endif

extern void smbchg_fb_notify_update_cb(bool is_on);

extern int mdss_dsi_parse_dcs_cmds(struct device_node *np,
		struct dsi_panel_cmds *pcmds, char *cmd_key, char *link_key);

extern void mdss_dsi_panel_cmds_send(struct mdss_dsi_ctrl_pdata *ctrl,
			struct dsi_panel_cmds *pcmds);

#if defined (CONFIG_LGE_LCD_ESD_PANEL_POWER_RECOVERY)
int get_esd_power_recovery(void);
void set_esd_power_recovery(int esd_detection);
#endif

enum {
TIANMA_ILI9488_HVGA_CMD_PANEL,
TIANMA_ILI9488_HVGA_VIDEO_PANEL,
#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
LGD_SSD2068_FWVGA_VIDEO_PANEL,
#elif defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
JDI_R69326_FWVGA_VIDEO_PANEL,
#elif defined(CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
LGD_DB7400_HD_LH530WX2_VIDEO_PANEL,
#elif defined(CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
LGD_DB7400_HD_5INCH_VIDEO_PANEL,
#elif defined(CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
TCL_ILI9806E_FWVGA_VIDEO_PANEL,
#elif defined(CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL,
#elif defined(CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
TIANMA_FT860x_HD_VIDEO_PANEL,
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
LGD_LG4894_HD_VIDEO_PANEL,
TVS_TD4100_HD_VIDEO_PANEL,
#endif
UNKNOWN_PANEL
};

#endif /* OEM_MDSS_DSI_H */
