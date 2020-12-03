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
#include <linux/leds.h>

#include "mdss_dsi.h"
#include "mdss_mdp.h"

#include "oem_mdss_dsi_common.h"
#include "oem_mdss_dsi.h"
#include "oem_mdss_dsi_panel.h"

#if defined (CONFIG_MFD_RT4832)
#include <linux/mfd/rt4832.h>
#endif

#if defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
#include <soc/qcom/lge/lge_boot_mode.h>
struct mdss_dsi_ctrl_pdata *cp_ctrl_pdata;
#endif

#if defined (CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
#include <linux/regulator/sm5107.h>
extern int sm5107_ctrl(int mode);
#endif

#if defined(CONFIG_MSM8909_K6P)
#include <soc/qcom/lge/lge_boot_mode.h>
#include <linux/input/lge_touch_notify.h>
#endif

#if IS_ENABLED(CONFIG_LGE_READER_MODE)
#include "lge/common/reader_mode.h"
#endif

#if IS_ENABLED(CONFIG_LGE_READER_MODE)
static struct dsi_panel_cmds reader_mode_step1_cmds;
static struct dsi_panel_cmds reader_mode_step2_cmds;
static struct dsi_panel_cmds reader_mode_step3_cmds;
static struct dsi_panel_cmds reader_mode_off_cmds;
//static struct dsi_panel_cmds reader_mode_mono_enable_cmds;
//static struct dsi_panel_cmds reader_mode_mono_disable_cmds;
#endif

#define PIN_DDVDH "DDVDH"
#define PIN_RESET "RESET"
#define PANEL_SEQUENCE(name, state) pr_info("[PanelSequence][%s] %d\n", name, state);

#ifdef CONFIG_TOUCHSCREEN_MIT300
extern void MIT300_Reset(int status, int delay);
#endif
bool oem_shutdown_pending;

int tianma_hvga_cmd_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		rc = gpio_request(ctrl_pdata->disp_en_gpio,
						"disp_enable");
		if (rc) {
			pr_err("request disp_en gpio failed, rc=%d\n",
				       rc);
			goto disp_en_gpio_err;
		}
	}

	rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
	if (rc) {
		pr_err("request reset gpio failed, rc=%d\n",
			rc);
		goto rst_gpio_err;
	}

	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
		rc = gpio_request(ctrl_pdata->bklt_en_gpio,
						"bklt_enable");
		if (rc) {
			pr_err("request bklt gpio failed, rc=%d\n",
				       rc);
			goto bklt_en_gpio_err;
		}
	}
	if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
		rc = gpio_request(ctrl_pdata->mode_gpio, "panel_mode");
		if (rc) {
			pr_err("request panel mode gpio failed,rc=%d\n",
								rc);
			goto mode_gpio_err;
		}
	}
	return rc;

mode_gpio_err:
	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
		gpio_free(ctrl_pdata->bklt_en_gpio);
bklt_en_gpio_err:
	gpio_free(ctrl_pdata->rst_gpio);
rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
disp_en_gpio_err:
	return rc;

}

int tianma_hvga_cmd_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
				PANEL_SEQUENCE(PIN_DDVDH, 1);
			}

		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		PANEL_SEQUENCE(PIN_RESET, 0);
		mdelay(5);

		for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
			gpio_set_value((ctrl_pdata->rst_gpio),
				pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
			if (pdata->panel_info.rst_seq[++i])
				usleep(pinfo->rst_seq[i] * 1000);
		}

		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 1);
		}

		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 0);
			gpio_free(ctrl_pdata->bklt_en_gpio);
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
			PANEL_SEQUENCE(PIN_DDVDH, 0);
			gpio_free(ctrl_pdata->disp_en_gpio);
		}

		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		PANEL_SEQUENCE(PIN_RESET, 0);
		gpio_free(ctrl_pdata->rst_gpio);

		if (gpio_is_valid(ctrl_pdata->mode_gpio))
			gpio_free(ctrl_pdata->mode_gpio);
	}
	return rc;

}

int jdi_qhd_dual_cmd_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	return rc;
}

int lgd_qhd_dual_cmd_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	return rc;
}

int tianma_hvga_cmd_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tianma_hvga_cmd_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{

	return 0;
}

int lgd_jdi_qhd_cmd_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{

	return 0;
}

#if defined(CONFIG_JDI_R69326_FWVGA_VIDEO_PANEL)
int jdi_fwvga_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	struct device_node *dsi_ctrl_np = NULL;
	struct platform_device *ctrl_pdev = NULL;
	//int rc;

	dsi_ctrl_np = of_parse_phandle(np,"qcom,mdss-dsi-panel-controller", 0);

	if (!dsi_ctrl_np) {
			pr_err("%s: Dsi controller node not initialized\n", __func__);
			return -EPROBE_DEFER;
	}

	ctrl_pdev = of_find_device_by_node(dsi_ctrl_np);

	if (!ctrl_pdev) {
			pr_err("%s: Invalid ctrl_pdev\n", __func__);
			return -EINVAL;
	}
	/*
	if (lge_get_mfts_mode()) {
			ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
			"lge,platform-mfts-ldo-en-gpio", 0);

			if (gpio_is_valid(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio)) {
					rc = gpio_request_one(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio,
											GPIOF_OUT_INIT_HIGH, "mfts_ldo_enable");

			if (rc) {
					pr_err("request mfts_ldo_enable gpio failed, rc=%d\n", rc);
			}
		} else {
				pr_err("%s:%d, VDDIO EN gpio for MFTS not specified\n",__func__, __LINE__);
		}
	}
*/
	return 0;
}

int jdi_fwvga_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int jdi_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata) /* TODO */
{
		int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (pinfo->cont_splash_enabled) {
		rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
		if (rc) {
			pr_err("request reset gpio failed, rc=%d\n",
					rc);
			goto rst_gpio_err;
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio,
					"disp_enable");
			if (rc)
				pr_err("request disp_en gpio failed, rc=%d\n", rc);
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_p_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_p_gpio, "dsv_p_enable");
			if (rc)
				pr_err("request disp_en_p gpio failed, rc=%d\n", rc);
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_n_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_n_gpio,
					"dsv_n_enable");
			if (rc)
				pr_err("request disp_en_n gpio failed, rc=%d\n", rc);
		}
	}

	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
		rc = gpio_request(ctrl_pdata->bklt_en_gpio,
				"bklt_enable");
		if (rc) {
			pr_err("request bklt gpio failed, rc=%d\n",
					rc);
			goto bklt_en_gpio_err;
		}
	}

	if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
		rc = gpio_request(ctrl_pdata->mode_gpio, "panel_mode");
		if (rc) {
			pr_err("request panel mode gpio failed,rc=%d\n",
					rc);
			goto mode_gpio_err;
		}
	}

	return rc;

mode_gpio_err:
	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
		gpio_free(ctrl_pdata->bklt_en_gpio);
bklt_en_gpio_err:
	gpio_free(ctrl_pdata->rst_gpio);
rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
	return rc;
}

int jdi_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable) /* TODO */
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, enable gpio is not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			mdelay(5);

			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}

			if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
				gpio_set_value((ctrl_pdata->bklt_en_gpio), 1);
		}

		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 0);
			gpio_free(ctrl_pdata->bklt_en_gpio);
		}

		if (oem_shutdown_pending) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_gpio);
			}

			if (gpio_is_valid(ctrl_pdata->disp_en_p_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_p_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_p_gpio);
			}

			if (gpio_is_valid(ctrl_pdata->disp_en_n_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_n_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_n_gpio);
			}
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			gpio_free(ctrl_pdata->rst_gpio);
		}
		if (gpio_is_valid(ctrl_pdata->mode_gpio))
			gpio_free(ctrl_pdata->mode_gpio);
	}
	return rc;
}
#endif

#if defined(CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
int lgd_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (pinfo->cont_splash_enabled) {
		rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
		if (rc) {
			pr_err("request reset gpio failed, rc=%d\n",
					rc);
			goto rst_gpio_err;
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio,
					"disp_enable");
			if (rc)
				pr_err("request disp_en gpio failed, rc=%d\n", rc);
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_p_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_p_gpio, "dsv_p_enable");
			if (rc)
				pr_err("request disp_en_p gpio failed, rc=%d\n", rc);
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_n_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_n_gpio,
					"dsv_n_enable");
			if (rc)
				pr_err("request disp_en_n gpio failed, rc=%d\n", rc);
		}
	}

	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
		rc = gpio_request(ctrl_pdata->bklt_en_gpio,
				"bklt_enable");
		if (rc) {
			pr_err("request bklt gpio failed, rc=%d\n",
					rc);
			goto bklt_en_gpio_err;
		}
	}

	if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
		rc = gpio_request(ctrl_pdata->mode_gpio, "panel_mode");
		if (rc) {
			pr_err("request panel mode gpio failed,rc=%d\n",
					rc);
			goto mode_gpio_err;
		}
	}

	return rc;

mode_gpio_err:
	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
		gpio_free(ctrl_pdata->bklt_en_gpio);
bklt_en_gpio_err:
	gpio_free(ctrl_pdata->rst_gpio);
rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
	return rc;
}

int lgd_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, enable gpio is not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
#if defined (CONFIG_MFD_RT4832)
			if (ctrl_pdata->dsv_toggle_enabled)
				rt4832_periodic_ctrl(enable);
#endif
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			mdelay(5);

			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}

			if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
				gpio_set_value((ctrl_pdata->bklt_en_gpio), 1);
		}

		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 0);
			gpio_free(ctrl_pdata->bklt_en_gpio);
		}

		if (oem_shutdown_pending) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_gpio);
			}

			if (gpio_is_valid(ctrl_pdata->disp_en_p_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_p_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_p_gpio);
			}

			if (gpio_is_valid(ctrl_pdata->disp_en_n_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_n_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_n_gpio);
			}
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			gpio_free(ctrl_pdata->rst_gpio);
		}
#if defined (CONFIG_MFD_RT4832)
		if (ctrl_pdata->dsv_toggle_enabled)
			rt4832_periodic_ctrl(enable);
#endif
		if (gpio_is_valid(ctrl_pdata->mode_gpio))
			gpio_free(ctrl_pdata->mode_gpio);
	}
	return rc;
}

int lgd_fwvga_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	struct device_node *dsi_ctrl_np = NULL;
	struct platform_device *ctrl_pdev = NULL;
	int rc;

	dsi_ctrl_np = of_parse_phandle(np,"qcom,mdss-dsi-panel-controller", 0);

	if (!dsi_ctrl_np) {
			pr_err("%s: Dsi controller node not initialized\n", __func__);
			return -EPROBE_DEFER;
	}

	ctrl_pdev = of_find_device_by_node(dsi_ctrl_np);

	if (!ctrl_pdev) {
			pr_err("%s: Invalid ctrl_pdev\n", __func__);
			return -EINVAL;
	}
	if (lge_get_mfts_mode()) {
			ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
			"lge,platform-mfts-ldo-en-gpio", 0);

			if (gpio_is_valid(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio)) {
					rc = gpio_request_one(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio,
											GPIOF_OUT_INIT_HIGH, "mfts_ldo_enable");

			if (rc) {
					pr_err("request mfts_ldo_enable gpio failed, rc=%d\n", rc);
			}
		} else {
				pr_err("%s:%d, VDDIO EN gpio for MFTS not specified\n",__func__, __LINE__);
		}
	}

	return 0;
}

int lgd_fwvga_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{

	return 0;
}
#elif defined (CONFIG_LGD_DB7400_HD_LH530WX2_VIDEO_PANEL)
int lgd_db7400_hd_lh530wx2_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		rc = gpio_request(ctrl_pdata->disp_en_gpio,
						"disp_enable");
		if (rc) {
			pr_err("request disp_en gpio failed, rc=%d\n",
				       rc);
		}
	}

	rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
	if (rc) {
		pr_err("request reset gpio failed, rc=%d\n",
			rc);
		goto rst_gpio_err;
	}

	if (gpio_is_valid(ctrl_pdata->touch_en_gpio)) {
		rc = gpio_request(ctrl_pdata->touch_en_gpio,
				"touch_enable");
		if (rc)
			pr_err("request touch_enable gpio failed, rc=%d\n", rc);
	}

	return rc;

rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
	return rc;

}

int lgd_db7400_hd_lh530wx2_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, enable gpio is not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return rc;
	}

	pinfo = &(ctrl_pdata->panel_data.panel_info);
	pr_info("%s: enable = %d\n", __func__, enable);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
			if (sm5107_ctrl(LCD_ON))
				pr_err("[LCD] SM5107 power on setting fail\n");

#ifdef CONFIG_TOUCHSCREEN_MIT300
			MIT300_Reset(0,2);
#else
			gpio_set_value((ctrl_pdata->touch_en_gpio), 0);
#endif

			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			mdelay(5);

			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}

#ifdef CONFIG_TOUCHSCREEN_MIT300
			MIT300_Reset(1, 50);
#else
			if (gpio_is_valid(ctrl_pdata->touch_en_gpio)){
				gpio_set_value((ctrl_pdata->touch_en_gpio), 1);
				mdelay(10);
			}
#endif
		}

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (oem_shutdown_pending) {
			if (sm5107_ctrl(LCD_OFF))
				pr_err("[LCD] SM5107 power off setting fail\n");
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			gpio_free(ctrl_pdata->rst_gpio);
			gpio_free(ctrl_pdata->touch_en_gpio);
			gpio_free(ctrl_pdata->disp_en_gpio);
		}
	}
	return rc;

}

int lgd_db7400_hd_lh530wx2_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{

	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->lge_pan_data->post_off_cmds,
		"qcom,mdss-dsi-post-off-command", "qcom,mdss-dsi-post-off-command-state");

	return 0;
}

int lgd_db7400_hd_lh530wx2_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int lgd_db7400_hd_lh530wx2_video_post_mdss_dsi_panel_off(struct mdss_panel_data *pdata)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct mdss_panel_info *pinfo;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	pinfo = &pdata->panel_info;
	ctrl = container_of(pdata, struct mdss_dsi_ctrl_pdata,
			panel_data);

	if (sm5107_ctrl(LCD_OFF_TOGGLE))
		pr_err("[LCD] SM5107 Knock on mode setting fail\n");

	if (ctrl->lge_pan_data->post_off_cmds.cmd_cnt){
		pr_info("Sending post off commands\n");
		mdss_dsi_panel_cmds_send(ctrl, &ctrl->lge_pan_data->post_off_cmds);
	} else {
		pr_info("Could not find post off commands\n");
	}

	pr_info("%s done\n", __func__);

	return 0;
}

#elif defined (CONFIG_LGD_DB7400_HD_5INCH_VIDEO_PANEL)
int lgd_db7400_hd_5inch_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
	if (rc) {
		pr_err("request reset gpio failed, rc=%d\n",
			rc);
		goto rst_gpio_err;
	}

	if (gpio_is_valid(ctrl_pdata->touch_en_gpio)) {
		rc = gpio_request(ctrl_pdata->touch_en_gpio,
				"touch_enable");
		if (rc)
			pr_err("request touch_enable gpio failed, rc=%d\n", rc);
	}

	return rc;

rst_gpio_err:
	return rc;

}

int lgd_db7400_hd_5inch_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;
	printk("lgd_db7400_hd_5inch_video_mdss_dsi_panel_reset\n");
	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
			panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_err("%s:%d, enable gpio is not configured\n",
				__func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
				__func__, __LINE__);
		return rc;
	}

	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

#ifdef CONFIG_TOUCHSCREEN_MIT300
			MIT300_Reset(0,2);
#else
			if (gpio_is_valid(ctrl_pdata->touch_en_gpio)){
				printk("touch_reset_low\n");
				gpio_set_value((ctrl_pdata->touch_en_gpio), 0);
				mdelay(50);
			}

#endif

		if (!pinfo->cont_splash_enabled) {
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			mdelay(5);

			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}

#ifdef CONFIG_TOUCHSCREEN_MIT300
			MIT300_Reset(1,50);
#else
			if (gpio_is_valid(ctrl_pdata->touch_en_gpio)){
				gpio_set_value((ctrl_pdata->touch_en_gpio), 0);
				mdelay(1);
				gpio_set_value((ctrl_pdata->touch_en_gpio), 1);
				mdelay(10);
			}
#endif
		}

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
					__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (oem_shutdown_pending) {
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			gpio_free(ctrl_pdata->rst_gpio);
		}
	}

	return rc;

}

int lgd_db7400_hd_5inch_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int lgd_db7400_hd_5inch_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

#elif defined(CONFIG_TCL_ILI9806E_FWVGA_VIDEO_PANEL)
int tcl_ili9806e_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		rc = gpio_request(ctrl_pdata->disp_en_gpio,
						"disp_enable");
		if (rc) {
			pr_err("request disp_en gpio failed, rc=%d\n",
				       rc);
			goto disp_en_gpio_err;
		}
	}

	rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
	if (rc) {
		pr_err("request reset gpio failed, rc=%d\n",
			rc);
		goto rst_gpio_err;
	}

	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
		rc = gpio_request(ctrl_pdata->bklt_en_gpio,
						"bklt_enable");
		if (rc) {
			pr_err("request bklt gpio failed, rc=%d\n",
				       rc);
			goto bklt_en_gpio_err;
		}
	}
	if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
		rc = gpio_request(ctrl_pdata->mode_gpio, "panel_mode");
		if (rc) {
			pr_err("request panel mode gpio failed,rc=%d\n",
								rc);
			goto mode_gpio_err;
		}
	}
	return rc;

mode_gpio_err:
	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
		gpio_free(ctrl_pdata->bklt_en_gpio);
bklt_en_gpio_err:
	gpio_free(ctrl_pdata->rst_gpio);
rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
disp_en_gpio_err:
	return rc;

}

int tcl_ili9806e_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
				panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			   __func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
				PANEL_SEQUENCE(PIN_DDVDH, 1);
			}

		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		PANEL_SEQUENCE(PIN_RESET, 0);
		mdelay(5);

		for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
			gpio_set_value((ctrl_pdata->rst_gpio),
				pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
			if (pdata->panel_info.rst_seq[++i])
				usleep(pinfo->rst_seq[i] * 1000);
		}

		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 1);
		}

		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
						__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 0);
			gpio_free(ctrl_pdata->bklt_en_gpio);
		}

		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
			PANEL_SEQUENCE(PIN_DDVDH, 0);
			gpio_free(ctrl_pdata->disp_en_gpio);
		}

		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		PANEL_SEQUENCE(PIN_RESET, 0);
		gpio_free(ctrl_pdata->rst_gpio);

		if (gpio_is_valid(ctrl_pdata->mode_gpio))
			gpio_free(ctrl_pdata->mode_gpio);
	}
	return rc;

}

int tcl_ili9806e_fwvga_video_mdss_panel_parse_dts(struct device_node *np,
			struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tcl_ili9806e_fwvga_video_panel_device_create(struct device_node *node,
	struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{

	return 0;
}
#elif defined(CONFIG_TCL_ONCELL_ILI9806E_FWVGA_VIDEO_PANEL)
extern bool lge_get_mfts_mode(void);
int tcl_oncell_ili9806e_fwvga_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		rc = gpio_request(ctrl_pdata->disp_en_gpio,
				"disp_enable");
		if (rc) {
			pr_err("request disp_en gpio failed, rc=%d\n",
					rc);
			goto disp_en_gpio_err;
		}
	}

	rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
	if (rc) {
		pr_err("request reset gpio failed, rc=%d\n",
				rc);
		goto rst_gpio_err;
	}

	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
		rc = gpio_request(ctrl_pdata->bklt_en_gpio,
				"bklt_enable");
		if (rc) {
			pr_err("request bklt gpio failed, rc=%d\n",
					rc);
			goto bklt_en_gpio_err;
		}
	}
	if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
		rc = gpio_request(ctrl_pdata->mode_gpio, "panel_mode");
		if (rc) {
			pr_err("request panel mode gpio failed,rc=%d\n",
					rc);
			goto mode_gpio_err;
		}
	}
	return rc;

mode_gpio_err:
	if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
		gpio_free(ctrl_pdata->bklt_en_gpio);
bklt_en_gpio_err:
	gpio_free(ctrl_pdata->rst_gpio);
rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
disp_en_gpio_err:
	return rc;

}

int tcl_oncell_ili9806e_fwvga_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;
	bool isMFTS = lge_get_mfts_mode();

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata,
			panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
				__func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
				__func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
#if defined(CONFIG_APDS9930_BMCHAL)
			if (isMFTS) {
				if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
					gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
					PANEL_SEQUENCE(PIN_DDVDH, 1);
				}
			}
#else
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
                                gpio_set_value((ctrl_pdata->disp_en_gpio), 1);
                                PANEL_SEQUENCE(PIN_DDVDH, 1);
                        }
#endif
			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}

			if (gpio_is_valid(ctrl_pdata->bklt_en_gpio))
				gpio_set_value((ctrl_pdata->bklt_en_gpio), 1);
		}

		if (gpio_is_valid(ctrl_pdata->mode_gpio)) {
			if (pinfo->mode_gpio_state == MODE_GPIO_HIGH)
				gpio_set_value((ctrl_pdata->mode_gpio), 1);
			else if (pinfo->mode_gpio_state == MODE_GPIO_LOW)
				gpio_set_value((ctrl_pdata->mode_gpio), 0);
		}
		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n",
					__func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		pr_err("%s: isMFTS=%d\n", __func__, isMFTS);
		if (gpio_is_valid(ctrl_pdata->bklt_en_gpio)) {
			gpio_set_value((ctrl_pdata->bklt_en_gpio), 0);
			gpio_free(ctrl_pdata->bklt_en_gpio);
		}
#if defined(CONFIG_APDS9930_BMCHAL)
		if (isMFTS) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
				PANEL_SEQUENCE(PIN_DDVDH, 0);
				gpio_free(ctrl_pdata->disp_en_gpio);
			}
		}
#else
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
			PANEL_SEQUENCE(PIN_DDVDH, 0);
			gpio_free(ctrl_pdata->disp_en_gpio);
		}
#endif
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
		PANEL_SEQUENCE(PIN_RESET, 0);
		gpio_free(ctrl_pdata->rst_gpio);

		if (gpio_is_valid(ctrl_pdata->mode_gpio))
			gpio_free(ctrl_pdata->mode_gpio);
	}
	return rc;

}

int tcl_oncell_ili9806e_fwvga_video_mdss_panel_parse_dts(struct device_node *np,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tcl_oncell_ili9806e_fwvga_video_panel_device_create(struct device_node *node,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{

	return 0;
}
#elif defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)
int tianma_ft860x_hd_video_dsv_ctrl(struct mdss_dsi_ctrl_pdata *ctrl_pdata,
		int enable)
{
	int ret = 0;
#if defined (CONFIG_MFD_RT4832)
	if (enable) {
		ret = rt4832_i2c_ctrl(RT4832_REG_DSV_DELAY, 0xA0);
		ret |= rt4832_i2c_ctrl(RT4832_REG_DSV_EXT_EN, 0x0A);
		ret |= rt4832_i2c_ctrl(RT4832_REG_VPOS_CP_CTRL, 0x48);
		mdelay(5);
	} else {
		ret = rt4832_i2c_ctrl(RT4832_REG_DSV_DELAY, 0xE0);
		ret |= rt4832_i2c_ctrl(RT4832_REG_DSV_EXT_EN, 0x0A);
		ret |= rt4832_i2c_ctrl(RT4832_REG_VPOS_CP_CTRL, 0x37);
		mdelay(5);
	}
#endif

	if (ret) {
		pr_info("%s write fail!! : %d\n", __func__, enable);
		ret = -1;
	}

	return ret;
}
int tianma_ft860x_hd_video_reset_ctrl(struct mdss_dsi_ctrl_pdata *ctrl_pdata,
		int enable)
{
	int rc = 0;

	if (ctrl_pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
				__func__, __LINE__);
		return -EINVAL;
	}

	if (enable) {
		gpio_set_value((ctrl_pdata->rst_gpio), 1);
	} else {
		gpio_set_value((ctrl_pdata->rst_gpio), 0);
	}

	return rc;
}

int tianma_ft860x_hd_video_cmd_transfer(struct mdss_dsi_ctrl_pdata *ctrl_pdata,
                int enable)
{
	int rc = 0;

	if (ctrl_pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	if (enable) {
		if (ctrl_pdata->on_cmds.cmd_cnt) {
			mdss_dsi_panel_cmds_send(ctrl_pdata, &ctrl_pdata->on_cmds);
			pr_info("%s: on cmds sent, link_state[%d] \n", __func__, ctrl_pdata->on_cmds.link_state);
		}
	} else {
		if (ctrl_pdata->off_cmds.cmd_cnt) {
			mdss_dsi_panel_cmds_send(ctrl_pdata, &ctrl_pdata->off_cmds);
			pr_info("%s: off cmds sent, link_state[%d] \n", __func__, ctrl_pdata->off_cmds.link_state);
		}
	}

	return rc;
}

int tianma_ft860x_hd_video_panel_external_api(int type, int enable)
{
	int rc = 0;
	switch(type) {
		case VDDI_CTRL:
			break;
		case AVDD_AVEE_CTRL:
			rc = tianma_ft860x_hd_video_dsv_ctrl(cp_ctrl_pdata, enable);
			break;
		case LCD_RESET_CTRL:
			rc = tianma_ft860x_hd_video_reset_ctrl(cp_ctrl_pdata, enable);
			break;
		case LCD_INIT_CMD_TRANSFER:
			rc = tianma_ft860x_hd_video_cmd_transfer(cp_ctrl_pdata, enable);
			break;
		case NONE:
		default:
			break;
	}

	return rc;
}

static atomic_t boot_fw_recovery = ATOMIC_INIT(0);

int get_boot_fw_recovery(void)
{
        return atomic_read(&boot_fw_recovery);
}

void set_boot_fw_recovery(int recovery)
{
        atomic_set(&boot_fw_recovery, recovery);
}

int lge_mdss_report_firmware_recovery(void)
{
	struct mdss_panel_data *pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	u32 backup_level;
	int rc = 0;

	pr_info("firmware update detected, LCD recovery function called\n");

	if (cp_ctrl_pdata == NULL) {
		pr_err("%s: no mdss_dsi_ctrl_pdata\n", __func__);
		return -EINVAL;
	}

	pdata = &(cp_ctrl_pdata->panel_data);
	if (pdata == NULL) {
		pr_err("%s: no panel_data\n", __func__);
		return -EINVAL;
	}

	pinfo = &(cp_ctrl_pdata->panel_data.panel_info);
	if (pinfo->cont_splash_enabled) {
		pr_info("%s: firmware recovery called before dsi init\n", __func__);
		set_boot_fw_recovery(1);
		return -EPERM;
	}

	backup_level = rt4832_lcd_backlight_get_level();
	if (pdata->set_backlight)
                pdata->set_backlight(pdata, 0);

	rc = tianma_ft860x_hd_video_panel_external_api(LCD_INIT_CMD_TRANSFER, 0);
	rc |= tianma_ft860x_hd_video_panel_external_api(LCD_RESET_CTRL, 1);
	mdelay(1);
	rc |= tianma_ft860x_hd_video_panel_external_api(LCD_RESET_CTRL, 0);
	mdelay(1);
	rc |= tianma_ft860x_hd_video_panel_external_api(LCD_RESET_CTRL, 1);
	mdelay(200);
	rc |= tianma_ft860x_hd_video_panel_external_api(LCD_INIT_CMD_TRANSFER, 1);

	if (backup_level < 0)
		backup_level = 0xAC;//default value:172
	if (pdata->set_backlight)
		pdata->set_backlight(pdata, backup_level);

	if (rc) {
		pr_err("%s: panel recovery fail\n", __func__);
		return -1;
	}
	return 0;
}

int tianma_ft860x_hd_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (pinfo->cont_splash_enabled) {
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio, "disp_enable");
			if (rc) {
				pr_err("request disp_en gpio failed, rc=%d\n", rc);
				goto disp_en_gpio_err;
			}
		}

		rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
		if (rc) {
			pr_err("request reset gpio failed, rc=%d\n", rc);
			goto rst_gpio_err;
		}
	}

	return rc;

rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
disp_en_gpio_err:
	return rc;
}

int tianma_ft860x_hd_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;
	int err = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_err("%s:%d, enable gpio is not configured\n",
			__func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			__func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
#if defined (CONFIG_MFD_RT4832)
			if ((lge_get_mfts_mode() && !get_mfts_lpwg())
				|| (get_esd_power_recovery() == ESD_NOK)) {
				err = rt4832_i2c_ctrl(RT4832_REG_DSV_DELAY, 0xA0);
				err |= rt4832_i2c_ctrl(RT4832_REG_DSV_EXT_EN, 0x0A);
				err |= rt4832_i2c_ctrl(RT4832_REG_VPOS_CP_CTRL, 0x48);
				pr_info("%s: mfts or esd recvoery dsv on [1]\n", __func__);
				mdelay(5);
				if (get_esd_power_recovery() == ESD_NOK)
					set_esd_power_recovery(ESD_OK);
			} else if (get_esd_power_recovery() == ESD_POWEROFF_PENDING) {
				err = rt4832_i2c_ctrl(RT4832_REG_DSV_DELAY, 0xE0);
				err |= rt4832_i2c_ctrl(RT4832_REG_DSV_EXT_EN, 0x0A);
				err |= rt4832_i2c_ctrl(RT4832_REG_VPOS_CP_CTRL, 0x37);
				pr_info("%s: mfts or esd recovery dsv off [0]\n", __func__);
				mdelay(5);
				err |= rt4832_i2c_ctrl(RT4832_REG_DSV_DELAY, 0xA0);
				err |= rt4832_i2c_ctrl(RT4832_REG_DSV_EXT_EN, 0x0A);
				err |= rt4832_i2c_ctrl(RT4832_REG_VPOS_CP_CTRL, 0x48);
				pr_info("%s: mfts or esd recvoery dsv on [1]\n", __func__);
				mdelay(5);
				set_esd_power_recovery(ESD_OK);
			}
#endif
			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}
		}

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n", __func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (oem_shutdown_pending) {
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			gpio_free(ctrl_pdata->rst_gpio);

			err = rt4832_i2c_ctrl(RT4832_REG_DSV_DELAY, 0xE0);
			err |= rt4832_i2c_ctrl(RT4832_REG_DSV_EXT_EN, 0x0A);
			err |= rt4832_i2c_ctrl(RT4832_REG_VPOS_CP_CTRL, 0x37);
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_gpio);
			}
			mdelay(1);
		}
#if defined (CONFIG_MFD_RT4832)
		else {
			if ((lge_get_mfts_mode() && !get_mfts_lpwg())
				|| get_esd_power_recovery()) {
				if (get_esd_power_recovery())
					set_esd_power_recovery(ESD_NOK);
				err = rt4832_i2c_ctrl(RT4832_REG_DSV_DELAY, 0xE0);
				err |= rt4832_i2c_ctrl(RT4832_REG_DSV_EXT_EN, 0x0A);
				err |= rt4832_i2c_ctrl(RT4832_REG_VPOS_CP_CTRL, 0x37);
				pr_info("%s: mfts or esd recovery dsv off [0]\n", __func__);
				mdelay(5);
			}
		}
#endif
	}

	return rc;
}

int tianma_ft860x_hd_video_mdss_panel_parse_dts(struct device_node *np,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	struct device_node *dsi_ctrl_np = NULL;
	struct platform_device *ctrl_pdev = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int rc;

	dsi_ctrl_np = of_parse_phandle(np,
		"qcom,mdss-dsi-panel-controller", 0);

	if (!dsi_ctrl_np) {
		pr_err("%s: Dsi controller node not initialized\n", __func__);
		return -EPROBE_DEFER;
	}

	ctrl_pdev = of_find_device_by_node(dsi_ctrl_np);

	if (!ctrl_pdev) {
		pr_err("%s: Invalid ctrl_pdev\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata->lge_pan_data->disp_vdd_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
		"lge,platform-disp-vdd-gpio", 0);
	if (gpio_is_valid(ctrl_pdata->lge_pan_data->disp_vdd_gpio)) {
		rc = gpio_request_one(ctrl_pdata->lge_pan_data->disp_vdd_gpio,
				GPIOF_OUT_INIT_HIGH, "disp_vdd_enable");
		if (rc) {
			pr_err("request disp_vdd_enable gpio failed, rc=%d\n", rc);
		}
	} else {
		pr_err("%s:%d, vdd gpio not specified\n",
						__func__, __LINE__);
	}

	if (lge_get_mfts_mode()) {
		ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio = of_get_named_gpio(ctrl_pdev->dev.of_node,
			"lge,platform-mfts-ldo-en-gpio", 0);
		if (gpio_is_valid(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio)) {
			rc = gpio_request_one(ctrl_pdata->lge_pan_data->mfts_ldo_en_gpio,
					GPIOF_OUT_INIT_LOW, "mfts_ldo_enable");
			if (rc) {
				pr_err("request mfts_ldo_enable gpio failed, rc=%d\n", rc);
			}
		} else {
			pr_err("%s:%d, VDDIO EN gpio for MFTS not specified\n",
							__func__, __LINE__);
		}
	}

	if (!lcd_connection_boot) {
		pinfo = &(ctrl_pdata->panel_data.panel_info);
		pinfo->esd_check_enabled = false;
		pinfo->ulps_suspend_enabled = false;
		pr_info("%s: Disable esd check and ulps suspend\n", __func__);
	} else if (lge_get_boot_mode() == LGE_BOOT_MODE_CHARGERLOGO || lge_get_mfts_mode()) {
		pinfo = &(ctrl_pdata->panel_data.panel_info);
		pinfo->esd_check_enabled = false;
		pr_info("%s: CHARGING MODE or mfts mode : Disable esd check\n", __func__);
	}

	cp_ctrl_pdata = ctrl_pdata;

	return 0;
}

int tianma_ft860x_hd_video_panel_device_create(struct device_node *node,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}
#elif defined(CONFIG_LGD_LG4894_HD_VIDEO_PANEL) || \
	defined(CONFIG_TVS_TD4100_HD_VIDEO_PANEL)
extern struct mdss_panel_data *lgd_lg4894_pdata_base;
int lgd_lg4894_hd_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (pinfo->cont_splash_enabled) {
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio, "disp_enable");
			if (rc) {
				pr_err("request disp_en gpio failed, rc=%d\n", rc);
			}
		}

		rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
		if (rc) {
			pr_err("request reset gpio failed, rc=%d\n", rc);
			goto rst_gpio_err;
		}
	}

	return rc;

rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
	return rc;
}
int lgd_lg4894_hd_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_err("%s:%d, enable gpio is not configured\n",
			__func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			__func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
			touch_notifier_call_chain(LCD_EVENT_TOUCH_RESET_START, NULL);
			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}
			touch_notifier_call_chain(LCD_EVENT_TOUCH_RESET_END, NULL);
			mdelay(5);
		}

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n", __func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (oem_shutdown_pending) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_gpio);
			}
			touch_notifier_call_chain(LCD_EVENT_TOUCH_RESET_START, NULL);
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			gpio_free(ctrl_pdata->rst_gpio);
		}
	}

	return rc;
}

int lgd_lg4894_hd_video_mdss_panel_parse_dts(struct device_node *np,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int i = 0;

	int lg4894_rev = dic_rev;
	static char common_lgd_lg4894_propname[MAX_LEN_OF_PROPNAME];

	struct mdss_panel_info *pinfo;

	if (!np || !ctrl_pdata) {
		pr_err("%s: Invalid arguments\n", __func__);
		return -ENODEV;
	}

	touch_osc_cmds_set = kzalloc(sizeof(struct touch_osc_cmds_desc), GFP_KERNEL);
	for (i = 0; i < TOUCH_OSC_CMDS; i++) {
		mdss_dsi_parse_dcs_cmds(np, &touch_osc_cmds_set->touch_osc_cmds[i],
			touch_osc_dt[i], "qcom,mdss-dsi-touch-osc-command-state");
	}

	if (dic_rev > LG4894_REV_UNKNOWN - 1) {
		pr_err("%s: Invalid DDIC rev data.\n", __func__);
		return -EINVAL;
	}

	// on-command
	memset(common_lgd_lg4894_propname, 0, sizeof(char) * MAX_LEN_OF_PROPNAME);
	sprintf(common_lgd_lg4894_propname, "%s%d", "qcom,mdss-dsi-on-command-rev", lg4894_rev);
	mdss_dsi_parse_dcs_cmds(np, &ctrl_pdata->on_cmds,
		common_lgd_lg4894_propname, "qcom,mdss-dsi-on-command-state");

	if (!lcd_connection_boot ||
		lge_get_boot_mode() == LGE_BOOT_MODE_CHARGERLOGO) {
		pinfo = &ctrl_pdata->panel_data.panel_info;
		pinfo->esd_check_enabled = false;
		pr_info("%s: Disable esd check\n", __func__);
	}

	return 0;
}

int lgd_lg4894_hd_video_panel_device_create(struct device_node *node,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int set_touch_osc(int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl = NULL;
	struct dsi_cmd_desc *cm;
	u32 osc_cmd, osc_data1, osc_data2;

	if (!lgd_lg4894_pdata_base) {
		pr_err("no panel connected!\n");
		return -EINVAL;
	}
	ctrl = container_of(lgd_lg4894_pdata_base, struct mdss_dsi_ctrl_pdata,
					panel_data);
	if (!ctrl) {
		pr_err("%s: ctrl is null\n", __func__);
		return -EINVAL;
	}

	if (touch_osc_cmds_set->touch_osc_cmds[enable].cmd_cnt) {
		cm = touch_osc_cmds_set->touch_osc_cmds[enable].cmds;
		osc_cmd = cm->payload[0];
		osc_data1 = cm->payload[1];
		cm++;
		osc_data2 = cm->payload[1];

		lgd_lg4894_hd_video_msm_mdss_enable_vreg(ctrl, MDSS_PANEL_POWER_ON);
		mdss_dsi_clk_ctrl(ctrl, DSI_ALL_CLKS, 1);

		if (lgd_lg4894_pdata_base->panel_info.panel_power_state == MDSS_PANEL_POWER_OFF) {
			touch_osc_cmds_set->touch_osc_cmds[enable].link_state = DSI_LP_MODE;
			mdss_dsi_sw_reset(ctrl, true);
		} else {
			touch_osc_cmds_set->touch_osc_cmds[enable].link_state = DSI_HS_MODE;
		}

		mdss_dsi_panel_cmds_send(ctrl, &touch_osc_cmds_set->touch_osc_cmds[enable]);

		mdss_dsi_clk_ctrl(ctrl, DSI_ALL_CLKS, 0);
		lgd_lg4894_hd_video_msm_mdss_enable_vreg(ctrl, MDSS_PANEL_POWER_OFF);

		pr_info("%s:request touch osc %s, panel=%s, cmd=0x%02X data=[0x%02X 0x%02X] link=%s\n", __func__,
				enable ? "on" : "off",
				lgd_lg4894_pdata_base->panel_info.panel_power_state ? "on" : "off",
				osc_cmd, osc_data1, osc_data2,
				touch_osc_cmds_set->touch_osc_cmds[enable].link_state ? "HS" : "LP"
		);
	}

	return 0;
}
EXPORT_SYMBOL(set_touch_osc);

int tvs_td4100_hd_video_mdss_dsi_request_gpios(struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	int rc = 0;
	struct mdss_panel_info *pinfo = NULL;
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (pinfo->cont_splash_enabled) {
		if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
			rc = gpio_request(ctrl_pdata->disp_en_gpio, "disp_enable");
			if (rc) {
				pr_err("request disp_en gpio failed, rc=%d\n", rc);
			}
		}

		rc = gpio_request(ctrl_pdata->rst_gpio, "disp_rst_n");
		if (rc) {
			pr_err("request reset gpio failed, rc=%d\n", rc);
			goto rst_gpio_err;
		}
	}

	return rc;

rst_gpio_err:
	if (gpio_is_valid(ctrl_pdata->disp_en_gpio))
		gpio_free(ctrl_pdata->disp_en_gpio);
	return rc;
}

int tvs_td4100_hd_video_mdss_dsi_panel_reset(struct mdss_panel_data *pdata, int enable)
{
	struct mdss_dsi_ctrl_pdata *ctrl_pdata = NULL;
	struct mdss_panel_info *pinfo = NULL;
	int i, rc = 0;

	if (pdata == NULL) {
		pr_err("%s: Invalid input data\n", __func__);
		return -EINVAL;
	}

	ctrl_pdata = container_of(pdata, struct mdss_dsi_ctrl_pdata, panel_data);

	if (!gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
		pr_err("%s:%d, enable gpio is not configured\n",
			__func__, __LINE__);
	}

	if (!gpio_is_valid(ctrl_pdata->rst_gpio)) {
		pr_debug("%s:%d, reset line not configured\n",
			__func__, __LINE__);
		return rc;
	}

	pr_info("%s: enable = %d\n", __func__, enable);
	pinfo = &(ctrl_pdata->panel_data.panel_info);

	if (enable) {
		if (lge_mdss_dsi.lge_mdss_dsi_request_gpios)
			lge_mdss_dsi.lge_mdss_dsi_request_gpios(ctrl_pdata);
		if (rc) {
			pr_err("gpio request failed\n");
			return rc;
		}

		if (!pinfo->cont_splash_enabled) {
			for (i = 0; i < pdata->panel_info.rst_seq_len; ++i) {
				gpio_set_value((ctrl_pdata->rst_gpio),
						pdata->panel_info.rst_seq[i]);
				PANEL_SEQUENCE(PIN_RESET, pdata->panel_info.rst_seq[i]);
				if (pdata->panel_info.rst_seq[++i])
					usleep(pinfo->rst_seq[i] * 1000);
			}
		}

		if (ctrl_pdata->ctrl_state & CTRL_STATE_PANEL_INIT) {
			pr_debug("%s: Panel Not properly turned OFF\n", __func__);
			ctrl_pdata->ctrl_state &= ~CTRL_STATE_PANEL_INIT;
			pr_debug("%s: Reset panel done\n", __func__);
		}
	} else {
		if (oem_shutdown_pending) {
			if (gpio_is_valid(ctrl_pdata->disp_en_gpio)) {
				gpio_set_value((ctrl_pdata->disp_en_gpio), 0);
				gpio_free(ctrl_pdata->disp_en_gpio);
			}
			gpio_set_value((ctrl_pdata->rst_gpio), 0);
			PANEL_SEQUENCE(PIN_RESET, 0);
			gpio_free(ctrl_pdata->rst_gpio);
		}
	}

	return rc;
}

int tvs_td4100_hd_video_mdss_panel_parse_dts(struct device_node *np,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}

int tvs_td4100_hd_video_panel_device_create(struct device_node *node,
		struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	return 0;
}
#endif

#if IS_ENABLED(CONFIG_LGE_READER_MODE)
int lge_mdss_dsi_parse_reader_mode_cmds(struct device_node *np, struct mdss_dsi_ctrl_pdata *ctrl_pdata)
{
	mdss_dsi_parse_dcs_cmds(np, &reader_mode_step1_cmds,
		"qcom,panel-reader-mode-step1-command", "qcom,mdss-dsi-reader-mode-command-state");
	mdss_dsi_parse_dcs_cmds(np, &reader_mode_step2_cmds,
		"qcom,panel-reader-mode-step2-command", "qcom,mdss-dsi-reader-mode-command-state");
	mdss_dsi_parse_dcs_cmds(np, &reader_mode_step3_cmds,
		"qcom,panel-reader-mode-step3-command", "qcom,mdss-dsi-reader-mode-command-state");
	mdss_dsi_parse_dcs_cmds(np, &reader_mode_off_cmds,
		"qcom,panel-reader-mode-off-command", "qcom,mdss-dsi-reader-mode-command-state");
	//mdss_dsi_parse_dcs_cmds(np, &reader_mode_mono_enable_cmds,
		//"qcom,panel-reader-mode-mono-enable-command", "qcom,mdss-dsi-reader-mode-command-state");
	//mdss_dsi_parse_dcs_cmds(np, &reader_mode_mono_disable_cmds,
		//"qcom,panel-reader-mode-mono-disable-command", "qcom,mdss-dsi-reader-mode-command-state");
	return 0;
}

int lge_mdss_dsi_panel_send_on_cmds(struct mdss_dsi_ctrl_pdata *ctrl, struct dsi_panel_cmds *default_on_cmds, int cur_mode)
{
	if (default_on_cmds->cmd_cnt) {
		msleep(50);
		mdss_dsi_panel_cmds_send(ctrl, default_on_cmds);
	} else if (cur_mode != READER_MODE_OFF) {
		msleep(50);
	}

	pr_info("%s: reader_mode (%d).\n", __func__, cur_mode);

	switch(cur_mode)
	{
		case READER_MODE_STEP_1:
			pr_info("%s: reader_mode STEP1\n", __func__);
			if (reader_mode_step1_cmds.cmd_cnt) {
				pr_info("%s: reader_mode_step1_cmds: cnt = %d \n",
					__func__, reader_mode_step1_cmds.cmd_cnt);
				mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step1_cmds);
			}
			break;
		case READER_MODE_STEP_2:
			pr_info("%s: reader_mode STEP2\n", __func__);
			if (reader_mode_step2_cmds.cmd_cnt) {
				pr_info("%s: reader_mode_step2_cmds: cnt = %d \n",
					__func__, reader_mode_step2_cmds.cmd_cnt);
				mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step2_cmds);
			}
			break;
		case READER_MODE_STEP_3:
			pr_info("%s: reader_mode STEP3\n", __func__);
			if (reader_mode_step3_cmds.cmd_cnt) {
				pr_info("%s: reader_mode_step3_cmds: cnt = %d \n",
					__func__, reader_mode_step3_cmds.cmd_cnt);
				mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step3_cmds);
			}
			break;
		case READER_MODE_MONO:
			pr_info("%s: reader_mode MONO \n", __func__);
			/*
			if(lge_get_panel_type() == PH2_JDI) {
				if (reader_mode_initial_mono_enable_cmds.cmd_cnt) {
					pr_info("%s: reader_mode_mono_enable_cmds: cnt = %d \n",
						__func__, reader_mode_initial_mono_enable_cmds.cmd_cnt);
					mdss_dsi_panel_cmds_send(ctrl, &reader_mode_initial_mono_enable_cmds, CMD_REQ_COMMIT);
				}
			} else if(lge_get_panel_type() == PH2_SHARP) {
				if (reader_mode_initial_step2_cmds.cmd_cnt) {
					// 5500K
					pr_info("%s: reader_mode_initial_step2_cmds: cnt = %d \n",
						__func__, reader_mode_initial_step2_cmds.cmd_cnt);
					mdss_dsi_panel_cmds_send(ctrl, &reader_mode_initial_step2_cmds, CMD_REQ_COMMIT);
				}
			}
			*/
			if (reader_mode_step2_cmds.cmd_cnt) {
				pr_info("%s: reader_mode_step2_cmds: cnt = %d \n",
					__func__, reader_mode_step2_cmds.cmd_cnt);
				mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step2_cmds);
			}
			break;
		case READER_MODE_OFF:
		default:
			break;
	}

	return 0;
}

bool lge_change_reader_mode(struct mdss_dsi_ctrl_pdata *ctrl, int old_mode, int new_mode)
{
	if(new_mode == READER_MODE_OFF) {
		switch(old_mode)
		{
			case READER_MODE_STEP_1:
			case READER_MODE_STEP_2:
			case READER_MODE_STEP_3:
				if(reader_mode_off_cmds.cmd_cnt) {
					pr_info("%s: sending reader mode OFF commands: cnt = %d\n",
						__func__, reader_mode_off_cmds.cmd_cnt);
					//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
					mdss_dsi_panel_cmds_send(ctrl, &reader_mode_off_cmds);
					//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
				}
			    break;
			case READER_MODE_MONO:
				if(reader_mode_off_cmds.cmd_cnt) {
					pr_info("%s: sending reader mode OFF commands: cnt = %d\n",
						__func__, reader_mode_off_cmds.cmd_cnt);
					//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
					mdss_dsi_panel_cmds_send(ctrl, &reader_mode_off_cmds);
					//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
				}
				break;
			default:
				pr_err("%s: Invalid old status : %d\n", __func__, old_mode);
				break;
		}
	} else {
		switch(old_mode)
		{
			case READER_MODE_OFF:
			case READER_MODE_STEP_1:
			case READER_MODE_STEP_2:
			case READER_MODE_STEP_3:
				if(old_mode == new_mode)
				{
					pr_info("%s: Same as older mode\n", __func__);
					break;
				}
				//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
				switch(new_mode)
				{
					case READER_MODE_STEP_1:
						if(reader_mode_step1_cmds.cmd_cnt) {
							pr_info("%s: sending reader mode STEP1 commands: cnt = %d\n",
								__func__, reader_mode_step1_cmds.cmd_cnt);
							mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step1_cmds);
						}
						break;
					case READER_MODE_STEP_2:
						if(reader_mode_step2_cmds.cmd_cnt) {
							pr_info("%s: sending reader mode STEP2 commands: cnt = %d\n",
								__func__, reader_mode_step2_cmds.cmd_cnt);
							mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step2_cmds);
						}
						break;
					case READER_MODE_STEP_3:
						if (reader_mode_step3_cmds.cmd_cnt) {
							pr_info("%s: sending reader mode STEP3 commands: cnt = %d\n",
								__func__, reader_mode_step3_cmds.cmd_cnt);
							mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step3_cmds);
						}
						break;
					case READER_MODE_MONO:
						// D-IC does not support MONO Mode
						if (reader_mode_step2_cmds.cmd_cnt) {
							pr_info("%s: sending reader mode STEP2 commands: cnt : %d \n",
								__func__, reader_mode_step2_cmds.cmd_cnt);
							mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step2_cmds);
						}
						break;
					default:
						pr_err("%s: Input Invalid parameter: %d \n", __func__, new_mode);
						break;
				}
				//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
				break;
			case READER_MODE_MONO:
				//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_ON);
				switch(new_mode)
				{
					case READER_MODE_STEP_1:
						if (reader_mode_step1_cmds.cmd_cnt) {
							pr_info("%s: sending reader mode STEP1 commands: cnt : %d \n",
								__func__, reader_mode_step1_cmds.cmd_cnt);
							mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step1_cmds);
						}
						break;
					case READER_MODE_STEP_2:
						if (reader_mode_step2_cmds.cmd_cnt) {
							pr_info("%s: sending reader mode STEP2 commands: cnt : %d \n",
								__func__, reader_mode_step2_cmds.cmd_cnt);
							mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step2_cmds);
						}
						break;
					case READER_MODE_STEP_3:
						if (reader_mode_step3_cmds.cmd_cnt) {
							pr_info("%s: sending reader mode STEP3 commands: cnt = %d \n",
								__func__, reader_mode_step3_cmds.cmd_cnt);
							mdss_dsi_panel_cmds_send(ctrl, &reader_mode_step3_cmds);
						}
						break;
					case READER_MODE_MONO:
						break;
					default:
						pr_err("%s: Input Invalid parameter : %d \n", __func__, new_mode);
						break;
				}
				//mdss_mdp_clk_ctrl(MDP_BLOCK_POWER_OFF);
				break;
			default:
				pr_err("%s: Invalid old status : %d\n", __func__, old_mode);
				break;
		}
	}

	return true;
}
#endif
