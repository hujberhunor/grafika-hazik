#include "include/framework.h"
// csúcspont árnyaló
const char *vertSource = R"(
	#version 330				
    precision highp float;

	layout(location = 0) in vec2 cP;	// 0. bemeneti regiszter
	uniform mat4 MVP;
	void main() {
		gl_Position = MVP * vec4(cP.x, cP.y, 0, 1); 	// bemenet már normalizált eszközkoordinátákban
	}
)";

// pixel árnyaló
const char *fragSource = R"(
	#version 330
    precision highp float;

	uniform vec3 color;			// konstans szín
	out vec4 fragmentColor;		// pixel szín

	void main() {
		fragmentColor = vec4(color, 1); // RGB -> RGBA
	}
)";
const int winWidth = 600, winHeight = 600;

GPUProgram *gpuProgram;

// --- Kamera osztály ---
// Másolva a ppt-ből
class Camera2D {
  vec2 wCenter = vec2(0, 0); // center in world coords
  vec2 wSize = vec2(20, 20); // width and height in world coords
public:
  mat4 V() { return translate(vec3(-wCenter.x, -wCenter.y, 0)); }
  mat4 P() { // projection matrix
    return scale(vec3(2 / wSize.x, 2 / wSize.y, 1));
  }
  mat4 Vinv() { // inverse view matrix
    return translate(vec3(wCenter.x, wCenter.y, 0));
  }
  mat4 Pinv() { // inverse projection matrix
    return scale(vec3(wSize.x / 2, wSize.y / 2, 1));
  }
  void Zoom(float s) { wSize = wSize * s; }
  void Pan(vec2 t) { wCenter = wCenter + t; }
  vec3 screenToWorld(vec2 screenPos) {
    float nx = 2.0f * screenPos.x / winWidth - 1.0f;
    float ny = 1.0f - 2.0f * screenPos.y / winHeight;
    vec4 ndcCoords = vec4(nx, ny, 0.0f, 1.0f);
    vec4 worldCoords = Pinv() * Vinv() * ndcCoords;
    return vec3(worldCoords.x, worldCoords.y, 1.0f);
  }
};

// Görbe class
class Spline;

// Kocsi class
class Gondola;

class Main : public glApp {
  Geometry<vec2> *triangle;
  GPUProgram *gpuProgram;

public:
  Main() : glApp("Lab02") {}

  void onInitialization() {
    triangle = new Geometry<vec2>;
    triangle->Vtx() = {vec2(-0.8f, -0.8f), vec2(-0.6f, 1.0f),
                       vec2(0.8f, -0.2f)};
    triangle->updateGPU();
    gpuProgram = new GPUProgram(vertSource, fragSource);
  }

  void onDisplay() {
    glClearColor(0, 0, 0, 0);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, winWidth, winHeight);
    triangle->Draw(gpuProgram, GL_TRIANGLES, vec3(0.0f, 1.0f, 0.0f));
  }
};

Main app;
