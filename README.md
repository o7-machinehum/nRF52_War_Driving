# nRF52 War Driving
This a repository dedicated to Bluetooth wardriving on the Nordic nRF52. The
idea is to deaddrop the device rather than drive around with it. The nRF52 is a
perfect device for wardriving because...  
    - They're cheap
    - They're available
    - They're extremely low power

## Getting Started
Before getting started, make sure you have a proper Zephyr development
environment. You can follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization
The first step is to initialize the workspace folder (``my-workspace``) where
the ``example-application`` and all Zephyr modules will be cloned. You can do
that by running:

```shell
# initialize my-workspace for the example-application (main branch)
west init -m https://github.com/Machine-Hum/nRF52_War_Driving --mr main nrf52-wd
# update Zephyr modules
cd nrf52-wd
west update
```

### Build & Run
The application can be built by running:

```shell
west build -b $BOARD -s app
```
where `$BOARD` is the target board. 

```shell
west flash
```
