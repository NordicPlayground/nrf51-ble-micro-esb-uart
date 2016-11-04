# nrf51-ble-uesb-uart
Concurrent BLE and micro-ESB wireless UART example

Example code from https://devzone.nordicsemi.com/blogs/733/running-micro-esb-concurrently-with-ble/
that illustrates how a wireless UART application can be run concurrently over micro-ESB and BLE.

# Requirements
- nRF5 SDK version 11.0.0, non-pack version
- S130 SoftDevice 2.0.0 
- S132 SoftDevice 2.0.0
- 2 x nRF5x-DK 

Tested with compilers: Keil v4, Keil v5, GCC 4.9 2015q1 and IAR.
Tested with nRF51-DK and nRF52-DK.

# How to compile the code
Copy/link the examples\ble_peripheral\ble_app_uart.ESB_Timeslot and examples\ble_peripheral\common folders into the ble_peripheral example folder of your local SDK.
Open Keil project file (nRF5_SDK_11.0.0_89a8197\examples\ble_peripheral\ble_uesb_app_uart\pca10028\s130\arm5_no_packs), or Makefile (nRF5_SDK_11.0.0_89a8197\examples\ble_peripheral\ble_uesb_app_uart\pca10028\s130\armgcc).
