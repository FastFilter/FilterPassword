
## What is this?

Suppose that you have a database made of half a billion passwords. You want to check quickly whether a given password is in this database. We allow a small probability of false positives (that can be later checked against the full database) but we otherwise do not want any false negatives (if the password is in the set, we absolutely want to know about it).

The typical approach to this problem is to apply a Bloom filters. We test here an Xor Filter.


## Requirement


We expect Linux or macOS.


## Limitations

The expectation is that the filter is built once. To build the filter over the full 550 million passwords, you currently need a machine with a sizeable amount of free RAM. It will almost surely fail on a laptop with 4GB or 8GB of RAM.

Queries are very cheap, however.



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

The process is similar under Linx. On Linux Ubuntu, try:

```
sudo apt-get install p7zip-full
7z x  pwned-passwords-sha1-ordered-by-count-v4.7z
```

This takes a long time. The rest of our process is faster.

## Usage


There are two executables:

- `build_filter` is the expensive program that parses the large text files containing password hashes.
- `query_filter` is a simple program that takes a 64-bit hash in hexadecimal for (the first 16 hexadecimal characters from the SHA1 hash) and checks whether the hash is contained in our set.

Do 

```
make
./build_filter -o filter.bin pwned-passwords-sha1-ordered-by-count-v4.txt
./query_filter filter.bin 7C4A8D09CA3762AF
````

where 'pwned-passwords-sha1-ordered-by-count-v4.txt' is the file you want to process.

Sample output:

```
./build_filter pwned-passwords-sha1-ordered-by-count-v4.txt
I read 551509767 hashes in total (121.942 seconds).
Constructing the filter...
Done in 102.147 seconds.
```
