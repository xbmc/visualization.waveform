include $(XBMCROOT)/tools/depends/Makefile.include

LIBNAME=xbmc-visualization-waveform
VERSION=master
GIT_DIR=$(TARBALLS_LOCATION)/$(LIBNAME).git
BASE_URL=git://github.com/cptspiff/visualization.waveform.git
DYLIB=$(PLATFORM)/addons/visualization.waveform/waveform.so
XBMC_ADDONSDIR=$(XBMCROOT)/addons
ADDON=visualization.waveform

#tell git to use the addons repo rather than xbmc's repo
export GIT_DIR
export GIT_WORK_TREE=$(PLATFORM)

#mysql_config is remarkably useless. Help configure find the right one.
export MYSQL_CONFIG=$(PREFIX)/bin/mysql_config

# configuration settings
CONFIGURE="cmake . -DCMAKE_INSTALL_PREFIX=$(PREFIX) -DCMAKE_BUILD_TYPE=Release"

all: .installed-$(PLATFORM)

$(GIT_DIR)/HEAD:
	cd $(TARBALLS_LOCATION); git clone --bare $(BASE_URL)

$(GIT_DIR)/current/$(VERSION): $(GIT_DIR)/HEAD $(DEPS)
	git rev-list -1 $(VERSION) >/dev/null || git fetch origin "+refs/heads/*:refs/remotes/origin/*"
	git rev-list -1 $(VERSION) >/dev/null
	rm -rf $(GIT_DIR)/current; mkdir -p $(GIT_DIR)/current
	touch $@

$(PLATFORM)/bootstrap: $(GIT_DIR)/current/$(VERSION)
	rm -rf $(PLATFORM); mkdir -p $(PLATFORM)
	git checkout $(VERSION) -- .

$(PLATFORM)/Makefile: $(PLATFORM)/CMakeLists.txt
	cd $(PLATFORM); $(CONFIGURE)

$(DYLIB): $(PLATFORM)/Makefile
	make -C $(PLATFORM)
	touch $@

.installed-$(PLATFORM): $(DYLIB)
	make package
	touch $@

clean:
	-make -C $(PLATFORM) clean
	rm -f .installed-$(PLATFORM)

distclean::
	rm -rf $(PLATFORM) .installed-$(PLATFORM)
	rm -rf $(GIT_DIR)/current
