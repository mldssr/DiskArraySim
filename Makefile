BUILD := $(PWD)/build/
CXX ?= g++

INSTALL = cp
DEB_PATH ?= deb_package
VERSION = 1
RELEASE = 0
ARCH = x86_64
# nbftp_1.0_x86_64.deb
DEB_PACKAGE = nbftp_$(VERSION).$(RELEASE)_$(ARCH).deb

CFLAGS := -O3 -Wall -std=c++11
INCLUDE := $(shell pkg-config --cflags glib-2.0 openssl zlib)
LIBFLAGS := $(shell pkg-config --libs glib-2.0 openssl zlib)

# searching directory for .cpp and .h files.
VPATH := src:test
INCLUDE += -Isrc
LIBFLAGS += -pthread

# make sure log comes before config, so that ~Log() comes after ~Config()
UTILS := basic.cpp file.cpp log.cpp \
		config.cpp monitor.cpp
FILES := $(addprefix utils/,$(UTILS)) \
		model.cpp \
		data.cpp \
		req.cpp
AFILES := sim.cpp
TFILES := test_basic.cpp \
		test_file.cpp \
		test_file_lock.cpp \
		test_config.cpp \
		test_config_change.cpp \
		test_log.cpp \
		test_monitor.cpp \
		test_model.cpp \
		test_test.cpp \
		test_data.cpp \
		test_req.cpp

OBJS := $(addprefix $(BUILD),$(subst .cpp,.o,$(FILES)))
AOBJS := $(addprefix $(BUILD),$(subst .cpp,.o,$(AFILES)))
TOBJS := $(addprefix $(BUILD),$(subst .cpp,.o,$(TFILES)))

APPS := $(basename $(AOBJS))
TAPPS := $(basename $(TOBJS))

# all: test

.PHONY: all sim test clean

sim: $(APPS)
	$(BUILD)sim $(PWD)/conf.conf

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
	$(BUILD)test_file $(BUILD)tmpfile

test_file_lock: $(BUILD)test_file_lock
	touch $(BUILD)tmpfile
	$(BUILD)test_file_lock $(BUILD)tmpfile

test_config: $(BUILD)test_config test/test.conf
	$(BUILD)test_config test/test.conf

test_config_change: $(BUILD)test_config_change
	$(BUILD)test_config_change $(BUILD)

test_log: $(BUILD)test_log
	$(BUILD)test_log $(BUILD)tmplog

test_monitor: $(BUILD)test_monitor
	$(BUILD)test_monitor $(BUILD)logs/test $(BUILD)test

test_model: $(BUILD)test_model
	$(BUILD)test_model $(PWD)/conf.conf

test_test: $(BUILD)test_test
	$(BUILD)test_test $(PWD)/conf.conf

test_data: $(BUILD)test_data
	$(BUILD)test_data $(PWD)/conf.conf

test_req: $(BUILD)test_req
	$(BUILD)test_req $(PWD)/conf.conf

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
	@rm -rf $(OBJS) $(AOBJS) $(TOBJS) $(APPS) $(TAPPS)
#	@rm -rf $(BUILD)*
