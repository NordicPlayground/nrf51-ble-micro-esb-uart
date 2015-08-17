# nrf51-ble-uesb-uart
Concurrent BLE and micro-ESB wireless UART example

Example code from https://devzone.nordicsemi.com/blogs/733/running-micro-esb-concurrently-with-ble/
that illustrates how a wireless UART application can be run concurrently over micro-ESB and BLE.

# Requirements
- nRF51 SDK version 9.0.0, non-pack version
- S130 SoftDevice 1.0.0 (note: API compatible with S110 v8.0.0)
- 2 x nRF51-DK

Tested with Keil v5 and GCC 4.9 2015q1

# How to compile the code
Copy/link the examples\ble_peripheral\ble_uesb_app_uart and examples\ble_peripheral\common folders into the ble_peripheral example folder of your local SDK.
Open Keil project file (nRF51_SDK_9.0.0_2e23562\examples\ble_peripheral\ble_uesb_app_uart\pca10028\s130\arm5_no_packs), or Makefile (nRF51_SDK_9.0.0_2e23562\examples\ble_peripheral\ble_uesb_app_uart\pca10028\s130\armgcc).
