SUBDIRS=src

INSTALLDIRS=addon

install-data-local:
	@for dir in $(INSTALLDIRS); do \
	  cp -r $$dir/* $(prefix)/usr/lib/xbmc/addons/visualization.waveform; \
   done
