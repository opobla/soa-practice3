# POSIX services to work with files

## Open a file with `open`

The call to `open()` is the first step that must be followed by any process that wants to access the data of a file. It allows you to request read/write operations on the open file at a later time. Its syntax is:

```c
int open(const char* pathname, int flags, mode_t modes)
```

Parameters:
- `pathname` is the file name, optionally including the path.
- `flags` determines if the file should be open in read, write, append, etc. mode.
- `modes` determine the file mode in the case that the file needs to be created.

Return value: integer representing the file descriptor; greater or equal to 0 if the execution was successful. The file descriptor will be used every time you want to perform an operation on the file.

### Recommended activities with the `open` service

Check the manual with `man 2 open` and write a program for the following scenarios:

1. A file that exists and another one that does not exist is opened.
2. A file is opened with permissions and without permissions.
3. Several consecutive open calls are made on different files and on the same file with the same opening mode.
4.	Two different programs perform `open()` on different files and on the same file. Use several terminals, or run in the background (&) to check if the programs can access the same file at the same time.

## Read the contents of a file with `read()`

The call to `read()` allows access to the data stored as contents of a file by saving them in memory positions (variables). Its syntax is:

```c
ssize_t read(int fd, void* buffer, size_t count)
```

Parameters:
- `fd` file descriptor (returned by a call to open()) on which the reading is going to be done.
- `buffer` memory address where the read data will be placed.
- `count` number of bytes the user wants to read.

Return value: integer that indicates the number of bytes read; greater or equal to 0 if the execution has been successful.

The number of bytes read allows to control when the end of the file has been reached. There is not a specific End of File mark. The readings made when the file pointer is at the end of the file, will return zero bytes read.

### Recommended activities with the `read` service

Refer to the manual and program the experiments you consider necessary until you have clear the behavior and the value returned by `read()` in the following cases:
1.	Read from a file a smaller number of bytes and a larger number of bytes than it contains.
2.	Read from a file when all the data contained in it have been read (equivalent to reading from an empty file).
3.	Read from a file using two different descriptors (two `open` on the same file).

## Write new data to a file with the `write` service

The call to `write` allows you to modify the data, or add more, stored as a file content from information stored in memory locations (variables). Its syntax is:

```c 
ssize_t write (int fd, const void* buffer, size_t count)
```

Parameters:
- `fd` file descriptor (returned by a call to `open`) on which the writing is going to be done.
- `buffer` memory address from where the data to be written will be taken.
- `count` number of bytes the user wants to write.
 
Return value: integer that indicates the number of bytes written; greater or equal to 0 if the execution has been successful.

### Recommended activities with the `write` service

Consult the manual and program the experiments you consider necessary until you have clear the behavior and the value returned by `write` in the following cases:

1.	Write in a file a smaller and larger number of bytes than it contains.
2.	Write in a file when you are at the end of the file (equivalent to write in an empty file).
3.	Writing to a file using two different descriptors (two `open` on the same file).


## Repositioning the read/write pointer of a file with `lseek`

The `lseek` call allows you to modify the position where the next read or write operation will be performed on a file already open, that is, it modifies the pointer value that is stored in the corresponding entry of the open file table. Its syntax is:

```c
off_t lseek(int fd, off_t offset, int whence)
```

Parameters:
- `fd` file descriptor (returned by a call to open()) to which the pointer will be modified.
- `offset` number of bytes to move the pointer. It can be positive or negative. The exact meaning of this parameter depends on the value of the whence parameter.
- `whence` position from which the displacement indicated in the previous parameter will be added. It can be `SEEK_SET` (beginning of the file), `SEEK_CUR` (current position of the pointer) or `SEEK_END` (end of the file).

Return value: resulting position in bytes from the beginning of the file (pointer value), or -1 in case of an error.

It is possible to place the pointer beyond the end of a file. In case of writing at that position, the intermediate unwritten gap will be filled with zeros.

### Recommended activities with the `lseek` service

Consult the manual and program the experiments you consider necessary until you have clear the behavior and the value returned by `lseek` in the following cases:
1. Jump to the end of the file and write data.
2. Jump to the beginning of the file and read data.
3. Jump to a place bigger than the file size and write data.
4. Find out the current position with a jump to `{offset=0, whence=SEEK_CUR}`.
5. Find out the size of the file.

## Projecting in memory the content of a file with `mmap`

The call to the `mmap` service allows to project the content of a file in memory positions, being able, after the projection, to access the content of the file through variables that point to the projected area. This means that, after calling `mmap`, it is possible to write or read the content of a file using the corresponding positions of the array that starts in the direction where it has been projected. The syntax of `mmap` is:

```c
void* mmap(void* start, size_t length, int prot, int flags, int fd, off_t offset)
```

Parameters:
- `start` suggested memory address to perform the projection. Usually the address 0 (NULL) is specified, which tells the operating system to search and assign a valid address.
- `length` number of bytes to project.
- `prot` protection of the desired memory. It can be `PROT_EXEC` (pages can be executed, i.e. can contain code), `PROT_READ` (pages can be read), `PROT_WRITE` (pages can be written) or `PROT_NONE` (virtual space will be reserved, but pages will not be accessible until protection is changed with `mprotect`). These values can be combined with the OR operation bit by bit (C language | operator), for example: `PROT_READ|PROT_WRITE`. 
- `flags` projection options. It can be `MAP_FIXED` (do not use a different address than the pass), `MAP_SHARED` (the projected area can be shared with other processes) or `MAP_PRIVATE` (the memory area is private). It is necessary to specify exactly one of the last two options. Additionally, `MAP_FIXED` can be specified, combining it with `MAP_SHARED` or `MAP_PRIVATE` by means of an OR operation bit by bit.
- `fd` file descriptor (returned by a call to `open`) to be projected.
- `offset` position of the file from which the projection is going to be made. It must be a multiple of the page size. The page size can be obtained with a call to `sysconf(_SC_PAGE_SIZE)`.
 
Returned value: memory position where the projection has been done in case it has been done correctly, or MAP_FAILED (`void* -1`) in case of an error or if the requested memory area could not be assigned.

The access to files by projection in memory is faster than the one made by the read and write calls for several reasons. Firstly, with access by projection it is not necessary to access the intermediate tables for each operation. Secondly, accesses outside the memory space of a process are also slower. Finally, accesses with `read` and `write` require a system call for each operation, while with projection in memory, thanks to the spatial and temporal locality, most read/write will not require any immediate action by the operating system.

The time it takes to project a part of a file is the same as it takes to project the whole file, since the operating system only stores the relationship between the virtual memory and the disk positions, leaving the read/write operations for the moments when they are actually performed. For this reason, it is frequent that when several operations are going to be performed on a file, it is projected in a complete way, for which it is necessary to know the size of the file before.

### Recommended activities with `mmap`

Consult the manual and program the experiments you consider necessary until you have clear the behavior and the value returned by `mmap` in the following cases:

1.	Read data from the file content.

## Release the memory where a file is projected with `munmap`

The `munmap` service allows you to release the memory addresses associated with a previously projected file. Its syntax is:

```c
int munmap(void* start, size_t length)
```

Parameters:
- `start` memory address to be released. It must match with a value returned by a previous `mmap` call.
- `length` number of bytes to release.

Return value: integer that indicates if the operation has been done correctly (0) or incorrectly (-1).

After releasing a previously projected memory area, all subsequent accesses to that area will produce invalid memory references.

### Recommended activities for `munmap`

Consult the manual and program the experiments you consider necessary until you have clear the behavior and the value returned by `munmap` in the following cases:
1.	Read data from the file contents after calling `munmap`.

## Close a file with `close`

The `close` service makes a previously opened descriptor available again, thus releasing all the information that the operating system stores related to that open file. When a process is no longer going to use a file it has open, it must be closed to ensure that all operations with that file are completed (the buﬀering system used by UNIX systems, makes an operation not occur just at the time the system call is made, but at the time that is most convenient for system performance). The syntax of this call is:

```c
int close(int fd)
```

Parameters:

- `fd` file descriptor (returned by a call to open()) to be closed.

Returned value: integer that indicates if the operation was performed correctly (0) or incorrectly (-1).

Closing a file releases the corresponding entry in the file descriptor table, and if there are no more entries from other processes pointing to the same entry in the open file table, this last entry is also released.

### Recommended activities with `close`

Consult the manual and program the experiments you consider necessary until you have clear the behavior and the value returned by `close`) in the following cases:

1. Close a non-existent descriptor.
2. Read or write data to a file after closing it.

