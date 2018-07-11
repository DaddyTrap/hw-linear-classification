CXX=g++
CXX_STANDARD=c++14
CXX_FLAGS=-g -lpthread -O3

BUILD_DIR=build

rf.out: main.cpp data-reader.h decision-tree.h random-forest.h sample.h util.h
	mkdir -p $(BUILD_DIR) && g++ main.cpp -o $(BUILD_DIR)/rf.out -std=$(CXX_STANDARD) $(CXX_FLAGS)

clean:
	if [ -e $(BUILD_DIR) ]; then rm $(BUILD_DIR)/*; fi
