#include "ui/App2.h"
#include <iostream>

int main() {
    std::cout << "Inditas: Projekt 2 - Mandelbrot & Snowflake\n";
    App app;
    if (!app.init()) {
        std::cerr << "Inicializalasi hiba!\n";
        return -1;
    }
    app.run();
    return 0;
}
