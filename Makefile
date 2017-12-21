BUILD := $(PWD)/build/
CXX ?= g++

INSTALL = cp
DEB_PATH ?= deb_package
VERSION = 1
RELEASE = 0
ARCH = x86_64
DEB_PACKAGE = nbftp_$(VERSION).$(RELEASE)_$(ARCH).deb

CFLAGS := -O3 -Wall -std=c++11
INCLUDE := $(shell pkg-config --cflags glib-2.0 openssl zlib)
LIBFLAGS := $(shell pkg-config --libs glib-2.0 openssl zlib)

# searching directory for .cpp and .h files.
VPATH := src:test
INCLUDE += -Isrc
LIBFLAGS += -pthread

UTILS := basic.cpp file.cpp config.cpp \
		log.cpp rawdata.cpp protect.cpp \
		transfer.cpp monitor.cpp\
		taskfile.cpp taskqueue.cpp
FILES := $(addprefix utils/,$(UTILS)) \
		taskop.cpp dataop.cpp \
		communication.cpp monitorop.cpp \
		extension.cpp
AFILES := nbftp_server.cpp nbftp.cpp
TFILES := test_basic.cpp \
		test_file.cpp \
		test_file_lock.cpp \
		test_config.cpp \
		test_log.cpp \
		test_rawdata.cpp \
		test_transfer.cpp \
		test_taskqueue.cpp \
		test_taskop.cpp \
		test_monitor.cpp \
		test_extension.cpp \
		test_protect.cpp \
		test_config_change.cpp \
		test_task_transfer.cpp

OBJS := $(addprefix $(BUILD),$(subst .cpp,.o,$(FILES)))
AOBJS := $(addprefix $(BUILD),$(subst .cpp,.o,$(AFILES)))
TOBJS := $(addprefix $(BUILD),$(subst .cpp,.o,$(TFILES)))

APPS := $(basename $(AOBJS))
TAPPS := $(basename $(TOBJS))

all: test

.PHONY: all server test clean

server: $(APPS)

$(OBJS) $(AOBJS) $(TOBJS): $(BUILD)%.o : %.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CFLAGS) $(INCLUDE) -c $< -o $@

$(APPS) $(TAPPS): % : %.o $(OBJS)
	$(CXX) $^ -o $@ $(LIBFLAGS)

test: server $(basename $(TFILES))

.PHONY: $(basename $(TFILES))

test_basic: $(BUILD)test_basic
	$(BUILD)test_basic

test_file: $(BUILD)test_file
	$(BUILD)test_file $(BUILD)/tmpfile

test_file_lock: $(BUILD)test_file_lock
	touch $(BUILD)/tmpfile
	$(BUILD)test_file_lock $(BUILD)/tmpfile

test_config: $(BUILD)test_config test/test.conf
	$(BUILD)test_config test/test.conf

test_log: $(BUILD)test_log
	$(BUILD)test_log $(BUILD)logs/test

test_rawdata: $(BUILD)test_rawdata
	$(BUILD)test_rawdata

test_transfer: $(BUILD)test_transfer
	$(BUILD)test_transfer server 9999 $(BUILD)logs/testserver &
	sleep 1
	$(BUILD)test_transfer 127.0.0.1 9999 $(BUILD)logs/testclient

test_taskqueue: $(BUILD)test_taskqueue
	$(BUILD)test_taskqueue $(BUILD)test/taskfile

test_taskop: $(BUILD)test_taskop
	$(BUILD)test_taskop client.conf

test_monitor: $(BUILD)test_monitor
	$(BUILD)test_monitor $(BUILD)logs/test $(BUILD)/test

test_extension: $(BUILD)test_extension
	$(BUILD)test_extension client.conf

test_protect: $(BUILD)test_protect
	$(BUILD)test_protect client.conf &
	sleep 2
	echo 0 > build/client/control

test_config_change: $(BUILD)test_config_change
	$(BUILD)test_config_change $(BUILD)

test_task_transfer: $(BUILD)test_task_transfer
	$(BUILD)test_task_transfer server.conf &
	sleep 1
	$(BUILD)test_task_transfer client.conf

.PHONY: build_dir pre_pkg deb

build_dir:
	rm -rf $(DEB_PATH)
	mkdir -p $(DEB_PATH)/DEBIAN/
	mkdir -p $(DEB_PATH)/usr/bin/
	mkdir -p $(DEB_PATH)/etc/nbftp/
	mkdir -p $(DEB_PATH)/etc/init.d/

pre_pkg: build_dir server
	$(INSTALL) $(APPS) $(DEB_PATH)/usr/bin/
	$(INSTALL) files/etc/* $(DEB_PATH)/etc/nbftp/
	$(INSTALL) files/nbftp.init $(DEB_PATH)/etc/init.d/nbftp
	$(INSTALL) files/debian/* $(DEB_PATH)/DEBIAN/

deb: pre_pkg
	cd $(DEB_PATH); dpkg -b . ../$(DEB_PACKAGE)

clean:
	@rm -rf $(OBJS) $(AOBJS) $(APPS)
	@rm -rf $(BUILD)/*
