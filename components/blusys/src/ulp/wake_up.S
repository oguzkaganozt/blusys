#include "sdkconfig.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/soc_ulp.h"

	.global wake_up
wake_up:
#if CONFIG_IDF_TARGET_ESP32
	/* ESP32 ULP FSM example sequence: bit 27 indicates the main state is idle. */
	READ_RTC_REG(RTC_CNTL_LOW_POWER_ST_REG, 27, 1)
#else
	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_MAIN_STATE_IN_IDLE)
#endif
	move r1, r0
	READ_RTC_FIELD(RTC_CNTL_LOW_POWER_ST_REG, RTC_CNTL_RDY_FOR_WAKEUP)
	or r0, r0, r1
	jump wake_up, eq

	wake
	halt
