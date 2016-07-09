# mnemon
Yet another hard link based snapshot backup system
------------------------------------------------------------------
Purpose of this project is the creation of a backup system able to generate snapshots of specified target directories. The first snapshot is a 1:1 copy of the target. Then, each following snapshot is stored in a new directory, which will contain hard link copies of unmodified files, and a full copy of new or modified files.

A SQLite database is used to store the list of all files involved, in order to keep inode data like last modification timestamp, associated filename, and number of links.

The advantages of this method consist in a very fast differences check, full backup directory for each snapshot, and the possibility of move file or entire directories around without having to back they up again in the next snapshot.
