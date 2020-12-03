ifeq ($(CONFIG_OF),y)

ifeq ($(CONFIG_LGE_HW_REV_DTS),y)

ifeq ($(CONFIG_MACH_MSM8909_E1Q_VZW),y)
DTS_TARGET := msm8909-e1q_vzw
endif

ifeq ($(CONFIG_MACH_MSM8909_E1Q_TRF_US),y)
DTS_TARGET := msm8909-e1q_trf_us
endif

ifeq ($(CONFIG_MACH_MSM8909_E1Q_TRF_US_VZW),y)
DTS_TARGET := msm8909-e1q_trf_us_vzw
endif

ifeq ($(CONFIG_MACH_MSM8909_E1Q_CRK_US),y)
DTS_TARGET := msm8909-e1q_crk_us
endif

ifeq ($(CONFIG_MACH_MSM8909_M1_SPR_US),y)
DTS_TARGET := msm8909-m1_spr_us
endif

ifeq ($(CONFIG_MACH_MSM8909_M1_TMO_US),y)
DTS_TARGET := msm8909-m1_tmo_us
endif

ifeq ($(CONFIG_MACH_MSM8909_M1_MPCS_US),y)
DTS_TARGET := msm8909-m1_mpcs_us
endif

ifeq ($(CONFIG_MACH_MSM8909_M1_TRF_US),y)
DTS_TARGET := msm8909-m1_trf_us
endif

ifeq ($(CONFIG_MACH_MSM8909_M1_TRF_US_VZW),y)
DTS_TARGET := msm8909-m1_trf_us_vzw
endif

ifeq ($(CONFIG_MACH_MSM8909_M1DS_GLOBAL_COM),y)
DTS_TARGET := msm8909-m1ds_global_com
endif

ifeq ($(CONFIG_MACH_MSM8909_CLING_VZW),y)
DTS_TARGET := msm8909-cling_vzw
endif

ifeq ($(CONFIG_MACH_MSM8909_ME0_SPR_US),y)
DTS_TARGET := msm8909-me0_spr_us
endif

ifeq ($(CONFIG_MACH_MSM8909_K6B_LGU_KR),y)
DTS_TARGET := msm8909-k6b_lgu_kr
endif

ifeq ($(CONFIG_MACH_MSM8909_K6B_GLOBAL_COM),y)
DTS_TARGET := msm8909-k6b_global_com
endif

ifeq ($(CONFIG_MACH_MSM8909_K6B_TRF_US),y)
DTS_TARGET := msm8909-k6b_trf_us
endif

ifeq ($(CONFIG_MACH_MSM8909_K6B_TRF_US_VZW),y)
DTS_TARGET := msm8909-k6b_trf_us_vzw
endif

ifeq ($(CONFIG_MACH_MSM8909_K6B_SPR_US),y)
DTS_TARGET := msm8909-k6b_spr_us
endif

ifeq ($(CONFIG_MACH_MSM8909_K6B_TMO_US),y)
DTS_TARGET := msm8909-k6b_tmo_us
endif

ifeq ($(CONFIG_MACH_MSM8909_LEAP_VZW),y)
	DTS_TARGET := msm8909-leap_vzw
endif

ifeq ($(CONFIG_MACH_MSM8909_LV0_TRF_US),y)
	DTS_TARGET := msm8909-lv0_trf_us
endif

ifeq ($(CONFIG_MACH_MSM8909_LV0_TRF_US_VZW),y)
	DTS_TARGET := msm8909-lv0_trf_us_vzw
endif

ifeq ($(CONFIG_MACH_MSM8909_CF2_SKT_KR),y)
	DTS_TARGET := msm8909-cf2_skt_kr
endif

ifeq ($(CONFIG_MACH_MSM8909_CF2_LGU_KR),y)
	DTS_TARGET := msm8909-cf2_lgu_kr
endif

ALL_DTS := $(notdir $(wildcard echo $(srctree)/$(src)/$(DTS_TARGET)/*.dts))
dtb-y += $(patsubst %.dts,%.dtb,$(ALL_DTS))

else
dtb-$(CONFIG_MACH_MSM8909_LGPS22)          += msm8909-lgps22.dtb
dtb-$(CONFIG_MACH_MSM8909_LGPS22)          += msm8909-pm8916-lgps22.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_LGU_KR)       += msm8909-cf-kr.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_LGU_KR)       += msm8909-pm8916-cf-kr.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_SKT_KR)       += msm8909-cf-kr.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_SKT_KR)       += msm8909-pm8916-cf-kr.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_KT_KR)        += msm8909-cf-kr.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_KT_KR)        += msm8909-pm8916-cf-kr.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_GLOBAL_COM)        += msm8909-cf_global_com.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_GLOBAL_COM)        += msm8909-pm8916-cf_global_com.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_JCM_JP)       += msm8909-cf_jcm_jp.dtb
dtb-$(CONFIG_MACH_MSM8909_CF_JCM_JP)       += msm8909-pm8916-cf_jcm_jp.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_SPR_US)        += msm8909-m1_spr_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_SPR_US)        += msm8909-pm8916-m1_spr_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_TMO_US)        += msm8909-m1_tmo_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_TMO_US)        += msm8909-pm8916-m1_tmo_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_MPCS_US)        += msm8909-m1_mpcs_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_MPCS_US)        += msm8909-pm8916-m1_mpcs_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_TRF_US)        += msm8909-m1_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_TRF_US)        += msm8909-pm8916-m1_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_TRF_US_VZW)        += msm8909-m1_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_M1_TRF_US_VZW)        += msm8909-pm8916-m1_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_M1V_ATT_US)        += msm8909-m1v_att_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1V_CRK_US)        += msm8909-m1v_crk_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1V_USC_US)        += msm8909-m1v_usc_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1V_ACG_US)        += msm8909-m1v_acg_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M1V_VZW)        += msm8909-m1v_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_M1V_LRA_US)        += msm8909-m1v_lra_us.dtb
dtb-$(CONFIG_MACH_MSM8909_LEAP_VZW)        += msm8909-leap_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_K6P_GLOBAL_CA)        += msm8909-k6p_global_ca.dtb
dtb-$(CONFIG_MACH_MSM8909_K6P_CRK_US)        += msm8909-k6p_crk_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6P_USC_US)        += msm8909-k6p_usc_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6P_CCT_US_VZW)        += msm8909-k6p_cct_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_M209_TRF_US)        += msm8909-m209_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M209_TRF_US)        += msm8909-pm8916-m209_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M209_TRF_US_VZW)        += msm8909-m209_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_M209_TRF_US_VZW)        += msm8909-pm8916-m209_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_M209N_TMO_US)        += msm8909-m209n_tmo_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M209N_MPCS_US)        += msm8909-m209n_mpcs_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M209N_ATT_US)        += msm8909-m209n_att_us.dtb
dtb-$(CONFIG_MACH_MSM8909_M209N_CRK_US)        += msm8909-m209n_crk_us.dtb
dtb-$(CONFIG_MACH_MSM8909_E1Q_VZW)        += msm8909-e1q_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_E1Q_TRF_US)        += msm8909-e1q_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_E1Q_TRF_US)        += msm8909-pm8916-e1q_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_E1Q_CRK_US)        += msm8909-e1q_crk_us.dtb
dtb-$(CONFIG_MACH_MSM8909_E1Q_CRK_US)        += msm8909-pm8916-e1q_crk_us.dtb
dtb-$(CONFIG_MACH_MSM8909_E1Q_TRF_US_VZW)        += msm8909-e1q_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_E1Q_TRF_US_VZW)        += msm8909-pm8916-e1q_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_M1DS_GLOBAL_COM)        += msm8909-m1ds_global_com.dtb
dtb-$(CONFIG_MACH_MSM8909_M1DS_GLOBAL_COM)        += msm8909-pm8916-m1ds_global_com.dtb
dtb-$(CONFIG_MACH_MSM8909_CLING_VZW)        += msm8909-cling_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_CLING_VZW)        += msm8909-pm8916-cling_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_ME0_SPR_US)        += msm8909-me0_spr_us.dtb
dtb-$(CONFIG_MACH_MSM8909_ME0_SPR_US)        += msm8909-pm8916-me0_spr_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_LGU_KR)        += msm8909-k6b_lgu_kr.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_LGU_KR)        += msm8909-pm8916-k6b_lgu_kr.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_GLOBAL_COM)        += msm8909-k6b_global_com.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_GLOBAL_COM)        += msm8909-pm8916-k6b_global_com.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_TRF_US)        += msm8909-k6b_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_TRF_US)        += msm8909-pm8916-k6b_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_TRF_US_VZW)        += msm8909-k6b_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_TRF_US_VZW)        += msm8909-pm8916-k6b_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_SPR_US)        += msm8909-k6b_spr_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_SPR_US)        += msm8909-pm8916-k6b_spr_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_TMO_US)        += msm8909-k6b_tmo_us.dtb
dtb-$(CONFIG_MACH_MSM8909_K6B_TMO_US)        += msm8909-pm8916-k6b_tmo_us.dtb
dtb-$(CONFIG_MACH_MSM8909_LV0_TRF_US)        += msm8909-lv0_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_LV0_TRF_US_VZW)        += msm8909-lv0_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_LV1_VZW)        += msm8909-lv1_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_LV1_TRF_US)        += msm8909-lv1_trf_us.dtb
dtb-$(CONFIG_MACH_MSM8909_LV1_GLOBAL_COM)    += msm8909-lv1_global_com.dtb
dtb-$(CONFIG_MACH_MSM8909_LV1_ATT_US)    += msm8909-lv1_att_us.dtb
dtb-$(CONFIG_MACH_MSM8909_LV1_TRF_US_VZW)        += msm8909-lv1_trf_us_vzw.dtb
dtb-$(CONFIG_MACH_MSM8909_LV1_CRK_US)        += msm8909-lv1_crk_us.dtb
dtb-$(CONFIG_MACH_MSM8909_CF2_SKT_KR)        += msm8909-cf2_skt_kr.dtb
dtb-$(CONFIG_MACH_MSM8909_CF2_LGU_KR)        += msm8909-cf2_lgu_kr.dtb
endif

DTB_NAMES := $(subst $\",,$(CONFIG_BUILD_ARM_APPENDED_DTB_IMAGE_NAMES))
ifneq ($(DTB_NAMES),)
DTB_LIST := $(addsuffix .dtb,$(DTB_NAMES))
else
DTB_LIST := $(dtb-y)
endif

targets += dtbs
ifeq ($(CONFIG_LGE_HW_REV_DTS),y)
targets += $(addprefix ../, $(DTB_LIST))
endif
endif

ifneq ($(CONFIG_LGE_HW_REV_DTS),y)
DTS_TARGET := $(patsubst "%",%,$(call CONFIG_DTS_TARGET,$(1)))
endif

$(obj)/../%.dtb: $(src)/$(DTS_TARGET)/%.dts
	$(call if_changed_dep,dtc)

dtbs: $(addprefix $(obj)/../,$(DTB_LIST))

clean-files := *.dtb
