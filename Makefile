all: build_filter

dependencies/fastfilter_cpp/src/xorfilter_plus.h:
	git submodule update --init --recursive

build_filter: src/build_filter.cpp dependencies/fastfilter_cpp/src/xorfilter_plus.h
	c++ -O3 -o build_filter src/build_filter.cpp -std=c++11 -Wall -Idependencies/fastfilter_cpp/src