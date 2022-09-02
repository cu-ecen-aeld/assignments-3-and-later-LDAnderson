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
if [ ! -d $dir ]
then
    mkdir $dir
fi


echo $2 > $1
