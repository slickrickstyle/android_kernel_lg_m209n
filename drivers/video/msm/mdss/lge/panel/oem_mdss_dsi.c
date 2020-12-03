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

#if defined (CONFIG_LGE_LCD_ESD_PANEL_POWER_RECOVERY)
#include "mdss_fb.h"
#endif

#define XO_CLK_RATE	19200000

#include "oem_mdss_dsi_common.h"
#include "oem_mdss_dsi.h"

extern int panel_esd_state;
#if defined(CONFIG_LGE_LCD_ESD_WITH_QCT_ESD)
/*
 * esd_status : status about esd
 * value 0 : not detected or init Statust
 * value BIT(0) : detected by Touch Interrupt (LGE_LCD_ESD)
 */
static int esd_status = 0;

void init_esd_status(void)
{
	esd_status = 0;
}
EXPORT_SYMBOL(init_esd_status);

int get_esd_status(void)
{
	return esd_status;
}
EXPORT_SYMBOL(get_esd_status);

void set_esd_status(int esd_detection)
{
	esd_status |= esd_detection;
	pr_info("%s:detection=0x%x, status=0x%x\n",
				__func__, esd_detection, esd_status);
}
EXPORT_SYMBOL(set_esd_status);
#endif

#if defined (CONFIG_LGE_LCD_ESD_PANEL_POWER_RECOVERY)
static atomic_t esd_power_recovery = ATOMIC_INIT(ESD_OK);

int get_esd_power_recovery(void)
{
	return atomic_read(&esd_power_recovery);
}
EXPORT_SYMBOL(get_esd_power_recovery);

void set_esd_power_recovery(int esd_detection)
{
	atomic_set(&esd_power_recovery, esd_detection);
}
EXPORT_SYMBOL(set_esd_power_recovery);
#endif

#if defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL)
#include "../../../../../input/touchscreen/lge/touch_core.h"
#include "../../../../../input/touchscreen/lge/lgsic/touch_lg4894.h"
#endif

#if defined (CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
#include <soc/qcom/lge/lge_boot_mode.h>
#endif
int lge_mdss_dsi_lane_config(struct mdss_panel_data *pdata, int enable)
{
	struct mipi_panel_info *mipi;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	mipi = &pdata->panel_info.mipi;

	if (mipi->force_clk_lane_hs) {
		u32 tmp;

		tmp = MIPI_INP((ctrl_pdata->ctrl_base) + 0xac);
		if (enable) {
			tmp |= BIT(28);
		} else {
			tmp &= ~BIT(28);
		}
		MIPI_OUTP((ctrl_pdata->ctrl_base) + 0xac, tmp);
		wmb();
		pr_info("%s: current mode=%s dsi_lane_ctrl=0x%08x\n",
			__func__, (enable ? "hs" : "lp"), MIPI_INP((ctrl_pdata->ctrl_base) + 0xac));
	}
	return 0;
}

int tianma_hvga_cmd_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;

	if (!enable) {
		lge_mdss_dsi_lane_config(pdata, enable);

		mdelay(10);

		ret = lge_mdss_dsi_panel_reset(pdata, 0);
		if (ret) {
			pr_warn("%s: Panel reset failed. rc=%d\n", __func__, ret);
			ret = 0;
		}
	}
	return ret;
}

int tianma_hvga_cmd_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	if (IS_ENABLED(CONFIG_LGE_PM_PARALLEL_CHARGING))
		smbchg_fb_notify_update_cb(enable);

	return 0;
}

int lgd_fhd_video_mdss_panel_parse_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tianma_hvga_cmd_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
int jdi_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;

	if (!enable) {
		lge_mdss_dsi_lane_config(pdata, enable);

		mdelay(10);

		ret = lge_mdss_dsi_panel_reset(pdata, 0);
		if (ret) {
			pr_warn("%s: Panel reset failed. rc=%d\n", __func__, ret);
			ret = 0;
		}
	}
	return ret;
}

int jdi_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	if (IS_ENABLED(CONFIG_LGE_PM_PARALLEL_CHARGING))
		smbchg_fb_notify_update_cb(enable);

	return 0;
}

int jdi_fwvga_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int jdi_fwvga_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;

	struct mdss_panel_info *pinfo;
	pinfo = &ctrl_pdata->panel_data.panel_info;

	if (enable) {
		for (i = 0; i < DSI_MAX_PM; i++) {
			if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled) {
			//		if (!lge_get_mfts_mode() || (lge_get_mfts_mode() && get_mfts_lpwg())) {

						if(!panel_esd_state){
							if (DSI_PANEL_PM == i)
								continue;
						}
							if (DSI_CORE_PM == i)
								continue;
			//	}
		}

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);

			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
						__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
		if(panel_esd_state) panel_esd_state = 0;
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
			//	if (!lge_get_mfts_mode() || (lge_get_mfts_mode() && get_mfts_lpwg())) {

						if(!panel_esd_state){
							if (DSI_PANEL_PM == i)
								continue;
						}
							if (DSI_CORE_PM == i)
								continue;

			//	}

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);

			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
						__func__, __mdss_dsi_pm_name(i));
				return ret;
			}

		}

	}

	return ret;
}

#endif

#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
int lgd_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;

	if (!enable) {
		lge_mdss_dsi_lane_config(pdata, enable);

		mdelay(10);

		ret = lge_mdss_dsi_panel_reset(pdata, 0);
		if (ret) {
			pr_warn("%s: Panel reset failed. rc=%d\n", __func__, ret);
			ret = 0;
		}
	}
	return ret;
}

int lgd_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	if (IS_ENABLED(CONFIG_LGE_PM_PARALLEL_CHARGING))
		smbchg_fb_notify_update_cb(enable);

	return 0;
}

int lgd_fwvga_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int lgd_fwvga_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;

	struct mdss_panel_info *pinfo;
	pinfo = &ctrl_pdata->panel_data.panel_info;

	if (enable) {
		for (i = 0; i < DSI_MAX_PM; i++) {
			if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled) {
					if (!lge_get_mfts_mode() || (lge_get_mfts_mode() && get_mfts_lpwg())) {

						if(!panel_esd_state){
							if (DSI_PANEL_PM == i)
								continue;
						}
							if (DSI_CORE_PM == i)
								continue;
				}
		}
			if (lge_get_mfts_mode() && !get_mfts_lpwg()) {
					if (DSI_PANEL_PM == i)
							gpio_set_value(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio, 1);
							pr_info("mfts_ldo set 1\n");
			}

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);

			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
						__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
		if(panel_esd_state) panel_esd_state = 0;
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
				if (!lge_get_mfts_mode() || (lge_get_mfts_mode() && get_mfts_lpwg())) {

						if(!panel_esd_state){
							if (DSI_PANEL_PM == i)
								continue;
						}
							if (DSI_CORE_PM == i)
								continue;

				}

				if(lge_get_mfts_mode() && !get_mfts_lpwg()) {
						if (DSI_PANEL_PM == i) {
								gpio_set_value(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio, 0);
								pr_info("mfts_ldo set 0\n");
						}
				}
			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);

			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
						__func__, __mdss_dsi_pm_name(i));
				return ret;
			}

		}

	}

	return ret;
}
#elif defined (CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
int lgd_db7400_hd_lh530wx2_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;

	if (!enable) {
		lge_mdss_dsi_lane_config(pdata, enable);
	}

	return ret;
}

int lgd_db7400_hd_lh530wx2_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	return 0;
}

int lgd_db7400_hd_lh530wx2_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int lgd_db7400_hd_lh530wx2_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;

	if (enable) {
		for (i = 0; i < DSI_MAX_PM; i++) {
			if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled)
				if (DSI_PANEL_PM == i)
					continue;

			if (DSI_CORE_PM == i)
				continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);

			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
			if (DSI_PANEL_PM == i)
				continue;

			if (DSI_CORE_PM == i)
				continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);

			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}

		}
	}

	return ret;
}
#elif defined (CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
int lgd_db7400_hd_5inch_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;

	if (!enable) {
		lge_mdss_dsi_lane_config(pdata, enable);
	}

	return ret;
}

int lgd_db7400_hd_5inch_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	if (IS_ENABLED(CONFIG_LGE_PM_PARALLEL_CHARGING))
		smbchg_fb_notify_update_cb(enable);

	return 0;
}

int lgd_db7400_hd_5inch_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int lgd_db7400_hd_5inch_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;

	if (enable) {
		for (i = 0; i < DSI_MAX_PM; i++) {
			if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled)
				if (DSI_PANEL_PM == i)
					continue;

			if (DSI_CORE_PM == i)
				continue;

			if(i == DSI_PANEL_PM){
				ret = msm_dss_enable_vreg(
						ctrl_pdata->power_data[i].vreg_config,
						ctrl_pdata->power_data[i].num_vreg, 0);
				mdelay(3);
			}

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);

			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
			if (DSI_PANEL_PM == i)
				continue;

			if (DSI_CORE_PM == i)
				continue;
			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);

			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}

		}

	}

	return ret;
}

#elif defined(CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
int tcl_ili9806e_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;

	if (!enable) {
		lge_mdss_dsi_lane_config(pdata, enable);

		mdelay(10);

		ret = lge_mdss_dsi_panel_reset(pdata, 0);
		if (ret) {
			pr_warn("%s: Panel reset failed. rc=%d\n", __func__, ret);
			ret = 0;
		}
	}
	return ret;
}

int tcl_ili9806e_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	if (IS_ENABLED(CONFIG_LGE_PM_PARALLEL_CHARGING))
		smbchg_fb_notify_update_cb(enable);

	return 0;
}

int tcl_ili9806e_fwvga_video_mdss_panel_parse_dt(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tcl_ili9806e_fwvga_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}
#elif defined(CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
extern bool lge_get_mfts_mode(void);
int tcl_oncell_ili9806e_fwvga_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	int ret = 0;

	if (!enable) {
		lge_mdss_dsi_lane_config(pdata, enable);

		mdelay(10);

		ret = lge_mdss_dsi_panel_reset(pdata, 0);
		if (ret) {
			pr_warn("%s: Panel reset failed. rc=%d\n", __func__, ret);
			ret = 0;
		}
	}
	return ret;
}

int tcl_oncell_ili9806e_fwvga_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	if (IS_ENABLED(CONFIG_LGE_PM_PARALLEL_CHARGING))
		smbchg_fb_notify_update_cb(enable);

	return 0;
}

int tcl_oncell_ili9806e_fwvga_video_mdss_panel_parse_dt(struct device_node *np,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tcl_oncell_ili9806e_fwvga_video_dsi_panel_device_register(struct device_node *pan_node,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tcl_oncell_ili9806e_fwvga_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;
	bool isMFTS = lge_get_mfts_mode();

	if (enable) {
		for (i = 0; i < DSI_MAX_PM; i++) {
			/*
			 * Core power module will be enabled when the
			 * clocks are enabled
			 */
#if defined(CONFIG_APDS9930_BMCHAL)
			if (!isMFTS)
				if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled)
					if (DSI_PANEL_PM == i)
						continue;
#endif
			if (DSI_CORE_PM == i)
				continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);
			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
			pr_err("%s: isMFTS=%d\n", __func__, isMFTS);
			/*
			 * Core power module will be disabled when the
			 * clocks are disabled
			 */
#if defined(CONFIG_APDS9930_BMCHAL)
			if (!isMFTS) {
				if (DSI_PANEL_PM == i)
					continue;
			}
#endif
			if (DSI_CORE_PM == i)
				continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);
			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
						__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	}

	return ret;
}
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
int tianma_ft860x_hd_video_mdss_dsi_event_handler(struct mdss_panel_data *pdata, int event, void *arg)
{
	int rc = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	switch (event) {
	case MDSS_EVENT_POST_PANEL_ON:
		if (get_boot_fw_recovery() == 1) {
			lge_mdss_report_firmware_recovery();
			set_boot_fw_recovery(0);
		}
		break;
	default:
		break;
	}

	return rc;
}

int tianma_ft860x_hd_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	return 0;
}

int tianma_ft860x_hd_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	if (IS_ENABLED(CONFIG_LGE_PM_PARALLEL_CHARGING))
		smbchg_fb_notify_update_cb(enable);

	return 0;
}

int tianma_ft860x_hd_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tianma_ft860x_hd_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;

	struct mdss_panel_info *pinfo;
	pinfo = &ctrl_pdata->panel_data.panel_info;

	if (enable) {
		if ((lge_get_mfts_mode() && !get_mfts_lpwg())
			|| (get_esd_power_recovery() == ESD_NOK)) {
			if (gpio_is_valid(ctrl_pdata->lge_pan_data->disp_vdd_gpio)) {
				gpio_set_value(ctrl_pdata->lge_pan_data->disp_vdd_gpio, 1);
				pr_info("%s: disp vdd gpio [1]\n", __func__);
			}
		}

		for (i = 0; i < DSI_MAX_PM; i++) {
			if (!pinfo->cont_splash_enabled
				&& (get_esd_power_recovery() != ESD_NOK)) {
				if (!lge_get_mfts_mode()
					|| (lge_get_mfts_mode() && get_mfts_lpwg())) {
					if (DSI_PANEL_PM == i)
						continue;

					if (DSI_CORE_PM == i)
						continue;
				}
			}

			if (lge_get_mfts_mode() && !get_mfts_lpwg()) {
				if (DSI_PANEL_PM == i)
					gpio_set_value(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio, 0);
			}

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);

			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
		usleep(1000);
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
			if ((get_esd_power_recovery() != ESD_NOK)) {
				if (!lge_get_mfts_mode()
					|| (lge_get_mfts_mode() && get_mfts_lpwg())) {
					if (DSI_PANEL_PM == i)
						continue;

					if (DSI_CORE_PM == i)
						continue;
				}
			}

			if (lge_get_mfts_mode() && !get_mfts_lpwg()) {
				if (DSI_PANEL_PM == i)
					gpio_set_value(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio, 1);
			}

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);

			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
		if ((lge_get_mfts_mode() && !get_mfts_lpwg())
			|| (get_esd_power_recovery() == ESD_NOK)) {
			if (gpio_is_valid(ctrl_pdata->lge_pan_data->disp_vdd_gpio)) {
				gpio_set_value(ctrl_pdata->lge_pan_data->disp_vdd_gpio, 0);
				pr_info("%s: disp vdd gpio [0]\n", __func__);
			}
		}
	}

	return ret;
}
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
/* For shutdown event */
extern bool oem_shutdown_pending;

/* Touch LPWG Status */
static unsigned int pre_panel_mode = LCD_MODE_STOP;
static unsigned int cur_panel_mode = LCD_MODE_STOP;

/* For Sending MIPI Commands */
struct mdss_panel_data *lgd_lg4894_pdata_base;
EXPORT_SYMBOL(lgd_lg4894_pdata_base);
int lgd_lg4894_hd_video_mdss_dsi_event_handler(struct mdss_panel_data *pdata, int event, void *arg)
{
	int rc = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	switch (event) {
	case MDSS_EVENT_POST_PANEL_ON:
		lgd_lg4894_pdata_base = pdata;
		cur_panel_mode = LCD_MODE_U3;
		pr_info("%s: event=MDSS_EVENT_POST_PANEL_ON panel_mode=%d,%d\n",
				__func__, pre_panel_mode, cur_panel_mode);
		break;
	case MDSS_EVENT_PANEL_OFF:
		cur_panel_mode = LCD_MODE_U0;
		pr_info("%s: event=MDSS_EVENT_PANEL_OFF panel_mode=%d,%d\n",
				__func__, pre_panel_mode, cur_panel_mode);
		break;
	default:
		pr_info("%s: nothing to do about this event=%d\n", __func__, event);
	}

	if (pre_panel_mode != cur_panel_mode) {
		rc = touch_notifier_call_chain(LCD_EVENT_LCD_MODE, (void *)&cur_panel_mode);
		pre_panel_mode = cur_panel_mode;
	}
	return rc;
}
int lgd_lg4894_hd_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	return 0;
}
int lgd_lg4894_hd_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	return 0;
}
int lgd_lg4894_hd_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}
int lgd_lg4894_hd_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;

	if (enable) {
		for (i = 0; i < DSI_MAX_PM; i++) {
			/*
			 * Core power module will be enabled when the
			 * clocks are enabled
			 */
			if (DSI_CORE_PM == i)
				continue;

			if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled)
				if (DSI_PANEL_PM == i)
					continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);

			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
			if (!oem_shutdown_pending)
				if (DSI_PANEL_PM == i)
					continue;

			/*
			 * Core power module will be disabled when the
			 * clocks are disabled
			 */
			if (DSI_CORE_PM == i)
				continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);

			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	}

	return ret;
}

/* Touch LPWG */
extern void SetNextState_For_Touch(int lcdState);

/* For Sending MIPI Commands */
struct mdss_panel_data *tvs_td4100_pdata_base;
EXPORT_SYMBOL(tvs_td4100_pdata_base);
int tvs_td4100_hd_video_mdss_dsi_event_handler(struct mdss_panel_data *pdata, int event, void *arg)
{
	int rc = 0;
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	switch (event) {
	case MDSS_EVENT_POST_PANEL_ON:
		tvs_td4100_pdata_base = pdata;
		break;
	case MDSS_EVENT_PANEL_OFF:
		pr_info("%s: event=MDSS_EVENT_PANEL_OFF SetNextState_For_touch(0) called\n",
				__func__);
		SetNextState_For_Touch(0);
		break;
	default:
		break;
	}

	return rc;
}
int tvs_td4100_hd_video_pre_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	return 0;
}
int tvs_td4100_hd_video_post_mdss_dsi_panel_power_ctrl(struct mdss_panel_data *pdata, int enable)
{
	return 0;
}
int tvs_td4100_hd_video_dsi_panel_device_register(struct device_node *pan_node,
				struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}
int tvs_td4100_hd_video_msm_mdss_enable_vreg(struct mdss_dsi_ctrl_pdata *ctrl_pdata, int enable)
{
	int i = 0;
	int ret = 0;

	if (enable) {
		for (i = 0; i < DSI_MAX_PM; i++) {
			/*
			 * Core power module will be enabled when the
			 * clocks are enabled
			 */
			if (DSI_CORE_PM == i)
				continue;

			if (!ctrl_pdata->panel_data.panel_info.cont_splash_enabled)
				if (DSI_PANEL_PM == i)
					continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 1);

			if (ret) {
				pr_err("%s: failed to enable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	} else {
		for (i = DSI_MAX_PM - 1; i >= 0; i--) {
			if (!oem_shutdown_pending)
				if (DSI_PANEL_PM == i)
					continue;

			/*
			 * Core power module will be disabled when the
			 * clocks are disabled
			 */
			if (DSI_CORE_PM == i)
				continue;

			ret = msm_dss_enable_vreg(
					ctrl_pdata->power_data[i].vreg_config,
					ctrl_pdata->power_data[i].num_vreg, 0);

			if (ret) {
				pr_err("%s: failed to disable vregs for %s\n",
					__func__, __mdss_dsi_pm_name(i));
				return ret;
			}
		}
	}

	return ret;
}
#endif
