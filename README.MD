Tinyfatfs Low footprint Embedded FAT file system

 EBS - RTFS (Real Time File Manager)

* Copyright, Peter Van Oudenaren
*  EBS Inc. 1987-2015
* All rights reserved.
* This code may not be redistributed in source or linkable object form
* without the consent of its author.

* contact sales@ebsembeddedsoftware.com

Rtfs tiny is a very low footprint implementation of the FAT file system.

This software was written as an experiment to create the smallest footprint FAT file system with the most attainable functionality.

The code achieves small code size by relying on experience providing commercial FAT file system products over many years.

The current code does not support vfat but that can be added pretty easilly in we estimate 1 to 2 K of code space.


The code is still not totally refined so beware, send us an email if you'd find a bug or you would like to conrtibute a patch.

The ram and rom requirements for rtfsl built under various configurations is provided below.

These are for the ARM Processor using the IAR compiler, similar builds need to be made for AVR.

<table>
<tr><td>read only code bytes</td><td>read only data bytes</td> <td>read/write data bytes</td><td>build description</td><tr>
<tr><td> 7422  </td><td>  52 </td><td> 1340 </td><td> Full build with journaling optimized for size </td><tr>
<tr><td> 7098 </td><td>      86 </td><td>     792 </td><td> Full build minus journaling un-optimized</td><tr>
<tr><td>5480  </td><td>      52 </td><td>     792 </td><td> Full build minus journaling optimized</td><tr>
<tr><td>5192  </td><td>      20 </td><td>     780 </td><td> Full build minus journaling, minus subdirectory support optimized for size</td><tr>
<tr><td>3588  </td><td>      20 </td><td>     780 </td><td> Read only file io, directory traversal and file stat functions.</td><tr>
<tr><td>2896  </td><td>      12 </td><td>     532 </td><td> Functionality to load a file from the root directory into memory, optimized for size.</td><tr>
</table>

<br>
Full build includes the follow functionality for fat12, fat16 and fat32.<br:
<ul>
    <li> create a sub-directory in the root or in a subdirectory.</li>
    <li> delete a sub-directory </li>
    <li> create a file in the root or in a subdirectory.</li>
    <li> write to a file.</li>
    <li> read from a file.</li>
    <li> seek within a file.</li>
    <li> close a file.</li>
    <li> delete a file</li>
 </ul>
 
When Failsafe support is enabled, the follow functionality is included.
<ul>
    <li>. journaling flush.</li>
    <li>. journaling journal of FAT table changes and directory entry changes.</li>
    <li>. journaling restore.</li>
</ul>



To build the test applcation for a Linux target:
type:.
  make

To run the symple command shell based example:

type:
   sudo ./main devicename

for example, to access a USB stick at /dev/sdb1:
   sudo ./main /dev/sdb1

If that is successful you should see the following help screen:

LOAD FILENAME<br>
LMOUNT   - re-mount rtfs lite drive<br>
LEXIT    - Exit Lite mode refreshes device mount for rtfs<br>
LFLUSH   - Flush rtfs lite buffers<br>
DIR<br>
RENAME oldname newname<br>
DELETE filename<br>
CHDIR path<br>
MKDIR dirname 0|1 (1 = fragment)<br>
RMDIR dirname<br>
FILLPAT filename nlongs 0|1 (1=fragment)<br>
APPENDPAT filename nlongs 0|1 (1=fragment)<br>
READPAT filename<br>
RANDREAD  filename<br>
RANDWRITE filename<br>
FILLDISK filenamebase<br>
FSSTART Start Journaling<br>
FSSTOP  Stop  Journaling<br>
FSSYNC  Sync  volume, keep journaling<br>
FSFLUSH Flush journal keep journaling<br>
FSRESTORE Retore the volume from the journal<br>


The currently supported features include:<br>
. File io (create,reopen, read,write,seek,delete).<br>
. Subdirectory support (mkdir, rmdir and set working directory).<br>
. Failsafe journaling and restore support.<br>
. All features have been tested both with using Failsafe and not using Failsafe.<br>
. All tests have been performed with FAT12 so far, typically the most difficult case.<br>
<br>
Ongoing development efforts include the following:<br>
. Tests still need to be performed on FAT16 and FAT32.<br>
. Testing is on-going with development of more rigorous tests planned for tomorrow.<br>

