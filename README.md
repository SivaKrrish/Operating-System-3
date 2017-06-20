# Operating-System-3
File Managment
Design and implement a file management module for our Operating System Simulator oss.

You will implement the following functions to manage the files: files_init which should perform any required

initializations to your data structures; openf which is called by a process requesting access to a file; closef, which

is called by a process when it is done with the file; readf to get data from an open file; writef to put data into a

file; and notify_files which lets a user process know that an I/O request has been completed.

You will also have to implement internal routines for file creation and deletion. These routines are not part of the

interface, but are necessary for the implementation.

Operating System Simulator

This will be your main program and serve as the master process. You will start the operating system simulator (call

the executable oss) as one main process who will fork multiple children at random times. The randomness will be

simulated by a logical clock that will be updated by oss as well as user processes. Thus, the logical clock resides

in shared memory and is accessed as a critical resource using a semaphore. You should have two unsigned integers

for the clock; one will show the time in seconds and the other will show the time in nanoseconds, offset from the

beginning of a second.

In the beginning, oss will allocate shared memory for system data structures, including file directory. You can create

fixed sized arrays for system file directory, with a total of 1024 files. The user processes will have their own local

directory that will contain the name of the file and an index number that points to the file entry in the system

directory. All the file attributes, such as ownership, size, location, access and modification times, and link count are

kept in the system directory.

Assume that your system has a total storage space of 8MB with a block size of 1K. Use a bit vector to keep track of

the unallocated blocks.

After the resources have been set up, fork a user process at random times (between 1 and 500 milliseconds of your

logical clock). Make sure that you never have more than 18 user processes in the system. If you already have

18 processes, do not create any more until some process terminates. 18 processes is a hard limit and you should

implement it using a #define macro. Your user processes execute concurrently and there is no scheduling performed.

They run in a loop constantly till they have to terminate or are killed.

oss will monitor all file references from user processes. Implement the file references through a semaphore for each

process. Effectively, if there is no file reference, oss just increments the clock by 10 microseconds and sends a signal

on the corresponding semaphore. In case of a file reference, oss queues the request to the device. Each request for

disk read/write takes about 15ms to be fulfilled. Multiple requests to the device get queued and serviced in the order

they are received.

When a process terminates, oss should log its termination in the log file and also indicate the average time spent

waiting for the file operation. Log all the operations as well.

User Processes

Each user process generates file references at random. Make sure that you open a file before any operation and check

to make sure that you have appropriate permissions to read/write.

December 04, 2015

File Management 2

At random times, say every 1000 ± 100 ms, the user process will check whether it should terminate. If so, all its files

are closed and oss should be informed of its termination.

Make sure that you have signal handling to terminate all processes, if needed. In case of abnormal termination, make

sure to remove shared memory and semaphores.

