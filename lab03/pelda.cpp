//=============================================================================================
// Textúra leképzés
//=============================================================================================
#include "framework.h"

// csúcspont árnyaló
const char *vertSource = R"(
	#version 330

	layout(location = 0) in vec2 vertexXY;	// Attrib Array 0
	layout(location = 1) in vec2 vertexUV;			// Attrib Array 1

	out vec2 texCoord;								// output attribute

	void main() {
		texCoord = vertexUV;														// copy texture coordinates
		gl_Position = vec4(vertexXY, 0, 1); 		
	}
)";

// pixel árnyaló
const char *fragSource = R"(
	#version 330

	uniform sampler2D textureUnit;

	in vec2 texCoord;			// variable input: interpolated texture coordinates
	out vec4 outColor;		// output that goes to the raster memory as told by glBindFragDataLocation

	void main() { outColor = texture(textureUnit, texCoord); }
)";

class TexturedQuad {
  unsigned int vao, vbo[2]; // vao és két vbo: geometria + uv
  std::vector<vec2> vtx;    // geometria a CPU-n
  Texture texture;          // kép a tapétázáshoz
  int picked = -1;          // kiválasztott csúcspont sorszáma
public:
#ifdef FILE_OPERATIONS
  TexturedQuad(const std::string &pathname) : texture(pathname) {
    glGenVertexArrays(1, &vao);
    glBindVertexArray(vao);
    glGenBuffers(2, &vbo[0]); // egyszerre két vbo-t kérünk
    // a négyszög csúcsai kezdetben normalizált eszközkoordinátákban
    vtx = {vec2(-1, -1), vec2(1, -1), vec2(1, 1), vec2(-1, 1)};
    updateGPU();                  // GPU-ra másoljuk
    glEnableVertexAttribArray(0); // a 0. vbo a 0. bemeneti regisztert táplálja
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0,
                          NULL); // csúcsonként 2 float-tal
    // a négyszög csúcsai textúratérben
    std::vector<vec2> uvs = {vec2(0, 1), vec2(1, 1), vec2(1, 0), vec2(0, 0)};
    glBindBuffer(GL_ARRAY_BUFFER, vbo[1]); // GPU-ra másoljuk
    glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(vec2), &uvs[0],
                 GL_STATIC_DRAW);
    glEnableVertexAttribArray(1); // a 1. vbo a 1. bemeneti regisztert táplálja
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0,
                          NULL); // csúcsonként 2 float-tal
  }
#endif
  void updateGPU() { // vtx tömb átmásolása a GPU-ra a vbo[0] VBO-ba
    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo[0]);
    glBufferData(GL_ARRAY_BUFFER, vtx.size() * sizeof(vec2), &vtx[0],
                 GL_DYNAMIC_DRAW);
  }
  void Draw(GPUProgram *gpuProgram) {
    int textureUnit = 0; // textúra mintavevő egység
    gpuProgram->setUniform(textureUnit, "textureUnit"); // türkisz nyíl
    texture.Bind(textureUnit);                          // piros nyíl
    glBindVertexArray(vao);
    glDrawArrays(GL_TRIANGLE_FAN, 0, 4); // négyszög rajzolás
  }
  void PickVertex(vec2 cP) { // csúcs kiválasztás
    for (int i = 0; i < 4; ++i) {
      if (length(cP - vtx[i]) < 0.1f)
        picked = i;
    }
  }
  void MoveVertex(vec2 cP) { // kiválasztott csúcs mozgatás
    if (picked >= 0) {
      vtx[picked] = cP;
      updateGPU();
    }
  }
  void UnPickVertex() { picked = -1; } // kiválasztás vége
  virtual ~TexturedQuad() {
    glDeleteBuffers(2, vbo); // VBO-k és VAO felszabadítása
    glDeleteVertexArrays(1, &vao);
  }
};

const int winWidth = 960, winHeight = 540;

class TextureApp : public glApp {
  GPUProgram *gpuProgram;
  TexturedQuad *quad;
  bool mousePressed = false;

public:
  TextureApp() : glApp(3, 3, winWidth, winHeight, "Texturing") {}
  void onInitialization() {
#ifdef FILE_OPERATIONS
    quad = new TexturedQuad("D:"
                            "\\3DProjects\\GrafikaTananyag\\glProgram\\images\\"
                            "texture.png"); // négyszög + textúra
#endif
    gpuProgram = new GPUProgram(vertSource, fragSource);
    glClearColor(0, 0, 0, 0); // háttér fekete
  }
  void onDisplay() {
    glClear(GL_COLOR_BUFFER_BIT); // törlés
    glViewport(0, 0, winWidth, winHeight);
    quad->Draw(gpuProgram); // rajzolás
  }
  void onMousePressed(MouseButton button, int pX, int pY) {
    quad->PickVertex(
        vec2(2.0f * pX / winWidth - 1.0f, 1.0f - 2.0f * pY / winHeight));
  }
  void onMouseReleased(MouseButton button, int pX, int pY) {
    quad->UnPickVertex();
  }
  void onMouseMotion(int pX, int pY) {
    quad->MoveVertex(
        vec2(2.0f * pX / winWidth - 1.0f, 1.0f - 2.0f * pY / winHeight));
    refreshScreen();
  }
};

TextureApp app;
