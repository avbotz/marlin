#!/bin/bash

trap "exit" INT TERM
trap "echo caught exit signal; kill 0" EXIT

for pipe in $(./pipes.sh); do
	tail -f /dev/null > ${pipe} &
	echo opened ${pipe}
done

cat

