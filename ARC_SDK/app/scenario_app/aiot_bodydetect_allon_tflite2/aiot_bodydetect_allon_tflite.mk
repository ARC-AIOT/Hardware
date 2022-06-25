override SCENARIO_APP_SUPPORT_LIST := $(APP_TYPE)

ifneq ("$(ALGO_TYPE)","")
ifeq ("$(ALGO_TYPE)","TFLITE_MICRO_GOOGLE_PERSON")
LIB_SEL += tflitemicro_24
endif
else #default algo
override ALGO_TYPE := DATA_BSS_SRAM
endif
APPL_DEFINES += -DAIOT_BODYDETECT_ALLON_GOOGLE

LIB_SEL += sensordp pwrmgmt
LIB_SEL += event
LIB_SEL += i2c_comm ota

EVENTHANDLER_SUPPORT = event_handler
EVENTHANDLER_SUPPORT_LIST += evt_i2ccomm evt_datapath evt_peripheral_cmd
APPL_DEFINES += -DI2C_COMM
APPL_DEFINES += -DEVT_I2CCOMM
APPL_DEFINES += -DEVT_DATAPATH
APPL_DEFINES += -DEVENT_HANDLER_LIB


ifeq ($(BOARD), fpga)
else
APPL_DEFINES += -DSPI_MASTER_SEND
endif