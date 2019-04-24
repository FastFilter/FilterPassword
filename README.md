
## What is this?

Suppose that you have a database made of half a billion passwords. You want to check quickly whether a given password is in this database. We allow a small probability of false positives (that can be later checked against the full database) but we otherwise do not want any false negatives (if the password is in the set, we absolutely want to know about it).

The typical approach to this problem is to apply a Bloom filters. We test here an Xor Filter. The goal is for the filter to use very little memory.


## Requirement


We expect Linux or macOS.

Though the constructed filter may use only about a byte per set entry, the construction itself may need 64 bytes per entry (only during the construction process). A machine with much RAM is recommended.

## Limitations

The expectation is that the filter is built once. To build the filter over the full 550 million passwords, you currently need a machine with a sizeable amount of free RAM. It will almost surely fail on a laptop with 4GB or 8GB of RAM; 64 GB of RAM or more is recommended. We could further partition the problem (by dividing up the set) for lower memory usage or better parallelization.

Queries are very fast, however.

We support up to 4 billion entries, if you have the available memory.


## Preparing the data file

Grab password file from
https://haveibeenpwned.com/passwords

Most likely, you will download a 7z archive. On macOS, you can
uncompress it as follows assuming you are a brew user:

```
brew install p7zip
7z x  pwned-passwords-sha1-ordered-by-count-v4.7z
```

where 'pwned-passwords-sha1-ordered-by-count-v4.7z' is the name of the file you downloaded.

The process is similar under Linux. On Linux Ubuntu, try:

```
sudo apt-get install p7zip-full
7z x  pwned-passwords-sha1-ordered-by-count-v4.7z
```

This takes a long time. The rest of our process is faster.


## Design of the filter

Though the filter can use little memory (less than a GB), it seems unwarranted to load it up entirely in memory to query a few times. Thus we use memory-file mapping instead.  Given that the filter is immutable, it is even safe to use it as part of multithreaded applications.


## Usage


There are two executables:

- `build_filter` is the expensive program that parses the large text files containing password hashes.
- `query_filter` is a simple program that takes a 64-bit hash in hexadecimal for (the first 16 hexadecimal characters from the SHA1 hash) and checks whether the hash is contained in our set.

Do 

```
make
./build_filter -o filter.bin pwned-passwords-sha1-ordered-by-count-v4.txt
./query_filter filter.bin 7C4A8D09CA3762AF
./query_filter -s filter.bin shadow
````

where 'pwned-passwords-sha1-ordered-by-count-v4.txt' is the file you want to process.

Sample output:

```
$./build_filter pwned-passwords-sha1-ordered-by-count-v4.txt
I read 551509767 hashes in total (121.942 seconds).
Constructing the filter...
Done in 102.147 seconds.

$ ./query_filter filter.bin 7C4A8D09CA3762AF
hexval = 0x7c4a8d09ca3762af
I expect the file to span 678357069 bytes.
memory mapping is a success.
Probably in the set.
Processing time 58.000 microseconds.
Expected number of queries per second: 17241.379
```


## Performance comparisons

For a comparable false positive probability (about 0.3%), the Bloom filter uses more space
and is slower. The main downside of the xor filter is a more expensive construction.


```
$ ./build_filter -m 10000000 -o xor8.bin -f xor8 pwned-passwords-sha1-ordered-by-count-v4.txt
setting the max. number of entries to 10000000
read 10000000 hashes.Reached the maximum number of lines 10000000.
I read 10000000 hashes in total (0.915 seconds).
Bytes read = 452288199.
Constructing the filter...
Done in 1.455 seconds.
filter data saved to xor8.bin. Total bytes = 12300054.

$ ./build_filter -m 10000000 -o bloom12.bin -f bloom12 pwned-passwords-sha1-ordered-by-count-v4.txt
setting the max. number of entries to 10000000
read 10000000 hashes.Reached the maximum number of lines 10000000.
I read 10000000 hashes in total (0.914 seconds).
Bytes read = 452288199.
Constructing the filter...
Done in 0.448 seconds.
filter data saved to bloom12.bin. Total bytes = 15000024.



$ for i in {1..3} ; do ./query_filter xor8.bin 7C4A8D09CA3762AF6 ; done
hexval = 0x7c4a8d09ca3762af
Xor filter detected.
I expect the file to span 12300054 bytes.
memory mapping is a success.
Probably in the set.
Processing time 88.000 microseconds.
Expected number of queries per second: 11363.637
hexval = 0x7c4a8d09ca3762af
Xor filter detected.
I expect the file to span 12300054 bytes.
memory mapping is a success.
Probably in the set.
Processing time 59.000 microseconds.
Expected number of queries per second: 16949.152
hexval = 0x7c4a8d09ca3762af
Xor filter detected.
I expect the file to span 12300054 bytes.
memory mapping is a success.
Probably in the set.
Processing time 68.000 microseconds.
Expected number of queries per second: 14705.883

$ for i in {1..3} ; do ./query_filter bloom12.bin 7C4A8D09CA3762AF6 ; done
hexval = 0x7c4a8d09ca3762af
Bloom filter detected.
I expect the file to span 15000024 bytes.
memory mapping is a success.
Surely not in the set.
Processing time 99.000 microseconds.
Expected number of queries per second: 10101.010
hexval = 0x7c4a8d09ca3762af
Bloom filter detected.
I expect the file to span 15000024 bytes.
memory mapping is a success.
Surely not in the set.
Processing time 88.000 microseconds.
Expected number of queries per second: 11363.637
hexval = 0x7c4a8d09ca3762af
Bloom filter detected.
I expect the file to span 15000024 bytes.
memory mapping is a success.
Surely not in the set.
Processing time 86.000 microseconds.
Expected number of queries per second: 11627.907
```


