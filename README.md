# hardware-allocator #

This module contains the STMicroelectronics android.hardware.graphics.allocator
and the mapper stable-c source code.

It is part of the STMicroelectronics delivery for Android.

## Description ##

This module provides the STM32MPU-specific implementation for Android graphics allocator and mapper.
Please see the Android delivery release notes for more details.

## Documentation ##

* The [release notes][] provide information on the release.
[release notes]: https://wiki.st.com/stm32mpu/wiki/Android-based_OpenSTDroid_ecosystem_release_note_-_v6.2.1

## Dependencies ##

This module can't be used alone. It is part of the STMicroelectronics delivery for Android.

```
PRODUCT_PACKAGES += \
    android.hardware.graphics.allocator-service.stm \
    android.hardware.graphics.allocator-aidl-impl \
    mapper.stm
```

## Contents ##

This directory contains the implementation of android.hardware.graphics.allocator AIDL version 2 and
android.hardware.graphics.mapper (stable-c interface).

## License ##

This module is distributed under the Apache License, Version 2.0 found in the [LICENSE](./LICENSE) file.
