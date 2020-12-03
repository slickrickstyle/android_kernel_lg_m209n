/*
 * RT4832 Backlight Driver
 *
 * Copyright 2015 LG Electronics Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */
#include <linux/backlight.h>
#include <linux/delay.h>
#include <linux/err.h>
#include <linux/mfd/rt4832.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/platform_device.h>
#include <linux/pwm.h>
#include <linux/slab.h>

#ifdef CONFIG_LGE_DISPLAY_LCD_OFF_DIMMING
#include <soc/qcom/lge/lge_boot_mode.h>
#include <soc/qcom/lge/lge_cable_detection.h>
#include <soc/qcom/lge/lge_battery_id_checker.h>
extern bool fb_blank_called;
#endif

#define DEFAULT_BL_NAME			"lcd-backlight"

struct rt4832_bl {
	struct device *dev;
	struct backlight_device *bl_dev;

	struct rt4832 *rt4832;
	struct rt4832_backlight_platform_data *pdata;

	struct pwm_device *pwm;
};

#define BL_OFF 0
#define BL_ON 1
static int backlight_status = BL_OFF;

struct backlight_device *rt4832_device;
static int cur_main_lcd_level;

static int rt4832_bl_enable(struct rt4832_bl *rt4832_bl, int enable)
{
	backlight_status = enable;
	pr_info("%s:%s\n", __func__, enable?"on":"off");

	rt4832_update_bits(rt4832_bl->rt4832, RT4832_REG_LED_CTRL, RT4832_BL_EN_MASK, enable);

	if(enable)
		rt4832_bl->rt4832->pdata->power_state |= BIT(0);
	else
		rt4832_bl->rt4832->pdata->power_state &= ~BIT(0);

	return 0;
}

#ifdef CONFIG_BACKLIGHT_RT4832_WITH_SW_FREQ
static int rt4832_bl_set_sw_freq(struct rt4832_bl *rt4832_bl, int frequency)
{
	/*
		Register : 0x03[7]
		frequency 0 : set switching frequency to 500kHz
		frequency 1 : set switching frequency to 1MHz
	*/
	rt4832_update_bits(rt4832_bl->rt4832, RT4832_REG_BL_OSC,
							RT4832_BL_SW_FREQ_MASK, frequency);
	pr_info("%s:set bl switching frequency to %s\n",
				__func__, frequency ? "1MHz" : "500KHz");
	return 0;
}
#endif

static inline int rt4832_bl_set_brightness(struct rt4832_bl *rt4832_bl, int val)
{
	u8 data;
	int ret;

	cur_main_lcd_level = val;

	data = (val >> RT4832_BRT_MSB_SHIFT) & RT4832_BRT_MSB_MASK;
	ret = rt4832_write_byte(rt4832_bl->rt4832, RT4832_REG_BRT_MSB, data);

	data = val & 0xFF;
	ret = rt4832_write_byte(rt4832_bl->rt4832, RT4832_REG_BRT_LSB, data);

	return ret;
}

#ifdef CONFIG_LGE_DISPLAY_LCD_OFF_DIMMING
bool check_factory_boot(void){
	int boot_id;
	boot_id = lge_get_boot_mode();

	switch(boot_id){
		case LGE_BOOT_MODE_QEM_56K:
		case LGE_BOOT_MODE_QEM_910K:
		case LGE_BOOT_MODE_PIF_56K:
		case LGE_BOOT_MODE_PIF_910K:
			pr_err("BL : factory boot\n");
			return true;
		default:
			pr_err("BL : normal boot\n");
			return false;
	}
}
#endif

void rt4832_lcd_backlight_set_level(int level) {
	struct rt4832_bl *rt4832_bl = bl_get_data(rt4832_device);
	int ret = 0;
	struct rt4832_backlight_platform_data *pdata = rt4832_bl->pdata;
	int cal_level;

	pr_info("%s : %d\n",__func__, level);
#ifdef CONFIG_LGE_DISPLAY_LCD_OFF_DIMMING
	if((lge_get_bootreason() == FOTA_REBOOT_LCDOFF && level != 0) ||
			(lge_get_bootreason() == FOTA_REBOOT_OUT_LCDOFF && !fb_blank_called && level != 0)) {
		pr_info("lcd dimming reboot: change to min-brightness, %d\n", RT4832_MIN_BRIGHTNESS);
		level = RT4832_MIN_BRIGHTNESS;
	} else if((read_lge_battery_id() == BATT_NOT_PRESENT) &&
									 check_factory_boot() && !fb_blank_called) {
		level = RT4832_MIN_BRIGHTNESS;
		pr_info("No Battery and Factory cable is connected. set LCD brightness minimum = %d\n",level);
	} else {
		 rt4832_bl->bl_dev->props.brightness = level;
	}
#else
	rt4832_bl->bl_dev->props.brightness = level;
#endif

	if (level == 0) {
		if (backlight_status == BL_ON)
		{
#ifdef CONFIG_BACKLIGHT_RT4832_WITH_SW_FREQ
			rt4832_bl_set_sw_freq(rt4832_bl, 0);
#endif
			ret = rt4832_bl_enable(rt4832_bl, 0);
		}
		return;
	} else{
		if (backlight_status == BL_OFF)
		{
#if defined (CONFIG_LGD_SSD2068_FWVGA_VIDEO_PANEL)
			mdelay(100);
#endif
			ret = rt4832_bl_enable(rt4832_bl, 1);
#ifdef CONFIG_BACKLIGHT_RT4832_WITH_SW_FREQ
			mdelay(30);
			rt4832_bl_set_sw_freq(rt4832_bl, 1);
#endif
		}
	}
	if (ret) {
		pr_err("%s DEBUG error enable or disable backlight\n", __func__);
	}

	if (level >= pdata->blmap_size)
		level = pdata->blmap_size - 1;

	if(pdata->blmap)
		cal_level = pdata->blmap[level];
	else
		cal_level = level;

	if (cal_level == cur_main_lcd_level)
		return;

	ret = rt4832_bl_set_brightness(rt4832_bl, cal_level);
	if (ret) {
		pr_err("%s DEBUG error set backlight\n", __func__);
	}
}
EXPORT_SYMBOL(rt4832_lcd_backlight_set_level);

int rt4832_lcd_backlight_get_level(void)
{
	if (rt4832_device == NULL) {
		return -ENOMEM;
	} else {
		return rt4832_device->props.brightness;
	};
}
EXPORT_SYMBOL(rt4832_lcd_backlight_get_level);

void rt4832_set_dsv_addressC(void) {
	struct rt4832_bl *rt4832_bl = bl_get_data(rt4832_device);

	rt4832_write_byte(rt4832_bl->rt4832,RT4832_REG_VPOS_CP_CTRL, 0x01);
}
EXPORT_SYMBOL(rt4832_set_dsv_addressC);

static int rt4832_bl_update_status(struct backlight_device *bl_dev)
{
	rt4832_lcd_backlight_set_level(bl_dev->props.brightness);

	return 0;
}

static int rt4832_bl_get_brightness(struct backlight_device *bl_dev)
{
	return bl_dev->props.brightness;
}

static void rt4832_lcd_backlight_set_level_nomapping(int level){
	struct rt4832_bl *rt4832_bl = bl_get_data(rt4832_device);
	int ret;
	pr_err("%s : level is %d\n",__func__, level);

	if(level > 2047)
		level = 2047;

	ret = rt4832_bl_set_brightness(rt4832_bl, level);
	if (ret){
		pr_err("%s DEBUG error set backlight\n",__func__);
	}
}

static const struct backlight_ops rt4832_bl_ops = {
	.options = BL_CORE_SUSPENDRESUME,
	.update_status = rt4832_bl_update_status,
	.get_brightness = rt4832_bl_get_brightness,
};

static int rt4832_bl_register(struct rt4832_bl *rt4832_bl)
{
	struct backlight_device *bl_dev;
	struct backlight_properties props;
	struct rt4832_backlight_platform_data *pdata = rt4832_bl->pdata;
	char name[20];

	props.type = BACKLIGHT_PLATFORM;
#ifdef CONFIG_LGE_DISPLAY_LCD_OFF_DIMMING
	if(lge_get_bootreason() == FOTA_REBOOT_LCDOFF || lge_get_bootreason() == FOTA_REBOOT_OUT_LCDOFF ) {
		pr_info("%s : lcd dimming reboot: change to min-brightness \n", __func__);
		props.brightness = RT4832_MIN_BRIGHTNESS;
	} else {
		props.brightness = pdata ? pdata->init_brightness : RT4832_MAX_BRIGHTNESS;
	}
#else
	props.brightness = pdata ? pdata->init_brightness : RT4832_MAX_BRIGHTNESS;
#endif
	props.max_brightness = RT4832_MAX_BRIGHTNESS;

	if (!pdata || !pdata->name)
		snprintf(name, sizeof(name), "%s", DEFAULT_BL_NAME);
	else
		snprintf(name, sizeof(name), "%s", pdata->name);

	bl_dev = backlight_device_register(name, rt4832_bl->dev, rt4832_bl,
					   &rt4832_bl_ops, &props);
	rt4832_device = bl_dev;
	if (IS_ERR(bl_dev))
		return PTR_ERR(bl_dev);

	rt4832_bl->bl_dev = bl_dev;

	return 0;
}

static void rt4832_bl_unregister(struct rt4832_bl *rt4832_bl)
{
	if (rt4832_bl->bl_dev)
		backlight_device_unregister(rt4832_bl->bl_dev);
}

static ssize_t lcd_backlight_show_on_off(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r = 0;

	r = snprintf(buf, PAGE_SIZE, "LCD Backlight is : %s\n",
			backlight_status?"ON":"OFF");

	return r;
}

static void rt4832_lcd_power_direct(int onoff){
	struct rt4832_bl *rt4832_bl = bl_get_data(rt4832_device);
	int ret;

	ret = rt4832_bl_enable(rt4832_bl, onoff);
	if (ret){
		pr_err("%s DEBUG error backlight power ctrl\n",__func__);
	}
}

static ssize_t lcd_backlight_store_on_off(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int on_off;

	if (!count)
		return -EINVAL;

	on_off = simple_strtoul(buf, NULL, 10);
	rt4832_lcd_power_direct(on_off);

	return count;
}
DEVICE_ATTR(bl_power, 0644, lcd_backlight_show_on_off,
		lcd_backlight_store_on_off);

static ssize_t lcd_backlight_show_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r = 0;

	r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n",
			cur_main_lcd_level);

	return r;
}

static ssize_t lcd_backlight_store_level(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int level;

	if(!count)
		return -EINVAL;

	level = simple_strtoul(buf, NULL, 10);
	rt4832_lcd_backlight_set_level_nomapping(level);

	return count;
}
DEVICE_ATTR(bl_level, 0644, lcd_backlight_show_level,
		lcd_backlight_store_level);

static int rt4832_bl_parse_dt(struct device *dev, struct rt4832_bl *rt4832_bl)
{
	struct device_node *node = dev->of_node;
	struct rt4832_backlight_platform_data *pdata;
	int i;
	u32* array;

	pdata = devm_kzalloc(dev, sizeof(*pdata), GFP_KERNEL);
	if (!pdata)
		return -ENOMEM;

	of_property_read_string(node, "bl-name", &pdata->name);
	of_property_read_u32(node, "initial-brightness",
			    (u32 *)&pdata->init_brightness);

	of_property_read_u32(node, "blmap_size", &pdata->blmap_size);

	if(pdata->blmap_size){
		array = kzalloc(sizeof(u32) * pdata->blmap_size, GFP_KERNEL);
		if (!array) {
			pr_err("no more mem for array\n");
			return -ENOMEM;
		}
		of_property_read_u32_array(node, "blmap", array, pdata->blmap_size);
		pdata->blmap = kzalloc(sizeof(u16) * pdata->blmap_size, GFP_KERNEL);
		if (!pdata->blmap){
			pr_err("no more mem for blmap\n");
			kfree(array);
			return -ENOMEM;
		}

		for (i = 0; i < pdata->blmap_size; i++)
			pdata->blmap[i] = (u16)array[i];

		kfree(array);
	}else{
		pr_err("not defined blmap_size");
	}
	rt4832_bl->pdata = pdata;

	return 0;
}

static int rt4832_bl_probe(struct platform_device *pdev)
{
	struct rt4832 *rt4832 = dev_get_drvdata(pdev->dev.parent);
	struct rt4832_backlight_platform_data *pdata = rt4832->pdata->bl_pdata;
	struct rt4832_bl *rt4832_bl;
	int ret;

	rt4832_bl = devm_kzalloc(&pdev->dev, sizeof(*rt4832_bl), GFP_KERNEL);
	if (!rt4832_bl)
		return -ENOMEM;

	rt4832_bl->pdata = pdata;
	if (!rt4832_bl->pdata) {
		if (IS_ENABLED(CONFIG_OF))
			ret = rt4832_bl_parse_dt(&pdev->dev, rt4832_bl);
		else
			return -ENODEV;

		if (ret)
			return ret;
	}

	rt4832_bl->dev = &pdev->dev;
	rt4832_bl->rt4832 = rt4832;
	platform_set_drvdata(pdev, rt4832_bl);

	ret = rt4832_bl_register(rt4832_bl);
	if (ret) {
		dev_err(&pdev->dev, "register backlight err: %d\n", ret);
		return ret;
	}

	device_create_file(&pdev->dev, &dev_attr_bl_level);
	device_create_file(&pdev->dev, &dev_attr_bl_power);

	backlight_status = BL_ON;
	backlight_update_status(rt4832_bl->bl_dev);

	return 0;
}

static int rt4832_bl_remove(struct platform_device *pdev)
{
	struct rt4832_bl *rt4832_bl = platform_get_drvdata(pdev);
	struct backlight_device *bl_dev = rt4832_bl->bl_dev;

	bl_dev->props.brightness = 0;
	backlight_update_status(bl_dev);
	rt4832_bl_unregister(rt4832_bl);

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id rt4832_bl_of_match[] = {
	{ .compatible = "richtek,rt4832-backlight", },
	{ }
};
MODULE_DEVICE_TABLE(of, rt4832_bl_of_match);
#endif

static struct platform_driver rt4832_bl_driver = {
	.probe = rt4832_bl_probe,
	.remove = rt4832_bl_remove,
	.driver = {
		.name = "rt4832-backlight",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(rt4832_bl_of_match),
	},
};
module_platform_driver(rt4832_bl_driver);

MODULE_DESCRIPTION("Richtek RT4832 Backlight Driver");
MODULE_AUTHOR("YJ Kim");
MODULE_LICENSE("GPL");
MODULE_ALIAS("platform:rt4832-backlight");
