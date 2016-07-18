#!/bin/bash

trap "exit" INT TERM
trap "echo caught exit signal; kill 0" EXIT

bin/wiimote	< pipe/control_out	| tee pipe/control_in
