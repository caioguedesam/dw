#include "./generated/build_includes.hpp"

App gApp;

DW_MAIN()
{
    BEGIN_MAIN;

    initApp(800, 600, "DW App", &gApp);

    while(gApp.mRunning)
    {
        poll(&gApp);
        // update and render
    }

    destroyApp(&gApp);

    END_MAIN;
}

