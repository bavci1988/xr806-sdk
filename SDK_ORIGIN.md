# XR806 SDK provenance

This repository contains the Allwinner/XRadio XR806 SDK
and local modifications for native ESPHome integration.

## Original source

Repository: https://github.com/openshwprojects/OpenXR806

Base commit: 3113bbb32f8378206f52d72406aa123a1368bfec

## Local modifications

- WXU board configuration
- TRV16 firmware project
- ESPHome-compatible startup and linker integration
- SDK build configuration and compiler adjustments

## Third-party components

Original copyright notices and license files are retained.

The SDK contains third-party source code, precompiled
libraries, firmware binaries and development utilities.

This repository is public. The base, OpenXR806, publishes the same
vendor sources, precompiled libraries (for example
`lib/xradio_v3/libwlan.a`) and firmware binaries (for example
`bin/xradio_v3/wlan_fw.bin`). The XRadio sources carry a BSD-style
license in their file headers. The precompiled libraries and firmware
binaries come without a separate license file; their redistribution
terms still need a review before this SDK becomes an official ESPHome
dependency.

## Development status

Native ESPHome integration is under development.
Successful compilation does not imply hardware validation.
