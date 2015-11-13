#!/bin/sh
SOURCE=/home/charles/Scalable/qualnet-edu-7.3
DEST=./qualnet-edu-7.3
rsync -v --files-from=rsync.txt --relative --perms --group $SOURCE $DEST
