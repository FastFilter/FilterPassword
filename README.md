
## Requirement


You need a machine with a sizeable amount of free RAM. It will
almost surely fail on a laptop with 4GB or 8GB of RAM.

We expect Linux or macOS.

## Usage


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


Then do 

```
make
./build_filter pwned-passwords-sha1-ordered-by-count-v4.txt
````

where 'pwned-passwords-sha1-ordered-by-count-v4.txt' is the file you want to process.
