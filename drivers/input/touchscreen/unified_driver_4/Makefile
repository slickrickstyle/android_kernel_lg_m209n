

obj-y	+=  lgtp_common_driver.o
obj-y	+=  lgtp_model_config_i2c.o
obj-y	+=  lgtp_model_config_misc.o
obj-y	+=  lgtp_platform_api_i2c.o
obj-y	+=  lgtp_platform_api_misc.o
obj-y	+=  lgtp_platform_api_power.o

ifeq ($(CONFIG_JDI_INCELL), y)
obj-y 	+= S3320/
endif

ifeq ($(CONFIG_MTK_TD4191),y)
obj-y	+=  td4191/
endif

ifeq ($(CONFIG_MACH_MSM8939_M239DS_GLOBAL_COM), y)
obj-y 	+= MIT300/
endif

ifeq ($(CONFIG_MSM8909_M209), y)
obj-y   += MIT300/
endif

ifeq ($(CONFIG_TOUCHSCREEN_MELFAS_MIT300_PH1), y)
obj-y   += MIT300/
endif

ifeq ($(CONFIG_TOUCHSCREEN_MIT300), y)
obj-y   += MIT300/
endif

ifeq ($(CONFIG_TOUCHSCREEN_LGE_FT8606), y)
obj-y   += FT8606/
endif