/***************************************************************************
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 *    File  : lgtp_model_config_misc.h
 *    Author(s)   : D3 BSP Touch Team < d3-bsp-touch@lge.com >
 *    Description :
 *
 ***************************************************************************/

#if !defined(_LGTP_MODEL_CONFIG_MISC_H_)
#define _LGTP_MODEL_CONFIG_MISC_H_

/****************************************************************************
* Nested Include Files
****************************************************************************/


/****************************************************************************
* Mainfest Constants / Defines
****************************************************************************/

/* Hardware(Board) Configuration */
#if defined(TOUCH_MODEL_M2) || defined(TOUCH_MODEL_PH1) || defined(TOUCH_MODEL_M1V) || defined(TOUCH_MODEL_K6P)

#define TOUCH_GPIO_RESET            (12+911)
#define TOUCH_GPIO_INTERRUPT        (13+911)
#define GPIO_TOUCH_MAKER_ID         (16+911)
#define TOUCH_IRQ_FLAGS (IRQF_TRIGGER_LOW | IRQF_ONESHOT | IRQF_NO_SUSPEND)

#define PALM_REJECTION_ENABLE   1
#define USE_ESD_RECOVERY
#define USE_EARLY_FB_EVENT_BLANK
#define USE_EARLY_FB_EVENT_UNBLANK

#elif defined(TOUCH_MODEL_LV0)

#define TOUCH_GPIO_RESET		(12 + 911)
#define TOUCH_GPIO_INTERRUPT 		(13 + 911)
#define TOUCH_GPIO_MAKER_ID 		(9 + 911)

#define TOUCH_IRQ_FLAGS (IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND)
//#define USE_EARLY_FB_EVENT_BLANK
//#define USE_EARLY_FB_EVENT_UNBLANK
#define USE_FB_EVENT_BLANK
#define USE_FB_EVENT_UNBLANK

#define PALM_REJECTION_ENABLE   0

#elif defined(TOUCH_MODEL_LV1)

#define TOUCH_GPIO_RESET            (12+911)
#define TOUCH_GPIO_INTERRUPT        (13+911)

#define TOUCH_IRQ_FLAGS (IRQF_TRIGGER_LOW | IRQF_ONESHOT | IRQF_NO_SUSPEND)

#define PALM_REJECTION_ENABLE   1
#define USE_ESD_RECOVERY
#define USE_EARLY_FB_EVENT_BLANK
#define USE_EARLY_FB_EVENT_UNBLANK

#elif defined(TOUCH_MODEL_K6B)

#define TOUCH_GPIO_RESET            (2+911)
#define TOUCH_GPIO_INTERRUPT        (3+911)

#define TOUCH_IRQ_FLAGS (IRQF_TRIGGER_LOW | IRQF_ONESHOT | IRQF_NO_SUSPEND)

#define PALM_REJECTION_ENABLE   1
#define USE_ESD_RECOVERY
#define USE_EARLY_FB_EVENT_BLANK
#define USE_EARLY_FB_EVENT_UNBLANK

#elif defined (TOUCH_MODEL_E1)

#define TOUCH_GPIO_RESET		(12 + 911)
#define TOUCH_GPIO_INTERRUPT 		(13 + 911)
#define TOUCH_GPIO_MAKER_ID 		(9 + 911)

#define TOUCH_IRQ_FLAGS (IRQF_ONESHOT)
#define USE_EARLY_FB_EVENT_BLANK
#define USE_EARLY_FB_EVENT_UNBLANK

#elif defined (TOUCH_MODEL_CLING)

#define TOUCH_GPIO_RESET		(12 + 911)
#define TOUCH_GPIO_INTERRUPT 		(31 + 911)
#define TOUCH_GPIO_MAKER_ID 		(9 + 911)

#define TOUCH_IRQ_FLAGS (IRQF_ONESHOT)
//#define USE_EARLY_FB_EVENT_BLANK
//#define USE_EARLY_FB_EVENT_UNBLANK
#define USE_FB_EVENT_BLANK
#define USE_FB_EVENT_UNBLANK

#elif defined (TOUCH_MODEL_ME0)

#define TOUCH_GPIO_RESET		(12 + 911)
#define TOUCH_GPIO_INTERRUPT 		(13 + 911)
#define TOUCH_GPIO_MAKER_ID 		(9 + 911)

#define TOUCH_IRQ_FLAGS (IRQF_TRIGGER_FALLING | IRQF_ONESHOT | IRQF_NO_SUSPEND)
//#define USE_EARLY_FB_EVENT_BLANK
//#define USE_EARLY_FB_EVENT_UNBLANK
#define USE_FB_EVENT_BLANK
#define USE_FB_EVENT_UNBLANK

#define PALM_REJECTION_ENABLE   0

#else
#error "Model should be defined"
#endif

/****************************************************************************
* Type Definitions
****************************************************************************/


/****************************************************************************
* Exported Variables
****************************************************************************/


/****************************************************************************
* Macros
****************************************************************************/


/****************************************************************************
* Global Function Prototypes
****************************************************************************/
void TouchVddPowerModel(int isOn);
void TouchVioPowerModel(int isOn);
void TouchAssertResetModel(void);
TouchDeviceControlFunction * TouchGetDeviceControlFunction(int index);
void TouchGetModelConfig(TouchDriverData *pDriverData);


#endif /* _LGTP_MODEL_CONFIG_MISC_H_ */

/* End Of File */

