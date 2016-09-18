CC = g++
SRC = src
BUILD = build
BIN = bin

CFLAGS = -ggdb -c -std=c++14 -Iinclude
LFLAGS = 

THIRDPARTY = ./3rdparty/

OPENCV_CFLAGS = `pkg-config --cflags opencv`
OPENCV_LFLAGS = `pkg-config --libs opencv` -pthread

FLYCAP = $(THIRDPARTY)/flycapture/
FLYCAP_CFLAGS = -I$(SRC) -I$(FLYCAP)/include/
FLYCAP_LFLAGS = -L$(FLYCAP)/lib -lflycapture -Wl,-rpath=$(FLYCAP)/lib/


COMMON = $(patsubst %,$(BUILD)/common/%.o,matrix state observation config)
COMMON_CFLAGS = 

MODEL = $(patsubst %,$(BUILD)/model/%.o,system hypothesis evidence)
MODEL_CFLAGS = 

IMAGE = $(patsubst %,$(BUILD)/image/%.o,image)
IMAGE_CFLAGS = $(OPENCV_CFLAGS)

VISION = $(patsubst %,$(BUILD)/vision/%.o,vision blob blob_detection neural_network config)
VISION_CFLAGS = $(OPENCV_CFLAGS)

MODELING = $(patsubst %,$(BUILD)/modeling/%.o,main) $(MODEL) $(COMMON)
MODELING_CFLAGS = 
MODELING_LFLAGS = 

INTERFACE = $(patsubst %,$(BUILD)/interface/%.o, main functions data config) $(MODEL) $(IMAGE) $(COMMON)
INTERFACE_CFLAGS = $(OPENCV_CFLAGS)
INTERFACE_LFLAGS = $(OPENCV_LFLAGS)

MISSION = $(patsubst %,$(BUILD)/mission/%.o, main mission command query goal action config) $(MODEL) $(IMAGE) $(COMMON)
MISSION_CFLAGS = $(OPENCV_CFLAGS) $(IMAGE_CFLAGS) $(MODEL_CFLAGS)
MISSION_LFLAGS = $(OPENCV_LFLAGS)

CAMERA = $(patsubst %,$(BUILD)/camera/%.o,main) $(IMAGE) $(COMMON)
CAMERA_CFLAGS = $(OPENCV_CFLAGS) $(FLYCAP_CFLAGS)
CAMERA_LFLAGS = $(OPENCV_LFLAGS) $(FLYCAP_LFLAGS)

BUOYS = $(patsubst %,$(BUILD)/buoys/%.o,buoys) $(IMAGE) $(VISION) $(COMMON)
BUOYS_CFLAGS = $(OPENCV_CFLAGS)
BUOYS_LFLAGS = $(OPENCV_LFLAGS)

PVC = $(patsubst %,$(BUILD)/pvc/%.o,pvc) $(IMAGE) $(VISION) $(COMMON)
PVC_CFLAGS = $(OPENCV_CFLAGS)
PVC_LFLAGS = $(OPENCV_LFLAGS)

TORPEDOES = $(patsubst %,$(BUILD)/torpedoes/%.o,main) $(IMAGE) $(VISION) $(COMMON)
TORPEDOES_CFLAGS = $(OPENCV_CFLAGS)
TORPEDOES_LFLAGS = $(OPENCV_LFLAGS)

DROPPER = $(patsubst %,$(BUILD)/dropper/%.o,main) $(IMAGE) $(VISION) $(MODEL) $(COMMON)
DROPPER_CFLAGS = $(OPENCV_CFLAGS)
DROPPER_LFLAGS = $(OPENCV_LFLAGS)

HYDROPHONES = $(patsubst %,$(BUILD)/hydrophones/%.o,main) $(COMMON)
HYDROPHONES_CFLAGS =
HYDROPHONES_LFLAGS =

CAMERAPIPE = $(patsubst %,$(BUILD)/camera_pipe/%.o,main) $(IMAGE) $(COMMON)
CAMERAPIPE_CFLAGS = $(OPENCV_CFLAGS)
CAMERAPIPE_LFLAGS = $(OPENCV_LFLAGS)

IMAGE_READ = $(patsubst %,$(BUILD)/image_read/%.o,main) $(IMAGE) $(COMMON)
IMAGE_READ_CFLAGS = $(OPENCV_CFLAGS) $(IMAGE_CFLAGS)
IMAGE_READ_LFLAGS = $(OPENCV_LFLAGS)

IMAGE_SHOW = $(patsubst %,$(BUILD)/image_show/%.o,main) $(IMAGE) $(COMMON)
IMAGE_SHOW_CFLAGS = $(OPENCV_CFLAGS) $(IMAGE_CFLAGS)
IMAGE_SHOW_LFLAGS = $(OPENCV_LFLAGS)

IMAGE_LOG = $(patsubst %,$(BUILD)/image_log/%.o,main) $(IMAGE) $(COMMON)
IMAGE_LOG_CFLAGS = $(OPENCV_CFLAGS) $(IMAGE_CFLAGS)
IMAGE_LOG_LFLAGS = $(OPENCV_LFLAGS)

SIM_STATE = $(patsubst %,$(BUILD)/sim_state/%.o,main) $(COMMON)
SIM_STATE_CFLAGS = -pthread
SIM_STATE_LFLAGS = -pthread -latomic


EXE_NAMES = modeling interface mission camera camera_pipe buoys pvc torpedoes dropper hydrophones image_read image_show image_log sim_state

EXE = $(patsubst %,$(BIN)/%,$(EXE_NAMES))

all: $(EXE)

$(BIN)/modeling: $(MODELING)
	$(CC) $^ $(LFLAGS) $(MODELING_LFLAGS) -o $@

$(BIN)/interface: $(INTERFACE)
	$(CC) $^ $(LFLAGS) $(INTERFACE_LFLAGS) -o $@

$(BIN)/mission: $(MISSION)
	$(CC) $^ $(LFLAGS) $(MISSION_LFLAGS) -o $@

$(BIN)/camera: $(CAMERA)
	$(CC) $^ $(LFLAGS) $(CAMERA_LFLAGS) -o $@

$(BIN)/buoys: $(BUOYS) 
	$(CC) $^ $(LFLAGS) $(BUOYS_LFLAGS) -o $@

$(BIN)/pvc: $(PVC) 
	$(CC) $^ $(LFLAGS) $(PVC_LFLAGS) -o $@

$(BIN)/torpedoes: $(TORPEDOES) 
	$(CC) $^ $(LFLAGS) $(TORPEDOES_LFLAGS) -o $@

$(BIN)/dropper: $(DROPPER) 
	$(CC) $^ $(LFLAGS) $(DROPPER_LFLAGS) -o $@

$(BIN)/hydrophones: $(HYDROPHONES) 
	$(CC) $^ $(LFLAGS) $(HYDROPHONES_LFLAGS) -o $@

$(BIN)/camera_pipe: $(CAMERAPIPE)
	$(CC) $^ $(LFLAGS) $(CAMERAPIPE_LFLAGS) -o $@

$(BIN)/image_read: $(IMAGE_READ)
	$(CC) $^ $(LFLAGS) $(IMAGE_READ_LFLAGS) -o $@

$(BIN)/image_show: $(IMAGE_SHOW)
	$(CC) $^ $(LFLAGS) $(IMAGE_SHOW_LFLAGS) -o $@

$(BIN)/image_log: $(IMAGE_LOG)
	$(CC) $^ $(LFLAGS) $(IMAGE_LOG_LFLAGS) -o $@

$(BIN)/sim_state: $(SIM_STATE)
	$(CC) $^ $(LFLAGS) $(SIM_STATE_LFLAGS) -o $@

$(BUILD)/common/%.o: $(SRC)/common/%.cpp
	$(CC) $(CFLAGS) $< -o $@

$(BUILD)/model/%.o: $(SRC)/model/%.cpp
	$(CC) $(CFLAGS) $(MODEL_CFLAGS) $< -o $@

$(BUILD)/image/%.o: $(SRC)/image/%.cpp
	$(CC) $(CFLAGS) $(IMAGE_CFLAGS) $< -o $@

$(BUILD)/vision/%.o: $(SRC)/vision/%.cpp
	$(CC) $(CFLAGS) $(VISION_CFLAGS) $< -o $@

$(BUILD)/modeling/%.o: $(SRC)/modeling/%.cpp
	$(CC) $(CFLAGS) $(MODELING_CFLAGS) $< -o $@

$(BUILD)/interface/%.o: $(SRC)/interface/%.cpp
	$(CC) $(CFLAGS) $(INTERFACE_CFLAGS) $< -o $@

$(BUILD)/mission/%.o: $(SRC)/mission/%.cpp
	$(CC) $(CFLAGS) $(MISSION_CFLAGS) $< -o $@

$(BUILD)/camera/%.o: $(SRC)/camera/%.cpp
	$(CC) $(CFLAGS) $(CAMERA_CFLAGS) $< -o $@

$(BUILD)/torpedoes/%.o: $(SRC)/torpedoes/%.cpp
	$(CC) $(CFLAGS) $(TORPEDOES_CFLAGS) $< -o $@

$(BUILD)/buoys/%.o: $(SRC)/buoys/%.cpp
	$(CC) $(CFLAGS) $(BUOYS_CFLAGS) $< -o $@

$(BUILD)/pvc/%.o: $(SRC)/pvc/%.cpp
	$(CC) $(CFLAGS) $(PVC_CFLAGS) $< -o $@

$(BUILD)/dropper/%.o: $(SRC)/dropper/%.cpp
	$(CC) $(CFLAGS) $(DROPPER_CFLAGS) $< -o $@

$(BUILD)/hydrophones/%.o: $(SRC)/hydrophones/%.cpp
	$(CC) $(CFLAGS) $(HYDROPHONES_CFLAGS) $< -o $@

$(BUILD)/camera_pipe/%.o: $(SRC)/camera_pipe/%.cpp
	$(CC) $(CFLAGS) $(CAMERAPIPE_CFLAGS) $< -o $@

$(BUILD)/image_read/%.o: $(SRC)/image_read/%.cpp
	$(CC) $(CFLAGS) $(IMAGE_READ_CFLAGS) $< -o $@

$(BUILD)/image_show/%.o: $(SRC)/image_show/%.cpp
	$(CC) $(CFLAGS) $(IMAGE_SHOW_CFLAGS) $< -o $@

$(BUILD)/image_log/%.o: $(SRC)/image_log/%.cpp
	$(CC) $(CFLAGS) $(IMAGE_LOG_CFLAGS) $< -o $@

$(BUILD)/sim_state/%.o: $(SRC)/sim_state/%.cpp
	$(CC) $(CFLAGS) $(SIM_STATE_CFLAGS) $< -o $@

clean:
	rm -f $(EXE)
	rm -f $(BUILD)/**/*.o

