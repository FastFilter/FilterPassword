all: build_filter query_filter

dependencies/fastfilter_cpp/src/xorfilter/xorfilter.h:
	git submodule update --init --recursive

dependencies/xor_singleheader/include/binaryfusefilter.h:
	git submodule update --init --recursive

query_filter: src/query_filter.cpp \
	src/hexutil.h \
	src/mappeablebloomfilter.h \
	src/util.h \
	src/sha.h \
	dependencies/xor_singleheader/include/binaryfusefilter.h \
	dependencies/xor_singleheader/include/xorfilter.h
	c++ -O3 -o query_filter src/query_filter.cpp -Wall -std=c++11 -Idependencies

build_filter: src/build_filter.cpp \
	src/hexutil.h \
	src/mappeablebloomfilter.h \
	src/util.h \
	dependencies/fastfilter_cpp/src/bloom/bloom.h \
	dependencies/fastfilter_cpp/src/hashutil.h \
	dependencies/xor_singleheader/include/binaryfusefilter.h \
	dependencies/xor_singleheader/include/xorfilter.h
	c++ -O3 -o build_filter src/build_filter.cpp -std=c++11 -Wall -Idependencies/fastfilter_cpp/src -Idependencies

test: build_filter query_filter
	./build_filter -V -f xor8 -o filter.bin sample.txt  && \
	./query_filter filter.bin 7C4A8D09CA3762AF | grep "Probably in the set" && \
	./build_filter -V -f binaryfuse8 -o filter.bin sample.txt  && \
	./query_filter filter.bin 7C4A8D09CA3762AF | grep "Probably in the set" && \
	./build_filter -V -f binaryfuse16 -o filter.bin sample.txt  && \
	./query_filter filter.bin 7C4A8D09CA3762AF | grep "Probably in the set" && \
	./build_filter -V -f bloom12 -o filter.bin sample.txt && \
	./query_filter filter.bin 7C4A8D09CA3762AF | grep "Probably in the set" && \
	echo "SUCCESS" || (echo "Failure. There is a bug."| exit -1)

clean:
	rm -f build_filter query_filter
