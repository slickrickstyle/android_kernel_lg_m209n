/* Copyright (C) 2016 LG Electronics Co., Ltd.
 *
 * This file defines functions for blocking/releasing charging to support
 *  the "showcase mode" in device stores.
 *
 * Basically, there is a polling worker monitoring battery SoC, and it controls
 *  charging to maintain 50(==SHOWCASE_SOC_MIN) < SoC < 45(==SHOWCASE_SOC_MAX).
 *  It is implemented with 3 hookers in below :
 *   1) smb1351_battery_set_property_lge  for smb1351_battery_set_property
 *   2) smb1351_battery_get_property_lge  for smb1351_battery_get_property
 *   3) smb1351_main_probe_lge            for smb1351_main_probe
 *  with an extended primitive SHOWCASE_MODE = BIT(11)
 *
 * This file is designed only for the showcase mode in short-term, but anyone
 *  can reuse this structure to implement their own purpose which extends
 *  the base charger driver.
 */

 /* Include base charger driver here and modify conditional build options.
 * : Refer to ./Makefile
 */
#include "smb1351-charger.c"

/* MIN/MAX thresholds for charging control
 */
#define SHOWCASE_SOC_MAX 50
#define SHOWCASE_SOC_MIN 45
#define MONITOR_BATTEMP_POLLING_PERIOD (600*10)

/* an extended reason for en/disabling charging : "by Showcase mode"
 */
enum {
	SHOWCASE_MODE = BIT(11)
};

enum showcase_status {
	SHOWCASE_NOT_ENABLED = 0,
	SHOWCASE_NO_CHARGER,
	SHOWCASE_CHARGING_ENABLE,
	SHOWCASE_CHARGING_DISABLE
};

/* Members :
 *   1) States
 *     - struct delayed_work	showcase_charging_work;
 *     - bool			showcase_charging_mode;
 *     - bool			showcase_charging_blocked;
 *   2) Helpers
 *     - void 			showcase_charging_block(*)
 *     - void 			showcase_charging_release(*)
 *     - enum showcase_status 	showcase_charging_status(*)
 *     - void 			showcase_charging_monitor(*)
 *   3) Hookers
 *     - smb1351_battery_set_property_lge  for qpnp_batt_power_set_property
 *     - smb1351_battery_get_property_lge  for qpnp_batt_power_get_property
 *     - smb1351_main_probe_lge            for smb1351_main_probe
 */

static struct delayed_work showcase_charging_work;
static bool showcase_charging_mode;
static bool showcase_charging_blocked;

static void showcase_charging_block(void)
{
	if( showcase_charging_blocked ) {
		return;
	}
	pr_info("[Showcase mode] charging is blocked\n");

	smb1351_charging_disable(smb1351_chip, SHOWCASE_MODE, 1);
	showcase_charging_blocked = true;
}

static void showcase_charging_release(void)
{
	if( !showcase_charging_blocked ) {
		return;
	}
	pr_info("[Showcase mode] charging is released\n");
	smb1351_charging_disable(smb1351_chip, SHOWCASE_MODE, 0);
	showcase_charging_blocked = false;
}

static enum showcase_status showcase_charging_status(void)
{
	enum showcase_status status;

	if( !showcase_charging_mode ) {
		status = SHOWCASE_NOT_ENABLED;
		pr_info("[Showcase mode][polling] SHOWCASE_NOT_ENABLED, !IllegalStateException!\n");
	}
	else if( !smb1351_chip->chg_present ) {
		status = SHOWCASE_NO_CHARGER;
		pr_info("[Showcase mode][polling] SHOWCASE_NO_CHARGER\n");
	}
	else { // In the case of (showcase is set && charger is plugged)
		int capacity = smb1351_get_prop_batt_capacity(smb1351_chip);

		if( showcase_charging_blocked ) {
			if( capacity < SHOWCASE_SOC_MIN ) {
				status = SHOWCASE_CHARGING_ENABLE;
				pr_info("[Showcase mode][polling] SHOWCASE_CHARGING_ENABLE, capacity=%d\n",
					capacity);
			} else {
				status = SHOWCASE_CHARGING_DISABLE;
				pr_info("[Showcase mode][polling] SHOWCASE_CHARGING_DISABLE, capacity=%d\n",
					capacity);
			}
		} else {
			if( capacity > SHOWCASE_SOC_MAX ) {
				status = SHOWCASE_CHARGING_DISABLE;
				pr_info("[Showcase mode][polling] SHOWCASE_CHARGING_DISABLE, capacity=%d\n",
					capacity);
			} else {
				status = SHOWCASE_CHARGING_ENABLE;
				pr_info("[Showcase mode][polling] SHOWCASE_CHARGING_ENABLE, capacity=%d\n",
					capacity);
			}
		}
	}

	return status;
}

static void showcase_charging_monitor(struct work_struct *work)
{
	enum showcase_status status = showcase_charging_status();

	if( status == SHOWCASE_CHARGING_DISABLE ){
		showcase_charging_block();
	} else {
		showcase_charging_release();
	}

	schedule_delayed_work(&showcase_charging_work,
		MONITOR_BATTEMP_POLLING_PERIOD);
}

int smb1351_battery_set_property_lge(struct power_supply *psy,
	enum power_supply_property psp,
	const union power_supply_propval *val)
{
	struct smb1351_charger *chip = container_of(psy, struct smb1351_charger,
		batt_psy);
	switch( psp )
	{
	case POWER_SUPPLY_PROP_STORE_DEMO_ENABLED:
		showcase_charging_mode = val->intval;
		pr_info("[Showcase mode] showcase_charging_mode is set %d\n",
			val->intval);

		if( showcase_charging_mode ) {
			schedule_delayed_work(&showcase_charging_work,1);
		} else {
			cancel_delayed_work_sync(&showcase_charging_work);
			showcase_charging_release();
		}
		break;
	default :
		return smb1351_battery_set_property(psy, psp, val);
	}

	power_supply_changed(&chip->batt_psy);
	return 0;
}

int smb1351_battery_get_property_lge(struct power_supply *psy,
	enum power_supply_property psp,
	union power_supply_propval *val)
{
	switch( psp )
	{
	case POWER_SUPPLY_PROP_STORE_DEMO_ENABLED:
		pr_info("[Showcase mode] Is the showcase mode enabled? %d\n",
			showcase_charging_mode);
		val->intval = showcase_charging_mode;
		break;
	default:
		return smb1351_battery_get_property(psy, psp, val);
	}

	return 0;
}

void smb1351_main_probe_lge(void)
{
	pr_info("[Showcase mode] Initializing showcase_charging_work.\n");
	INIT_DELAYED_WORK(&showcase_charging_work, showcase_charging_monitor);
}
