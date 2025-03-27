#include "include/framework.h"
#include <GLFW/glfw3.h>
#include <glm/matrix.hpp>
#include <iostream>

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

GPUProgram *gpuProgram;

/*
* --- Kamera osztály ---
* Másolva a ppt-ből
*/

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

/*
* CatmullRom spline
*/
class Spline {
    Geometry<vec3> cps; // control points
    Geometry<float> ts; // parameter (knot) values
    // Folytonos, sima görbét hozhatunk létre a pontok és azokhoz tartozó érintővektorok (sebességek) alapján.
    vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1, float t) {
        float dt = t1 - t0;
        float u = (t - t0) / dt;
        float u2 = u * u;
        float u3 = u2 * u;

        vec3 a0 = p0;
        vec3 a1 = v0;
        vec3 a2 = (3.0f * (p1 - p0) / (dt * dt)) - ((v1 + 2.0f * v0) / dt);
        vec3 a3 = (2.0f * (p0 - p1) / (dt * dt * dt)) + ((v1 + v0) / (dt * dt));

        return a3 * u3 + a2 * u2 + a1 * u + a0;
    }

    std::vector<vec3>& cp = cps.Vtx(); // alias a vezérlőpontokra
    std::vector<float>& t = ts.Vtx();  // alias a csomóértékekre

    public:

    void AddControlPoint(vec3 p) {
        float ti = cp.size();
        cp.push_back(p);
        t.push_back(ti);

        cps.updateGPU();
    }

    // Lokálja meglyik szakaszra esik a t paraméter
    vec3 r(float param) {
        for (int i = 0; i < cp.size() - 1; i++) {
            if (t[i] <= param && param <= t[i + 1]) {
                vec3 v0(0, 0, 0), v1(0, 0, 0);
                if (i > 0) v0 = (cp[i] - cp[i - 1]) / (t[i] - t[i - 1]);
                if (i < cp.size() - 2) v1 = (cp[i + 1] - cp[i]) / (t[i + 1] - t[i]);
                return Hermite(cp[i], v0, t[i], cp[i + 1], v1, t[i + 1], param);
            }
        }
        // TOOD KEZELNI
        return vec3(0, 0, 0); // fallback, ha nem találunk megfelelő intervallumot
    }


    void drawSpline(GPUProgram* gpu, vec3 color) {
        std::vector<vec3> points;
        if (cp.size() < 2) return;

        // Görbe pontjainak kiszámítása
        float dt = 0.01f; // lépésköz
        for (float param = t[0]; param <= t[t.size() - 1]; param += dt) {
            points.push_back(r(param));
        }

        Geometry<vec3> curve;
        curve.Vtx() = points;
        curve.updateGPU();
        curve.Draw(gpu, GL_LINE_STRIP, color);

        cps.updateGPU(); // ha nem biztos, hogy friss
        cps.Draw(gpu, GL_POINTS, vec3(1.0f, 0.0f, 0.0f)); // piro
    }


}; // END OF SPLINE


// Kocsi class
class Gondola;

class Main : public glApp {
  Spline* spline;
  GPUProgram *gpuProgram;

public:
  Main() : glApp("Lab02") {}

  void onInitialization() {
    spline = new Spline();
    gpuProgram = new GPUProgram(vertSource, fragSource);
  }

  // Pontok hozzáadása
  void onMousePressed(MouseButton but, int pX, int pY) override{
      if (but == MOUSE_LEFT) {
        float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
        float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

        spline->AddControlPoint(vec3(ndcX, ndcY, 0));
        refreshScreen();
      }
  }

  void onDisplay() {
      glClearColor(0.5f, 0.5f, 0.5f, 1);
      glClear(GL_COLOR_BUFFER_BIT);
      glViewport(0,0, winWidth, winHeight);
      glPointSize(10.0f); // FONTOS!

      spline->drawSpline(gpuProgram, vec3(1, 0, 0));
  }

};


Main app;
