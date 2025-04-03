//=============================================================================================
// Z�ld h�romsz�g: A framework.h oszt�lyait felhaszn�l� megold�s
//=============================================================================================
#include "./include/framework.h"
#include <iostream>
#include <vector>

// cs�cspont �rnyal�
const char *vertSource = R"(
	#version 330
    precision highp float;

	layout(location = 0) in vec2 cP;	// 0. bemeneti regiszter

	void main() {
		gl_Position = vec4(cP.x, cP.y, 0, 1); 	// bemenet m�r normaliz�lt eszk�zkoordin�t�kban
	}
)";

// pixel �rnyal�
const char *fragSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;			// konstans sz�n
	out vec4 fragmentColor;		// pixel sz�n

	void main() {
		fragmentColor = vec4(color, 1); // RGB -> RGBA
	}
)";

const int winWidth = 600, winHeight = 600;
const int mapWidth = 64, mapHeight = 64;

class Map {
  std::vector<int> mapIn = { // Run Length Enc
      252, 252, 252, 252, 252, 252, 252, 252, 252, 0,   9,   80,  1,   148, 13,
      72,  13,  140, 25,  60,  21,  132, 41,  12,  1,   28,  25,  128, 61,  0,
      17,  4,   29,  124, 81,  8,   37,  116, 89,  0,   69,  16,  5,   48,  97,
      0,   77,  0,   25,  8,   1,   8,   253, 253, 253, 253, 101, 10,  237, 14,
      237, 14,  241, 10,  141, 2,   93,  14,  121, 2,   5,   6,   93,  14,  49,
      6,   57,  26,  89,  18,  41,  10,  57,  26,  89,  18,  41,  14,  1,   2,
      45,  26,  89,  26,  33,  18,  57,  14,  93,  26,  33,  18,  57,  10,  93,
      18,  5,   2,   33,  18,  41,  2,   5,   2,   5,   6,   89,  22,  29,  2,
      1,   22,  37,  2,   1,   6,   1,   2,   97,  22,  29,  38,  45,  2,   97,
      10,  1,   2,   37,  42,  17,  2,   13,  2,   5,   2,   89,  10,  49,  46,
      25,  10,  101, 2,   5,   6,   37,  50,  9,   30,  89,  10,  9,   2,   37,
      50,  5,   38,  81,  26,  45,  22,  17,  54,  77,  30,  41,  22,  17,  58,
      1,   2,   61,  38,  65,  2,   9,   58,  69,  46,  37,  6,   1,   10,  9,
      62,  65,  38,  5,   2,   33,  102, 57,  54,  33,  102, 57,  30,  1,   14,
      33,  2,   9,   86,  9,   2,   21,  6,   13,  26,  5,   6,   53,  94,  29,
      26,  1,   22,  29,  0,   29,  98,  5,   14,  9,   46,  1,   2,   5,   6,
      5,   2,   0,   13,  0,   13,  118, 1,   2,   1,   42,  1,   4,   5,   6,
      5,   2,   4,   33,  78,  1,   6,   1,   6,   1,   10,  5,   34,  1,   20,
      2,   9,   2,   12,  25,  14,  5,   30,  1,   54,  13,  6,   9,   2,   1,
      32,  13,  8,   37,  2,   13,  2,   1,   70,  49,  28,  13,  16,  53,  2,
      1,   46,  1,   2,   1,   2,   53,  28,  17,  16,  57,  14,  1,   18,  1,
      14,  1,   2,   57,  24,  13,  20,  57,  0,   2,   1,   2,   17,  0,   17,
      2,   61,  0,   5,   16,  1,   28,  25,  0,   41,  2,   117, 56,  25,  0,
      33,  2,   1,   2,   117, 52,  201, 48,  77,  0,   121, 40,  1,   0,   205,
      8,   1,   0,   1,   12,  213, 4,   13,  12,  253, 253, 253, 141};

  vec4 colors[4] = {
      vec4(1, 1, 1, 1), // fehér
      vec4(0, 0, 1, 1), // kék
      vec4(0, 1, 0, 1), // zöld
      vec4(0, 0, 0, 1)  // fekete
  };

  Geometry<vec4> map;

public:
  std::vector<vec4> &mapVtx = map.Vtx(); // alias

  bool decode() {
    mapVtx.clear(); // ürítés

    for (int i = 0; i < mapIn.size(); i++) {
      int length = 0; // pixelek száma
      int color = 0;  // szín idx

      length = (mapIn[i] >> 2); // 2-vel right bitshiftelem
      color = mapIn[i] & 3;     // bit mask 3 = 11

      for (int j = 0; j < length + 1; j++) {
        mapVtx.push_back(colors[color]);
      }
    }
    return mapVtx.size() == 64 * 64; // különben túl lett indexelve
  }

  Map() {
    decode();
    map.updateGPU(); // gpu-ra feltöltés
  }

}; // END OF MAP

/* --- */


class Main : public glApp {
  Map* map;
  GPUProgram* gpuProgram;

public:
  Main() : glApp("Mercator Map") {}

  void onInitialization() override {
    map = new Map();

    gpuProgram = new GPUProgram();
    gpuProgram->create(vertSource, fragSource);
  }

  void onDisplay() override {
    glClearColor(0, 0, 0, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, winWidth, winHeight);

    gpuProgram->Use();
    map->draw(); // lásd következő lépés
  }

  void onKeyboard(int key) override {
    if (key == 'r') {
      delete map;
      map = new Map(); // újratöltés, debug célra pl.
      refreshScreen();
    }
  }
};
