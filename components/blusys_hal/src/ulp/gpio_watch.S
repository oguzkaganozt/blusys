#include "sdkconfig.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/rtc_io_reg.h"
#include "soc/sens_reg.h"
#include "soc/soc_ulp.h"

	.bss
	.global signature
signature:
	.long 0

	.global job
job:
	.long 0

	.global io_number
io_number:
	.long 0

	.global wake_level
wake_level:
	.long 0

	.global required_match_count
required_match_count:
	.long 0

	.global period_ms
period_ms:
	.long 0

	.global match_count
match_count:
	.long 0

	.global run_count
run_count:
	.long 0

	.global last_level
last_level:
	.long 0

	.global wake_triggered
wake_triggered:
	.long 0

	.text
	.global entry
entry:
	move r3, run_count
	ld r2, r3, 0
	add r2, r2, 1
	st r2, r3, 0

	move r3, io_number
	ld r3, r3, 0

#if CONFIG_IDF_TARGET_ESP32S3
	WRITE_RTC_FIELD(SENS_SAR_PERI_CLK_GATE_CONF_REG, SENS_IOMUX_CLK_EN, 1)
#endif

	move r0, r3
	jumpr read_io_high, 16, ge
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S, 16)
	rsh r0, r0, r3
	jump read_done

read_io_high:
	READ_RTC_REG(RTC_GPIO_IN_REG, RTC_GPIO_IN_NEXT_S + 16, 2)
	sub r3, r3, 16
	rsh r0, r0, r3

read_done:
	and r0, r0, 1
	move r3, last_level
	st r0, r3, 0

	move r3, wake_level
	ld r3, r3, 0
	sub r3, r0, r3
	jump level_match, eq

	move r3, match_count
	move r2, 0
	st r2, r3, 0
	halt

level_match:
	/* Once the debounce count is satisfied, keep requesting wake until
	 * the main CPU clears or restarts the job.
	 */
	move r3, required_match_count
	ld r0, r3, 0
	move r3, match_count
	ld r2, r3, 0
	sub r0, r2, r0
	jump trigger_wake, eq

	add r2, r2, 1
	st r2, r3, 0
	move r3, required_match_count
	ld r0, r3, 0
	sub r0, r2, r0
	jump trigger_wake, eq
	halt

trigger_wake:
	move r3, wake_triggered
	move r2, 1
	st r2, r3, 0
	jump wake_up
