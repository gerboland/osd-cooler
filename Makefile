# *  Copyright(C) 2007 Neuros Technology International LLC. 
# *               <www.neurostechnology.com>
# *
# *  Cooler master make file.........................MG 04-14-2007


ifndef PRJROOT
    $(error You must first source the BSP environment: "source neuros-env")
endif

COOLER_CORE        := core
COOLER_MEDIA       := media

.PHONY: usage
usage:
	@echo "Cooler Library Build Help"
	@echo "    make all        -- build the entire project "
	@echo "    make install    -- install to target "
	@echo "    make clean      -- clean project "
	@echo "    make cleanall   -- deep clean project "
	@echo "    make langsync   -- automatic synchronize the language package"
	@echo


.PHONY: all cleanall clean install langsync
all:
	@for dir in  $(COOLER_CORE) $(COOLER_MEDIA) $(COOLER_EXTRA); do \
	make -C $$dir/build all || exit 1;\
	make -C $$dir/build install || exit 1;\
	done;

clean:
	@for dir in  $(COOLER_CORE) $(COOLER_MEDIA) $(COOLER_EXTRA); do \
	make -C $$dir/build clean || exit 1;\
	done;
	if [ -f findout.txt ]; then \
		echo "removing findout.txt"; \
	rm findout.txt; \
	fi; 
	if [ -f sync.config ]; then \
		echo "removing sync.config"; \
	rm sync.config; \
	fi; 
	if [ -d syncfolder ]; then \
		echo "removing syncfolder"; \
		rm -R syncfolder; \
	fi; 


cleanall:
	@for dir in  $(COOLER_CORE) $(COOLER_MEDIA) $(COOLER_EXTRA); do \
	make -C $$dir/build cleanall || exit 1;\
	done;
	if [ -f findout.txt ]; then \
		echo "removing findout.txt"; \
	rm findout.txt; \
	fi; 
	if [ -f sync.config ]; then \
		echo "removing sync.config"; \
	rm sync.config; \
	fi; 
	if [ -d syncfolder ]; then \
		echo "removing syncfolder"; \
		rm -R syncfolder; \
	fi; 


install:
	@for dir in  $(COOLER_CORE) $(COOLER_MEDIA) $(COOLER_EXTRA); do \
	make -C $$dir/build install || exit 1;\
	done;

langsync:
	$(PRJROOT)/scripts/synctool ;
	if [ -f findout.txt ]; then \
		echo "removing findout.txt"; \
	rm findout.txt; \
	fi; 
	if [ -f sync.config ]; then \
		echo "removing sync.config"; \
	rm sync.config; \
	fi; 
	if [ -d syncfolder ]; then \
		echo "removing syncfolder"; \
		rm -R syncfolder; \
	fi; 
