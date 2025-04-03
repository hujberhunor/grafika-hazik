#include "include/framework.h"
#include <GLFW/glfw3.h>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/matrix.hpp>
#include <iostream>

const char *vertSource = R"(
    #version 330
    precision highp float;

    layout(location = 0) in vec2 cP;

    uniform mat4 MVP;  // <-- EZ ITT A KULCS

    void main() {
        gl_Position = MVP * vec4(cP, 0.0, 1.0); // <-- transzformáljuk
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

  mat4 MVP() { return P() * V(); }
};

/*
 * CatmullRom spline
 */
class Spline {
  Geometry<vec3> cps; // control points
  Geometry<float> ts; // parameter (knot) values
  vec3 Hermite(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1,
               float t) {
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

public:
  std::vector<vec3> &cp = cps.Vtx(); // alias a vezérlőpontokra
  std::vector<float> &t = ts.Vtx();  // alias a csomóértékekre
  void AddControlPoint(vec3 p) {
    float ti = cp.size();
    cp.push_back(p);
    t.push_back(ti);

    std::cout << '\n' << '\n';
    std::cout << "DEBUG: Controll point placed: " << p.x << p.y;

    cps.updateGPU();
  }

  // Lokálja meglyik szakaszra esik a t paraméter
  vec3 r(float tParam) {
    for (int i = 0; i < cp.size() - 1; i++) {
      if (t[i] <= tParam && tParam <= t[i + 1]) {
        vec3 v0(0, 0, 0), v1(0, 0, 0);

        // Belső szegmensek
        if (i > 0 && i < cp.size() - 2) {
          vec3 right0 = (cp[i + 1] - cp[i]) / (t[i + 1] - t[i]);
          vec3 left0 = (cp[i] - cp[i - 1]) / (t[i] - t[i - 1]);
          v0 = 0.5f * (right0 + left0);

          vec3 right1 = (cp[i + 2] - cp[i + 1]) / (t[i + 2] - t[i + 1]);
          vec3 left1 = (cp[i + 1] - cp[i]) / (t[i + 1] - t[i]);
          v1 = 0.5f * (right1 + left1);
        }
        // Első szegmens (i == 0)
        else if (i == 0 && cp.size() >= 3) {
          v1 = 0.5f * ((cp[i + 2] - cp[i + 1]) / (t[i + 2] - t[i + 1]) +
                       (cp[i + 1] - cp[i]) / (t[i + 1] - t[i]));
        }
        // Utolsó szegmens (i == n-2)
        else if (i == cp.size() - 2 && cp.size() >= 3) {
          v0 = 0.5f * ((cp[i + 1] - cp[i]) / (t[i + 1] - t[i]) +
                       (cp[i] - cp[i - 1]) / (t[i] - t[i - 1]));
        }

        return Hermite(cp[i], v0, t[i], cp[i + 1], v1, t[i + 1], tParam);
      }
    }
    return vec3(0, 0, 0); // Ha nincs megfelelő intervallum
  }

  vec3 HermiteDeriv(vec3 p0, vec3 v0, float t0, vec3 p1, vec3 v1, float t1,
                    float t) {
    float dt = t1 - t0;
    float u = (t - t0) / dt;
    float u2 = u * u;

    vec3 a2 = (3.0f * (p1 - p0) / (dt * dt)) - ((v1 + 2.0f * v0) / dt);
    vec3 a3 = (2.0f * (p0 - p1) / (dt * dt * dt)) + ((v1 + v0) / (dt * dt));

    return (3.0f * a3 * u2 + 2.0f * a2 * u + v0);
  }

  vec3 dr(float tParam) {
    for (int i = 0; i < cp.size() - 1; i++) {
      if (t[i] <= tParam && tParam <= t[i + 1]) {
        vec3 v0(0, 0, 0), v1(0, 0, 0);

        if (i == 0 && cp.size() > 2)
          v1 = 0.5f * ((cp[i + 2] - cp[i + 1]) + (cp[i + 1] - cp[i]));
        else if (i == cp.size() - 2 && cp.size() > 2)
          v0 = 0.5f * ((cp[i + 1] - cp[i]) + (cp[i] - cp[i - 1]));
        else if (i > 0 && i < cp.size() - 2) {
          v0 = 0.5f * ((cp[i + 1] - cp[i]) + (cp[i] - cp[i - 1]));
          v1 = 0.5f * ((cp[i + 2] - cp[i + 1]) + (cp[i + 1] - cp[i]));
        }

        return HermiteDeriv(cp[i], v0, t[i], cp[i + 1], v1, t[i + 1], tParam);
      }
    }
    return vec3(0, 0, 0);
  }

  void drawSpline(GPUProgram *gpu, const mat4 &viewMatrix) {
    std::vector<vec3> points;
    if (cp.size() < 2)
      return;

    mat4 MVP = viewMatrix;
    gpu->setUniform(MVP, "MVP");

    // spline kirajzolása
    float dt = 0.01f;
    for (float param = t[0]; param <= t[t.size() - 1]; param += dt) {
      points.push_back(r(param));
    }

    Geometry<vec3> curve;
    curve.Vtx() = points;
    curve.updateGPU();
    curve.Draw(gpu, GL_LINE_STRIP, vec3(1.0f, 1.0f, 0.0f));

    cps.updateGPU();
    cps.Draw(gpu, GL_POINTS, vec3(1.0f, 0.0f, 0.0f)); // piros pontok
  }

}; // END OF SPLINE

enum GondolaState { WAITING, ROLLING, FALLEN };

class Gondola {
private:
  Spline *spline;
  GondolaState state;

  float param;    // spline menti pozíció
  vec2 pos;       // világkoordinátás pozíció
  float velocity; // sebesség (skalár)
  float radius;   // kerék sugara
  float lambda;   // tehetetlenségi tényező
  float mass;     // tömeg
  float rotation = 0.0f;
  vec2 prevNormal = vec2(0, -1); // kezdetben lefelé

public:
  Gondola(Spline *spline) : spline(spline) {
    state = WAITING;
    param = 0.0f;
    pos = vec2(0, 0);
    velocity = 0.0f;
    radius = 1.0f;
    lambda = 1.0f;
    mass = 1.0f;
  }

  void Start() {
    if (state == WAITING && spline) {
      param = 0.01f;
      vec3 p = spline->r(param);
      pos = vec2(p.x, p.y);
      velocity = 0.0f;
      state = ROLLING;
    }
  }

  void Animate(float dt) {
    if (state != ROLLING || spline == nullptr)
      return;

    // spline pont és derivált
    vec3 p3 = spline->r(param);
    vec3 d3 = spline->dr(param);
    vec2 r = vec2(p3.x, p3.y);
    vec2 dr = normalize(vec2(d3.x, d3.y)); // érintő
    vec2 normal = vec2(-dr.y, dr.x);

    if (dot(normal, prevNormal) < 0)
      normal = -normal;

    prevNormal = normal; // elmentjük a következő lépéshez

    const vec2 gravity = vec2(0, -40);              // gravitáció gyorsulás
    float acc = dot(gravity, dr) / (1.0f + lambda); // tangenciális gyorsulás
    velocity += acc * dt;

    float derivLength = length(vec2(d3.x, d3.y));
    if (derivLength < 0.0001f)
      derivLength = 0.0001f;

    float paramStep = velocity * dt / derivLength;
    param += paramStep;

    // aktuális görbe pont (pozíció)
    vec3 newPos3 = spline->r(param);
    vec2 newPos2D = vec2(newPos3.x, newPos3.y);

    pos = newPos2D - normal * radius;

    // forgás frissítése (gördülés)
    float deltaDistance = paramStep * derivLength;
    rotation += deltaDistance / radius;

    // nyomóerő számítása
    float nyomoEro =
        dot(gravity, normal) + lambda * velocity * velocity / radius;

    // leesés vagy visszagurulás
    if (nyomoEro < 0) {
      std::cout << "FALLEN! nyomoEro < 0\n";
      state = FALLEN;
      return;
    }

    if (velocity < 0) {
      std::cout << "STOPPED! velocity < 0\n";

      // Kezdő pozíció
      param = 0.01f;

      vec3 startPos3D = spline->r(param);
      vec3 startDeriv = spline->dr(param);
      vec2 tangent = normalize(vec2(startDeriv.x, startDeriv.y));
      vec2 normal = vec2(-tangent.y, tangent.x);

      if (dot(normal, vec2(0, -1)) < 0)
        normal = -normal;

      pos = vec2(startPos3D.x, startPos3D.y) - normal * radius;
      velocity = 0.0f;
      rotation = 0.0f;
      prevNormal = normal;

      state = ROLLING;
      return;
    }

    if (param > spline->t.back()) {
      std::cout << "FALLEN! param > spline->t.back()\n";
      state = FALLEN;
      return;
    }
  }

  vec2 getPosition() const { return pos; }

  GondolaState getState() const { return state; }

  void Draw(GPUProgram *gpu, const mat4 &viewMatrix) {
    if (state == WAITING)
      return;

    mat4 model =
        translate(vec3(pos.x, pos.y, 0)) * rotate(rotation, vec3(0, 0, 1));
    mat4 MVP = viewMatrix * model;

    // kör
    Geometry<vec2> circle;
    std::vector<vec2> vtx;
    const int n = 32;
    vtx.push_back(vec2(0, 0));
    for (int i = 0; i <= n; ++i) {
      float angle = i * 2 * M_PI / n;
      vtx.push_back(vec2(radius * cos(angle), radius * sin(angle)));
    }
    circle.Vtx() = vtx;
    circle.updateGPU();

    gpu->setUniform(MVP, "MVP");
    circle.Draw(gpu, GL_TRIANGLE_FAN, vec3(0, 0, 1));

    // std::cout << "DEBUG: Gondola megrajzolva";
    DrawSpokes(gpu, viewMatrix * model);
  }

  void DrawSpokes(GPUProgram *gpu, const mat4 &modelMatrix) {
    const int spokeCount = 4;
    Geometry<vec2> spokes;
    std::vector<vec2> lines;

    for (int i = 0; i < spokeCount; ++i) {
      float angle = i * 2 * M_PI / spokeCount;
      vec2 tip = vec2(radius * cos(angle), radius * sin(angle));
      lines.push_back(vec2(0, 0)); // középpont
      lines.push_back(tip);        // küllő vége
    }

    spokes.Vtx() = lines;
    spokes.updateGPU();

    mat4 MVP = modelMatrix;
    gpu->setUniform(MVP, "MVP");
    spokes.Draw(gpu, GL_LINES, vec3(1, 1, 1));
    // std::cout << "DEBUG: Küllők megrajzolva";
  }
};

class Main : public glApp {
  Spline *spline;
  Gondola *gondola;
  Camera2D camera;
  GPUProgram *gpuProgram;

public:
  Main() : glApp("Lab02") {}

  void onInitialization() {
    spline = new Spline();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gondola = new Gondola(spline);
    // camera = new Camera2D();
    gpuProgram = new GPUProgram(vertSource, fragSource);

    // spline->AddControlPoint(vec3(-8.33333f, 7.16667f, 0));
    // spline->AddControlPoint(vec3(-4.56667f, -6.76667f, 0));
    // spline->AddControlPoint(vec3(3.6f, -6.7f, 0));
    // spline->AddControlPoint(vec3(1.46667f, -0.366666f, 0));
    // spline->AddControlPoint(vec3(-2.73333f, -2.53333f, 0));
    // spline->AddControlPoint(vec3(-0.433334f, -5.93333f, 0));
    // spline->AddControlPoint(vec3(5.03333f, -4.66667f, 0));
    // spline->AddControlPoint(vec3(8.56667f, 8.16667f, 0));
  }

  // Pontok hozzáadása
  void onMousePressed(MouseButton but, int pX, int pY) override {
    if (but == MOUSE_LEFT) {
      vec3 world = camera.screenToWorld(vec2(pX, pY));
      spline->AddControlPoint(vec3(world.x, world.y, 0));
      refreshScreen();
    }
  }

  void onKeyboard(int key) {
    if (key == ' ') {
      if (gondola)
        gondola->Start();
      refreshScreen();
    }
  }

  void onTimeElapsed(float startTime, float endTime) override {
    static float tend = 0;
    const float dt = 0.01f; // fix kis időlépés
    float tstart = tend;
    tend = endTime;

    for (float t = tstart; t < tend; t += dt) {
      float Dt = fmin(dt, tend - t);
      if (gondola && gondola->getState() == ROLLING) {
        gondola->Animate(Dt);
      }
    }

    refreshScreen();
  }

  void onDisplay() {
    glClearColor(0.0f, 0.0f, 0.0f, 1);
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, winWidth, winHeight);
    glPointSize(10.0f);
    glLineWidth(3.0f);

    mat4 viewMatrix = camera.MVP();

    spline->drawSpline(gpuProgram, viewMatrix);
    if (gondola)
      gondola->Draw(gpuProgram, viewMatrix);
  }
};

Main app;
