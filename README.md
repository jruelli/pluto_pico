# Pluto-pico

This repository is forked from the Zephyr example application.
[example-application]: https://github.com/zephyrproject-rtos/example-application

## Getting Started

Before getting started, make sure you have a proper Zephyr development
environment. Follow the official
[Zephyr Getting Started Guide](https://docs.zephyrproject.org/latest/getting_started/index.html).

### Initialization

The first step is to initialize the workspace folder (``pluto-pico-workspace``) where
the ``pluto_pico`` and all necessary zephyr modules will be cloned. Run the following
command:

```shell
# initialize pluto-pico-workspace for the pluto_pico (main branch)
west init -m https://gitlab.com/pluto-ipek/pluto_pico.git --mr main pluto-pico-workspace
# update Zephyr modules
cd pluto-pico-workspace
west update
```

### Building and running the main application

To build the application for the pluto_pico board, run the following command from the workspace path:

```shell
cd pluto_pico.git
west build -b rpi_pico -p auto app
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```
