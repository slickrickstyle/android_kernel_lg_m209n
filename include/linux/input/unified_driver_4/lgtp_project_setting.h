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
 *    File  : lgtp_project_setting.h
 *    Author(s)   : D3 BSP Touch Team < d3-bsp-touch@lge.com >
 *    Description :
 *
 ***************************************************************************/

#if !defined(_LGTP_PROJECT_SETTING_H_)
#define _LGTP_PROJECT_SETTING_H_


/****************************************************************************
* Project Setting ( Model )
****************************************************************************/

#if defined(CONFIG_MSM8909_M209)
#define TOUCH_MODEL_M2
#endif

#if defined(CONFIG_MSM8909_E1Q)
#define TOUCH_MODEL_E1
#endif

#if defined(CONFIG_MSM8909_M1V_VZW)
#define TOUCH_MODEL_M1V_VZW
#endif

#if defined(CONFIG_MSM8909_M1V)
#define TOUCH_MODEL_M1V
#endif

#if defined(CONFIG_MSM8909_LV0)
#define TOUCH_MODEL_LV0
#endif

#if defined(CONFIG_MSM8909_LV1)
#define TOUCH_MODEL_LV1
#endif

#if defined(CONFIG_TOUCHSCREEN_MELFAS_MIT300_PH1)
#define TOUCH_MODEL_PH1
#endif

#if defined(CONFIG_MSM8909_CLING)
#define TOUCH_MODEL_CLING
#endif

#if defined(CONFIG_MSM8909_ME0)
#define TOUCH_MODEL_ME0
#endif

#if defined(CONFIG_MSM8909_K6B)
#define TOUCH_MODEL_K6B
#endif

#if defined(CONFIG_MSM8909_K6P)
#define TOUCH_MODEL_K6P
#endif

/****************************************************************************
* Available Feature supported by Unified Driver
* If you want to use it, define it inside of model feature
****************************************************************************/
/* #define ENABLE_HOVER_DETECTION */
/* #define ENABLE_TOUCH_AT_OFF_CHARGING */

/****************************************************************************
* Project Setting ( AP Solution / AP Chipset / Touch Device )
****************************************************************************/
#if defined(TOUCH_MODEL_M2)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8939
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_MIT300

#elif defined(TOUCH_MODEL_M1V)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_MIT300

#elif defined(TOUCH_MODEL_PH1)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8916

/* Touch Device */
#define TOUCH_DEVICE_MIT300

#elif defined(TOUCH_MODEL_E1)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_LU202X
#define TOUCH_DEVICE_SN280H
#define TOUCH_DEVICE_FT6X36

#elif defined(TOUCH_MODEL_CLING)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_LU202X

/* Driver Feature */

/* IC Type */
//#define TOUCH_TYPE_COF

#elif defined(TOUCH_MODEL_ME0)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_FT3407

#elif defined(TOUCH_MODEL_K6B)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_FT8606

#elif defined(TOUCH_MODEL_K6P)
    /* AP Solution */
    #define TOUCH_PLATFORM_QCT
    /* AP Chipset */
    #define TOUCH_PLATFORM_MSM8909
    /* Touch Device */
    #define TOUCH_DEVICE_MIT300

#elif defined(TOUCH_MODEL_LV0)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_FT3407

#elif defined(TOUCH_MODEL_LV1)

/* AP Solution */
#define TOUCH_PLATFORM_QCT

/* AP Chipset */
#define TOUCH_PLATFORM_MSM8909

/* Touch Device */
#define TOUCH_DEVICE_MIT300

#else
#error "Model should be defined"
#endif

#endif /* _LGTP_PROJECT_SETTING_H_ */

/* End Of File */

