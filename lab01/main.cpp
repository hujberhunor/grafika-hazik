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
    Geometry<vec2> *lines2;
    GPUProgram* gpuProgram;

public:
    VertexApp() : glApp("VertexApp-lab01") {}
    int selectedLine1 = -1;

    void onInitialization(){
        vertices = new Geometry<vec2>;
        line = new Geometry<vec2>;
        lines2 = new Geometry<vec2>;
        gpuProgram = new GPUProgram(vertSource, fragSource);
    }

    void onKeyboard(int key) override{
        keyState = key;
        selectedLine1 = -1;
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
            // if(line->Vtx()[line->Vtx().size()-1].x == p1.x && line->Vtx()[line->Vtx().size()-1].y == p1.y) return;
            line->Vtx().push_back(p1);

            // irányvektorral való nyújtás
            if(line->Vtx().size() % 2 == 0){
                // v = vec2(v.x * 10000, v.y * 10000);
                // pl1 = pl1 + v;
                // pl2 = pl2 - v ;
                vec2 pl1 = line->Vtx()[line->Vtx().size()-1]; // az utolsó pont amit leraktam
                vec2 pl2 = line->Vtx()[line->Vtx().size()-2]; // Utolsó előtti pont
                vec2 v = pl2-pl1; // irányvektor
                line->Vtx()[line->Vtx().size()-1] = computeIntersection(pl1, pl2, vec2(-1, 1), vec2(1,1));
                line->Vtx()[line->Vtx().size()-2] = computeIntersection(pl1, pl2, vec2(-1,-1), vec2(1,-1));

                // képernyp szélével való metszések
                if(line->Vtx()[line->Vtx().size()-1].x == 10.0f && line->Vtx()[line->Vtx().size()-1].y == 10.0f){
                    line->Vtx()[line->Vtx().size()-1] = computeIntersection(pl1, pl2, vec2(1,-1), vec2(1,1));
                    line->Vtx()[line->Vtx().size()-2] = computeIntersection(pl1, pl2, vec2(-1,-1), vec2(-1,1));
                }
            }
            // line->Vtx()[line->Vtx().size()-1] = pl1;
            // line->Vtx()[line->Vtx().size()-2] = pl2;
            line->updateGPU();
    }


    // katthoz legközelebi indexet adja vissza
    //
    // KÉT EGYENESRE KÜLSÉSENÉL NEM JÓ
    int closestLine(float pX, float pY) {
            float ndcX = pX;
            float ndcY = pY;

            vec2 p1n(ndcX, ndcY); // egér normalizáslt

            float minDist = 2.0f; // Maximális távolság (mivel az NDC [-1,1] között van)
            int closestIndex = 0; // Az egyenes indexe a tömbben


            for (int i = 0; i < line->Vtx().size() - 1; i += 2) {

                vec2 p1 = line->Vtx()[i];
                vec2 p2 = line->Vtx()[i + 1];
                vec2 ab = p2 - p1;   // Az egyenes irányvektora
                vec2 v(-ab.y, ab.x); // Normálvektor az egyenesre

                vec2 seged = computeIntersection(p1 , p1 + ab , p1n, p1n + v); //

                float dist = sqrt(pow(p1n.x - seged.x, 2) + pow(p1n.y - seged.y, 2));

                if (dist < minDist){
                    minDist = dist;
                    closestIndex = i;
                }
            }
            return closestIndex;
        }

        // Nem működött a cuvv


        void moveLine(float pX, float pY, int selectedLine) {
            float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
            float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

            if (selectedLine == -1) return;

            // Az egyenes két végpontja
            // vec2 p1 = line->Vtx()[selectedLine];
            // vec2 p2 = line->Vtx()[selectedLine + 1];

            // Az eltolás kiszámítása
            // static vec2 lastMousePos = vec2(ndcX, ndcY);
            // vec2 offset = vec2(ndcX, ndcY) - lastMousePos;

            // // Mindkét végpontot ugyanazzal az offsettel mozdítjuk el
            // line->Vtx()[selectedLine] = p1 + offset;
            // line->Vtx()[selectedLine + 1] = p2 + offset;

            // Új végpontok frissítése a képernyő széleinek metszéspontjával
            vec2 pl1 = line->Vtx()[selectedLine];     // Frissített kezdőpont
            vec2 pl2 = line->Vtx()[selectedLine + 1]; // Frissített végpont
            vec2 v = pl2 - pl1; // Az egyenes irányvektora

            // Metszéspont számítás a képernyő széleivel
            vec2 mousePos(ndcX, ndcY);
            line->Vtx()[selectedLine] = computeIntersection(mousePos, mousePos+ v, vec2(-1, 1), vec2(1, 1)); // katt
            line->Vtx()[selectedLine + 1] = computeIntersection(mousePos , mousePos + v, vec2(-1, -1), vec2(1, -1));

            // Ha az egyenes nem metszi a felső vagy alsó élt, akkor az oldalakkal metszük
            if (line->Vtx()[selectedLine].x == 10.0f && line->Vtx()[selectedLine].y == 10.0f) {
                line->Vtx()[selectedLine] = computeIntersection(pl1, pl2, vec2(1, -1), vec2(1, 1));
                line->Vtx()[selectedLine + 1] = computeIntersection(pl1, pl2, vec2(-1, -1), vec2(-1, 1));
            }

            // Az új egérpozíciót elmentjük
            // lastMousePos = vec2(ndcX, ndcY);

            // GPU frissítés és újrarajzolás
            line->updateGPU();
            refreshScreen();
        }


        // 2 egyenes metszése
        vec2 computeIntersection(vec2 A1, vec2 A2, vec2 B1, vec2 B2) {
            // Egyenesek irányvektorai
            vec2 dA = A2 - A1;
            vec2 dB = B2 - B1;

            // Determináns kiszámítása
            float det = dA.x * dB.y - dA.y * dB.x;

            // Ha a determináns közel nulla, az egyenesek párhuzamosak
            if (fabs(det) == 0) {
                return vec2(10.0f, 10.0f);  // Nem érvényes metszéspont
            }

            // Az egyenesek paraméteres egyenletrendszerének megoldása
            float t = ((B1.x - A1.x) * dB.y - (B1.y - A1.y) * dB.x) / det;

            // Metszéspont kiszámítása
            vec2 intersection = A1 + t * dA;

            return intersection;
        }

        // egymás után 2 linek kiválasztása
        void pickedLines(int pX, int pY) {
            // Normalizált koordináták számítása
            float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
            float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

            int closestLineLocal = closestLine(ndcX, ndcY);

            if (closestLineLocal == -1) {
                printf("Nincs elég egyenes a metszéshez!\n");
                return;
            }

            if (selectedLine1 != -1) {
                // Második egyenes kiválasztása
                int selectedLine2 = closestLineLocal;

                // Ha a két kiválasztott egyenes azonos, akkor nem csinálunk semmit
                if (selectedLine1 == selectedLine2) {
                    printf("Ugyanazt az egyenest választottad!\n");
                    // selectedLine1 = -1;
                    return;
                }

                // Kiválasztott egyenesek végpontjainak lekérése
                vec2 A1 = line->Vtx()[selectedLine1];
                vec2 A2 = line->Vtx()[selectedLine1 + 1];
                vec2 B1 = line->Vtx()[selectedLine2];
                vec2 B2 = line->Vtx()[selectedLine2 + 1];

                // Metszéspont számítása
                vec2 intersection = computeIntersection(A1, A2, B1, B2);

                // Kiválasztás visszaállítása
                selectedLine1 = -1;
            } else {
                // Első egyenes kiválasztása
                selectedLine1 = closestLineLocal;
            }
        }


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
                else if (keyState == 'm') {
                    selectedLine1 = closestLine(ndcX, ndcY);
                }
                else if (keyState == 'i') { // Metszéspont keresés
                    pickedLines(pX, pY);

               }
                refreshScreen();
            }
        }

        void onMouseReleased(MouseButton but, int pX, int pY) override {
            if (but == MOUSE_LEFT) {
                if (keyState == 'm') {
                    selectedLine1 = -1;
                }
            }
        }

        void onMouseMotion(int pX, int pY) override {
            float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
            float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

            if (keyState == 'm' && selectedLine1 != -1) {
                moveLine(pX, pY, selectedLine1);
            }
        }

    void onDisplay(){
        glClearColor(0.5f, 0.5, 0.5, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0,0, winWidth, winHeight);
        glPointSize(10.0f);
        glLineWidth(3.0f);

        line -> Draw(gpuProgram, GL_LINES, vec3(0.0f, 1.0f, 1.0f));
        lines2 -> Draw(gpuProgram, GL_LINES, vec3(0.0f, 1.0f, 0.0f));
        vertices -> Draw(gpuProgram, GL_POINTS, vec3(1.0f, 0.0f, 0.0f));

    }
};

VertexApp app;
