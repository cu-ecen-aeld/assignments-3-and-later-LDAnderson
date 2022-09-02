#!/bin/sh
# Leif Anderson U Colorado

# check # of args
if [ $# -lt 2 ]
then
    echo "Invalid arguments"
    exit 1
fi


if [ -e $1 ]
then
    rm $1
fi

dir=$(dirname $2)
mkdir -p $dir
if [ ! -d $dir ]
then
    echo "error creating files"
    exit 1
fi


echo $2 > $1
