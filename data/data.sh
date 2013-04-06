#!/bin/bash

sed '1,7d' $1".txt" > temp

awk '{print $2, $3}' temp > $1".in"

rm -f temp $1".txt"

