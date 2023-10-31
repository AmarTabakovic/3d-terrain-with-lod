#include <iostream>

#include "application.h"

int main()
{

    if (Application::setup() != 0)
        return 1;

    return Application::run();
}
