#!/bin/bash

trap "exit" INT TERM
trap "echo caught exit signal; kill 0" EXIT

bin/wiiuse	< pipe/control_out	| tee pipe/control_in
