#include "./include/framework.h"
#include "include/lodepng.h"
#include <iostream>
#include <vector>

const char *vertSource = R"(
    #version 330
    layout(location = 0) in vec2 vertexXY;
    layout(location = 1) in vec2 vertexUV;
    out vec2 texCoord;
    void main() {
        texCoord = vertexUV;
        gl_Position = vec4(vertexXY, 0, 1);
    }
)";

const char *fragSource = R"(
    #version 330
    uniform sampler2D textureUnit;
    uniform bool fixPoints;  // Keep your existing uniform
    uniform vec3 color;      // This is already being set by Geometry::Draw
    in vec2 texCoord;
    out vec4 outColor;

    void main() {
        // Check if texCoord is valid (this assumes texCoord is (0,0) when drawing non-textured geometry)
        if (texCoord.x > 0.0001 || texCoord.y > 0.0001) {
            vec2 coord = texCoord;
            if (fixPoints) {
                vec2 texSize = textureSize(textureUnit, 0);
                coord = floor(texCoord * texSize) / texSize;
            }
            outColor = texture(textureUnit, coord);
        } else {
            // Use the color uniform for non-textured elements
            outColor = vec4(color, 1.0);
        }
    }
)";

const int winWidth = 600, winHeight = 600;
const int mapWidth = 64, mapHeight = 64;

class Map {
  unsigned int vao, vbo[2];
  Texture *texture = nullptr;
  std::vector<unsigned char> rleData = { // mao input
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

  vec3 colorTable[4] = {
      vec3(1, 1, 1), // feh
      vec3(0, 0, 1), // k
      vec3(0, 1, 0), // z
      vec3(0, 0, 0)  // fek
  };

  std::vector<vec3> decode() {
    std::vector<vec3> pixels;

    for (unsigned char byte : rleData) {
      int len = byte >> 2;
      int color = byte & 0x3;
      for (int i = 0; i <= len; ++i)
        pixels.push_back(colorTable[color]);
    }

    return pixels;
  }

public:
  Map() {
    std::vector<vec3> image = decode();
    texture = new Texture(64, 64, image);

    float vertexCoords[] = {-1, -1, 1, -1, 1, 1, -1, 1};

    float texCoords[] = {0, 0, 1, 0, 1, 1, 0, 1};

    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);

    glGenBuffers(2, vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertexCoords), vertexCoords,
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, nullptr);

    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texCoords), texCoords, GL_STATIC_DRAW);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, nullptr);
  }

  void Draw(GPUProgram *program) {
    program->setUniform(0, "textureUnit");
    texture->Bind(0);
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4);
  }

  ~Map() { delete texture; }
};

// --- //

class App : public glApp {
  Geometry<vec3> *vert;
  Geometry<vec3> *lines;
  GPUProgram *program;
  Map *map;

public:
  App() : glApp("cuccos") {}

  void onInitialization() {
    map = new Map();
    vert = new Geometry<vec3>;
    lines = new Geometry<vec3>;
    program = new GPUProgram(vertSource, fragSource);
    glClearColor(0, 0, 0, 1);
  }

  Geometry<vec3> line(vec3 A_ndc, vec3 B_ndc) {
    int segments = 100;

    // Extract original coordinates (assuming A_ndc and B_ndc are already in the expected format)
    float lon1 = A_ndc.x * 180;
    float lat1 = A_ndc.y * (180.0f / 2.0f); // Adjust based on how you scaled in onMousePressed
    float lon2 = B_ndc.x * 180;
    float lat2 = B_ndc.y * (180.0f / 2.0f);

    // Convert to radians
    float phi1 = lat1 * M_PI / 180.0f, lambda1 = lon1 * M_PI / 180.0f;
    float phi2 = lat2 * M_PI / 180.0f, lambda2 = lon2 * M_PI / 180.0f;

    // Convert to 3D sphere coordinates
    vec3 p1 = vec3(cos(phi1) * cos(lambda1), sin(phi1), cos(phi1) * sin(lambda1));
    vec3 p2 = vec3(cos(phi2) * cos(lambda2), sin(phi2), cos(phi2) * sin(lambda2));

    Geometry<vec3> result;

    // Start with first point exactly as provided
    result.Vtx().push_back(A_ndc);

    // Interpolate interior points
    for (int i = 1; i < segments; ++i) {
      float t = i / (float)segments;

      float omega = acos(dot(p1, p2));
      // Avoid division by zero if points are very close
      if (omega < 1e-6) {
        vec3 pi = p1 * (1-t) + p2 * t; // Linear interpolation for close points
        pi = normalize(pi); // Ensure point is on sphere
      } else {
        vec3 pi = (sin((1 - t) * omega) * p1 + sin(t * omega) * p2) / sin(omega);

        // Convert back to Mercator projection
        float lat = asin(pi.y);
        float lon = atan2(pi.z, pi.x);

        // Convert to NDC using the same scale as in onMousePressed
        float x = lon / M_PI;
        float y = lat * 2.0f / M_PI;

        result.Vtx().push_back(vec3(x, y, 0));
      }
    }

    // End with last point exactly as provided
    result.Vtx().push_back(B_ndc);

    return result;
  }

  void onMousePressed(MouseButton but, int pX, int pY) {
    float ndcX = 2.0f * (pX / (float)winWidth) - 1.0f;
    float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

    // Make sure this scales correctly to match what's expected in the line function
    vec3 newPoint(ndcX, ndcY, 0);
    vert->Vtx().push_back(newPoint);

    if (vert->Vtx().size() >= 2) {
      vec3 a = vert->Vtx()[vert->Vtx().size() - 2];
      vec3 b = vert->Vtx()[vert->Vtx().size() - 1];

      Geometry<vec3> arc = line(a, b);
      for (vec3 v : arc.Vtx())
        lines->Vtx().push_back(v);
    }

    vert->updateGPU();
    lines->updateGPU();
    refreshScreen();
  }

  void onDisplay() {
    glClear(GL_COLOR_BUFFER_BIT);
    glViewport(0, 0, winWidth, winHeight);
    glPointSize(10.0f);
    glLineWidth(3.0f);

    map->Draw(program);

    lines->Draw(program, GL_LINE_STRIP, vec3(1.0f, 1.0f, 0.0f));
    vert->Draw(program, GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
  }
};

App app;
