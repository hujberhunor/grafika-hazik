
# A projekt neve
TARGET = main

# A forrás fájlok
SRCS = main.cpp ./lib/glad.c ./include/framework.cpp ./lib/lodepng.cpp

# A könyvtárak és az include fájlok
INCLUDES = -I./include -I/usr/include/glm -I./lib/

# A használt könyvtárak
LIBS = -lstdc++fs -lglfw -lGL -lpng

# A fordító és a flags
CXX = g++
CXXFLAGS = -Wall -std=c++17

# Az alapértelmezett cél
all: $(TARGET)

# A cél (kimeneti fájl) elkészítése
$(TARGET): $(SRCS)
	$(CXX) $(CXXFLAGS) $(SRCS) $(INCLUDES) $(LIBS) -o $(TARGET)

# A "clean" cél a build fájlok törlésére
clean:
	rm -f $(TARGET)

# A "make run" parancs futtatásához
run: $(TARGET)
	./$(TARGET)
