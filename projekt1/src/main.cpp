#include "ui/App.h"
#include <iostream>

int main() {
    App app;
    if (!app.init(1400, 900)) {
        std::cerr << "Application initialization failed!\n";
        return 1;
    }
    app.run();
    return 0;
}
