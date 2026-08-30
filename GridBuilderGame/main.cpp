#include "EditorApplication.h"

#include "MM3GameModule.h"
#include "StateReader.h"

#include <iostream>
#include <stdexcept>

int main()
{
    try
    {
        EditorApplication editor;

        MightAndMagic3::StateReader
            mm3StateReader(
                editor.memoryReader()
            );

        MM3GameModule
            mm3GameModule(
                mm3StateReader
            );

        editor.gameModuleManager().add(
            "Might and Magic III",
            &mm3GameModule
        );

        editor.run();
    }
    catch (const std::runtime_error& e)
    {
        std::cerr << e.what() << '\n';
        return 1;
    }

    return 0;
}