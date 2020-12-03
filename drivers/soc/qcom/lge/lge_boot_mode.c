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

#include <soc/qcom/lge/lge_boot_mode.h>

#ifdef CONFIG_LGE_USB_G_ANDROID
#include <linux/platform_data/lge_android_usb.h>
#include <linux/platform_device.h>
#endif
/* get boot mode information from cmdline.
 * If any boot mode is not specified,
 * boot mode is normal type.
 */
static enum lge_boot_mode_type lge_boot_mode = LGE_BOOT_MODE_NORMAL;
static int __init lge_boot_mode_init(char *s)
{
	if (!strcmp(s, "charger"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGER;
	else if (!strcmp(s, "chargerlogo"))
		lge_boot_mode = LGE_BOOT_MODE_CHARGERLOGO;
	else if (!strcmp(s, "qem_56k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_56K;
	else if (!strcmp(s, "qem_130k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_130K;
	else if (!strcmp(s, "qem_910k"))
		lge_boot_mode = LGE_BOOT_MODE_QEM_910K;
	else if (!strcmp(s, "pif_56k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_56K;
	else if (!strcmp(s, "pif_130k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_130K;
	else if (!strcmp(s, "pif_910k"))
		lge_boot_mode = LGE_BOOT_MODE_PIF_910K;
	else if (!strcmp(s, "miniOS"))
		lge_boot_mode = LGE_BOOT_MODE_MINIOS;
	pr_info("ANDROID BOOT MODE : %d %s\n", lge_boot_mode, s);

	return 1;
}
__setup("androidboot.mode=", lge_boot_mode_init);

#if defined (CONFIG_MACH_MSM8909_E1Q_VZW) || defined (CONFIG_MACH_MSM8909_CLING_VZW)
unsigned int touch_module;
EXPORT_SYMBOL(touch_module);

static int __init touch_module_check(char *touch)
{
	if(!strncmp(touch,"PRIMARY_MODULE", 14)) {
		touch_module = 0;
		printk("touch_module 0\n");
	} else if(!strncmp(touch, "SECONDARY_MODULE", 16)) {
		touch_module = 1;
		printk("touch_module 1\n");
	} else if(!strncmp(touch, "TERITARY_MODULE", 15)) {
		touch_module = 2;
	} else if(!strncmp(touch, "QUATENARY_MODULE", 16)) {
		touch_module = 3;
	}

	printk("[TOUCH] kernel touch module check : %s\n", touch);
	return 0;
}
__setup("lge.touchModule=", touch_module_check);
#endif

#if defined (CONFIG_TIANMA_FT860X_HD_VIDEO_PANEL)	// MSM8909 K6B
unsigned int ft860x_revision_id;
EXPORT_SYMBOL(ft860x_revision_id);

static int __init ft860x_revision_check(char *module)
{
	sscanf(module, "%d", &ft860x_revision_id);
	printk("%s[%d]\n", __func__, ft860x_revision_id);

	return 0;
}
__setup("lge.ft860xRevision=", ft860x_revision_check);

bool lcd_connection_boot;
EXPORT_SYMBOL(lcd_connection_boot);

static int __init boot_with_lcd_check(char *connection)
{
	if (!strcmp(connection, "connect")) {
		lcd_connection_boot = true;
	} else {
		lcd_connection_boot = false;
	}
	printk("%s: %s\n", __func__, lcd_connection_boot? "connect" : "disconnect");

	return 0;
}
__setup("lge.bootWithLcd=", boot_with_lcd_check);
#endif

#if defined(CONFIG_MSM8909_K6P)     // MSM8909 K6P
unsigned int lcd_maker = 0;
EXPORT_SYMBOL(lcd_maker);

static int __init lcd_maker_check(char *lm)
{
	sscanf(lm, "%d", &lcd_maker);
	printk("lcd_maker=%d %s\n", lcd_maker, (lcd_maker == 0) ? "LGD" : "TVS");

	return 0;
}
__setup("lge.panel=", lcd_maker_check);

unsigned int dic_rev = 0;
EXPORT_SYMBOL(dic_rev);

static int __init dic_revision_check(char *module)
{
	if (lcd_maker == 0) {
		sscanf(module, "%d", &dic_rev);
		printk("dic_rev=%d\n", dic_rev);
	}
	return 0;
}
__setup("lge.dic_rev=", dic_revision_check);

bool lcd_connection_boot;
EXPORT_SYMBOL(lcd_connection_boot);

static int __init boot_with_lcd_check(char *connection)
{
	if (!strcmp(connection, "connect")) {
		lcd_connection_boot = true;
	} else {
		lcd_connection_boot = false;
	}
	printk("%s: %s\n", __func__, lcd_connection_boot? "connect" : "disconnect");

	return 0;
}
__setup("lge.bootWithLcd=", boot_with_lcd_check);
#endif

int check_recovery_boot = 0;
static int __init lge_check_recoveryboot(char *reason)
{
	if (!strcmp(reason, "true")) {
		check_recovery_boot = LGE_RECOVERY_BOOT;
	}

	if (check_recovery_boot == LGE_RECOVERY_BOOT) {
		pr_info("LGE RECOVERY BOOT : %d\n", check_recovery_boot);
	}

	return 0;
}
__setup("androidboot.recovery=", lge_check_recoveryboot);

int is_mfts_mode = 0;
EXPORT_SYMBOL(is_mfts_mode);

static int __init lge_mfts_mode_init(char *s)
{
	if(strncmp(s, "1", 1) == 0)
		is_mfts_mode=1;
printk("[TOUCH] kernel bootmode check for mfts : %d\n", is_mfts_mode);
return 0;
}
__setup("mfts.mode=", lge_mfts_mode_init);

bool lge_get_mfts_mode(void)
{
	return is_mfts_mode;
}

enum lge_boot_mode_type lge_get_boot_mode(void)
{
	return lge_boot_mode;
}
#if defined (CONFIG_TIANMA_ILI9488_HVGA_CMD_PANEL) || defined (CONFIG_MACH_MSM8909_K6B_LGU_KR) || defined (CONFIG_MSM8909_K6P)
static enum lge_boot_reason_type lge_boot_reason = UNKNOWN_REASON; /*  undefined for error checking */
static int __init lge_check_bootreason(char *reason)
{

	if (!strcmp(reason, "FOTA_Reboot")) {
		lge_boot_reason = FOTA_REBOOT;
#ifdef CONFIG_LGE_DISPLAY_LCD_OFF_DIMMING
	} else if (!strcmp(reason, "FOTA_Reboot_LCDOFF")) {
		lge_boot_reason = FOTA_REBOOT_LCDOFF;
	} else if (!strcmp(reason, "FOTA_Reboot_OUT_LCDOFF")) {
		lge_boot_reason = FOTA_REBOOT_OUT_LCDOFF;
#endif
	} else if (!strcmp(reason, "Recovery_mode") || (!strcmp(reason, "Key_recovery_mode"))) {
		lge_boot_reason = RECOVERY_MODE;
	} else if (!strcmp(reason, "Hidden_factory_reset")) {
		lge_boot_reason = HIDDEN_FACTORY_RESET;
	}

	if(lge_boot_reason == UNKNOWN_REASON) {
		pr_info("LGE REBOOT REASON: Couldn't get bootreason : %d\n", lge_boot_reason);
	} else {
		pr_info("LGE REBOOT REASON : %d %s\n", lge_boot_mode, reason);
	}

	return 1;
}
__setup("lge.bootreason=", lge_check_bootreason);

enum lge_boot_reason_type lge_get_bootreason(void)
{
	return lge_boot_reason;
}

static int check_fota_boot = 0;
static int __init lge_check_fotaboot(char *reason)
{

	if (!strcmp(reason, "true")) {
		check_fota_boot = LGE_FOTA_BOOT;
	}

	if(check_fota_boot == LGE_FOTA_BOOT) {
		pr_info("LGE FOTA BOOT : %d\n", check_fota_boot);
	}

	return 0;
}
__setup("androidboot.fota=", lge_check_fotaboot);

int lge_fota_boot(void)
{
	return check_fota_boot;
}

#else
static int lge_boot_reason = -1; /* undefined for error checking */
static int __init lge_check_bootreason(char *reason)
{
	int ret = 0;

	/* handle corner case of kstrtoint */
	if (!strcmp(reason, "0xffffffff")) {
		lge_boot_reason = 0xffffffff;
		return 1;
	}

	ret = kstrtoint(reason, 16, &lge_boot_reason);
	if (!ret)
		printk(KERN_INFO "LGE REBOOT REASON: %x\n", lge_boot_reason);
	else
		printk(KERN_INFO "LGE REBOOT REASON: Couldn't get bootreason - %d\n",
				ret);

	return 1;
}
__setup("lge.bootreasoncode=", lge_check_bootreason);

int lge_get_bootreason(void)
{
	return lge_boot_reason;
}
#endif

int lge_get_factory_boot(void)
{
	int res;

	/*   if boot mode is factory,
	 *   cable must be factory cable.
	 */
	switch (lge_boot_mode) {
	case LGE_BOOT_MODE_QEM_56K:
	case LGE_BOOT_MODE_QEM_130K:
	case LGE_BOOT_MODE_QEM_910K:
	case LGE_BOOT_MODE_PIF_56K:
	case LGE_BOOT_MODE_PIF_130K:
	case LGE_BOOT_MODE_PIF_910K:
	case LGE_BOOT_MODE_MINIOS:
		res = 1;
		break;
	default:
		res = 0;
		break;
	}
	return res;
}

static int get_factory_cable(void)
{
	int res = 0;

	/* if boot mode is factory, cable must be factory cable. */
	switch (lge_boot_mode) {
	case LGE_BOOT_MODE_QEM_56K:
	case LGE_BOOT_MODE_PIF_56K:
		res = LGEUSB_FACTORY_56K;
		break;

	case LGE_BOOT_MODE_QEM_130K:
	case LGE_BOOT_MODE_PIF_130K:
		res = LGEUSB_FACTORY_130K;
		break;

	case LGE_BOOT_MODE_QEM_910K:
	case LGE_BOOT_MODE_PIF_910K:
		res = LGEUSB_FACTORY_910K;
		break;

	default:
		res = 0;
		break;
	}

	return res;
}

struct lge_android_usb_platform_data lge_android_usb_pdata = {
	.vendor_id = 0x1004,
	.factory_pid = 0x6000,
	.iSerialNumber = 0,
	.product_name = "LGE Android Phone",
	.manufacturer_name = "LG Electronics Inc.",
	.factory_composition = "acm,diag",
	.get_factory_cable = get_factory_cable,
};

struct platform_device lge_android_usb_device = {
	.name = "lge_android_usb",
	.id = -1,
	.dev = {
		.platform_data = &lge_android_usb_pdata,
	},
};

int __init lge_add_android_usb_devices(void)
{
	return platform_device_register(&lge_android_usb_device);
}
arch_initcall(lge_add_android_usb_devices);
