#config.mk START
# Select maple_RET6 for Flymaple
BOARD = maple_RET6
# HAL_BOARD determines default HAL target.
HAL_BOARD ?= HAL_BOARD_FLYMAPLE
# The communication port used to communicate with the Flymaple
PORT = /dev/ttyACM0
# You must provide the path to the libmaple library directory:
LIBMAPLE_PATH = ./../libmaple
# Also, the ARM compiler tools MUST be in your current PATH like:
# export PATH=$PATH:~/libmaple/arm/bin
#config.mk END
