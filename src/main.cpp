#include <iostream>

#include "application.h"

/**
 * @brief main
 *
 * TODO: Arg parsing for terrain modes, heightmap file, texture file(s), etc.
 *
 * @return
 */
int main(int argc, char** argv)
{
    if (Application::setup() != 0)
        return 1;

    if (Application::parseArguments(argc, argv) != 0)
        return 1;

    return Application::run();
}
