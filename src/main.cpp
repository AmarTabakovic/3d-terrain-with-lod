#include <iostream>

#include "application.h"

int main(int argc, char** argv)
{
    if (Application::setup() != 0)
        return 1;

    if (Application::parseArguments(argc, argv) != 0)
        return 1;

    return Application::run();
}
