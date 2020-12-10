###### tags: `SOA` `Practice 3`
# The FAT32 file system

This document describes the FAT32 file system. This format is widely used and is the most common one in USB storage systems. 

In practice 3 we will read and analyze a FAT32 formatted disk but instead of using a physical disk we will use an image obtained by dumping the physical content, sector by sector, from a USB memory. A sector defines a space of 512 bytes.

This document is based on the following WEB web documents, which you can consult for further information:
+ http://www.tolaemon.com/docs/fat32.htm
+ https://www.pjrc.com/tech/8051/ide/fat32.html

The structure overview of a FAT32 file system is as follows:

![](https://i.imgur.com/CvKgDuT.png)

The disk begins with a series of reserved sectors, the first sector known as the Volume ID contains the BIOS Parameter Block (BPB), also called Boot Sector (BS) or sector 0. It is the first sector of the media and contains the elementary information of the device.

Next, the "File Allocation Table" (FAT) is located. The FAT is in the form of a table organized in successive 32-bit entries. One entry for each cluster of the file system.  These entries allow to know the state of each one of these clusters, that is to say, they know if they are defective or not, if they contain or not information and, if it is like that, if they correspond with the last cluster of the file or, on the other hand, if the file continues in some other cluster. Of the 32 bits of each entry only the lower 28 are used, the top four are reserved.

At the end are the sectors of the disk dedicated to store the information of directories and files. The allocation unit is the cluster. A cluster is formed by one or several sectors. This is defined in the BPB.

## The `Volume ID`

The first sector of the disk known as "Volume ID" contains the basic information of the logical structure of the disk, the following is a C structure with all the fields.

```c=
//Structure to access boot sector data
struct  __attribute__((__packed__)) BS_Structure {
	uint8_t  jumpBoot[3];     
	uint8_t  OEMName[8];
	uint16_t bytesPerSector;      //bytes por sector, default: 512
	uint8_t  sectorPerCluster;    //sectores por cluster
	uint16_t reservedSectorCount; //sectores reservados
	uint8_t  numberofFATs;        //número de FATs
	uint16_t rootEntryCount;
	uint16_t totalSectors_F16;    //must be 0 for FAT32
	uint8_t  mediaType;
	uint16_t FATsize_F16;         //must be 0 for FAT32
	uint16_t sectorsPerTrack;
	uint16_t numberofHeads;
	uint32_t hiddenSectors;
	uint32_t totalSectors_F32;
	uint32_t FATsize_F32;       //count of sectors occupied by one FAT
	uint16_t extFlags;
	uint16_t FSversion;      //0x0000 (defines version 0.0)
	uint32_t rootCluster;    //first cluster of root directory (=2)
	uint16_t FSinfo;          //sector number of FSinfo structure (=1)
	uint16_t BackupBootSector;
	uint8_t  reserved[12];
	uint8_t  driveNumber;
	uint8_t  reserved1;
	uint8_t  bootSignature;
	uint32_t volumeID;
	uint8_t  volumeLabel[11];   //"NO NAME "
	uint8_t  fileSystemType[8];  //"FAT32"
	uint8_t  bootData[420];
	uint16_t bootEndSignature;  //0xaa55
};
```

Using this definition you can open and then read the boot sector (volume id) doing:

```c
ssize_t  bytes_read;
struct BS_Structure bs;
bytes_read = read(fd, &bs, sizeof(struct BS_Structure) );
```

## Hexadecimal dump of the file system

Using a tool such as `hexdump` o `hexedit` you can browse the contents of an image from a device such as a USB drive:

```
$ hexdump -C -n 512 fatsoa.fs
00000000  eb 58 90 6d 6b 66 73 2e  66 61 74 00 02 08 20 00  |.X.mkfs.fat... .|
00000010  02 00 00 00 00 f8 00 00  20 00 40 00 00 00 00 00  |........ .@.....|
00000020  00 20 03 00 c8 00 00 00  00 00 00 00 02 00 00 00  |. ..............|
00000030  01 00 06 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
00000040  80 00 29 0f 70 45 60 53  4f 41 2d 46 53 20 20 20  |..).pE`SOA-FS   |
00000050  20 20 46 41 54 33 32 20  20 20 0e 1f be 77 7c ac  |  FAT32   ...w|.|
00000060  22 c0 74 0b 56 b4 0e bb  07 00 cd 10 5e eb f0 32  |".t.V.......^..2|
00000070  e4 cd 16 cd 19 eb fe 54  68 69 73 20 69 73 20 6e  |.......This is n|
00000080  6f 74 20 61 20 62 6f 6f  74 61 62 6c 65 20 64 69  |ot a bootable di|
00000090  73 6b 2e 20 20 50 6c 65  61 73 65 20 69 6e 73 65  |sk.  Please inse|
000000a0  72 74 20 61 20 62 6f 6f  74 61 62 6c 65 20 66 6c  |rt a bootable fl|
000000b0  6f 70 70 79 20 61 6e 64  0d 0a 70 72 65 73 73 20  |oppy and..press |
000000c0  61 6e 79 20 6b 65 79 20  74 6f 20 74 72 79 20 61  |any key to try a|
000000d0  67 61 69 6e 20 2e 2e 2e  20 0d 0a 00 00 00 00 00  |gain ... .......|
000000e0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
*
000001f0  00 00 00 00 00 00 00 00  00 00 00 00 00 00 55 aa  |..............U.|
00000200
```

In the previous dump you can find, among other information, the magic number (signature) `0xAA55` that marks the end of the boot sector.

## The File Allocation Table (FAT)

The FAT is an array of 32 bits integers in which each entry defines the state of a cluster. The stored value indicates which is the next cluster of a file, if it is the last one or if the cluster is empty or can't be used because it has errors. The FAT location with respect to the beginning of the image is calculated in the following way:

```c=
uint32_t fat_begin_offset = bs_data.reservedSectorCount * bs_data.bytesPerSector;
```

If we look for these values in the previous dump, the `bs_data.reservedSectorCount` offset is 14, it occupies 2 bytes (`uint_16t`), and the value of these two bytes are `20 00`. The value is in little endian, therefore with real value is `0x0020` o `32` in decimal. Following the same procedure, we can find that `bs_data.bytesPerSector` is 512 bytes per sector. The offset of the FAT in the disk is `32 * 512 = 16384`. With this information we can dump the FAT with hexdump doing:

```
hexdump -s 16384 -n 512 -C fatsoa.fs
00004000  f8 ff ff 0f ff ff ff 0f  f8 ff ff 0f ff ff ff 0f  |................|
00004010  ff ff ff 0f 06 00 00 00  ff ff ff 0f 07 ff ff ff  |................|
00004020  07 ff ff ff 07 ff ff ff  07 ff ff ff 07 ff ff ff  |................|
*
00004200
```

![](https://i.imgur.com/p3TrMth.png)

In the previous figure we can highlight the following entries:

* Label 1. `0xF8` `0xFF` `0xFF` `0x0F` = `0x0FFFFFF8` Corresponds to the first cluster, which is always reserved. The FAT entry of this cluster always contains the `mediaType` byte of the BS in its lowest byte, followed by all of a few. As in this case the mediaType is `0xF8`, the FAT entry is `0x0FFFFFF8`.


* Label 2. `0xFF` `0xFF` `0xFF` = `0xFFFFFFFF` Corresponds to the first reserved cluster of the FAT and as BS rootCluster is usually 2, it contains the information of the root directory. It is the last cluster in the directory.

* Labels 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15: `0xFF` `0xFF` `0xFF` `0x0F` = `0x0FFFFFFF` EOC mark of FAT32, which indicates that it is the last cluster of the file or directory. If the file or directory continues in another cluster, this entry will contain the number of the cluster where it continues. But as it does not continue in another cluster, it contains the EOC mark.

* Labels 16 , 17, ... - 0x00 0x00 0x00 0x00 = 0x000000 Indicates that the clusters are free. 

## Reading a directory 

In the clusters area you can find the information of directories and files present in the disk. The first cluster, numbered as cluster 2, corresponds to the information of the root directory (/). The information of the directory will indicate us which files and subdirectories it contains and in which cluster each one of them begins.

The way to calculate the offset of a cluster is given by the following expression, it is necessary to remember that the first cluster is numbered as cluster 2 and contains the information of the root directory:

```c=
uint32_t offset;
// struct BS_Structure bs contiene la información del primer sector 
       
offset = ( (cluster - 2) * // El cluster 2 es el direcorio raiz
           bs_data.bytesPerSector * bs_data.sectorPerCluster ) +
          (bs_data.bytesPerSector * bs_data.reservedSectorCount) +
          (bs.numberofFATs * data.FATsize_F32 * bs.bytesPerSector );
```

## Reading a directory entries.

The information of a directory present in a sector responds to the following structure:

```c=
//Structure to access Directory Entry in the FAT
struct DIR_Structure{
	uint8_t  DIR_name[11];
	uint8_t  DIR_attrib;     //file attributes
	uint8_t  NTreserved;     //always 0
	uint8_t  timeTenth;      //tenths of seconds, set to 0 here
	uint16_t createTime;     //time file was created
	uint16_t createDate;     //date file was created
	uint16_t lastAccessDate;
	uint16_t firstClusterHI; //higher word of the first cluster 
                                 //number
	uint16_t writeTime;      //time of last write
	uint16_t writeDate;      //date of last write
	uint16_t firstClusterLO; //lower word of the first cluster
                                 // number
	uint32_t fileSize;       //size of file in bytes
};
```
This structure occupies 32 bytes, so in one sector we can store 16 directory entries. For example:

```c=
struct DIR_Structure directory_info[16]; // One sector
```
The following code can be used to read the directory information:

```c=
offset = calculate_offset_for_cluster_2();
    
lseek(fd, offset, SEEK_SET);
bytes_read = read(fd, directory_info, sizeof(directory_info) );
```

Once you have read the structure, you can analyze each of the entries. For each entry you can tell whether it is a file or a subdirectory by looking at the attribute field of the `DIR_attrib` entry. This field can take the following values:



| Bit attr | Meaning     | Comments |
| -------- | --------    | -------- |
| 0 (LSB)  | Read Only   | Should not allow writing |
| 1        | Hidden      | Should not be shown      |
| 2        | System      | File is operating system |
| 3        | Volume ID   | File is volume id        |
| 4        | Directory   | File is a directory      |
| 5        | Archive     | File is regular file     |
| 6        | Unused      | Should be zero           |
| 7 (MSB)  | Unused      | Should be zero           |

Once a file or directory is located in one of the `DIR_Structure` entries, the fields `firstClusterHI` and `firstClusterLO` define the value of the cluster where the file or subdirectory information starts.

```c=
uint32_t cluster;
cluster = directory_info[ i ].firstClusterHI << 16) +
          directory_info[ i ].firstClusterLO;
```
