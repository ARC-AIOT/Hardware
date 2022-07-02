##
# application source dirs
##
override ALGO_TYPE := $(strip $(ALGO_TYPE))
override LINKER_SCRIPT_FILE := $(strip $(LINKER_SCRIPT_FILE))

APPL_CSRC_DIR = ./src
APPL_CXXSRC_DIR = ./src
APPL_ASMSRC_DIR = ./src
APPL_INC_DIR = ./inc 


include $(EMBARC_ROOT)/app/scenario_app/scenario_app.mk