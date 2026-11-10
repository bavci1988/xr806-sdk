/*
 * Copyright (C) 2017 XRADIO TECHNOLOGY CO., LTD. All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *    1. Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *    2. Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the
 *       distribution.
 *    3. Neither the name of XRADIO TECHNOLOGY CO., LTD. nor the names of
 *       its contributors may be used to endorse or promote products derived
 *       from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef _PRJ_CONFIG_H_
#define _PRJ_CONFIG_H_

#ifdef __cplusplus
extern "C" {
#endif

/*
 * project base config
 */

/* main thread priority */
#define PRJCONF_MAIN_THREAD_PRIO        OS_THREAD_PRIO_APP

/* main thread stack size */
#define PRJCONF_MAIN_THREAD_STACK_SIZE  (8 * 1024)

/* sys ctrl enable/disable */
#define PRJCONF_SYS_CTRL_EN             1

/* sys ctrl thread priority */
#define PRJCONF_SYS_CTRL_PRIO           OS_THREAD_PRIO_SYS_CTRL

/* sys ctrl stack size */
#define PRJCONF_SYS_CTRL_STACK_SIZE     (2 * 1024)

/* sys ctrl queue length for receiving message */
#define PRJCONF_SYS_CTRL_QUEUE_LEN      6

/* image flash ID */
#define PRJCONF_IMG_FLASH               0

/* image start address, including bootloader */
#define PRJCONF_IMG_ADDR                0x00000000

/* save sysinfo to flash or not */
#define PRJCONF_SYSINFO_SAVE_TO_FLASH   1

#if PRJCONF_SYSINFO_SAVE_TO_FLASH

/* sysinfo flash ID */
#define PRJCONF_SYSINFO_FLASH           0

/* WXU has a 2 MiB SPI flash. The compressed OTA layout reserves
 *   0..1276 KiB   running image
 *   1276..1280 KiB OTA metadata
 *   1280..1920 KiB compressed update
 *   1920..2048 KiB application data
 * The original 1020 KiB location falls inside the running image slot and
 * fails sysinfo_init()'s OTA overlap check. Keep sysinfo in the reserved
 * tail with its own 4 KiB erase-aligned sector. Other preferences must
 * have a separately validated sector and must never share this one.
 */
#define PRJCONF_SYSINFO_ADDR            ((2048 - 4) * 1024)

/* Two independent 4 KiB FDCM sectors for ESPHome's transactional preference
 * snapshots. Keep each erase area disjoint from sysinfo and OTA regions.
 * This map is valid ONLY on the 2 MiB WXU variant.
 */
#define PRJCONF_ESPHOME_PREFS_FLASH     0
#define PRJCONF_ESPHOME_PREFS0_ADDR     ((2048 - 12) * 1024)
#define PRJCONF_ESPHOME_PREFS1_ADDR     ((2048 - 8) * 1024)
#define PRJCONF_ESPHOME_PREFS_SECTOR    (4 * 1024)

/* sysinfo size */
#define PRJCONF_SYSINFO_SIZE            (4 * 1024)

/* enable/disable checking whether sysinfo is overlap with image */
#define PRJCONF_SYSINFO_CHECK_OVERLAP   1

#endif /* PRJCONF_SYSINFO_SAVE_TO_FLASH */

/* MAC address source */
#define PRJCONF_MAC_ADDR_SOURCE         SYSINFO_MAC_ADDR_CHIPID

/* watchdog enable/disable */
#define PRJCONF_WDG_EN                  1

/* watchdog timeout value */
#define PRJCONF_WDG_TIMEOUT             WDG_TIMEOUT_16SEC

/* watchdog feeding period (in ms), MUST less than PRJCONF_WDG_TIMEOUT */
#define PRJCONF_WDG_FEED_PERIOD         (10 * 1000)

/*
 * project hardware feature
 */

/* uart enable/disable */
#define PRJCONF_UART_EN                 1

/* h/w crypto engine enable/disable */
#define PRJCONF_CE_EN                   1

/* spi enable/disable */
#define PRJCONF_SPI_EN                  0

/* Xradio internal codec sound card enable/disable */
#define PRJCONF_INTERNAL_SOUNDCARD_EN   0

/* AC107 sound card enable/disable */
#define PRJCONF_AC107_SOUNDCARD_EN      0

/*
 * project service feature
 */

/* console enable/disable */
#define PRJCONF_CONSOLE_EN 0

/* network and wlan enable/disable */
#define PRJCONF_NET_EN                  1

/* environment variable "TZ" for time zone setting */
#define PRJCONF_ENV_TZ                  "TZ=GMT-8"

#ifdef __cplusplus
}
#endif

#endif /* _PRJ_CONFIG_H_ */

#define PRJCONF_TRV16_NO_STDOUT 1
