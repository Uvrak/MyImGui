#include "ScreenSignatures.h"

namespace MightAndMagic3
{
    const ScreenSignature&
        ScreenSignatures::mainMenu()
    {
        static const ScreenSignature
            signature{
                {
                    {
                        387,
                        191,
                        162,
                        73,
                        0
                    },
                    {
                        287,
                        179,
                        255,
                        199,
                        166
                    },
                    {
                        386,
                        274,
                        255,
                        255,
                        255
                    }
                }
        };

        return signature;
    }

    const ScreenSignature&
        ScreenSignatures::loadGame()
    {
        static const ScreenSignature
            signature{
                {
                    {
                        286,
                        250,
                        203,
                        0,
                        0
                    },
                    {
                        176,
                        34,
                        235,
                        235,
                        235
                    },
                    {
                        246,
                        38,
                        255,
                        255,
                        255
                    }
                }
        };

        return signature;
    }

    const ScreenSignature& ScreenSignatures::mainGame()
    {
        static const ScreenSignature signature 
        {
            {
                // Normal action buttons: middle left, middle center, lower right.
                { 490, 214, 52, 52, 52 },
                { 546, 214, 52, 52, 52 },
                { 600, 254, 170, 170, 170 }
            }
        };

        return signature;
    }
    const ScreenSignature&
        ScreenSignatures::characterScreen()
    {
        static const ScreenSignature
            signature{
                {
                    {
                        581,
                        29,
                        178,
                        178,
                        178
                    },
                    {
                        573,
                        135,
                        190,
                        117,
                        69
                    },
                    {
                        603,
                        237,
                        190,
                        190,
                        190
                    }
                }
        };

        return signature;
    }
    const ScreenSignature&
        ScreenSignatures::inventory()
    {
        static const ScreenSignature
            signature{
                {
                    {
                        58,
                        264,
                        255,
                        255,
                        255
                    },
                    {
                        340,
                        261,
                        182,
                        239,
                        239
                    },
                    {
                        590,
                        265,
                        255,
                        255,
                        255
                    }
                }
        };

        return signature;
    }

    const ScreenSignature& ScreenSignatures::element()
    {
        static const ScreenSignature signature{
            {
                // Fixed fire, electricity and cold icons in the element dialog.
                { 144, 200, 255, 207, 178 },
                { 204, 200, 134, 60, 0 },
                { 264, 200, 255, 158, 97 }
            }
        };
        return signature;
    }

    const ScreenSignature& ScreenSignatures::castSpellList()
    {
        static const ScreenSignature signature{
            {
                // Fixed "Sprueche fuer" heading and Cast icon; dialog chrome
                // is shared with Load Game and cannot identify this view.
                { 126, 34, 255, 255, 255 },
                { 218, 40, 255, 255, 255 },
                { 490, 234, 255, 199, 166 }
            }
        };

        return signature;
    }

    const ScreenSignature& ScreenSignatures::cast()
    {
        static const ScreenSignature signature{
            {
                // Fixed icons in the Cast, New and Escape buttons.
                { 490, 234, 255, 199, 166 },
                { 546, 234, 134, 60, 0 },
                { 600, 234, 134, 60, 0 }
            }
        };

        return signature;
    }

    const ScreenSignature& ScreenSignatures::yesNo()
    {
        static const ScreenSignature signature{
            {
                { 480, 174, 52, 52, 52 },
                { 572, 223, 93, 97, 255 },
                { 516, 263, 207, 207, 255 }
            }
        };

        return signature;
    }
}
