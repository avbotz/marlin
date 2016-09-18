#!/bin/bash

trap "exit" INT TERM
trap "echo caught exit signal; kill 0" EXIT

F='%Y-%m-%d_%H-%M-%S'

DATE=$(date +${F})
LOG=logs/${DATE}

mkdir -p ${LOG}
echo Logging in ${LOG}

tee >(ts ${F} >> ${LOG}/mission_in)  < pipe/mission_in  | bin/mission  | tee pipe/mission_out  | ts ${F} >> ${LOG}/mission_out    &
tee >(ts ${F} >> ${LOG}/modeling_in) < pipe/modeling_in | bin/modeling | tee pipe/modeling_out | ts ${F} >> ${LOG}/modeling_out   &

bin/buoys     < pipe/buoys_in 2> >(bin/image_log ${LOG}/ _b) | tee pipe/buoys_out | ts ${F} >> ${LOG}/buoys_out &
bin/pvc       < pipe/pvc_in   2> >(bin/image_log ${LOG}/ _p) | tee pipe/pvc_out   | ts ${F} >> ${LOG}/pvc_out   &
bin/torpedoes < pipe/torps_in 2> >(bin/image_log ${LOG}/ _t) | tee pipe/torps_out | ts ${F} >> ${LOG}/torps_out &
bin/dropper   < pipe/bins_in  2> >(bin/image_log ${LOG}/ _d) | tee pipe/bins_out  | ts ${F} >> ${LOG}/bins_out  &

bin/hydrophones < pipe/hydrophones_in 2> >(ts ${F} >> ${LOG}/hydrophones_log) | tee pipe/hydrophones_out  | ts ${F} >> ${LOG}/hydrophones_out &

bin/interface

