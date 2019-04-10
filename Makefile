all: build_filter query_filter

dependencies/fastfilter_cpp/src/xorfilter_plus.h:
	git submodule update --init --recursive

query_filter: src/query_filter.cpp src/hexutil.h            src/mappeablexorfilter.h
	c++ -O3 -o query_filter src/query_filter.cpp

build_filter: src/build_filter.cpp dependencies/fastfilter_cpp/src/xorfilter.h dependencies/fastfilter_cpp/src/xorfilter_plus.h src/hexutil.h            src/mappeablexorfilter.h
	c++ -O3 -o build_filter src/build_filter.cpp -std=c++11 -Wall -Idependencies/fastfilter_cpp/src

clean:
	rm -f build_filter query_filter

