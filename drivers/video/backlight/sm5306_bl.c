/* drivers/video/backlight/sm5306_bl.c
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <linux/module.h>
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <linux/backlight.h>
#include <linux/fb.h>
#include <linux/delay.h>
#include <linux/gpio.h>
#include <mach/board.h>
#include <linux/i2c.h>
#include <linux/of_gpio.h>
#include <linux/platform_data/sm5306_bl.h>

#define I2C_BL_NAME                              "sm5306"
#define MAX_BRIGHTNESS_SM5306                    0xFF
#define MIN_BRIGHTNESS_SM5306                    0x0F
#define DEFAULT_BRIGHTNESS                       0xFF
#define DEFAULT_FTM_BRIGHTNESS                   0x0F

#define BL_ON        1
#define BL_OFF       0

/* LGE_CHANGE  - To turn backlight on by setting default brightness while kernel booting */
#define BOOT_BRIGHTNESS 1

static struct i2c_client *sm5306_i2c_client;

static int store_level_used = 0;

struct backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;
	int max_current;
	int init_on_boot;
	int min_brightness;
	int max_brightness;
	int default_brightness;
	int factory_brightness;
	int blmap_size;
	char *blmap;
};

struct sm5306_device {
	struct i2c_client *client;
	struct backlight_device *bl_dev;
	int gpio;
	int max_current;
	int min_brightness;
	int max_brightness;
	int default_brightness;
	int factory_brightness;
	struct mutex bl_mutex;
	int blmap_size;
	char *blmap;
};

static const struct i2c_device_id sm5306_bl_id[] = {
	{ I2C_BL_NAME, 0 },
	{ },
};

static int sm5306_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val);

static int cur_main_lcd_level = DEFAULT_BRIGHTNESS;
static int saved_main_lcd_level = DEFAULT_BRIGHTNESS;
static int backlight_status = BL_OFF;
static int sm5306_pwm_enable;
static struct sm5306_device *main_sm5306_dev;

#ifdef CONFIG_LGE_WIRELESS_CHARGER
int wireless_backlight_state(void)
{
	return backlight_status;
}
EXPORT_SYMBOL(wireless_backlight_state);
#endif

static void sm5306_hw_reset(void)
{
	int gpio = main_sm5306_dev->gpio;
	/* LGE_CHANGE - Fix GPIO Setting Warning*/
	if (gpio_is_valid(gpio)) {
		gpio_direction_output(gpio, 1);
		gpio_set_value_cansleep(gpio, 1);
		mdelay(10);
	}
	else
		pr_err("%s: gpio is not valid !!\n", __func__);
}

static int sm5306_write_reg(struct i2c_client *client, unsigned char reg, unsigned char val)
{
	int err;
	u8 buf[2];
	struct i2c_msg msg = {
		client->addr, 0, 2, buf
	};

	buf[0] = reg;
	buf[1] = val;

	err = i2c_transfer(client->adapter, &msg, 1);
	if (err < 0)
		dev_err(&client->dev, "i2c write error reg: %d, val: %d\n", buf[0], buf[1]  );

	return 0;
}

static int exp_min_value = 150;
static int cal_value;

static void sm5306_set_main_current_level(struct i2c_client *client, int level)
{
	struct sm5306_device *dev = i2c_get_clientdata(client);
	int min_brightness = dev->min_brightness;
	int max_brightness = dev->max_brightness;

	if (level == -BOOT_BRIGHTNESS)
		level = dev->default_brightness;

	cur_main_lcd_level = level;
	dev->bl_dev->props.brightness = cur_main_lcd_level;

	store_level_used = 0;

	mutex_lock(&dev->bl_mutex);
	if (level != 0) {
		if (level > 0 && level <= min_brightness)
			level = min_brightness;
		else if (level > max_brightness)
			level = max_brightness;
		if (dev->blmap) {
			if (level < dev->blmap_size) {
				cal_value = dev->blmap[level];
				sm5306_write_reg(client, 0x03,
						cal_value);
			} else
				dev_warn(&client->dev, "invalid index %d:%d\n",
						dev->blmap_size,
						level);
		} else {
			cal_value = level;
			sm5306_write_reg(client, 0x03, cal_value);
		}
	} else
		sm5306_write_reg(client, 0x00, 0x00);

	mutex_unlock(&dev->bl_mutex);

	pr_info("[LCD][DEBUG] %s : backlight level=%d, cal_value=%d \n",
				__func__, level, cal_value);
}

static void sm5306_set_main_current_level_no_mapping(
		struct i2c_client *client, int level)
{
	struct sm5306_device *dev;
	dev = (struct sm5306_device *)i2c_get_clientdata(client);

	if (level > 255)
		level = 255;
	else if (level < 0)
		level = 0;

	cur_main_lcd_level = level;
	dev->bl_dev->props.brightness = cur_main_lcd_level;

	store_level_used = 1;

	mutex_lock(&main_sm5306_dev->bl_mutex);
	if (level != 0) {
		sm5306_write_reg(client, 0x03, level);
	} else {
		sm5306_write_reg(client, 0x00, 0x00);
	}
	mutex_unlock(&main_sm5306_dev->bl_mutex);
}

void sm5306_backlight_on(int level)
{
	if (backlight_status == BL_OFF) {

		pr_info("%s with level %d\n", __func__, level);
		sm5306_hw_reset();

		/*  OVP(24V),OCP(1.0A) , Boost Frequency(500khz) */
		sm5306_write_reg(main_sm5306_dev->client, 0x02, 0x30);

		if( sm5306_pwm_enable ) {
			/* eble Feedback , disable  PWM for BANK A,B */
			sm5306_write_reg(main_sm5306_dev->client, 0x01, 0x19);
		}
		else {
			/* eble Feedback , disable  PWM for BANK A,B */
			sm5306_write_reg(main_sm5306_dev->client, 0x01, 0x18);
		}

		/* Brightness Code Setting Max on Bank A */
		/* Full-Scale Current (20.2mA) of BANK A */
		/* 20.2mA : 0x13 , 17mA : 0x0F */
		sm5306_write_reg(main_sm5306_dev->client, 0x05, 0x13);

		/* Enable LED A to Exponential, LED2 is connected to BANK_A */
		sm5306_write_reg(main_sm5306_dev->client, 0x00, 0x16);

	}
	mdelay(1);

	sm5306_set_main_current_level(main_sm5306_dev->client, level);
	backlight_status = BL_ON;

	return;
}

void sm5306_backlight_off(void)
{
	int gpio = main_sm5306_dev->gpio;

	if (backlight_status == BL_OFF)
		return;

	saved_main_lcd_level = cur_main_lcd_level;
	sm5306_set_main_current_level(main_sm5306_dev->client, 0);
	backlight_status = BL_OFF;

	gpio_direction_output(gpio, 0);
	msleep(6);

	pr_info("%s\n", __func__);
	return;
}

void sm5306_lcd_backlight_set_level(int level)
{
	if (level > MAX_BRIGHTNESS_SM5306)
		level = MAX_BRIGHTNESS_SM5306;

	if (sm5306_i2c_client != NULL) {
		if (level == 0) {
			sm5306_backlight_off();
		} else {
			sm5306_backlight_on(level);
		}
	} else {
		pr_err("%s(): No client\n", __func__);
	}
}
EXPORT_SYMBOL(sm5306_lcd_backlight_set_level);

static int bl_set_intensity(struct backlight_device *bd)
{
	struct i2c_client *client = to_i2c_client(bd->dev.parent);

	/* LGE_CHANGE - if it's trying to set same backlight value, skip it.*/
	if(bd->props.brightness == cur_main_lcd_level){
		pr_debug("%s level is already set. skip it\n", __func__);
		return 0;
	}

	sm5306_set_main_current_level(client, bd->props.brightness);
	cur_main_lcd_level = bd->props.brightness;

	return 0;
}

static int bl_get_intensity(struct backlight_device *bd)
{
	unsigned char val = 0;
	val &= 0x1f;

	return (int)val;
}

static ssize_t lcd_backlight_show_level(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r = 0;

	if(store_level_used == 0)
		r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n",
				cal_value);
	else if(store_level_used == 1)
		r = snprintf(buf, PAGE_SIZE, "LCD Backlight Level is : %d\n",
				cur_main_lcd_level);

	return r;
}

static ssize_t lcd_backlight_store_level(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int level;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	level = simple_strtoul(buf, NULL, 10);

	sm5306_set_main_current_level_no_mapping(client, level);
	pr_info("[LCD][DEBUG] write %d direct to "
			"backlight register\n", level);

	return count;
}

static int sm5306_bl_resume(struct i2c_client *client)
{
	sm5306_lcd_backlight_set_level(saved_main_lcd_level);
	return 0;
}

static int sm5306_bl_suspend(struct i2c_client *client, pm_message_t state)
{
	pr_info("[LCD][DEBUG] %s: new state: %d\n",
			__func__, state.event);

	sm5306_lcd_backlight_set_level(saved_main_lcd_level);
	return 0;
}

static ssize_t lcd_backlight_show_on_off(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r = 0;

	pr_info("%s received (prev backlight_status: %s)\n",
			__func__, backlight_status ? "ON" : "OFF");

	return r;
}

static ssize_t lcd_backlight_store_on_off(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int on_off;
	struct i2c_client *client = to_i2c_client(dev);

	if (!count)
		return -EINVAL;

	pr_info("%s received (prev backlight_status: %s)\n",
			__func__, backlight_status ? "ON" : "OFF");

	on_off = simple_strtoul(buf, NULL, 10);

	pr_info("[LCD][DEBUG] %d", on_off);

	if (on_off == 1)
		sm5306_bl_resume(client);
	else if (on_off == 0)
		sm5306_bl_suspend(client, PMSG_SUSPEND);

	return count;

}
static ssize_t lcd_backlight_show_exp_min_value(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	int r;

	r = snprintf(buf, PAGE_SIZE, "LCD Backlight  : %d\n", exp_min_value);

	return r;
}

static ssize_t lcd_backlight_store_exp_min_value(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	int value;

	if (!count)
		return -EINVAL;

	value = simple_strtoul(buf, NULL, 10);
	exp_min_value = value;

	return count;
}

DEVICE_ATTR(sm5306_level, 0644, lcd_backlight_show_level,
		lcd_backlight_store_level);
DEVICE_ATTR(sm5306_backlight_on_off, 0644, lcd_backlight_show_on_off,
		lcd_backlight_store_on_off);
DEVICE_ATTR(sm5306_exp_min_value, 0644, lcd_backlight_show_exp_min_value,
		lcd_backlight_store_exp_min_value);

#ifdef CONFIG_OF
static int sm5306_parse_dt(struct device *dev,
		struct backlight_platform_data *pdata)
{
	int rc = 0, i;
	u32 *array;
	struct device_node *np = dev->of_node;

	pdata->gpio = of_get_named_gpio_flags(np,
			"sm5306,lcd_bl_en", 0, NULL);
	rc = of_property_read_u32(np, "sm5306,max_current",
			&pdata->max_current);
	rc = of_property_read_u32(np, "sm5306,min_brightness",
			&pdata->min_brightness);
	rc = of_property_read_u32(np, "sm5306,default_brightness",
			&pdata->default_brightness);
	rc = of_property_read_u32(np, "sm5306,max_brightness",
			&pdata->max_brightness);

	rc = of_property_read_u32(np, "sm5306,enable_pwm",
			&sm5306_pwm_enable);
	if(rc == -EINVAL)
		sm5306_pwm_enable = 1;

	rc = of_property_read_u32(np, "sm5306,blmap_size",
			&pdata->blmap_size);

	if (pdata->blmap_size) {
		array = kzalloc(sizeof(u32) * pdata->blmap_size, GFP_KERNEL);
		if (!array)
			return -ENOMEM;

		rc = of_property_read_u32_array(np, "sm5306,blmap", array, pdata->blmap_size);
		if (rc) {
			pr_err("%s:%d, uable to read backlight map\n",__func__, __LINE__);
			return -EINVAL;
		}
		pdata->blmap = kzalloc(sizeof(char) * pdata->blmap_size, GFP_KERNEL);

		if (!pdata->blmap)
			return -ENOMEM;

		for (i = 0; i < pdata->blmap_size; i++ )
			pdata->blmap[i] = (char)array[i];

		if (array)
			kfree(array);

	} else {
		pdata->blmap = NULL;
	}

	pr_debug("%s gpio: %d, max_current: %d, min: %d, "
			"default: %d, max: %d, pwm : %d , blmap_size : %d\n",
			__func__, pdata->gpio,
			pdata->max_current,
			pdata->min_brightness,
			pdata->default_brightness,
			pdata->max_brightness,
			sm5306_pwm_enable,
			pdata->blmap_size);

	return rc;
}
#endif

static struct backlight_ops sm5306_bl_ops = {
	.update_status = bl_set_intensity,
	.get_brightness = bl_get_intensity,
};

static int sm5306_probe(struct i2c_client *i2c_dev,
		const struct i2c_device_id *id)
{
	struct backlight_platform_data *pdata;
	struct sm5306_device *dev;
	struct backlight_device *bl_dev;
	struct backlight_properties props;
	int err;

	pr_info("[LCD][DEBUG] %s: i2c probe start\n", __func__);

#ifdef CONFIG_OF
	if (&i2c_dev->dev.of_node) {
		pdata = devm_kzalloc(&i2c_dev->dev,
				sizeof(struct backlight_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			pr_err("%s: Failed to allocate memory\n", __func__);
			return -ENOMEM;
		}
		err = sm5306_parse_dt(&i2c_dev->dev, pdata);
		if (err != 0)
			return err;
	} else {
		pdata = i2c_dev->dev.platform_data;
	}
#else
	pdata = i2c_dev->dev.platform_data;
#endif
	pr_info("[LCD][DEBUG] %s: gpio = %d\n", __func__,pdata->gpio);
	if (pdata->gpio && gpio_request(pdata->gpio, "sm5306 reset") != 0) {
		return -ENODEV;
	}

	sm5306_i2c_client = i2c_dev;

	dev = kzalloc(sizeof(struct sm5306_device), GFP_KERNEL);
	if (dev == NULL) {
		dev_err(&i2c_dev->dev, "fail alloc for sm5306_device\n");
		return 0;
	}
	main_sm5306_dev = dev;

	memset(&props, 0, sizeof(struct backlight_properties));
	props.type = BACKLIGHT_RAW;

	props.max_brightness = MAX_BRIGHTNESS_SM5306;
	bl_dev = backlight_device_register(I2C_BL_NAME, &i2c_dev->dev,
			NULL, &sm5306_bl_ops, &props);
	bl_dev->props.max_brightness = MAX_BRIGHTNESS_SM5306;
	bl_dev->props.brightness = DEFAULT_BRIGHTNESS;
	bl_dev->props.power = FB_BLANK_UNBLANK;

	dev->bl_dev = bl_dev;
	dev->client = i2c_dev;

	dev->gpio = pdata->gpio;
	dev->max_current = pdata->max_current;
	dev->min_brightness = pdata->min_brightness;
	dev->default_brightness = pdata->default_brightness;
	dev->max_brightness = pdata->max_brightness;
	dev->blmap_size = pdata->blmap_size;

	if (dev->blmap_size) {
		dev->blmap = kzalloc(sizeof(char) * dev->blmap_size, GFP_KERNEL);
		if (!dev->blmap) {
			pr_err("%s: Failed to allocate memory\n", __func__);
			return -ENOMEM;
		}
		memcpy(dev->blmap, pdata->blmap, dev->blmap_size);
	} else {
		dev->blmap = NULL;
	}

	if (gpio_get_value(dev->gpio))
		backlight_status = BL_ON;
	else
		backlight_status = BL_OFF;

	i2c_set_clientdata(i2c_dev, dev);

	mutex_init(&dev->bl_mutex);

	err = device_create_file(&i2c_dev->dev,
			&dev_attr_sm5306_level);
	err = device_create_file(&i2c_dev->dev,
			&dev_attr_sm5306_backlight_on_off);
	err = device_create_file(&i2c_dev->dev,
			&dev_attr_sm5306_exp_min_value);

	return 0;
}

static int sm5306_remove(struct i2c_client *i2c_dev)
{
	struct sm5306_device *dev;
	int gpio = main_sm5306_dev->gpio;

	device_remove_file(&i2c_dev->dev, &dev_attr_sm5306_level);
	device_remove_file(&i2c_dev->dev, &dev_attr_sm5306_backlight_on_off);
	dev = (struct sm5306_device *)i2c_get_clientdata(i2c_dev);
	backlight_device_unregister(dev->bl_dev);
	i2c_set_clientdata(i2c_dev, NULL);

	if (gpio_is_valid(gpio))
		gpio_free(gpio);

	return 0;
}

#ifdef CONFIG_OF
static struct of_device_id sm5306_match_table[] = {
	{ .compatible = "backlight,sm5306",},
	{ },
};
#endif

static struct i2c_driver main_sm5306_driver = {
	.probe = sm5306_probe,
	.remove = sm5306_remove,
	.suspend = NULL,
	.resume = NULL,
	.id_table = sm5306_bl_id,
	.driver = {
		.name = I2C_BL_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = sm5306_match_table,
#endif
	},
};

static int __init lcd_backlight_init(void)
{
	static int err;

	err = i2c_add_driver(&main_sm5306_driver);

	return err;
}

module_init(lcd_backlight_init);

MODULE_DESCRIPTION("sm5306 Backlight Control");
MODULE_AUTHOR("daewoo kwak");
MODULE_LICENSE("GPL");
