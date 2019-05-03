# DMX512
This project uses the TM4C123GXL Launchpad from Texas Instruments, which features a TM4C123GH6PM ARM processor. This processor is a feature-packed processor and in this project, we attempt to explore some of the functionality it offers, by setting it up as a DMX-512 Controller and Receiver. The Launchpad is also connected to a SN75HVD12 RS-485 transceiver, which allows the DMX data to be effortlessly used by 512 devices, by connecting it to the data lines in parallel over a long distance. The Launchpad sends signals to the transceiver by utilizing the UART1 module on the TM4C123GH6PM processor. Similar setups are made where one module is a controller and the rest are devices.
For the code, a reference manual is attached with this project report which contains descriptions of all variables and functions and what they are used for.