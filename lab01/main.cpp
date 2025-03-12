#include "./framework.h"
#include <cmath>
#include <cstddef>
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
    int selectedLine1 = -1;

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
                // v = vec2(v.x * 10000, v.y * 10000);
                // pl1 = pl1 + v;
                // pl2 = pl2 - v ;
                line->Vtx()[line->Vtx().size()-1] = computeIntersection(pl1, pl2, vec2(-1, 1), vec2(1,1));
                line->Vtx()[line->Vtx().size()-2] = computeIntersection(pl1, pl2, vec2(-1,-1), vec2(1,-1));
            }

            // line->Vtx()[line->Vtx().size()-1] = pl1;
            // line->Vtx()[line->Vtx().size()-2] = pl2;
            line->updateGPU();
    }

    int closestLine(float pX, float pY) {
            if (line->Vtx().size() < 2) return -1; // Ha nincs elég egyenes, nincs mit keresni

            float ndcX = pX;
            float ndcY = pY;

            vec2 p1n(ndcX, ndcY); // egér normalizáslt

            float minDist = 2.0f; // Maximális távolság (mivel az NDC [-1,1] között van)
            int closestIndex = -1; // Az egyenes indexe a tömbben

            for (int i = 0; i < line->Vtx().size() - 1; i += 2) {

                vec2 p1 = line->Vtx()[i];
                vec2 p2 = line->Vtx()[i + 1];
                vec2 ab = p2 - p1;   // Az egyenes irányvektora
                vec2 v(ab.y,-ab.x); // nomrálvektor

                vec2 seged = computeIntersection(p1 , p2, p1n, p1n + v); //

                float dist = sqrt(pow(p1n.x - seged.x, 2) + pow(p1n.y - seged.y, 2));

                if (dist < minDist){
                    minDist = dist;
                    closestIndex = i;
                }
            }
            return closestIndex;
        }

        // Nem működött a cuvv
        // void moveLine(float ndcX, float ndcY, int selectedLine) {
        //     // Az egyenes két végpontja
        //     vec2 p1 = line->Vtx()[selectedLine];
        //     vec2 p2 = line->Vtx()[selectedLine + 1];

        //     // Az eltolás kiszámítása az előző és az új egérpozíció alapján
        //     static vec2 lastMousePos = vec2(ndcX, ndcY);
        //     vec2 offset = vec2(ndcX, ndcY) - lastMousePos;

        //     // Mindkét végpontot egyformán eltoljuk
        //     line->Vtx()[selectedLine] += offset;
        //     line->Vtx()[selectedLine + 1] += offset;

        //     // Az új egérpozíciót eltároljuk a következő mozgásra
        //     lastMousePos = vec2(ndcX, ndcY);

        //     line->updateGPU();
        //     refreshScreen();
        // }



        vec2 computeIntersection(vec2 A1, vec2 A2, vec2 B1, vec2 B2) {
            vec2 nA = A2-A1;
            vec2 nB = B2-B1;

            nA = vec2(nA.y, -nA.x); // normél
            nB = vec2(nB.y, -nB.x); // normél

            float det = nA.x * nB.y - nA.y * nB.x;

            float Ac = nA.x * A1.x + nA.y * A1.y ;
            float Bc = nB.x * B1.x + nB.y * B1.y ;

           if(det == 0) {
               return  vec2(10.0, 10.0);
           }

            float x = (nB.y * Ac - nA.y * Bc) / det;
            float y = (nA.x * Bc - nB.x * Ac) / det;

            vec2 ret(x,y);
            vertices->Vtx().push_back(ret);
            vertices->updateGPU();
            return vec2(x, y);
        }


        void detectIntersect(size_t line1, size_t line2) {
            if (line->Vtx().size() < 4) return;

            vec2 A1 = line->Vtx()[line1];
            vec2 A2 = line->Vtx()[line1 + 1];
            vec2 B1 = line->Vtx()[line2];
            vec2 B2 = line->Vtx()[line2 + 1];

            vec2 intersection = computeIntersection(A1, A2, B1, B2);

            // Ha az egyenesek nem párhuzamosak, akkor lerakjuk a metszéspontot
            if (intersection.x != 0 || intersection.y != 0) {
                vertices->Vtx().push_back(intersection);
                vertices->updateGPU();
                refreshScreen();

                std::cout << "Metszéspont: (" << intersection.x << ", " << intersection.y << ")" << std::endl;
            }
        }


        void pickedLines(int pX, int pY) {
            float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
            float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);
            int closestLineLocal = -1;

            if( selectedLine1 != -1 ){
                closestLineLocal = closestLine(ndcX, ndcY);
                detectIntersect(selectedLine1, closestLineLocal);
                selectedLine1 = -1;
            }
            else {
                selectedLine1 = closestLine(ndcX, ndcY);
            }

            std::cout << selectedLine1;
        }

        // Az egyenesek kiválasztásának frissítése

        void onMousePressed(MouseButton but, int pX, int pY) {
            if (but == MOUSE_LEFT) {
                float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
                float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

                if (keyState == 'p') { // pontlerakás
                    vertices->Vtx().push_back(vec2(ndcX, ndcY));
                    vertices->updateGPU();
                    refreshScreen();
                }
                else if (keyState == 'l') { // egyenes rajzolás
                    drawLine(ndcX, ndcY);
                }
                else if (keyState == 'i') { // Metszéspont keresés
                    // MEGÖLÖM MAGAM!!
                    // pickedLines(ndcX, ndcY);
                    printf("%d \n", closestLine(ndcX, ndcY));
               }
                refreshScreen();
            }
        }





    void onMouseReleased(MouseButton but, int pX, int pY) override{
        selectedLine1 = -1; // Egyenes elengdsése
    }

    void onMouseMotion(int pX, int pY) override{
        float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
        float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

        if(selectedLine1 == -1) return;
        // if(keyState == 'm')      moveLine(ndcX, ndcY, selectedLine1);
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
