BLUEZ_DIR ?= .

#inlcude linux lib for /usr/include
LIBS = -lpthread -lgio-2.0 -lglib-2.0 -lc -lz -lm -lpcre -lgobject-2.0 -lgmodule-2.0 -lffi

#this libs is bluez5.65 lib for build,if had other lib can update
# LIBS += -L$(BLUEZ_DIR)/lib -lbluetooth-internal

#for some bluez h file include other lib path
OTHER_INC += -I/usr/include/glib-2.0
OTHER_INC += -I/usr/lib/arm-linux-gnueabihf/glib-2.0/include

#bluez add need complier c file
BLUEZ_SRC = $(BLUEZ_DIR)/vendor/btgatt-server.c
BLUEZ_SRC += $(BLUEZ_DIR)/vendor/btgatt-client.c

BLUEZ_SRC += $(BLUEZ_DIR)/lib/bluetooth.c
BLUEZ_SRC += $(BLUEZ_DIR)/lib/uuid.c
BLUEZ_SRC += $(BLUEZ_DIR)/lib/sdp.c
BLUEZ_SRC += $(BLUEZ_DIR)/lib/hci.c

BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/io-mainloop.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/mainloop.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/queue.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/mainloop-notify.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/mgmt.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/timeout-mainloop.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/util.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/crypto.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/gatt-helpers.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/att.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/gatt-server.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/gatt-client.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/gatt-db.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/ecc.c
BLUEZ_SRC += $(BLUEZ_DIR)/src/shared/log.c

#bluez include
BLUEZ_INC = -I .

#cpst file for send some event to other thread
CPOST_SRC = $(BLUEZ_DIR)/cpost/cpost.c

#cpost include
CPOST_INC = -I ./cpost

# user add file
APP_SRC = ./app/main.c
# APP_SRC +=

#for build use define
SRC = ${APP_SRC} ${BLUEZ_SRC} ${CPOST_SRC}
INC = ${OTHER_INC} ${BLUEZ_INC} ${CPOST_INC}

# build success, the executor file name
OBJ_NAME= demo_bluez

#fixed notation, not need modify
all : $(OBJ_NAME)

$(OBJ_NAME) : $(SRC)
	$(CC) $(SRC) $(LIBS) $(CC_FLAG) ${INC} -O2 -o $@
	
.PHONY : $(objects)

clean :
	rm -rf $(OBJ_NAME) $(objects)
