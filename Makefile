# Custom Makefile for UNI5ON

all: ## Execute the ./waf command in simulator directory
	@./waf

build: lib-build sim-build ## Rebuild the library and the simulator with current configuration

clean: lib-clean sim-clean ## Clean the library and the simulator

lib-build: ## Rebuild the library with current configuration
	rm -f build/lib/libns3*-ofswitch13-*.so
	cd contrib/ofswitch13/lib/ofsoftswitch13 && make -j4

lib-clean: ## Clean the library
	cd contrib/ofswitch13/lib/ofsoftswitch13 && git clean -fxd

lib-config-optimized: ## Configure the library in optimized mode
	cd contrib/ofswitch13/lib/ofsoftswitch13 && ./boot.sh && ./configure --enable-ns3-lib

lib-config-debug: ## Configure the library in debug mode
	cd contrib/ofswitch13/lib/ofsoftswitch13 && ./boot.sh && ./configure --enable-ns3-lib CFLAGS='-g -O0' CXXFLAGS='-g -O0'

sim-build: ## Rebuild the simulator with current configuration
	./waf

sim-clean: ## Clean the simulator
	./waf distclean

sim-config-optimized: ## Configure the simulator in optimized mode
	./waf configure -d optimized

sim-config-debug: ## Configure the simulator in debug mode
	./waf configure -d debug --enable-examples

help: ## Print this help message
	@grep -E '^[a-zA-Z_-]+:.*?## .*$$' $(MAKEFILE_LIST) | sort | awk 'BEGIN {FS = ":.*?## "}; {printf "\033[36m%-30s\033[0m %s\n", $$1, $$2}'

