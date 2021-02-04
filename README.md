# nrf51-ble-uesb-uart
Concurrent BLE and ESB wireless UART example

Example code from https://devzone.nordicsemi.com/blogs/733/running-micro-esb-concurrently-with-ble/
that illustrates how a wireless UART application can be run concurrently over ESB and BLE.

# Requirements
- nRF5 SDK version 17.0.2,
- S132 SoftDevice 7.2.0
- 2 x nRF52-DK 

Tested with compilers: Segger Embedded Studio.
Tested with nRF52-DK.

# How to compile the code
Copy/link the examples\ble_peripheral\ble_app_uart.ESB and examples\ble_peripheral\common folders into the ble_peripheral example folder of your local SDK.
