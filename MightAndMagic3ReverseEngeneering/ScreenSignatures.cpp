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
                        330,
                        80,
                        255,
                        255,
                        255
                    },
                    {
                        148,
                        234,
                        190,
                        117,
                        69
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
                { 490, 209, 52, 52, 52 },
                { 253,   5, 85, 85, 85 },
                {  11, 347, 235, 235, 235 }
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
}