/*
 * SM5107 Regulator Driver
 *
 * Copyright 2015 LG Electronics Inc,
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 */

#include <linux/delay.h>
#include <linux/err.h>
#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/regulator/sm5107.h>

static struct sm5107 *sm5107_base;

int sm5107_read_byte(struct sm5107 *sm5107, u8 reg, u8 *read)
{
	int ret;
	unsigned int val;

	ret = regmap_read(sm5107->regmap, reg, &val);
	if (ret < 0)
		return ret;

	*read = (u8)val;
	return 0;
}

int sm5107_write_byte(struct sm5107 *sm5107, u8 reg, u8 data)
{
	return regmap_write(sm5107->regmap, reg, data);
}

int sm5107_ctrl(int mode)
{
	int ret = 0;

	pr_info("%s: %d\n", __func__, mode);
	switch (mode) {
	case LCD_OFF:
		/* Set DSV_EN disable */
		ret = sm5107_write_byte(sm5107_base, 0x03, 0x03);
		usleep(5 * 1000);
		pr_err("SM5107 FD Enable [0x03, 0x03]\n");
		break;
	case LCD_OFF_KNOCKON:
		break;
	case LCD_OFF_TOGGLE:
		pr_err("SM5107 No Control\n");
		break;
	case LCD_ON:
	case LCD_ON_KNOCKON:
	case LCD_ON_TOGGLE:
		ret = sm5107_write_byte(sm5107_base, 0x03, 0x00);
		usleep(2 * 1000);
		ret = sm5107_write_byte(sm5107_base, 0xFF, 0x00);
		usleep(2 * 1000);
		pr_err("SM5107 FD Disable [0x03, 0x00]\n");
		break;
	default:
		ret = -EINVAL;
		break;
	}
	return ret;
}

static struct regmap_config sm5107_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
};

static int sm5107_probe(struct i2c_client *cl, const struct i2c_device_id *id)
{
	struct sm5107 *sm5107;
	struct device *dev = &cl->dev;
	struct sm5107_platform_data *pdata = dev_get_platdata(dev);
	int rc = 0;

	pr_err("%s start\n",__func__);

	sm5107 = devm_kzalloc(dev, sizeof(*sm5107), GFP_KERNEL);
	if (!sm5107)
		return -ENOMEM;

	sm5107->pdata = pdata;

	sm5107->regmap = devm_regmap_init_i2c(cl, &sm5107_regmap_config);
	if (IS_ERR(sm5107->regmap)){
		pr_err("Failed to allocate register map\n");
		devm_kfree(dev, sm5107);
		return PTR_ERR(sm5107->regmap);
	}

	sm5107->dev = &cl->dev;
	i2c_set_clientdata(cl,sm5107);
	sm5107_base = sm5107;

	return rc;
}

static int sm5107_remove(struct i2c_client *cl)
{
	return 0;
}

static const struct i2c_device_id sm5107_id[] = {
	{ "sm5107", 0 },
	{ }
};
MODULE_DEVICE_TABLE(i2c, sm5107_id);

#ifdef CONFIG_OF
static const struct of_device_id sm5107_of_match[] = {
	{ .compatible = "sm,dsv-sm5107", },
	{ }
};
MODULE_DEVICE_TABLE(of, sm5107_of_match);
#endif

static struct i2c_driver sm5107_driver = {
	.probe = sm5107_probe,
	.remove = sm5107_remove,
	.driver = {
		.name = "sm5107",
		.owner = THIS_MODULE,
		.of_match_table = of_match_ptr(sm5107_of_match),
	},
	.id_table = sm5107_id,
};
module_i2c_driver(sm5107_driver);

MODULE_DESCRIPTION("sm5107 Regulator");
