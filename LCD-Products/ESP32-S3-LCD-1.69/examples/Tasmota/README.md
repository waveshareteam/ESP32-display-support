
# Using Tasmota on the Waveshare ESP32-S3-LCD-1.69

The Waveshare ESP32-S3-LCD-1.69 dev board is an ESP32-S3R8-based development board. Per [its product page and docs](https://www.waveshare.com/ESP32-S3-LCD-1.69.htm), it sports a wide array of peripherals:

* QMI8658 IMU
* PCF85063 RTC
* ST7789V2-driven 240x280 rounded-corner LCD screen
* lithium-ion battery charger
* 16MiB (128Mib) SPI flash
* 12-pin breakout header (breakout-to-100-mil/Dupont cable provided)
* 3.3V regulator

This document aims to help the savvy user use as many of these peripherals as possible under [Tasmota](https://tasmota.github.io/docs/), with the caveat that a more expert Tasmota user may know how to do it more easily. (For example, one of the stock ESP32-S3 images may support everything needed, obviating the need for a custom build of Tasmota, and much of the work detailed here can probably be done in a pre-made board config.)

The dev board comes in two versions, the V1 and the V2, with V2 sporting some I/O rework described in [the product wiki](https://www.waveshare.com/wiki/ESP32-S3-LCD-1.69#09_LVGL_Keys_Bee). Most notably, V1 had some pins attached to peripherals that were also required to use the "R8" in-package octal SPI PSRAM that the V2 moved to less conflicted pins.

It also has a very similar touchscreen variant, [the ESP32-S3-Touch-LCD-1.69](https://www.waveshare.com/ESP32-S3-Touch-LCD-1.69.htm), that this document does not cover. That said, it is likely that the only difference will be adding a touch configuration section to the [LCD configuration in `display.ini`](https://tasmota.github.io/docs/Universal-Display-Driver/#descriptor-file); [Tasmota even has an example](https://github.com/arendst/Tasmota/blob/development/tasmota/displaydesc/ST7789_display.ini), though it may need significant rework.

## Custom Tasmota Build

Please follow [Tasmota's docs for building and installing a custom version of Tasmota](https://tasmota.github.io/docs/Compile-your-build/) with the changes described in this section.

### `platform_override.ini`

Follow the Tasmota instructions to copy `platform_override_sample.ini`, then:

* Change `default_envs` to `tasmota32s3`.
    * With a V2 board or with [a V1 with the buzzer diconnected](https://github.com/waveshareteam/ESP32-display-support/issues/7), using `tasmota32s3-qio_opi-all` instead will enable the in-package PSRAM.
    * If enabling PSRAM makes the buzzer crackle and the board heat up, the board is V1 (where GPIO33 (SPIIO4) is connected to the buzzer).
* (optional) Set `upload_port` and `monitor_port`

The default `board` setting is sufficient.

### `platform_tasmota_cenv.ini`

No changes when copying `platform_tasmota_cenv_sample.ini`.

### `tasmota/user_config_overide.h`

This is where the magic happens. Copy the sample per the Tasmota docs, then add (drawing from `tasmota/my_user_config.h`):

```C
// LVGL per https://tasmota.github.io/docs/LVGL/
// --> Optional, but useful for driving the LCD
#define USE_LVGL
#define USE_DISPLAY
#define USE_DISPLAY_LVGL_ONLY
#define USE_UNIVERSAL_DISPLAY
#undef USE_DISPLAY_MODES1TO5
#undef USE_DISPLAY_LCD
#undef USE_DISPLAY_SSD1306
#undef USE_DISPLAY_MATRIX
#undef USE_DISPLAY_SEVENSEG

// On-board devices
// W25Q128JVSIQ 16-Mbit external QIO flash
// --> already handled by the "qio_" in the board config.
// PCF85063 RTC
#define USE_RTC_CHIPS                          // Enable RTC chip support and NTP server - Select only one
#define USE_PCF85063                         // [I2cDriver92] Enable PCF85063 RTC support (I2C address 0x51)
// QMI8658 6-axis IMU
// Buzzer (GPIO-transistor-voicecoil arrangement)
// --> Further configuration is detailed later in this document.
#ifndef USE_BUZZER
#define USE_BUZZER
#endif
// 4-wire SPI ST7789V2 LCD controller, 240x280
#define USE_SPI                                  // Hardware SPI using GPIO12(MISO), GPIO13(MOSI) and GPIO14(CLK) in addition to two user selectable GPIOs(CS and DC)
#define USE_DISPLAY_ST7789                   // [DisplayModel 12] Enable ST7789 module
```

## Post-flash configuration

Once the custom Tasmota firmware is built and flashed, further configuration is needed. Follow the Tasmota docs to configure it and access its web interface.

### Allow ESP32 SPI0 OPI pin assignment

Tasmota's built-in ESP32-S3 configuration prevents the user from assigning any of the in-package PSRAM octal SPI (OPI) pins to peripherals. **When not using the PSRAM on the V1 board**, Tasmota needs to be reconfigured to allow this:

* Main Menu -> Configuration -> Module
    * Observe GPIOs 33 through 37 are not visible
* Main Menu -> Configuration -> Template
    * Set GPIOs 33, 35, 36 to User
    * Save
* Main Menu -> Configuration -> Module
    * Observe the GPIOS are now visible

N.B. Tasmota Templates are a helpful abstraction, but if this doesn't work then go to Main Menu -> Configuration -> Other, set all the GPIO `0`s to `1`s, check the Activate Template button, then click Save.

### Commands

Tasmota supports two different kinds of buzzer, one that takes an on/off signal and one that is driven directly. The buzzer on this board is the latter, so run the following Tasmota [Command](https://tasmota.github.io/docs/Commands/):

`BuzzerPwm 1`

(This is an alias of `SetOption111`.)

### Pin configuration

The next step is to tell Tasmota which pins are connected to which peripherals.

Navigate in the web UI to Main Menu -> Configuration -> Module.

**Reminder: this is currently for V1 hardware.**

* GPIO0 - Button - 1 (this is the middle button on the side of the board)
* GPIO1 - ADC Input
* GPIO2 - None (12-pin header)
* GPIO3 - None (12-pin header)
* GPIO4 - SPI DC - 1
* GPIO5 - SPI CS - 1
* GPIO6 - SPI CLK - 1
* GPIO7 - SPI MOSI - 1
* GPIO8 - Display Rst
* GPIO10 - I2C SCL - 1 (incl. 12-pin header)
* GPIO11 - I2C SDA - 1 (incl. 12-pin header)
* GPIO12 - Option A - 3 (this can be any unused pin; it only configures the display)
* GPIO15 - Output Hi
* GPIO16 - None (12-pin header)
* GPIO17 - None (12-pin header)
* GPIO18 - None (12-pin header)
* GPIO33 - Buzzer
    * This can be validated with the Tasmota Command `Buzzer 2,3`
* GPIO35 - Switch - 1 (this enables the battery to power the system, as does Key2)
* GPIO36 - Button - 2 (this is Key2, the top button on the side of the board)
* GPIO38 - None - Current simple Berry QMI8658 driver does not use the interrupt
* GPIO41 - None - Tasmota's PCF85063 driver does not offer a GPIO mapping for the interrupt
* GPIO43 - Serial Tx (12-pin header)
* GPIO44 - Serial Rx (12-pin header)

Notes:

* The V1 schematic labels the display SPI clock and data pins with I2C names. This is clarified in the LVGL sample from the Waveshare wiki for the board.
* The Tasmota `SetOption73` default value of `0` ties `Button<N>` to `Power<N>`. Since LVGL publishes a toggle for the screen's power, GPIO0 defaults to controlling screen power. This can be disabled with Tasmota Command `SetOption73 1`.
* At least on one board, GPIO15 does not drive to a low enough voltage to turn off Q1. Configuring it as Output Hi reduces waste heat in Q1 and increases backlight brightness slightly.
    * Specifically, GPIO15 feeds `0.1*VDD` to `0.8*VDD` into the 1K/10K network on the base of Q1. The high value is enough to saturate it, but the low value, 0.33V, plus the R16 voltage delta assuming Q1 is off, 0.27V, puts the base at 0.6V--which is right at the 8050 V_BE saturation voltage per at least BL Galaxy Electrical's graphs. Empirically this appears to be at the saturation end of the linear region.
    * Configuring GPIO15 as Backlight and then toggling the backlight appears to trigger PSRAM usage even when the `qio` board setting is used.
    * Waveshare's support suggests using PWM on GPIO15, but since the lowest voltage does not reach the cutoff region this doesn't appear to be useful.
    * Waveshare's support also suggested replacing R11 with a 100K resistor. This can be a challenge for someone without fine SMD rework equipment, but removing R11 entirely might work.
    * The schematic suggests that this is an issue with V2 hardware as well.
* GPIO35 (SYS_EN) and Key2 both enable the battery's output, B+, to flow to the 3.3V regulator.
    * This is described in the wiki under `09_LVGL_Keys_Bee`. In effect, Key2 can function as a power on initiator if the SoC then brings GPIO35 high to keep the power on. At that point the button can be used normally via GPIO36, for example by reacting to a double-press by lowering GPIO36 and cutting power to the system.
    * Q4 acts as a backflow prevention valve. The body diode will always let B+ via VBAT through to the 3.3V regulator input. If VBUS is present then D4 forces Q4 V_GS positive and thus Q4 into cutoff, so VBUS will not charge the battery. (That's what U9 is for.) If VBUS is absent, then D4 means Q4 V_GS is negative enough to put Q4 into saturation and VBAT can flow to the regulator without diode losses.
    * Q5 acts as a battery enable. When a battery is connected, pressing Key2 or setting SYS_EN high allows the battery to flow to the 3.3V regulator's input, whether through the Q4 body diode or source-drain conductance.
* GPIO36, SYS_OUT, reads the state of Key2.

### Files

Tasmota sets up a filesystem that can be explored via Main Menu -> Tools -> Filesystem. A handful of files are required to finish setting up the display and IMU.

#### display.ini

This file tells the Universal Display Driver how to talk to the screen.

[Field definitions](https://tasmota.github.io/docs/Universal-Display-Driver/)

Sample: `tasmota\displaydesc\ST7789_172x320_Waveshare_esp32c6_lcd_1_47.ini`

The adjacent `ST7789_display.ini` sample includes touchscreen configuration, but this is not for the touch module.

Copy the contents of the accompanying `display.ini` to the device filesystem.

Notes:

* The `*` in the `:H` line are filled in by the Module pin configuration but could be hardcoded instead.
* Loading/reloading display.ini requires a reboot.
* `21,80` in the `:I` section and `:i,21,20` should be, if the datasheet is to be trusted, reversed to `20,80` and `:i,20,21`. As it stands, though, `21` in practice disables inversion; it is possible this is due to an unrelated misconfiguration (perhaps in the LVGL pixel layout configuration). It matches what the sample code does during initialization. This can be experimented with using the `DisplayInvert <n>` Tasmota Command.
* Screen orientation can be changed with the `DisplayRotate <n>` Tasmota Command.

#### autoexec.be

On every boot, Tasmota will run `autoexec.be` from the filesystem. The one in this directory will load the IMU driver and start HASPmota.

N.B. The Tasmota device scan is not aware of the Device ID portion of the I2C spec, so it will report a spurious device present at address 0x7E (in the Device ID range of 0b11111xx or 0x7C-0x7F).

#### pages.jsonl

HASPmota is a JSON-based way to easily and concisely describe LVGL GUIs. The `pages.jsonl` file in this folder, when placed in the root of the filesystem and together with `haspmota.start()` in `autoexec.be`, will draw a simple UI and graph the X and Y values coming out of the IMU.
