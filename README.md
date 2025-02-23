# UART Bootloader for STM32F4 MCU

This is a simple UART bootloader written for STM32f4 MCUs based on the libopencm3 library.

## Building

clone this repo, build and setup the submodules and run make.
```
git clone git@github.com:mdnr/stm32f4-uart-bootloader.git --recurse-submodules --shallow-submodules
cd stm32f4-uart-bootloader
make -C src/shared/opencm3/ TARGETS=stm32/f4
cp configs/FreeRTOSConfig.h src/app/freertos/include/
make
```
The versioning of the build output is done automatically based on the latest tag and commit hash.
## Memory layout

Currently the flash memory of the MCU is divided as below:

 ```
 ****************************************
 0x0800000: Flash base  / Bootloader start
 Bootloader: 16KB
 ****************************************
 0x0804000: Bootloader end / Parameters start
 Parameters: 8KB (Config and application settings based on user's needs)
 ----------------------------------------
 0x8002000: Parameters end / Diagnostic area start
 Doggy litter box: 8KB  (Watchdog register dump area)
 ****************************************
 0x0808000: Application start
 Aplication Vector table
 ----------------------------------------
 Firmware info (Version, size, etc)
 ----------------------------------------
 Application code
 ****************************************
  ```
  The root directory Makefile takes care of compiling both binaries
  for bootloader and application and flashing them in the corresponding addresses.
  
  ```
  make all -> compile both application and bootloader binaries
  make app -> compile application binary
  make bl -> compile bootloader binary
  make flash -> flash both application and bootloader to MCU
  make flash-bl -> flash bootloader only without touching the application
  make flash-app -> flash application only without touching the bootloader
  ```
  
  Also there are 2 debug targets `(make fw-debug and make bl-debug)` for debugging the bootlaoder and application
  remotely on the target.
  
  ## Using the bootloader
  
  Run the python script from the root directory to connect to the serial port using a usb-serial converter
  ```
  ./bootloader.py
  ```
  Specify the port connected to the MCU board (e.g. `/dev/ttyUSB0`) and choose one of 
  the actions listed:
  1. `Read parameters` -> Read current parameters stored in the MCU's parameter section
  2. `Update parameters` -> Write new parameters to the flash without touching application code
  3. `Read firmware info` -> Read firmware verison, size, etc.
  4. `Reset the core` -> Send reboot command ans start bootloader process.
  5. `Lock serial line` -> Lock the UART line in case it's used by the application
  6. `UnLock serial line` -> Release the UART lock
  7. `Read diagnostics` -> Read the dumped diagnostic data from corresponding section
  8. `Abort` -> Quit the bootlaoder

## TODO
 
- Adding GSM layer to support OTA update
- Rewriting `bootloader.py` in C perhaps and adding proper UI to it
- Adding Encryption
