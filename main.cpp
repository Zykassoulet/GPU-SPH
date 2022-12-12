#include <iostream>

#include "App.h"
#include "PhysicsEngine.h"


std::shared_ptr<App> createSPHApp() {
    std::shared_ptr<App> app_ptr = std::make_shared<App>();
    std::shared_ptr<PhysicsEngine> physics_engine_ptr = std::make_shared<PhysicsEngine>(app_ptr);
    app_ptr->setPhysicsEngine(physics_engine_ptr);

    return app_ptr;
}

int main() {
    std::shared_ptr<App> sph_renderer = createSPHApp();
    sph_renderer->mainLoop();
    return 0;
}
