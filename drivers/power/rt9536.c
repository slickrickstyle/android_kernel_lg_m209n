#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/slab.h>
#include <linux/types.h>
#include <linux/err.h>
#include <linux/spinlock.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/of.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/pinctrl/consumer.h>
#include <linux/interrupt.h>
#include <linux/sched.h>

#include <linux/power/rt9536.h>

#include <linux/wakelock.h>
#include <linux/power_supply.h>
#include <linux/qpnp/qpnp-adc.h>

#ifdef CONFIG_LGE_PM
#include <soc/qcom/lge/lge_boot_mode.h>
#endif
#ifdef CONFIG_LGE_PM_CABLE_DETECTION
#include <soc/qcom/lge/lge_cable_detection.h>
#endif
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
#include <soc/qcom/lge/lge_battery_id_checker.h>
#endif
#ifdef CONFIG_LGE_PM_PSEUDO_BATTERY
#include <soc/qcom/lge/lge_pseudo_batt.h>
#endif
#ifdef CONFIG_LGE_PM_CHARGING_CONTROLLER
#include <soc/qcom/lge/lge_charging_scenario.h>
#endif

#include <soc/qcom/lge/lge_board_revision.h>

static char *pm_batt_supplied_to[] = {
	"fuelgauge",
};

static int is_first_boot;
static int chg_done;
static int chg_full;
static bool charging_hw_init_done;

#define MSET_UDELAY_L 2000	/* delay for mode setting (1.5ms ~ ) */
#define MSET_UDELAY_H 2100

#define START_UDELAY_L 100	/* delay for mode setting ready (50us ~ ) */
#define START_UDELAY_H 150

#define ISET_UDELAY 150		/* delay for current setting (100 us ~700 us) */

#define HVSET_UDELAY 760	/* delay for voltage setting (750 us ~ 1 ms) */

#define RT9536_RETRY_MS	500

#define CHARGING_INFORM_NORMAL_TIME 20000

enum rt9536_mode {
	RT9536_MODE_DISABLED,
	RT9536_MODE_USB500,
	RT9536_MODE_ISET,
	RT9536_MODE_USB100,
	RT9536_MODE_FACTORY,
	RT9536_MODE_MAX,
};

struct rt9536_info {
	struct platform_device *pdev;
	struct power_supply psy;
	struct delayed_work dwork;
	struct device		*dev;

	/*IRQ*/
	int irq;

	/* gpio */
	int en_set;
	int chgsb;
	int pgb;
	int iset_sel;

	struct pinctrl *pin;
	struct pinctrl_state *boot;
	struct pinctrl_state *charging;
	struct pinctrl_state *not_charging;
	struct pinctrl_state *vbus_det;

	/* current */
	int iset;

	/* status */
	int status;
	int enable;
	int usb_present;
	enum rt9536_mode mode;
	int prev_mode;

	/* lock */
	struct mutex mode_lock;
	spinlock_t pulse_lock;

	int cv_voltage;		/* in uV */

	int cur_present;	/* in mA */
	int cur_next;		/* in mA */

	/* psy */
	struct power_supply *usb_psy;
	struct power_supply batt_psy;
	struct power_supply *fg_psy;
	struct qpnp_vadc_chip		*vadc_dev;

	/* delayed work */
	struct delayed_work 	charging_inform_work;

	/* Wake lock*/
	struct wake_lock                cable_on_wake_lock;
	struct wake_lock                chg_wake_lock;
};

#ifdef CONFIG_LGE_PM_USB_CURRENT_MAX
unsigned int usb_current_max_enabled = 0;
#define USB_CURRENT_MAX 900
#endif

#ifdef CONFIG_LGE_PM_CHARGING_CONTROLLER
static struct rt9536_info *rt9536_info_controller;
extern void start_battemp_work(int delay);
extern void stop_battemp_work(void);
extern int get_btm_state(void);
extern int get_pseudo_ui(void);

extern int lgcc_is_probed;
#define BATT_TEMP_OVERHEAT 57
#define BATT_TEMP_COLD (-10)
static int lgcc_charging_current;
#endif

static struct workqueue_struct *rt9536_wq = NULL;

#ifndef CONFIG_LGE_PM_FACTORY_TESTMODE
bool stpchg_factory_testmode;
bool start_chg_factory_testmode;
#endif
#if defined(CONFIG_MSM8909_K6B) || defined(CONFIG_MACH_MSM8909_LV1_GLOBAL_COM)
extern int raw_capacity;
#endif

#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
static int get_prop_batt_id_valid(void);
static bool get_prop_batt_id_for_aat(void);
#endif

#define LT_CABLE_56K		6
#define LT_CABLE_130K		7
#define LT_CABLE_910K		11
static unsigned int cable_type;

static bool is_factory_cable(void)
{
	unsigned int cable_info;
	cable_info = lge_pm_get_cable_type();

	if ((cable_info == CABLE_56K ||
		cable_info == CABLE_130K ||
		cable_info == CABLE_910K) ||
		(cable_type == LT_CABLE_56K ||
		cable_type == LT_CABLE_130K ||
		cable_type == LT_CABLE_910K))
		return true;
	else
		return false;
}

static void rt9536_iset_sel_set(struct rt9536_info *info)
{
	if(info->cur_next <=500) {
		pr_info("[RT9536] No need to iset_sel %d\n", info->cur_next);
		return;
	}

	pr_info("[RT9536] select current %d", info->cur_next);

	if (info->cur_next > 760) {
		gpio_direction_output(info->iset_sel, 0);
	} else {
		gpio_direction_output(info->iset_sel, 1);
	}
}

static int rt9536_get_iset_sel(struct rt9536_info *info)
{
	return gpio_get_value(info->iset_sel);
}

static void rt9536_set_en_set(struct rt9536_info *info, int high)
{
	if (!high) {
		gpio_direction_output(info->en_set, 0);
	} else {
		gpio_direction_output(info->en_set, 1);
	}
}

static int rt9536_get_en_set(struct rt9536_info *info)
{
	return gpio_get_value(info->en_set);
}

static int rt9536_get_chgsb(struct rt9536_info *info)
{
	return gpio_get_value(info->chgsb);
}

static int rt9536_get_pgb(struct rt9536_info *info)
{
	return gpio_get_value(info->pgb);
}

static int rt9536_gpio_init(struct rt9536_info *info)
{
	int ret =  0;

	/*iset_sel_gpio */
	ret = gpio_request_one(info->iset_sel, GPIOF_DIR_OUT|GPIOF_INIT_LOW,
		"iset_sel");
	if (ret) {
		pr_err("failed to request iset_sel gpio ret = %d\n", ret);
	}

	/* en_set_gpio */
	ret = gpio_request_one(info->en_set, GPIOF_DIR_OUT|GPIOF_INIT_LOW,
		"en_set");
	if (ret) {
		pr_err("failed to request int_en_set gpio ret = %d\n", ret);
	}

	/* chgsb_gpio */
	ret = gpio_request_one(info->chgsb, GPIOF_DIR_IN|GPIOF_INIT_HIGH|GPIOF_OPEN_DRAIN,
		"chgsb");
	if (ret) {
		pr_err("failed to request int_gpio chgsb ret = %d\n", ret);
	}

		/* pgb_gpio */
	ret = gpio_request_one(info->pgb, GPIOF_DIR_IN|GPIOF_INIT_HIGH|GPIOF_OPEN_DRAIN,
			"pgb");
	if (ret) {
		pr_err("failed to request int_gpio pgb ret = %d\n", ret);
	}

	return 0;
}


static int get_prop_charge_type(struct rt9536_info *info)
{
	pr_info("[RT9536] info->status : charge_type = %d\n",info->status);

	if (info->status == POWER_SUPPLY_STATUS_CHARGING)
		return POWER_SUPPLY_CHARGE_TYPE_FAST;
	else if (info->status == POWER_SUPPLY_STATUS_FULL)
		return POWER_SUPPLY_CHARGE_TYPE_TAPER;
	else
		return POWER_SUPPLY_CHARGE_TYPE_NONE;
}

#define DEFAULT_CAPACITY	50
static int get_prop_capacity(struct rt9536_info *chip)
{
	union power_supply_propval ret = {0,};
	int soc;

	if ((read_lge_battery_id() == BATT_NOT_PRESENT) && lge_get_factory_boot()) {
		pr_info("[RT9536] NO battery+Factory cable update soc to 50 \n");
		return DEFAULT_CAPACITY;
	}

	if (chip->fg_psy == NULL)
		chip->fg_psy = power_supply_get_by_name("fuelgauge");

	if (chip->fg_psy) {
		chip->fg_psy->get_property(chip->fg_psy,
				POWER_SUPPLY_PROP_CAPACITY, &ret);
		soc = ret.intval;

		return soc;
	} else {
		pr_info("No Fuel gauge supply registered return %d\n", DEFAULT_CAPACITY);
	}

	/*
	 * Return default capacity to avoid userspace
	 * from shutting down unecessarily
	 */
	return DEFAULT_CAPACITY;
}

#define DEFAULT_VOLTAGE 4000000
static int get_prop_voltage_now(struct rt9536_info *chip)
{
	union power_supply_propval ret = {0,};
	int voltage_now;

	if (chip->fg_psy == NULL)
		chip->fg_psy = power_supply_get_by_name("fuelgauge");

	if (chip->fg_psy) {
		chip->fg_psy->get_property(chip->fg_psy,
				POWER_SUPPLY_PROP_VOLTAGE_NOW, &ret);
		voltage_now = ret.intval;

		return voltage_now*1000;
	} else {
		pr_info("No Fuel gauge supply registered return %d\n", DEFAULT_VOLTAGE);
	}

	/*
	 * Return default capacity to avoid userspace
	 * from shutting down unecessarily
	 */
	return DEFAULT_VOLTAGE;
}

#define DEFAULT_TEMP		250
static int get_prop_batt_temp(struct rt9536_info *info)
{
	int rc = 0;
	struct qpnp_vadc_result results;

#ifdef CONFIG_LGE_PM_PSEUDO_BATTERY
	if (get_pseudo_batt_info(PSEUDO_BATT_MODE)) {
		pr_debug("battery fake mode : %d\n", get_pseudo_batt_info(PSEUDO_BATT_MODE));
		return get_pseudo_batt_info(PSEUDO_BATT_TEMP) * 10;
	}
#endif
	if (lge_get_factory_boot()) {
		pr_info("[RT9536] Factory cable is connected\n");
		return DEFAULT_TEMP;
	}

	rc = qpnp_vadc_read(info->vadc_dev, LR_MUX1_BATT_THERM, &results);
	if (rc) {
		pr_debug("[RT9536] Unable to read batt temperature rc=%d\n", rc);
		return DEFAULT_TEMP;
	}

	pr_debug("get_bat_temp %d, %lld, %lld\n", results.adc_code,
							results.physical, results.measurement);

	return (int)results.physical;
}


static int get_prop_batt_health(struct rt9536_info *info)
{
#ifdef CONFIG_LGE_PM_CHARGING_CONTROLLER
	int battemp = 0;

	battemp = get_prop_batt_temp(info)/10;

	if(info->usb_present && (lgcc_is_probed == 1)){
		if (get_btm_state() == BTM_HEALTH_OVERHEAT)
			return POWER_SUPPLY_HEALTH_OVERHEAT;
		if (get_btm_state() == BTM_HEALTH_COLD)
			return POWER_SUPPLY_HEALTH_COLD;
		else
			return POWER_SUPPLY_HEALTH_GOOD;
	} else {
		if (battemp > BATT_TEMP_OVERHEAT)
			return POWER_SUPPLY_HEALTH_OVERHEAT;
		if (battemp < BATT_TEMP_COLD)
			return POWER_SUPPLY_HEALTH_COLD;
		else
			return POWER_SUPPLY_HEALTH_GOOD;
	}
#else
	return POWER_SUPPLY_HEALTH_GOOD;
#endif
}

static int get_prop_batt_present(struct rt9536_info *info)
{
	int temp = 0;

#ifdef CONFIG_LGE_PM_PSEUDO_BATTERY
	if (get_pseudo_batt_info(PSEUDO_BATT_MODE))
		return true;
#endif

	if (lge_get_factory_boot()) {
		pr_info("[RT9536] Factory cable is connected - batt_preset true\n");
		return true;
	}

	temp = get_prop_batt_temp(info);

	if (temp <= -300 || temp >= 790) {
		pr_err("\n\n  Battery missing(over temp : %d)\n\n", temp);
		return false;
		}

	return true;
}

static int rt9536_gen_mode_pulse(struct rt9536_info *info, int pulses)
{
	int i;

	spin_lock(&info->pulse_lock);

	for(i = 0 ; i < pulses ; i++) {
		rt9536_set_en_set(info, 1);
		udelay(ISET_UDELAY);
		rt9536_set_en_set(info, 0);
		udelay(ISET_UDELAY);
	}

	spin_unlock(&info->pulse_lock);

	return 0;
}

static int rt9536_gen_hv_pulse(struct rt9536_info *info)
{
	unsigned long flags;	/* spin_lock_irqsave */

	spin_lock_irqsave(&info->pulse_lock, flags);

	rt9536_set_en_set(info, 1);
	udelay(HVSET_UDELAY);
	rt9536_set_en_set(info, 0);

	spin_unlock_irqrestore(&info->pulse_lock, flags);

	return 0;
}

static int rt9536_set_mode(struct rt9536_info *info, enum rt9536_mode mode)
{
	int rc = 0;

	mutex_lock(&info->mode_lock);
	if (mode == RT9536_MODE_DISABLED) {
		rt9536_set_en_set(info, 1);
		usleep_range(MSET_UDELAY_L, MSET_UDELAY_H);

		info->mode = mode;
		info->cur_present = 0;
		info->status = POWER_SUPPLY_STATUS_NOT_CHARGING;

		mutex_unlock(&info->mode_lock);
		pr_err("[CHARGER] 1. rt9536_set_mode. mode(%d)\n",mode);
		return rc;
	}

#if 0       // TEST
	if (info->mode == mode) {
		mutex_unlock(&info->mode_lock);
        dev_err(&info->pdev->dev, "[CHARGER] 2. rt9536_set_mode. mode(%d)\n",mode);
		return rc;
	}
#endif /* 0 */

	/* to switch mode, need to disable charger ic */
	if (info->status == POWER_SUPPLY_STATUS_UNKNOWN) {
		/* do not disable charger ic for first command */
		pr_info("[CHARGER] 2. rt9536_set_mode. mode(%d), status(%d)\n",mode, info->status);
	} else {
		rt9536_set_en_set(info, 1);
		usleep_range(MSET_UDELAY_L, MSET_UDELAY_H);
	}

	/* start setting mode */
	rt9536_set_en_set(info, 0);
	usleep_range(START_UDELAY_L, START_UDELAY_H);

	switch(mode) {
	case RT9536_MODE_USB500:	/* 4 pulses */
		rt9536_gen_mode_pulse(info, 4);
		break;
	case RT9536_MODE_FACTORY:	/* 3 pulses */
		rt9536_gen_mode_pulse(info, 3);
		break;
	case RT9536_MODE_USB100:	/* 2 pulses */
		rt9536_gen_mode_pulse(info, 2);
		break;
	case RT9536_MODE_ISET:		/* 1 pulse  */
		rt9536_gen_mode_pulse(info, 1);
		break;
	default:
		break;

	}

	/* mode set */
	usleep_range(MSET_UDELAY_L, MSET_UDELAY_H);

	/* need extra pulse to set high voltage */
	if (info->cv_voltage > 4200000) {
		rt9536_gen_hv_pulse(info);
	}

	rt9536_iset_sel_set(info);

    pr_err("[CHARGER] 3. rt9536_set_mode. cv_voltage(%d) mode(%d)\n",info->cv_voltage,mode);

	info->mode = mode;
	info->cur_present = info->cur_next;
	info->status = POWER_SUPPLY_STATUS_CHARGING;

	mutex_unlock(&info->mode_lock);

	return rc;
}

static int rt9536_get_status(struct rt9536_info *info)
{
	int status;

	if(chg_full) {
		if (info->usb_present)
			status = POWER_SUPPLY_STATUS_FULL;
		else
			status = POWER_SUPPLY_STATUS_DISCHARGING;
		return status;
	}

#ifdef CONFIG_LGE_PM_CHARGING_CONTROLLER
	if (info->usb_present == 1
		&& (lgcc_is_probed == 1) && get_pseudo_ui()){
#ifdef CONFIG_LGE_PM
		if (!wake_lock_active(&info->chg_wake_lock))
			wake_lock(&info->chg_wake_lock);
#endif
		status = POWER_SUPPLY_STATUS_CHARGING;
		return status;
	}
#endif

#ifdef CONFIG_LGE_PM
	if (info->usb_present) {
		if( (info->cur_present > 100) ||(!rt9536_get_chgsb(info) ) ) {
			status = POWER_SUPPLY_STATUS_CHARGING;
		} else {
			if ((rt9536_get_en_set(info)) || (info->mode == RT9536_MODE_DISABLED)) {
				status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			} else
				status = POWER_SUPPLY_STATUS_DISCHARGING;
		}
	} else {
		status = POWER_SUPPLY_STATUS_DISCHARGING;
	}
#else
	if  (!rt9536_get_chgsb(info) ) {
		status = POWER_SUPPLY_STATUS_CHARGING;
	}  else  {
		if (!rt9536_get_en_set(info)) {
			status = POWER_SUPPLY_STATUS_DISCHARGING;
		} else {
			if (info->usb_present == true) {
				status = POWER_SUPPLY_STATUS_NOT_CHARGING;
			} else {
				status = POWER_SUPPLY_STATUS_DISCHARGING;
			}
		}
	}
#endif
	return status;
}

static int rt9536_enable(struct rt9536_info *info, int en)
{
	enum rt9536_mode mode = RT9536_MODE_DISABLED;
	int rc = 0;

#ifdef CONFIG_LGE_PM_CHARGING_CONTROLLER
	pr_err("rt9536_enable : lgcc_charging_current %d, info->cur_present %d\n", lgcc_charging_current, info->cur_present);
	if (lge_get_factory_boot()) {
			info->cur_next = 1500;
	} else {
		if((lgcc_charging_current != 0) && (info->usb_present))
			info->cur_next = lgcc_charging_current;
	}
	pr_info("[LGCC] setting charger current %d mA\n", info->cur_next);
#endif

#ifdef CONFIG_LGE_PM_USB_CURRENT_MAX
	if (usb_current_max_enabled) {
		if (info->usb_present) {
			pr_info("[LGCC] USB Current Max set charging current to %d(mA)\n", USB_CURRENT_MAX);
			info->cur_next  = USB_CURRENT_MAX;
		}
	}
#endif
	if (en) {
		if (info->cur_next > 1000)
			mode = RT9536_MODE_FACTORY;
		else if (info->cur_next > 500)
			mode = RT9536_MODE_ISET;
		else if (info->cur_next >= 400)
			mode = RT9536_MODE_USB500;
		else
			mode = RT9536_MODE_USB100;
	}

	pr_err("[CHARGER]rt9536_enable. en(%d) mode(%d)\n", en, mode);

	rc = rt9536_set_mode(info, mode);
	info->prev_mode = mode;

	if (rc) {
		pr_err("faild to set rt9536 mode to %d.\n", mode);
	}

	return rc;
}

static void rt9536_dwork(struct work_struct *work)
{
	struct rt9536_info *info = container_of(to_delayed_work(work), struct rt9536_info, dwork);
	int rc;

	union power_supply_propval ret = {0,};

	rc = info->usb_psy->get_property(info->usb_psy,
				POWER_SUPPLY_PROP_TYPE, &ret);
	if (rc)
		pr_err("Couldn't read USB current_max property, rc=%d\n", rc);

	pr_info("[RT9536] Charge_type=%d info->cur_present=%d info->cur_next=%d lgcc_charging_current=%d\n",
						ret.intval, info->cur_present, info->cur_next, lgcc_charging_current);

	if (ret.intval == POWER_SUPPLY_TYPE_USB && info->usb_present == 0)
		goto charge_type_unknown;

	if (ret.intval == POWER_SUPPLY_TYPE_USB_DCP ||
		ret.intval == POWER_SUPPLY_TYPE_MAINS) {
		info->enable = RT9536_MODE_ISET;
		info->cur_next = 810;
		pr_info("[RT9536] DCP is connected\n");
	} else if (is_factory_cable()) {
		info->enable = RT9536_MODE_FACTORY;
		info->cur_next = 1500;
		pr_info("[RT9536] Factory Cable is connected\n");
	} else if (ret.intval == POWER_SUPPLY_TYPE_USB){
		info->enable = RT9536_MODE_USB500;
		info->cur_next = 500;
		pr_info("[RT9536] USB is connected\n");
	} else {
		charge_type_unknown :
		info->enable = RT9536_MODE_USB100;
		pr_info("[RT9536] Calbe is not connected\n");
	}

	rc = rt9536_enable(info, info->enable);
	if (rc) {
		pr_err("retry after %dms\n", RT9536_RETRY_MS);
		queue_delayed_work(rt9536_wq, &info->dwork, msecs_to_jiffies(RT9536_RETRY_MS));
	}
	return;
}

static irqreturn_t rt9536_chg_start_irq_handler(int irq, void *_info)
{
	struct rt9536_info *info = _info;
	int en_set , cable_present = 0;

	info->status = rt9536_get_status(info);
	en_set = rt9536_get_en_set(info);
	cable_present = !rt9536_get_pgb(info);

	pr_info("[RT9536] First check cable_present = %d\n", cable_present);
	pr_info("[RT9536] charging start triggered! status = %d en_set=%d\n",
			info->status, en_set);

	/* Cable connected */
	if (cable_present == 1)
	{
		info->usb_present = true;
		rt9536_set_en_set(info, 0);
		if (!wake_lock_active(&info->cable_on_wake_lock))
			wake_lock(&info->cable_on_wake_lock);
	}
	/* Cable disconnected */
	else
	{
		info->usb_present = false;
		goto cable_removed;
	}

	pr_debug("[RT9536] usb_present = %d\n",info->usb_present);

#ifdef CONFIG_LGE_PM_CHARGING_CONTROLLER
	if (lgcc_is_probed == 1) {
		stop_battemp_work();
		start_battemp_work(2);
	}
#endif
	power_supply_set_present(info->usb_psy, info->usb_present);
	power_supply_set_online(info->usb_psy, info->usb_present);
	queue_delayed_work(rt9536_wq, &info->dwork, msecs_to_jiffies(RT9536_RETRY_MS));
	return IRQ_HANDLED;

cable_removed:

	pr_info("[RT9536] Cable removed! Charging disable\n");
	power_supply_set_present(info->usb_psy, info->usb_present);
	power_supply_set_online(info->usb_psy, info->usb_present);
	chg_done = false;
	info->mode = RT9536_MODE_DISABLED;
	rt9536_set_en_set(info, 1);

	if (wake_lock_active(&info->chg_wake_lock))
		wake_unlock(&info->chg_wake_lock);

	if (wake_lock_active(&info->cable_on_wake_lock))
		wake_unlock(&info->cable_on_wake_lock);

	return IRQ_HANDLED;
}

static irqreturn_t rt9536_chg_done_irq_handler(int irq, void *_info)
{
	struct rt9536_info *info = _info;

	info->status = rt9536_get_status(info);
	pr_info("[RT9536] chg_done handler triggered!status= %d\n",info->status);

	if( is_first_boot == true && chg_done == false){
		pr_info("[RT9536] is_first_boot, Skip charging done IRQ status = %d\n",info->status);
		return IRQ_HANDLED;
	}
	if(info->usb_present == 1 && raw_capacity > 97){
		pr_info("[RT9536] End of charging! status = %d\n", info->status);
		rt9536_set_en_set(info, 1);
		chg_done = true;
		power_supply_changed(&info->psy);
		if (wake_lock_active(&info->cable_on_wake_lock))
			wake_unlock(&info->cable_on_wake_lock);
	}

	return IRQ_HANDLED;
}

static int rt9536_request_irq(struct rt9536_info *info)
{
	int rc;

	/*PGB interrupt configuration*/
	info->irq = gpio_to_irq(info->pgb);
	rc = request_irq(info->irq, rt9536_chg_start_irq_handler,
				IRQF_TRIGGER_FALLING |IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"rt9536_pgb_start", info);
	if (rc < 0) {
		pr_err("Unable to request rt9536_pgb_start %d", rc);
		disable_irq_wake(info->irq);
	} else {
		enable_irq_wake(info->irq);
	}
	/*CHGSB interrupt configuration*/
	info->irq = gpio_to_irq(info->chgsb);
	rc = request_irq(info->irq, rt9536_chg_done_irq_handler,
				IRQF_TRIGGER_RISING | IRQF_ONESHOT,
				"rt9536_chg_done_irq_handler", info);
	if (rc < 0) {
		pr_err("Unable to request rt9536_chg_start %d", rc);
		disable_irq_wake(info->irq);
	} else {
		enable_irq_wake(info->irq);
	}

	pr_info("[DEBUG] rt9536 request_irq rc=%d\n",rc);
	return rc;
}

#define QPNP_CHG_I_MAX_MIN_90 90
static void rt9536_external_power_changed(struct power_supply *psp)
{
	struct rt9536_info *info = container_of(psp,
						struct rt9536_info, psy);
	union power_supply_propval ret = {0,};
	int current_ma = 0;

	pr_info("[RT9536] external_power_changed\n");

	info->fg_psy= power_supply_get_by_name("fuelgauge");

	if(info->prev_mode == info->mode)
		goto skip_current_config;

	 if (is_factory_cable()) {
		info->enable = RT9536_MODE_FACTORY;
		info->cur_next = 1500;
		pr_info("[RT9536] Factory Cable is connected!!!\n");
		rt9536_enable(info, info->enable);
		goto skip_current_config;
	 }
	if (info->usb_present ){
		info->usb_psy->get_property(info->usb_psy,
				POWER_SUPPLY_PROP_CURRENT_MAX, &ret);
		current_ma = ret.intval / 1000;

		if (info->mode == info->prev_mode)
			goto skip_current_config;

		info->cur_next = current_ma;
		rt9536_enable(info, 1);
	}

skip_current_config:
	power_supply_changed(&info->psy);
}

#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
static int get_prop_batt_id_valid(void)
{
	return (int)is_lge_battery_valid();
}

static bool get_prop_batt_id_for_aat(void)
{
	static int check_batt_id;
	if (read_lge_battery_id())
		check_batt_id = 1;
	else
		check_batt_id = 0;

	return check_batt_id;
}

#endif

static enum power_supply_property rt9536_props[] = {
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_CHARGE_TYPE,
	POWER_SUPPLY_PROP_HEALTH,
	POWER_SUPPLY_PROP_PRESENT,
	POWER_SUPPLY_PROP_TEMP,
	POWER_SUPPLY_PROP_CAPACITY,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGING_ENABLED,
	POWER_SUPPLY_PROP_CHARGING_COMPLETE,
#ifdef CONFIG_LGE_PM_PSEUDO_BATTERY
	POWER_SUPPLY_PROP_PSEUDO_BATT,
#endif
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
	POWER_SUPPLY_PROP_BATTERY_ID_CHECKER,
	POWER_SUPPLY_PROP_VALID_BATT,
	POWER_SUPPLY_PROP_CHECK_BATT_ID_FOR_AAT,
#endif
#ifdef CONFIG_LGE_PM_USB_CURRENT_MAX
	POWER_SUPPLY_PROP_USB_CURRENT_MAX,
#endif

};

static int rt9536_get_property(struct power_supply *psy_p,
				enum power_supply_property psp,
				union power_supply_propval *val)
{
	struct rt9536_info *info = container_of(psy_p, struct rt9536_info, psy);
	int rc = 0;

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		val->intval = rt9536_get_status(info);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		val->intval = info->cv_voltage;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = get_prop_voltage_now(info);
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		if (info->mode == RT9536_MODE_DISABLED)
			val->intval = 0;
		else
			val->intval = info->cur_present;
		break;
	case POWER_SUPPLY_PROP_CHARGE_TYPE:
		val->intval = get_prop_charge_type(info);
		break;
	case POWER_SUPPLY_PROP_HEALTH:
		val->intval = get_prop_batt_health(info);
		break;
	case POWER_SUPPLY_PROP_PRESENT:
		val->intval = get_prop_batt_present(info);
		break;
	case POWER_SUPPLY_PROP_TEMP:
		val->intval = get_prop_batt_temp(info);
		break;
	case POWER_SUPPLY_PROP_CAPACITY:
		val->intval = get_prop_capacity(info);
#ifdef CONFIG_LGE_PM_PSEUDO_BATTERY
		if (get_pseudo_batt_info(PSEUDO_BATT_MODE)) {
			val->intval = get_pseudo_batt_info(PSEUDO_BATT_CAPACITY);
		}
#endif
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
//		val->intval = get_prop_current_now(chip);
		break;
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		val->intval = !info->enable;
		break;
	case POWER_SUPPLY_PROP_CHARGING_COMPLETE:
		if (get_prop_capacity(info) >= 100) {
			val->intval = 0;
		} else {
			val->intval = 1;
		}
		break;
#ifdef CONFIG_LGE_PM_PSEUDO_BATTERY
	case POWER_SUPPLY_PROP_PSEUDO_BATT:
		val->intval = get_pseudo_batt_info(PSEUDO_BATT_MODE);
		break;
#endif
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
	case POWER_SUPPLY_PROP_BATTERY_ID_CHECKER:
		val->intval = read_lge_battery_id();
		break;
	case POWER_SUPPLY_PROP_VALID_BATT:
		val->intval = get_prop_batt_id_valid();
		break;
	case POWER_SUPPLY_PROP_CHECK_BATT_ID_FOR_AAT:
		val->intval = get_prop_batt_id_for_aat();
		break;
#endif
#ifdef CONFIG_LGE_PM_USB_CURRENT_MAX
	case POWER_SUPPLY_PROP_USB_CURRENT_MAX:
		if (usb_current_max_enabled)
			val->intval = 1;
		else
			val->intval = 0;
		break;
#endif

	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}

static int rt9536_set_property(struct power_supply *psy_p,
				enum power_supply_property psp,
				const union power_supply_propval *val)
{
	struct rt9536_info *info = container_of(psy_p, struct rt9536_info, psy);
	int rc = 0;

    pr_err("[CHARGER] rt9536_set_property -- \n");

	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		switch (val->intval) {
		case POWER_SUPPLY_STATUS_CHARGING:
			info->enable = 1;
			break;
		case POWER_SUPPLY_STATUS_DISCHARGING:
		case POWER_SUPPLY_STATUS_NOT_CHARGING:
			info->enable = 0;
			break;
		default:
			rc = -EINVAL;
			break;
		}
		if (!rc)
			queue_delayed_work(rt9536_wq, &info->dwork, 0);
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
		info->cv_voltage = val->intval;
		break;
	case POWER_SUPPLY_PROP_CURRENT_MAX:
		info->cur_next = val->intval;
		break;

	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
		info->enable = val->intval;
#ifdef CONFIG_LGE_PM_BATTERY_ID_CHECKER
		if(!get_prop_batt_id_valid()) {
			rc = rt9536_enable(info, 0);
			if (rc)
				pr_err("Failed to disable charging rc=%d\n", rc);
		} else {
			rc = rt9536_enable(info, info->enable);
			if (rc)
				pr_err("Failed to disable charging rc=%d\n", rc);
		}
		break;
#endif
#ifdef CONFIG_LGE_PM_USB_CURRENT_MAX
	case POWER_SUPPLY_PROP_USB_CURRENT_MAX:
		if (val->intval)
			usb_current_max_enabled = 1;
		else
			usb_current_max_enabled = 0;

		rc = rt9536_enable(info, 1);
		if (rc) {
			pr_err("retry after %dms\n", RT9536_RETRY_MS);
			queue_delayed_work(rt9536_wq, &info->dwork, msecs_to_jiffies(RT9536_RETRY_MS));
		}

		break;
#endif

	default:
		rc = -EINVAL;
		break;
	}

	rt9536_external_power_changed(&info->psy);

	return rc;
}

static int rt9536_property_is_writeable(struct power_supply *psy,
				     enum power_supply_property psp)
{
	int rc = 0;
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
	case POWER_SUPPLY_PROP_VOLTAGE_MAX:
	case POWER_SUPPLY_PROP_CURRENT_MAX:
	case POWER_SUPPLY_PROP_CHARGING_ENABLED:
#ifdef CONFIG_LGE_PM_USB_CURRENT_MAX
	case POWER_SUPPLY_PROP_USB_CURRENT_MAX:
#endif
		rc = 1;
		break;
	default:
		rc = 0;
		break;
	}
	return rc;
}


static struct power_supply rt9536_psy = {
	.name = "battery",
	.type = POWER_SUPPLY_TYPE_BATTERY,
	.properties	= rt9536_props,
	.num_properties = ARRAY_SIZE(rt9536_props),
	.get_property	= rt9536_get_property,
	.set_property	= rt9536_set_property,
	.external_power_changed = rt9536_external_power_changed,
	.supplied_to = pm_batt_supplied_to,
	.num_supplicants = ARRAY_SIZE(pm_batt_supplied_to),
	.property_is_writeable = rt9536_property_is_writeable,
};


static int rt9536_parse_dt(struct platform_device *pdev)
{
	struct rt9536_info *info =
		(struct rt9536_info*)platform_get_drvdata(pdev);
	struct device_node *np = pdev->dev.of_node;

    pr_err("[rt9536] rt9536_parse_dt - start \n");

	/* iset_sel gpio */
	info->iset_sel = of_get_named_gpio(np,"iset_sel", 0);
	pr_info("iset_sel = %d\n", info->iset_sel);
	if (info->iset_sel < 0) {
		pr_err("iset_sel is not defined.\n");
		return info->iset_sel;
		}

	/*en_set gpio */
	info->en_set = of_get_named_gpio(np,"en_set", 0);
	pr_info("en_set = %d\n", info->en_set);
	if (info->en_set < 0) {
		pr_err("en_set is not defined.\n");
		return info->en_set;
		}

	/*chgsb gpio */
	info->chgsb = of_get_named_gpio(np,"chgsb", 0);
	pr_info("chgsb = %d\n", info->chgsb);
	if (info->chgsb < 0) {
		pr_err("chgsb is not defined.\n");
		return info->chgsb;
		}

	/*pgb gpio */
	info->pgb = of_get_named_gpio(np,"pgb", 0);
	pr_info("pgb = %d\n", info->pgb);
	if (info->pgb < 0) {
		pr_err("pgb is not defined.\n");
		return info->pgb;
		}

	info->pin = devm_pinctrl_get(&pdev->dev);
	info->boot = pinctrl_lookup_state(info->pin, "default");
	info->charging = pinctrl_lookup_state(info->pin, "charging");
	info->not_charging = pinctrl_lookup_state(info->pin, "not_charging");
	info->vbus_det = pinctrl_lookup_state(info->pin, "vbus_det");


#ifdef CONFIG_LGE_PM_CABLE_DETECTION
	get_cable_data_from_dt(np);
#endif

    pr_err("[rt9536] rt9536_parse_dt - end \n");

	return 0;
}

#if defined (CONFIG_LGE_PM_CHARGING_CONTROLLER)
int lgcc_is_charger_present(void){

	union power_supply_propval prop = {0,};
	int rc, online = 0;

	if (rt9536_info_controller == NULL){
		pr_info("[RT9536] chip is NULL\n");
		return 0;
	}
	rc = rt9536_info_controller->usb_psy->get_property(rt9536_info_controller->usb_psy,
				POWER_SUPPLY_PROP_ONLINE, &prop);
	if (rc)
		pr_err("Couldn't read USB online property, rc=%d\n", rc);
	else
		online = prop.intval;

	return online;
}
EXPORT_SYMBOL(lgcc_is_charger_present);

int lgcc_set_ibat_current(int chg_current){

	int ret = 0;

	if (lge_get_factory_boot()){
		pr_info("[RT9536] Return lgcc for factory mode\n");
		return 0;
	}

	lgcc_charging_current = chg_current;
	if (!lgcc_charging_current) {
		pr_err("[RT9536] lgcc_charging_current is NULL\n");
		return 0;
	}
	if (rt9536_info_controller == NULL || rt9536_info_controller->cur_present == 0){
		pr_info("[RT9536] chip is NULL\n");
		return 0;
	}
#ifdef CONFIG_LGE_PM_CHARGING_CONTROLLER
	rt9536_info_controller->cur_next = lgcc_charging_current;
#endif

	pr_info("[RT9536] LGCC : lgcc_charging_current = %d\n", lgcc_charging_current);
	pr_err("[RT9536] LGCC : rt9536_info_controller->cur_next = %d\n", rt9536_info_controller->cur_next);
	ret = rt9536_enable(rt9536_info_controller, 1);

	return ret;
}
EXPORT_SYMBOL(lgcc_set_ibat_current);

int lgcc_set_charging_enable(int enable)
{
	int ret = 0;

	if (lge_get_factory_boot()){
		pr_info("[RT9536] Return lgcc for factory mode\n");
		return 0;
	}

	rt9536_info_controller->enable =  enable;
	ret = rt9536_enable(rt9536_info_controller, enable);

	return ret;
}
EXPORT_SYMBOL(lgcc_set_charging_enable);

void lgcc_charger_reginfo(void)
{
	int iset_sel, en_set, chgsb, pgb, batt_volt, batt_temp = 0;

	iset_sel = rt9536_get_iset_sel(rt9536_info_controller);
	en_set = rt9536_get_en_set(rt9536_info_controller);
	chgsb = rt9536_get_chgsb(rt9536_info_controller);
	pgb = rt9536_get_pgb(rt9536_info_controller);
	batt_volt = get_prop_voltage_now(rt9536_info_controller)/1000;
	batt_temp = get_prop_batt_temp(rt9536_info_controller)/10;

	pr_info("[LGCC] reginfo : en_set = %d, chgsb = %d, pgb = %d, iset_sel = %d, USB_IN = %d"
		    "mode = %d, batt_volt = %d, batt_temp = %d\n",
				en_set, chgsb, pgb, iset_sel, rt9536_info_controller->usb_present,
				rt9536_info_controller->mode, batt_volt, batt_temp);
}
EXPORT_SYMBOL(lgcc_charger_reginfo);
#endif

static char *cable_type_str[] = {
		"NOT INIT", "MHL 1K", "U_28P7K", "28P7K", "56K",
		"100K", "130K", "180K", "200K", "220K",
		"270K", "330K", "620K", "910K", "OPEN"
	};

static char *power_supply_type_str[] = {
		"Unknown", "Battery", "UPS", "Mains", "USB",
		"USB_DCP", "USB_CDP", "USB_ACA", "Wireless", "BMS",
		"USB_Parallel", "HVDCP", "fuelgauge", "Wipower"
	};

static char *batt_fake_str[] = {
		"FAKE_OFF", "FAKE_ON"
	};

static char *set_mode_str[] = {
		"DISABLED", "USB500","ISET","USB100","FACTORY","MAX"
	};

static char *chgsb_stat_str[] = {
		"chgsb_low", "chgsb_high"
	};
static char *sw_en_stat_str[] = {
		"sw_en_low", "sw_en_high"
	};

static char* get_usb_type(struct rt9536_info *info)
{
	union power_supply_propval ret = {0,};

	if (!info->usb_psy)
	{
		pr_info("usb power supply is not registerd\n");
		return "null";
	}

	info->usb_psy->get_property(info->usb_psy,
			POWER_SUPPLY_PROP_TYPE, &ret);

	return power_supply_type_str[ret.intval];
}

static void charging_information(struct work_struct *work)
{
	struct rt9536_info *info =
				container_of(work, struct rt9536_info, charging_inform_work.work);
	struct qpnp_vadc_result adc_result;
	int usb_present = info->usb_present;
	char *usb_type_name = get_usb_type(info);
	char *cable_type_name = cable_type_str[lge_pm_get_cable_type()];
	char *batt_fake = batt_fake_str[get_pseudo_batt_info(PSEUDO_BATT_MODE)];
	int batt_temp = get_prop_batt_temp(info)/10;
	int batt_soc = get_prop_capacity(info);
	int batt_vol = get_prop_voltage_now(info)/1000;
	char *set_mode = set_mode_str[info->mode];
	char *chgsb = chgsb_stat_str[rt9536_get_chgsb(info)];
	char *sw_en = sw_en_stat_str[rt9536_get_en_set(info)];
	int chgsb_val = rt9536_get_chgsb(info);
	int xo_therm, rc;

	if(batt_soc == 100)
		chg_full = true;
	else
		chg_full = false;

	rc = qpnp_vadc_read(info->vadc_dev, LR_MUX3_XO_THERM, &adc_result);
	if (rc) {
		pr_info("error XO_THERM read rc = %d\n", rc);
		return;
	}
	xo_therm = adc_result.physical;

	pr_info("[C], USB_PRESENT, USB_TYPE, CABLE_INFO, XO_THERM,"
			" BATT_FAKE, BATT_TEMP, BATT_SOC, BATT_VOL,"
			" set_mode,"
			" CHGSB, EN_SW,"
			"\n");
	pr_info("[I], %d, %s, %s, %d; "
			"%s, %d, %d, %d; "
			"%s "
			"%s, %s\n",
			usb_present, usb_type_name, cable_type_name, xo_therm,
			batt_fake, batt_temp, batt_soc, batt_vol,
			set_mode,
			chgsb, sw_en);

#ifdef CONFIG_LGE_PM_EXTERNAL_CHARGER_RT9536_SUPPORT
	/*It is for EOC (End Of Charging) Check polling *
	  *to prevent from battery OVP                          */

	if(info->usb_present == 1 && raw_capacity > 97 && chgsb_val){
		pr_info("[RT9536] End of charging!Polling check status = %d\n", info->status);
		rt9536_set_en_set(info, 1);
		chg_done = true;
		if (wake_lock_active(&info->cable_on_wake_lock))
			wake_unlock(&info->cable_on_wake_lock);
	}

	/*It is for recharging polling*/
#if defined(CONFIG_MSM8909_K6B) || defined(CONFIG_MACH_MSM8909_LV1_GLOBAL_COM)
	if (chg_done && (raw_capacity < 94)){
		pr_info("[RT9536] Recharging start! chg_done is false!\n");
		chg_done = false;
		rt9536_set_en_set(info, 0);
		if (!wake_lock_active(&info->cable_on_wake_lock))
			wake_lock(&info->cable_on_wake_lock);
		queue_delayed_work(rt9536_wq, &info->dwork, msecs_to_jiffies(RT9536_RETRY_MS));
	}
#endif
#endif
	schedule_delayed_work(&info->charging_inform_work,
		round_jiffies_relative(msecs_to_jiffies(CHARGING_INFORM_NORMAL_TIME)));
}


/* Get/Set initial state of charger */
static void determine_initial_status(struct rt9536_info *info)
{

	info->usb_present = !rt9536_get_pgb(info);

	/*
	 * Set USB psy online to avoid userspace from shutting down if battery
	 * capacity is at zero and no chargers online.
	 */
	pr_info("[RT9536] Initial Status usb_present = %d\n",info->usb_present);
	power_supply_set_present(info->usb_psy, info->usb_present);
	power_supply_set_online(info->usb_psy, info->usb_present);

	if (info->usb_present == true) {
		if (!wake_lock_active(&info->cable_on_wake_lock)){
			pr_info("[RT9536] Wake Lock\n");
			wake_lock(&info->cable_on_wake_lock);
		}
	} else {
		if (wake_lock_active(&info->cable_on_wake_lock))
			wake_unlock(&info->cable_on_wake_lock);
	}
	if (!lge_get_factory_boot())
		queue_delayed_work(rt9536_wq, &info->dwork, msecs_to_jiffies(RT9536_RETRY_MS));
	pr_err("[rt9536] rt9536_initial state usb_present=%d\n",info->usb_present);
}

static int rt9536_probe(struct platform_device *pdev)
{
	struct rt9536_info *info;
	struct power_supply *usb_psy;

	int rc;

	usb_psy = power_supply_get_by_name("usb");
	if (!usb_psy) {
		pr_info("USB psy not found; deferring probe\n");
		return -EPROBE_DEFER;
	}

	pr_err("[rt9536] rt9536_probe - start \n");

	info = (struct rt9536_info*)kzalloc(sizeof(*info), GFP_KERNEL);
	if (!info) {
		pr_err("memory allocation failed.\n");
		return -ENOMEM;
	}
	platform_set_drvdata(pdev, info);

	/* initialize device info */
	info->pdev = pdev;
#if defined(CONFIG_MACH_MSM8909_LV1_GLOBAL_COM)
	info->cv_voltage = 4400000;
#else
	info->cv_voltage = 4350000;
#endif
	info->mode = RT9536_MODE_DISABLED;
	info->status = POWER_SUPPLY_STATUS_UNKNOWN;

	info->usb_psy = usb_psy;

	spin_lock_init(&info->pulse_lock);
	mutex_init(&info->mode_lock);
	INIT_DELAYED_WORK(&info->dwork, rt9536_dwork);

	is_first_boot = true;
	/* read device tree */
	rc = rt9536_parse_dt(pdev);
	if (rc) {
		pr_err("cannot read from fdt.\n");
		return rc;
	}
	/* get ADC*/
	if (!lge_get_factory_boot()){
		info->vadc_dev = qpnp_get_vadc(&(pdev->dev), "rt9536");
		if (IS_ERR(info->vadc_dev)) {
			rc = PTR_ERR(info->vadc_dev);
			if (rc != -EPROBE_DEFER)
				pr_err("vadc property missing\n");
			return rc;
		}
	}
	/* initialize device */
	rt9536_gpio_init(info);
	if (lge_get_factory_boot()) {
		   info->enable = RT9536_MODE_FACTORY;
		   info->cur_next = 1500;
		   pr_info("[RT9536] Factory Cable is connected!!!\n");
		   rt9536_enable(info, RT9536_MODE_FACTORY);
	}
	if (rt9536_get_en_set(info))
		pr_err("[rt9536] : en_set high\n");
	else
		pr_err("[rt9536] : en_set low\n");

	/* initialize device */
	rt9536_request_irq(info);
	wake_lock_init(&info->cable_on_wake_lock,
		WAKE_LOCK_SUSPEND, "cable_on");
	wake_lock_init(&info->chg_wake_lock,
			WAKE_LOCK_SUSPEND, "chg_on");

	determine_initial_status(info);

	/* register class */
	info->psy = rt9536_psy;
	rc = power_supply_register(&pdev->dev, &info->psy);
	if (rc) {
		pr_err("power supply register failed.\n");
		return rc;
	}
#if defined (CONFIG_LGE_PM_CHARGING_CONTROLLER)
	rt9536_info_controller = info;
#endif

	pinctrl_select_state(info->pin, info->not_charging);

	INIT_DELAYED_WORK(&info->charging_inform_work, charging_information);
	schedule_delayed_work(&info->charging_inform_work,
		round_jiffies_relative(msecs_to_jiffies(CHARGING_INFORM_NORMAL_TIME)));

	charging_hw_init_done = 1;   /* TRUE */

	pr_err("[rt9536] rt9536_probe - done \n");
	is_first_boot = false;

	return 0;
}
static int rt9536_suspend(struct device *dev)
{
	pr_info("[RT9536] rt9536_suspend\n");
	cancel_delayed_work_sync(&rt9536_info_controller->charging_inform_work);

	return 0;
}

static int rt9536_resume(struct device *dev)
{
	pr_info("[RT9536] rt9536_resume\n");
	cancel_delayed_work_sync(&rt9536_info_controller->charging_inform_work);
	schedule_delayed_work(&rt9536_info_controller->charging_inform_work,
		round_jiffies_relative(msecs_to_jiffies(20)));

	return 0;
}

static int rt9536_remove(struct platform_device *pdev)
{
	struct rt9536_info *info;

	info = platform_get_drvdata(pdev);
	if (info)
		kfree(info);

	return 0;
}

static struct of_device_id rt9536_of_device_id[] = {
	{
		.compatible = "richtek,rt9536",
	},
};

static const struct dev_pm_ops rt9536_pm_ops = {
	.suspend	= rt9536_suspend,
	.resume		= rt9536_resume,
};


static struct platform_driver rt9536_driver = {
	.probe		= rt9536_probe,
	.remove		= rt9536_remove,
	.driver		= {
		.name	= "rt9536",
		.owner	= THIS_MODULE,
		.of_match_table = rt9536_of_device_id,
		.pm 	= &rt9536_pm_ops,
	},
};

static int __init rt9536_init(void)
{
	rt9536_wq = create_singlethread_workqueue("rt9536_wq");
	if (!rt9536_wq) {
		printk("cannot create workqueue for rt9536\n");
		return -ENOMEM;
	}

	if (platform_driver_register(&rt9536_driver))
		return -ENODEV;

	return 0;
}

static void __exit rt9536_exit(void)
{
	platform_driver_unregister(&rt9536_driver);

	if (rt9536_wq)
		destroy_workqueue(rt9536_wq);
}
module_init(rt9536_init);
module_exit(rt9536_exit);

MODULE_DESCRIPTION("Richtek RT9536 Driver");
MODULE_LICENSE("GPL");

