//=============================================================================================
// Móriczka teszt: Egyszerű háttérszín beállítás
//=============================================================================================
#include "./framework.h"

const int winWidth = 600, winHeight = 600;

class MoriczkaApp : public glApp {
public:
    MoriczkaApp() : glApp("Móriczka teszt") {}

    // Inicializáció: háttérszín inicializálása
    void onInitialization() {
        printf("Móriczka elindult!\n");
    }

    // Ablak újrarajzolás: háttérszín beállítása
    void onDisplay() {
        glClearColor(1.0f, 0.5f, 0.0f, 1.0f); // Narancssárga háttér
        glClear(GL_COLOR_BUFFER_BIT);
    }

    // Billentyű lenyomás esetén kiírja a billentyű kódját
    void onKeyboard(int key) {
        printf("Megnyomtál egy billentyűt: %d\n", key);
    }
};

MoriczkaApp app;
