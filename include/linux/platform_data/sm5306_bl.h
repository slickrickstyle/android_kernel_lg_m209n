/*
* Simple driver for Texas Instruments SM5306 LED Flash driver chip
* Copyright (C) 2012 Texas Instruments
*
* This program is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License version 2 as
* published by the Free Software Foundation.
*
*/

#ifndef __LINUX_SM5306_H
#define __LINUX_SM5306_H

#define SM5306_NAME "sm5306_bl"

enum sm5306_pwm_ctrl {
	PWM_CTRL_DISABLE = 0,
	PWM_CTRL_BANK_A,
	PWM_CTRL_BANK_B,
	PWM_CTRL_BANK_ALL,
};

enum sm5306_pwm_active {
	PWM_ACTIVE_HIGH = 0,
	PWM_ACTIVE_LOW,
};

enum sm5306_bank_a_ctrl {
	BANK_A_CTRL_DISABLE = 0x0,
	BANK_A_CTRL_LED1 = 0x4,
	BANK_A_CTRL_LED2 = 0x1,
	BANK_A_CTRL_ALL = 0x5,
};

enum sm5306_bank_b_ctrl {
	BANK_B_CTRL_DISABLE = 0,
	BANK_B_CTRL_LED2,
};

struct sm5306_platform_data {

	/* maximum brightness */
	int max_brt_led1;
	int max_brt_led2;

	/* initial on brightness */
	int init_brt_led1;
	int init_brt_led2;
	enum sm5306_pwm_ctrl pwm_ctrl;
	enum sm5306_pwm_active pwm_active;
	enum sm5306_bank_a_ctrl bank_a_ctrl;
	enum sm5306_bank_b_ctrl bank_b_ctrl;
	unsigned int pwm_period;
	void (*pwm_set_intensity) (int brightness, int max_brightness);
};

#endif /* __LINUX_SM5306_H */
