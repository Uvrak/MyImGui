#include "EditorApplication.h"

#include <iostream>
#include <stdexcept>

int main(int argc, char* argv[])
{
    try
    {
        EditorApplication editor;
        editor.run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}