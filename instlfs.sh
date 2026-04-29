#!/bin/bash

host=$1
shift
for file in "$@"
do
    echo Uploading $file
    curl -F name=$file -F path=/sys -F upload=@$file http://$host.local/upload
    echo
done
