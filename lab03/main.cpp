#include "./include/framework.h"
#include "include/lodepng.h"
#include <iostream>
#include <vector>

// Texture::Texture(int width, int height, std::vector<vec3> &image) {
//     glGenTextures(1, &textureId);            // azonos�t� gener�l�sa
//     glBindTexture(GL_TEXTURE_2D, textureId); // ez az akt�v innent�l
//     glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB,
//     GL_FLOAT,
//                  &image[0]); // To GPU
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
//                     GL_NEAREST); // sampling
//     glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
//   }

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
    #version 330 core
    uniform sampler2D textureUnit;
    uniform bool fixPoints;
    uniform vec3 color;
    uniform float time;
    in vec2 texCoord;
    out vec4 outColor;
    void main() {
        vec2 coord = texCoord;
        if (fixPoints) {
            vec2 texSize = vec2(textureSize(textureUnit, 0));
            coord = floor(texCoord * texSize) / texSize;
        }
        vec4 texColor = texture(textureUnit, coord);

        float PI = 3.14159265358979323846;
        float axisTilt = 23.0 * PI / 180.0;

        float longitude = (coord.x - 0.5) * 2.0 * PI;
        float latitude = (0.5 - coord.y) * PI;

        float hourAngle = time * PI / 12.0;

        float sunLat = axisTilt;
        float sunLong = -hourAngle;

        vec3 pointNormal = vec3(
            cos(latitude) * cos(longitude),
            sin(latitude),
            cos(latitude) * sin(longitude)
        );

        vec3 sunDir = vec3(
            cos(sunLat) * cos(sunLong),
            sin(sunLat),
            cos(sunLat) * sin(sunLong)
        );

        float illumination = dot(pointNormal, sunDir);
        float shadowFactor = smoothstep(-0.05, 0.05, -illumination);

        vec3 finalColor = mix(texColor.rgb * 0.5, texColor.rgb, shadowFactor);

        if (texCoord.x > 0.0001 || texCoord.y > 0.0001)
            outColor = vec4(finalColor, texColor.a);
        else
            outColor = vec4(color, 1.0);
    }
)";

const int winWidth = 600, winHeight = 600;
const int mapWidth = 64, mapHeight = 64;
float timeOfDay = 0; // indul 0 óránál

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

    texture = new Texture(64, 64);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    texture->Bind(0);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 64, 64, GL_RGB, GL_FLOAT,
                    &image[0]);

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
    GPUProgram *program;
    Map *map;
    Geometry<vec3> *vert, *lines;
    std::vector<vec2> stations;  // lat, lon
    float timeOfDay = 0;

public:
    App() : glApp("Mercator Route Planner - BME") {}

    void onInitialization() {
        map = new Map();
        vert = new Geometry<vec3>;
        lines = new Geometry<vec3>;
        program = new GPUProgram(vertSource, fragSource);
        glClearColor(0, 0, 0, 1);
    }

    vec3 SpherePos(float lat, float lon) {
        return vec3(cos(lat) * cos(lon), sin(lat), cos(lat) * sin(lon));
    }

    vec3 MercatorToNDC(float lat, float lon) {
        float x = lon / M_PI;

        float latMin = -85.0f * M_PI / 180.0f;
        float latMax =  85.0f * M_PI / 180.0f;
        float yMin = log(tan(M_PI/4 + latMin/2));
        float yMax = log(tan(M_PI/4 + latMax/2));

        float y = log(tan(M_PI/4 + lat/2));
        y = 1.0f - 2.0f * (y - yMin) / (yMax - yMin);

        return vec3(x, y, 0);
    }

    void generateRoute(vec2 a, vec2 b) {
        vec3 p1 = SpherePos(a.x, a.y);
        vec3 p2 = SpherePos(b.x, b.y);

        float omega = acos(dot(p1, p2));
        float distance = omega * 40000.0f / (2.0f * M_PI);
        std::cout << "Distance: " << distance << " km" << std::endl;

        int segments = 100;
        for (int i = 0; i <= segments; i++) {
            float t = (float)i / segments;
            vec3 pi = (sin((1 - t) * omega) * p1 + sin(t * omega) * p2) / sin(omega);
            float lat = asin(pi.y);
            float lon = atan2(pi.z, pi.x);
            lines->Vtx().push_back(MercatorToNDC(lat, lon));
        }
    }

    void onMousePressed(MouseButton but, int pX, int pY) {
        float ndcX = 2.0f * (pX / (float)winWidth) - 1.0f;
        float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);


        float lon = ndcX * M_PI;

        float latMin = -85.0f * M_PI / 180.0f;
        float latMax =  85.0f * M_PI / 180.0f;
        float yMin = log(tan(M_PI/4 + latMin/2));
        float yMax = log(tan(M_PI/4 + latMax/2));

        float mercY = ((1.0f - ndcY) / 2.0f) * (yMax - yMin) + yMin;
        float lat = 2 * atan(exp(mercY)) - M_PI / 2;

        stations.push_back(vec2(lat, lon));
        vert->Vtx().push_back(MercatorToNDC(lat, lon));

        int n = stations.size();
        if (n >= 2) {
            generateRoute(stations[n-2], stations[n-1]);
        }

        std::cout << "" << lat << "f, " << lon << std::endl;
        vert->updateGPU();
        lines->updateGPU();
        refreshScreen();
    }

    void onKeyboard(int key) override {
        if (key == 'n') {
            timeOfDay += 1.0f;
            if (timeOfDay >= 24.0f)
                timeOfDay = 0.0f;
            std::cout << "Time: " << timeOfDay << "h\n";
            program->setUniform(timeOfDay, "time");
            refreshScreen();
        }
    }

    void onDisplay() {
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0, 0, winWidth, winHeight);
        glPointSize(10.0f);
        glLineWidth(3.0f);

        map->Draw(program);

        lines->Draw(program, GL_LINE_STRIP, vec3(1, 1, 0));   // Sárga vonalak
        vert->Draw(program, GL_POINTS, vec3(1, 0, 0));        // Piros pontok
        program->setUniform(timeOfDay, "time");
    }
};

App app;
