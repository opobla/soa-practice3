# Practice 3. File Systems

This practice consists in the analysis of a code, and later contribution to that same code, that reads and process a volume formatted with FAT32. The practice is organized in three steps, and is designed to be completed in three weeks. 


In this repository you will find all the necessary elements to complete the practice. In particular:

- `fatsoa.fs`: is a file with an image of a FAT32 volume
- `fatsoa.h`, `fatsoa.c`, `parser.c` and `Makefile`: are the source files to be studied and contributed.

In addition to these files, we will be using tools to visualize and edit binary content (i.e `fatsoa.fs`, the filesystem image), through it hexadecimal representation, such as the hexedit viewer, or the `hexdump` tool.

You can find adtional information about the FAT in [this link](the_fat.md). To
perfom the activities required in the following steps, you will need to use
[posix services to access files](posix_services_files.md) in your code.

## Step 1. Introduction

Using the `make` command, compile the program and generate the binary `fatfs`. To build this executable you will only need the sources in the `step1` folder of this repository: `parser.c`, `parser.h`, `fatsoa.c`, `fatsoa.h` and `Makefile`.

Build `fatsoa` using the provided `Makefile`
```bash
$ make
gcc -g -Wall -c -o fatsoa.o fatsoa.c
gcc -g -Wall -c -o parser.o parser.c
gcc -g -Wall -o fatsoa fatsoa.o parser.o
```

This program features the following commands:

- `open <file>` : opens the image file `<file>`
- `volume`: shows the information of the Boot Sector (BS)
- `stat`: displays all current directory information
- `ls`: displays a listing of the directory contents
- `cd <directory>`: change directory

To run this tool, execute `./fatsoa`. A prompt will show up. The following transcription shows how the the `ls` command displays a listing of the root directory, and how the `volume` command displays the contents of the BS.

```
$ ./fatsoa
Introduzca órdenes (pulse Ctrl-D para terminar)
FATFS:open fatsoa.fs
fatsoa.fs opened.
FATFS:/ ls
LEEME   TXT
<DIR> UD4
FATFS:/ volume
Orden no conocida [volume]
FATFS:/ volumen

-------------------------
File system type: FAT32
Bytes per Sector: 512
Sectors per Cluster: 8
Reserved Sectors Count: 32
Number of FATs: 2
FAT sectors size: 200
FAT begin offset:      0x4000
CLUSTERs begin offset: 0x36000
End Signature: 0xAA55
-------------------------
FATFS:
```
With the command `stat` it is possible to visualize the information associated to all the elements of a directory:

```=
FATFS:/ stat
-------------------------
DIR name: SOA-FS
DIR attrib: 8
       - Volume ID
DIR firstClusterHI: 0
DIR firstClusterLO: 0
Image offset: 0x36000
DIR fileSize: 0 [0]
-------------------------
DIR name: Al
DIR attrib: 15
       - Long name
DIR firstClusterHI: 116
DIR firstClusterLO: 0
Image offset: 0x40034000
DIR fileSize: -1 [FFFFFFFF]
-------------------------
DIR name: LEEME   TXT
DIR attrib: 32
       - Archive
DIR firstClusterHI: 0
DIR firstClusterLO: 4
Image offset: 0x38000
DIR fileSize: 50 [32]
-------------------------
DIR name: UD4
DIR attrib: 16
       - Directory
DIR firstClusterHI: 0
DIR firstClusterLO: 3
Image offset: 0x37000
DIR fileSize: 0 [0]
FATFS:/
```

Using the `hexdump` tool, that is available is most of the Unix-like operating system distributions, you can inspect the contents of the filesystem image. For example, we can dump the contents of the `LEEME.TXT` file. From the previous output of the `stat` command, we know that file `LEEME.TXT` (line 19) starts in the offset 0x38000 doing. Let's dump the 512 first bytes from that position:

```
$ hexdump -C -s 0x38000 -n 512 fatsoa.fs
00038000  45 73 74 65 20 65 73 20  75 6e 20 66 69 63 68 65  |Este es un fiche|
00038010  72 6f 20 64 65 20 74 65  78 74 6f 20 65 6e 20 65  |ro de texto en e|
00038020  6c 20 64 69 72 65 63 74  6f 72 69 6f 20 72 61 69  |l directorio rai|
00038030  7a 0a 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |z...............|
00038040  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
00038200
```

With this information you can do the following activities:

- Analyze the code of the `fatfs.c` file and study the operation of the different commands and the data structures used.
- Use the `cd` command to navigate to the `UD4` directory, use `stat` to see the beginning of some of the files inside, and visualize them with your favorite tool.
- Analyze the code and answer why, when visualizing the content of a directory, the first character of the name of each directory entry is compared with the value `0xE5` and if it matches it, then it is not displayed. What does this value mean?



## Step 2. Look for a needle in a haystack

The given filesystem `fatsoa.fs` contains files, such as `LEEME.TXT`. The goal of this step is to contribute to the `fatsoa` tool with a command to `get` a file, given its name, extract it from its location in the filesystem, and write is to the filesystem where the tool is running.

If you open the file `fatsoa.c` you will find the skeleton of the `fs_get` function. This function is called when you issue the `get` command in the `fatsoa` tool. But as it is right now, it does not work:

```
 ./fatsoa
Introduzca órdenes (pulse Ctrl-D para terminar)
FATFS:open fatsoa.fs
fatsoa.fs opened.
FATFS:/ get LEEME.TXT
File name modificado: LEEME   TXTm��-[15]
File name: LEEME   TXT-[11]
LEEME.TXT not found
FATFS:/
```

Complete the code of the function `fs_get`. This function is given the name of a file as a parameter. You must:
1. check that the file exists
2. obtain the beginning of its cluster and its size 
3. make a copy of the clusters to an external file with the same name. 
 
The file can occupy more than one cluster so you should check in the FAT table if the starting cluster is the last one or the file occupies more than one cluster.

## Step 3. Do some forensic analysis

The disk image provided corresponds to a USB confiscated in a police operation. After a forensic analysis, it is discovered that the information on the disk has the following peculiarities:

1. There are FAT entries that describe a series of consecutive clusters with errors (`0xFFFFFF07`) starting from FAT entry 7, as we can see in the following dump of the FAT:

```
$ hexdump -C -s 0x4000 -v -n 128 fatsoa.fs
00004000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f ff ff ff 0f  |................|
00004010  ff ff ff 0f 06 00 00 00  ff ff ff 0f 07 ff ff ff  |................|
00004020  07 ff ff ff 07 ff ff ff  07 ff ff ff 07 ff ff ff  |................|
00004030  07 ff ff ff 07 ff ff ff  07 ff ff ff 07 ff ff ff  |................|
00004040  07 ff ff ff 07 ff ff ff  07 ff ff ff 07 ff ff ff  |................|
00004050  07 ff ff ff 07 ff ff ff  07 ff ff ff 07 ff ff ff  |................|
00004060  07 ff ff ff 07 ff ff ff  07 ff ff ff 07 ff ff ff  |................|
00004070  07 ff ff ff 07 ff ff ff  07 ff ff ff 07 ff ff ff  |................|
00004080
```

2. Also, the root directory entry shows a suspect file that appears to have been deleted. It´s name is `?DS.PDF` and its first cluster is precisely the cluster 7.

```
00036000  53 4f 41 2d 46 53 20 20  20 20 20 08 00 00 96 80  |SOA-FS     .....|
00036010  6a 51 6a 51 00 00 96 80  6a 51 00 00 00 00 00 00  |jQjQ....jQ......|
00036020  41 6c 00 65 00 65 00 6d  00 65 00 0f 00 06 2e 00  |Al.e.e.m.e......|
00036030  74 00 78 00 74 00 00 00  ff ff 00 00 ff ff ff ff  |t.x.t...........|
00036040  4c 45 45 4d 45 20 20 20  54 58 54 20 00 5a 89 75  |LEEME   TXT .Z.u|
00036050  6a 51 6a 51 00 00 89 75  6a 51 04 00 32 00 00 00  |jQjQ...ujQ..2...|
00036060  55 44 34 20 20 20 20 20  20 20 20 10 00 c4 92 76  |UD4        ....v|
00036070  6a 51 6a 51 00 00 92 76  6a 51 03 00 00 00 00 00  |jQjQ...vjQ......|
00036080  e5 6f 00 64 00 73 00 2e  00 70 00 0f 00 8c 64 00  |.o.d.s...p....d.|
00036090  66 00 00 00 ff ff ff ff  ff ff 00 00 ff ff ff ff  |f...............|
000360a0  e5 44 53 20 20 20 20 20  50 44 46 20 00 1c 99 82  |.DS     PDF ....|
000360b0  6a 51 6a 51 00 00 99 82  6a 51 07 00 36 6c 09 00  |jQjQ....jQ..6l..|
000360c0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000360d0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000360e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
000360f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00036100
```
3. The first bytes of the data cluster number 7 (which is in offset 0x3b000) are `%PDF` (0x25, 0x50, 0x44, 0x46). This is the **magic number** (like a signature) of a Portable Document File (PDF).

```
0003b000  25 50 44 46 2d 31 2e 36  0d 25 e2 e3 cf d3 0d 0a  |%PDF-1.6.%......|
0003b010  32 32 37 20 30 20 6f 62  6a 0d 3c 3c 2f 46 69 6c  |227 0 obj.<</Fil|
0003b020  74 65 72 2f 46 6c 61 74  65 44 65 63 6f 64 65 2f  |ter/FlateDecode/|
0003b030  46 69 72 73 74 20 38 30  35 2f 4c 65 6e 67 74 68  |First 805/Length|
0003b040  20 32 30 30 30 2f 4e 20  31 30 30 2f 54 79 70 65  | 2000/N 100/Type|
0003b050  2f 4f 62 6a 53 74 6d 3e  3e 73 74 72 65 61 6d 0d  |/ObjStm>>stream.|
0003b060  0a 99 56 a0 9b 79 ab 46  f8 9e 2c bb b9 4e 4d a3  |..V..y.F..,..NM.|
0003b070  d2 19 cd 5f 5d 44 42 6e  b3 33 5f 54 03 e5 40 49  |..._]DBn.3_T..@I|
0003b080
```

The police investigators suspect that those clusters that are marked as defective are in fact a file that has been hidden on purpose.

With this information:
- Obtain the file size
- Write a program to read all these consecutive clusters starting from cluster 7 into a file with name `_DS.PDF`. Be aware that the last cluster might not be completed, so you need to take the file size into consideration when writing the last cluster.
