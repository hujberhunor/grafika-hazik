#include "./framework.h"
#include <glm/detail/qualifier.hpp>
#include <glm/ext/vector_float2.hpp>

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

class VertexApp : public glApp{
    Geometry<vec2> *vertex1; // VERTECIES
    GPUProgram* gpuProgram;

public:
    VertexApp() : glApp("VertexApp-lab01") {}

    void onInitialization(){
        vertex1 = new Geometry<vec2>;

        vertex1->Vtx() = {vec2(0.0f, 0.0f)};

        vertex1 -> updateGPU(); // GPU-ra feltöltés

        gpuProgram = new GPUProgram(vertSource, fragSource);
    }

    void onMousePressed(MouseButton but, int pX, int pY) {
        if (but == MOUSE_LEFT) {

            // Windows koordináta NDC-be konvertálása
            float ndcX = 2.0f * (pX / (float) winWidth) - 1.0f;
            float ndcY = 1.0f - 2.0f * (pY / (float)winHeight);

            printf("Egérkattintás: (%.2f, %.2f) \n", ndcX, ndcY);

            vertex1->Vtx().push_back(vec2(ndcX, ndcY));
            vertex1->updateGPU(); // GPU frissítés
            refreshScreen(); // Azonnali újrarajzolás
        }
    }

    void onDisplay(){
        glClearColor(0.5f, 0.5, 0.5, 0);
        glClear(GL_COLOR_BUFFER_BIT);
        glViewport(0,0, winWidth, winHeight);
        glPointSize(10.0f);

        vertex1 -> Draw(gpuProgram, GL_POINTS, vec3(1.0f, 0.0f, 0.0f));
    }
};

VertexApp app;
