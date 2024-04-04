#!/bin/bash
FILES=test/*.c
for f in $FILES
do
  echo "Processing $f file..."
  filename="${f%.*}"
  gcc -I. $f -o ${filename}.out
done
