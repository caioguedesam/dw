#include "../generated/build_includes.hpp"

App gApp;

DW_MAIN()
{
    BEGIN_MAIN;

    initApp(800, 600, "DW App", &gApp);

    testCore(&gApp);
    testMath();

    while(gApp.mRunning)
    {
        poll(&gApp);
        //debugInput(&gApp);
        // update and render
    }

    destroyApp(&gApp);

    END_MAIN;
}

