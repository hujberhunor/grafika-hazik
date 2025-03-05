#include "./framework.h"
#include <cmath>
#include <cstdio>
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>
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
char keyState = ' ';

class VertexApp : public glApp{
    Geometry<vec2> *vertices; // Pontok tárolása
    Geometry<vec2> *line;
    GPUProgram* gpuProgram;

public:
    VertexApp() : glApp("VertexApp-lab01") {}

    void onInitialization(){
        vertices = new Geometry<vec2>;
        line = new Geometry<vec2>;
        gpuProgram = new GPUProgram(vertSource, fragSource);
    }

    void onKeyboard(int key) override{
        keyState = key;
    }

    vec2 closestPoint(float pX, float pY) {
        // NORMALIZÁLÁS
        // float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
        // float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

        // float minDist = std::numeric_limits<float>::max(); // Kezdetben végtelen nagy távolság
        float minDist = 2.0; // mert 1.0 a max
        vec2 closestPoint;

        // Végigmegyünk az összes ponton, és megkeressük a legközelebbit
        for (const vec2& point : vertices->Vtx()) {
            float dist = sqrt(pow(point.x - pX, 2) + pow(point.y - pY, 2));
            if (dist < minDist) {
                minDist = dist;
                closestPoint = point;
            }
        }
        printf("Legközelebbi pont: (%.2f, %.2f), Távolság: %.4f\n", closestPoint.x, closestPoint.y, minDist);
        return closestPoint;
    }

    void onMousePressed(MouseButton but, int pX, int pY) {
        if (but == MOUSE_LEFT) {
            float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
            float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

            if (keyState == 'p') {
                printf("Egérkattintás: (%.2f, %.2f) \n", ndcX, ndcY);
                vertices->Vtx().push_back(vec2(ndcX, ndcY));
                vertices->updateGPU();
            }
            else if (keyState == 'l') {
                printf("Egyenes: ");
                vec2 p1 = closestPoint(ndcX, ndcY);
                line->Vtx().push_back(p1);

                vec2 pl1 = line->Vtx()[line->Vtx().size()-1];
                vec2 pl2 = line->Vtx()[line->Vtx().size()-2];
                vec2 v = pl2-pl1; // irányvektor

                if(line->Vtx().size() % 2 == 0){
                    v = vec2(v.x * 10000, v.y * 10000);
                    pl1 = pl1 + v;
                    pl2 = pl2 - v ;
                }

                line->Vtx()[line->Vtx().size()-1] = pl1;
                line->Vtx()[line->Vtx().size()-2] = pl2;
                line->updateGPU();
            }
            else if (keyState == 'm') {
                // Mozgatás
            }
            else if (keyState == 'i') {
                // Metszéspont számítás
            }

            refreshScreen();
        }
    }

    void onDisplay(){
        glClearColor(0.5f, 0.5, 0.5, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0,0, winWidth, winHeight);
        glPointSize(10.0f);
        glLineWidth(3.0f);

        vertices -> Draw(gpuProgram, GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
        line -> Draw(gpuProgram, GL_LINES, vec3(0.0f, 1.0f, 1.0f));
    }
};

VertexApp app;
