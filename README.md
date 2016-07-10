# mnemon
------------------------------------------------------------------
Yet another hard link based snapshot backup system.
------------------------------------------------------------------

Purpose of this project is the creation of a backup system able to generate snapshots of specified target directories. The first snapshot is a 1:1 copy of the target. Then, each following snapshot is stored in a new directory, which will contain hard link copies of unmodified files, and a full copy of new or modified files.

A SQLite database is used to store the list of all files involved, in order to keep necessary inode data like last modification timestamp, associated filename, and number of files pointing to it.

Advantages of this method consist in a very fast differences check, full backup directory for each snapshot, and the possibility of moving files or entire directories around without having to back they up again in the next snapshot.

Compile instructions
------------------------------------------------------------------
To compile you must install SQLite 3 developer libraries. Under debian/ubuntu:
apt-get install libsqlite3-dev

Then to compile you have to:
gcc mnemon.c -o mnemon -lsqlite3

Usage
------------------------------------------------------------------
First of all keep in mind that Mnemon only works with root permissions.
You must specify the list of directories to back up, one per line, into a file called included_paths.txt, and store it in the same directory of mnemon (Next todo thing is the possibility to specify this file by CLI arguments, or paths directly).

CLI usage:
./mnemon /path/to/backup/dir/
