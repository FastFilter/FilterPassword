all: build_filter query_filter

dependencies/fastfilter_cpp/src/xorfilter.h:
	git submodule update --init --recursive

dependencies/xor_singleheader/include/xorfilter.h:
	git submodule update --init --recursive

query_filter: src/query_filter.cpp src/hexutil.h          dependencies/xor_singleheader/include/xorfilter.h
	c++ -O3 -o query_filter src/query_filter.cpp -Wall -std=c++11 -Idependencies

build_filter: src/build_filter.cpp dependencies/fastfilter_cpp/src/xorfilter.h dependencies/fastfilter_cpp/src/xorfilter_plus.h src/hexutil.h             dependencies/xor_singleheader/include/xorfilter.h
	c++ -O3 -o build_filter src/build_filter.cpp -std=c++11 -Wall -Idependencies/fastfilter_cpp/src -Idependencies

clean:
	rm -f build_filter query_filter
