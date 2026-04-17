/* blusys/blusys.h — C umbrella header (HAL + drivers + services)
 *
 * Include this from C code to pull in everything.
 * For finer-grained control, include specific layer headers directly.
 */

#ifndef BLUSYS_BLUSYS_H
#define BLUSYS_BLUSYS_H

/* --- Framework: observe (error domains, structured log, trace, counters) --- */
#include "blusys/framework/observe/observe.h"

/* --- HAL layer --- */
#include "blusys/hal/adc.h"
#include "blusys/hal/dac.h"
#include "blusys/hal/efuse.h"
#include "blusys/hal/error.h"
#include "blusys/hal/gpio.h"
#include "blusys/hal/gpio_expander.h"
#include "blusys/hal/i2c.h"
#include "blusys/hal/i2c_slave.h"
#include "blusys/hal/i2s.h"
#include "blusys/hal/log.h"
#include "blusys/hal/mcpwm.h"
#include "blusys/hal/nvs.h"
#include "blusys/hal/one_wire.h"
#include "blusys/hal/pcnt.h"
#include "blusys/hal/pwm.h"
#include "blusys/hal/rmt.h"
#include "blusys/hal/sd_spi.h"
#include "blusys/hal/sdm.h"
#include "blusys/hal/sdmmc.h"
#include "blusys/hal/sleep.h"
#include "blusys/hal/spi.h"
#include "blusys/hal/spi_slave.h"
#include "blusys/hal/system.h"
#include "blusys/hal/target.h"
#include "blusys/hal/temp_sensor.h"
#include "blusys/hal/timer.h"
#include "blusys/hal/touch.h"
#include "blusys/hal/twai.h"
#include "blusys/hal/uart.h"
#include "blusys/hal/ulp.h"
#include "blusys/hal/usb_device.h"
#include "blusys/hal/usb_host.h"
#include "blusys/hal/version.h"
#include "blusys/hal/wdt.h"

/* --- Drivers layer --- */
#include "blusys/drivers/button.h"
#include "blusys/drivers/buzzer.h"
#include "blusys/drivers/dht.h"
#include "blusys/drivers/display.h"
#include "blusys/drivers/encoder.h"
#include "blusys/drivers/lcd.h"
#include "blusys/drivers/led_strip.h"
#include "blusys/drivers/seven_seg.h"

/* --- Services layer (via framework wrappers) --- */
#include "blusys/framework/services/net.h"
#include "blusys/framework/services/session.h"
#include "blusys/framework/services/storage.h"
#include "blusys/framework/services/system.h"

#endif /* BLUSYS_BLUSYS_H */
