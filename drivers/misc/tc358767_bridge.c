#include <linux/gpio.h>
#include <linux/i2c.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/async.h>
#include <linux/regulator/consumer.h>
#include <linux/err.h>
#include <linux/of_gpio.h>
#include <linux/slab.h>
#include <linux/clk.h>

struct i2c_client *tc358767_i2c_client;

struct bridge_clk {
	struct clk *clk; /* clk handle */
	char clk_name[32];
	unsigned long rate;
};

struct tc358762_platform_data {
	struct gpio *gpio_req_tbl;
	uint8_t gpio_req_tbl_size;
	//struct gpio *gpio_array;
	struct pinctrl *pinctrl;
	struct pinctrl_state *gpio_state_active;
	struct pinctrl_state *gpio_state_suspend;

	unsigned num_clk;
	struct bridge_clk *clk_config;
};

struct tc358762_platform_data *g_pdata;

static int tc358762_get_dt_gpio_req_tbl(
	struct device *dev, struct tc358762_platform_data *pdata,
	uint16_t *gpio_array, uint16_t gpio_array_size)
{
	int rc = 0, i = 0;
	struct device_node *np = dev->of_node;
	uint32_t count = 0;
	uint32_t *val_array = NULL;

	if (!of_get_property(np, "qcom,gpio-req-tbl-num", &count)) {
		pr_err("%s: no gpio-req-tbl-num\n", __func__);
		return 0;
	}

	count /= sizeof(uint32_t);
	if (!count) {
		pr_err("%s: qcom,gpio-req-tbl-num 0\n", __func__);
		return 0;
	}

	val_array = kzalloc(sizeof(uint32_t) * count, GFP_KERNEL);
	if (!val_array) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		return -ENOMEM;
	}

	pdata->gpio_req_tbl = kzalloc(sizeof(struct gpio) * count,
					GFP_KERNEL);
	if (!pdata->gpio_req_tbl) {
		pr_err("%s failed alloc mem%d\n", __func__, __LINE__);
		rc = -ENOMEM;
		kfree(val_array);
		return rc;
	}
	pdata->gpio_req_tbl_size = count;

	rc = of_property_read_u32_array(np, "qcom,gpio-req-tbl-num",
		val_array, count);
	if (rc < 0) {
		pr_err("%s failed get gpio-req-tbl-num%d\n", __func__, __LINE__);
		kfree(pdata->gpio_req_tbl);
		kfree(val_array);
		return rc;
	}
	for (i = 0; i < count; i++) {
		if (val_array[i] >= gpio_array_size) {
			pr_err("%s gpio req tbl index %d invalid\n",
				__func__, val_array[i]);
			return -EINVAL;
		}
		pdata->gpio_req_tbl[i].gpio = gpio_array[val_array[i]];
		pr_err("%s: gpio_req_tbl[%d].gpio = %d\n", __func__, i,
			pdata->gpio_req_tbl[i].gpio);
	}

	rc = of_property_read_u32_array(np, "qcom,gpio-req-tbl-flags",
		val_array, count);
	if (rc < 0) {
		pr_err("%s failed %d\n", __func__, __LINE__);
		kfree(pdata->gpio_req_tbl);
		kfree(val_array);
		return rc;
	}
	for (i = 0; i < count; i++) {
		pdata->gpio_req_tbl[i].flags = val_array[i];
		pr_err("%s gpio_req_tbl[%d].flags = %ld\n", __func__, i,
			pdata->gpio_req_tbl[i].flags);
	}

	for (i = 0; i < count; i++) {
		rc = of_property_read_string_index(np,
			"qcom,gpio-req-tbl-label", i,
			&pdata->gpio_req_tbl[i].label);
		pr_err("%s gpio_req_tbl[%d].label = %s\n", __func__, i,
			pdata->gpio_req_tbl[i].label);
		if (rc < 0) {
			pr_err("%s failed %d\n", __func__, __LINE__);
			kfree(pdata->gpio_req_tbl);
			kfree(val_array);
			return rc;
		}
	}

	kfree(val_array);
	return rc;
}

static int tc358762_init_gpio_pin_tbl(
	struct device *dev, struct tc358762_platform_data *pdata,
	uint16_t *gpio_array, uint16_t gpio_array_size)
{
	int rc = 0;
	/* TODO: Init LDO, Reset Pin...*/
	return rc;

}

static int tc358762_parse_dt(
	struct device *dev, struct tc358762_platform_data *pdata)
{
	int32_t rc = 0, i = 0;
	struct device_node *np = dev->of_node;
	uint16_t *gpio_array = NULL;
	uint16_t gpio_array_size = 0;

	if (!dev || !np) {
		pr_err("%s: Invalid %p of_node %p\n", __func__, dev, np);
		return -EINVAL;
	}

	gpio_array_size = of_gpio_count(np);
	if (!gpio_array_size) {
		pr_err("%s: No gpios\n", __func__);
		return 0;
	}
	
	gpio_array = kzalloc(sizeof(uint16_t) * gpio_array_size, GFP_KERNEL);
	if (!gpio_array) {
		pr_err("%s: Failed alloc gpio_array\n", __func__);
		return -ENOMEM;
	}

	for (i = 0; i < gpio_array_size; i++) {
		gpio_array[i] = of_get_gpio(np, i);
		pr_err("%s: gpio_array[%d] = %d\n", __func__, i, gpio_array[i]);
	}

	rc = tc358762_get_dt_gpio_req_tbl(dev, pdata,
					gpio_array, gpio_array_size);
	if (rc < 0) {
		pr_err("%s: failed get_dt_gpio_req_tbl\n", __func__);
		kfree(gpio_array);
		return rc;
	}

	rc = tc358762_init_gpio_pin_tbl(dev, pdata,
					gpio_array, gpio_array_size);
	if (rc < 0) {
		pr_err("%s: failed init_gpio_pin_tbl\n", __func__);
		kfree(gpio_array);
		kfree(pdata->gpio_req_tbl);
		return rc;
	}

	kfree(gpio_array);
	return rc;
}

static int tc358762_pinctrl_init(
		struct device *dev, struct tc358762_platform_data *pdata)
{
	pdata->pinctrl = devm_pinctrl_get(dev);
	if (IS_ERR_OR_NULL(pdata->pinctrl)) {
		pr_err("%s: failed to get pinctrl\n", __func__);
		return PTR_ERR(pdata->pinctrl);
	}

	pdata->gpio_state_active = pinctrl_lookup_state(pdata->pinctrl,
				"cam_default");
	if (IS_ERR_OR_NULL(pdata->gpio_state_active))
		pr_warn("%s: can not get default pinstate\n", __func__);

	pdata->gpio_state_suspend
		= pinctrl_lookup_state(pdata->pinctrl,
				"cam_suspend");
	if (IS_ERR_OR_NULL(pdata->gpio_state_suspend))
		pr_warn("%s: can not get sleep pinstate\n", __func__);

	return 0;
}

static int tc358762_pinctrl_set_state(
			struct tc358762_platform_data *pdata, bool active)
{
	struct pinctrl_state *pin_state;
	int rc = -EFAULT;

	if (IS_ERR_OR_NULL(pdata->pinctrl))
		return PTR_ERR(pdata->pinctrl);

	pin_state = active ? pdata->gpio_state_active
				: pdata->gpio_state_suspend;
	if (!IS_ERR_OR_NULL(pin_state)) {
		rc = pinctrl_select_state(pdata->pinctrl, pin_state);
		if (rc)
			pr_err("%s: can not set %s pins\n", __func__,
			       active ? "cam_default" : "cam_suspend");
	} else {
		pr_err("%s: invalid '%s' pinstate\n", __func__,
		       active ? "cam_default" : "cam_suspend");
	}
	return rc;
}

static int tc358767_get_clk(
	struct device *dev, struct bridge_clk *clk_arry, int num_clk)
{
	uint32_t rc = 0, i = 0;

	for (i = 0; i < num_clk; i++) {
		clk_arry[i].clk = clk_get(dev, clk_arry[i].clk_name);
		rc = PTR_RET(clk_arry[i].clk);
		if (rc) {
			pr_err("%s: get [%s]clk failed. rc=%d\n",
				 __func__,
				clk_arry[i].clk_name, rc);
			return rc;
		}
	}
	return rc;
}

static int tc358767_parse_dt_clk(
	struct device *dev, struct tc358762_platform_data *pdata)
{
	uint32_t rc = 0, i = 0;
	struct device_node *np = dev->of_node;
	const char *clock_name;
	uint32_t clock_rate;

	pdata->num_clk = of_property_count_strings(np, "clock-names");
	if (pdata->num_clk <= 0) {
		pr_err("%s clocks are not defined\n", __func__);
		return rc;
	}

	pdata->clk_config = kzalloc(
		sizeof(struct bridge_clk) * pdata->num_clk, GFP_KERNEL);
	if (!pdata->clk_config) {
		pr_err("%s clock configuration allocation failed\n"
				, __func__);
		rc = -ENOMEM;
		pdata->num_clk = 0;
		return rc;
	}

	for (i = 0; i < pdata->num_clk; i++) {
		of_property_read_string_index(np, "clock-names",
							i, &clock_name);
		strlcpy(pdata->clk_config[i].clk_name, clock_name,
				sizeof(pdata->clk_config[i].clk_name));

		of_property_read_u32_index(np, "clock-rate",
							i, &clock_rate);
		pdata->clk_config[i].rate = clock_rate;
	}

	rc = tc358767_get_clk(dev, pdata->clk_config, pdata->num_clk);
	if (rc) {
		pr_err("%s get_clk\n", __func__);
	}

	return rc;
}

static int tc358762_clk_set_rate(struct bridge_clk *clk_arry, int num_clk)
{
	uint32_t rc = 0, i = 0;

	for (i = 0; i < num_clk; i++) {
		if (clk_arry[i].clk) {
			rc = clk_set_rate(clk_arry[i].clk,
					clk_arry[i].rate);
			if (rc) {
				pr_err("%s cannot set [%s]clk rate\n",
					__func__, clk_arry[i].clk_name);
				break;
			}
		} else {
			pr_err("%s [%s]clk is not available\n",
				__func__, clk_arry[i].clk_name);
			rc = -EPERM;
			break;
		}
	}

	return rc;
}

static int tc358762_enable_clk(
		struct bridge_clk *clk_arry, int num_clk, bool enable)
{
	uint32_t rc = 0, i = 0;

	if (enable) {
		for (i = 0; i < num_clk; i++) {
			pr_err("%s: enable '%s'\n",
				__func__, clk_arry[i].clk_name);
			if (clk_arry[i].clk) {
				rc = clk_prepare_enable(clk_arry[i].clk);
				if (rc)
					pr_err("%s: %s en fail. rc=%d\n",
						__func__,
						clk_arry[i].clk_name, rc);
			} else {
				pr_err("%s: '%s' is not available\n",
					 __func__, clk_arry[i].clk_name);
				rc = -EPERM;
			}

			if (rc) {
				tc358762_enable_clk(&clk_arry[i],
					i, false);
				break;
			}
		}
	} else {
		for (i = num_clk - 1; i >= 0; i--) {
			pr_err("%s: disable '%s'\n",
				__func__, clk_arry[i].clk_name);

			if (clk_arry[i].clk)
				clk_disable_unprepare(clk_arry[i].clk);
			else
				pr_err("%s: '%s' is not available\n",
					__func__, clk_arry[i].clk_name);
		}
	}

	return rc;
}

static int enable_bridge_ldo(bool enable) {
	int rc = 0;
        struct regulator *vana;
if(enable) {
        vana = regulator_get(NULL, "rt5058-ldo1");
        if (IS_ERR(vana)){
                pr_err("%s get failed rt5058-ldo1\n", __func__);
		rc = -EINVAL;
		return rc;
        } else {
                regulator_set_voltage(vana, 1800000, 1800000);
                rc = regulator_enable(vana);
                pr_err("%s enable rt5058-ldo1\n", __func__);
        }
        vana = regulator_get(NULL, "rt5058-ldo2");
        if (IS_ERR(vana)){
                pr_err("%s get failed rt5058-ldo2\n", __func__);
		rc = -EINVAL;
		return rc;
        } else {
                regulator_set_voltage(vana, 1200000, 1200000);
                rc = regulator_enable(vana);
                pr_err("%s enable rt5058-ldo2\n", __func__);
        }
}

return rc;
}

static int tc358762_i2c_probe(struct i2c_client *client,
			const struct i2c_device_id *id)
{
	struct tc358762_platform_data *pdata;
	int32_t ret = 0;

	pr_err("%s: probe start\n", __func__);
	if (client->dev.of_node) {
		pdata = devm_kzalloc(&client->dev,
				sizeof(struct tc358762_platform_data),
				GFP_KERNEL);
		if (!pdata) {
			pr_err("%s: Failed to allocate memory\n", __func__);
			tc358767_i2c_client = NULL;
			ret = -ENOMEM;
			return ret;
		}
		client->dev.platform_data = pdata;

		ret = tc358762_parse_dt(&client->dev, pdata);
		if(ret != 0) {
			pr_err("%s: Failed to parse dt\n", __func__);
			kfree(pdata);
			return ret;
		}
	} else {
		pdata = client->dev.platform_data;
		pr_err("%s no device tree node\n", __func__);
	}

	ret = tc358762_pinctrl_init(&client->dev, pdata);
	if (ret != 0) {
		pr_err("%s pinctrl_init err\n",__func__);
		kfree(pdata);
		return ret;
	}

	ret = tc358767_parse_dt_clk(&client->dev, pdata);
	if (ret != 0) {
		pr_err("%s parse dt clk err\n",__func__);
		kfree(pdata);
		return ret;
	}

	g_pdata = pdata;
	tc358767_i2c_client = client;

//testcode 
	ret = enable_bridge_ldo(true);
	ret = tc358762_pinctrl_set_state(pdata, true);
	ret = tc358762_clk_set_rate(pdata->clk_config, pdata->num_clk);
	ret = tc358762_enable_clk(pdata->clk_config, pdata->num_clk, true);

#if 0
        int rc = 0;
        struct regulator *vana;
        vana = regulator_get(NULL, "rt5058-ldo1");
        if (IS_ERR(vana)){
                pr_err("%s get failed rt5058-ldo1\n", __func__);
        } else {
                regulator_set_voltage(vana, 1800000, 1800000);
                rc = regulator_enable(vana);
                pr_err("%s enable rt5058-ldo1\n", __func__);
        }
        vana = regulator_get(NULL, "rt5058-ldo2");
        if (IS_ERR(vana)){
                pr_err("%s get failed rt5058-ldo2\n", __func__);
        } else {
                regulator_set_voltage(vana, 1200000, 1200000);
                rc = regulator_enable(vana);
                pr_err("%s enable rt5058-ldo2\n", __func__);
        }
#endif
	
	pr_err("%s probe\n", __func__);
	return 0;
}

static int tc358762_i2c_remove(struct i2c_client *client)
{
	return 0;
}

static int tc358762_i2c_suspend(struct i2c_client *client, pm_message_t state)
{
	return 0;
}


static int tc358762_i2c_resume(struct i2c_client *client)
{
	return 0;
}

static const struct i2c_device_id tc358762_id[] = {
	{ "tc358762", 0 },
	{ }
};

static struct of_device_id tc358762_match_table[] = {
    { .compatible = "toshiba,tc358762",},
    { },
};

static struct i2c_driver tc358762_driver = {
	.driver  = {
		.name  = "tc358762",
		.owner  = THIS_MODULE,
		.of_match_table = tc358762_match_table,
	},
	.probe  = tc358762_i2c_probe,
	.remove  = tc358762_i2c_remove,
	.suspend = tc358762_i2c_suspend,
	.resume = tc358762_i2c_resume,
	.id_table  = tc358762_id,
};

static void __init tc358762_init_async(void *data, async_cookie_t cookie)
{
	int ret = 0;
#if 0
	int rc = 0;
        struct regulator *vana;
#endif
	ret = i2c_add_driver(&tc358762_driver);
	if (ret < 0)
		pr_err("%s: failed to register tc358762 i2c drivern", __func__);

#if 0
//        int rc = 0;
//        struct regulator *vana;
        vana = regulator_get(NULL, "rt5058-ldo1");
        if (IS_ERR(vana)){
                pr_err("%s get failed rt5058-ldo1\n", __func__);
        } else {
                regulator_set_voltage(vana, 1800000, 1800000);
                rc = regulator_enable(vana);
                pr_err("%s enable rt5058-ldo1\n", __func__);
        }
        vana = regulator_get(NULL, "rt5058-ldo2");
        if (IS_ERR(vana)){
                pr_err("%s get failed rt5058-ldo2\n", __func__);
        } else {
                regulator_set_voltage(vana, 1200000, 1200000);
                rc = regulator_enable(vana);
                pr_err("%s enable rt5058-ldo2\n", __func__);
        }
#endif
}

static int __init tc358762_init(void)
{
	async_schedule(tc358762_init_async, NULL);
	return 0;
}

static void __exit tc358762_exit(void)
{
	i2c_del_driver(&tc358762_driver);
}

module_init(tc358762_init);
module_exit(tc358762_exit);
