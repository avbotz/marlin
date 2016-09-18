#!/bin/bash

trap "exit" INT TERM
trap "echo caught exit signal; kill 0" EXIT

UC=/dev/ttyUSB1
FPGA=/dev/ttyS1

FRONT=0
DOWN=1

F='%Y-%m-%d_%H-%M-%S'

stty -F ${UC} raw
stty -F ${UC} speed 9600
stty -F ${UC} -echo -echoe -echok

cat ${UC}           | tee pipe/control_out | ts ${F} >> logs/control_out &
cat pipe/control_in | tee ${UC}            | ts ${F} >> logs/control_in  &

stty -F ${FPGA} raw
stty -F ${FPGA} speed 9600
stty -F ${FPGA} -echo -echoe -echok

cat ${FPGA} | tee pipe/hydrophones_in >> logs/hydrophones_in &

DISPLAY=localhost:10.0

bin/camera i ${FRONT} 1 < pipe/camera_f_in | tee pipe/camera_f_out | bin/image_log logs/ _f &
bin/camera i ${DOWN}  1 < pipe/camera_d_in | tee pipe/camera_d_out | bin/image_log logs/ _d &

#bin/camera i ${FRONT} 1 < pipe/camera_f_in | tee pipe/camera_f_out > >(bin/image_show 1) | bin/image_log logs/ _f &
#bin/camera i ${DOWN}  0 < pipe/camera_d_in | tee pipe/camera_d_out > >(bin/image_show 1) | bin/image_log logs/ _d &

cat
