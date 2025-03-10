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

    // Closest point to the cursor
    vec2 closestPoint(float pX, float pY) { // MÁR NORMALIZÁLT KOORDINÁTÁKAT KAP!
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
        // printf("Legközelebbi pont: (%.2f, %.2f), Távolság: %.4f\n", closestPoint.x, closestPoint.y, minDist);
        return closestPoint;
    }

    // Vonal kirajzolása 2 pontból
    void drawLine(float ndcX, float ndcY){
        vec2 p1 = closestPoint(ndcX, ndcY);
            line->Vtx().push_back(p1);

            vec2 pl1 = line->Vtx()[line->Vtx().size()-1]; // az utolsó pont amit leraktam
            vec2 pl2 = line->Vtx()[line->Vtx().size()-2]; // Utolsó előtti pont
            vec2 v = pl2-pl1; // irányvektor

            // irányvektorral való nyújtás
            if(line->Vtx().size() % 2 == 0){
                v = vec2(v.x * 10000, v.y * 10000);
                pl1 = pl1 + v;
                pl2 = pl2 - v ;
            }

            line->Vtx()[line->Vtx().size()-1] = pl1;
            line->Vtx()[line->Vtx().size()-2] = pl2;
            line->updateGPU();
    }

    size_t closestLine(float pX, float pY) {
            if (line->Vtx().size() < 2) return -1; // Ha nincs elég egyenes, nincs mit keresni

            float minDist = 2.0f; // Maximális távolság (mivel az NDC [-1,1] között van)
            size_t closestIndex = -1; // Az egyenes indexe a tömbben

            for (size_t i = 0; i < line->Vtx().size() - 1; i += 2) {
                vec2 p1 = line->Vtx()[i];
                vec2 p2 = line->Vtx()[i + 1];

                vec2 ab = p2 - p1;   // Az egyenes irányvektora
                vec2 ac = vec2(pX, pY) - p1; // Az A pontból az egérhez mutató vektor

                // Vetítési arány kiszámítása: t = (AC · AB) / (AB · AB)
                float dotAC_AB = ac.x * ab.x + ac.y * ab.y;
                float dotAB_AB = ab.x * ab.x + ab.y * ab.y;
                float t = dotAC_AB / dotAB_AB;

                // Clampeljük t-t, hogy az egyenes szakaszra essen [0,1] tartományban
                if (t < 0.0f) t = 0.0f;
                if (t > 1.0f) t = 1.0f;

                // Kiszámítjuk a vetített pontot az egyenesen
                vec2 projectedPoint;
                projectedPoint.x = p1.x + t * (p2.x - p1.x);
                projectedPoint.y = p1.y + t * (p2.y - p1.y);

                // Távolság kiszámítása az egérkattintás és a legközelebbi pont között
                float dist = sqrt((projectedPoint.x - pX) * (projectedPoint.x - pX) +
                                  (projectedPoint.y - pY) * (projectedPoint.y - pY));

                if (dist < minDist) {
                    minDist = dist;
                    closestIndex = i;  // Az egyenes kezdőpontjának indexe
                }
            }
            // std::cout << "legközelebbi egyenes idexe a listában:" << closestIndex;
            return closestIndex; // Visszaadja a legközelebbi egyenes indexét
        }




    void onMousePressed(MouseButton but, int pX, int pY) {
        if (but == MOUSE_LEFT) {
            float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
            float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

            if (keyState == 'p') { // pontlerakás
                printf("Egérkattintás: (%.2f, %.2f) \n", ndcX, ndcY);
                vertices->Vtx().push_back(vec2(ndcX, ndcY));
                refreshScreen();
                vertices->updateGPU();
            }
            else if (keyState == 'l') { // egyenes rajzolás
                printf("Egyenes: ");
                drawLine(ndcX, ndcY);
            }
            else if (keyState == 'm') {  // Mozgatás
                // moveLine(ndcX, ndcY);

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

        line -> Draw(gpuProgram, GL_LINES, vec3(0.0f, 1.0f, 1.0f));
        vertices -> Draw(gpuProgram, GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
    }
};

VertexApp app;
