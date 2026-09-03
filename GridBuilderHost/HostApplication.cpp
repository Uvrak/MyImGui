#include "HostApplication.h"
#include "DosBoxFramePipeline.h"
#include "HostRenderer.h"
#include "FrameTexture.h"

#include <cstdint>

#include <SDL3/SDL.h>

namespace GridBuilderHost
{
    HostApplication::HostApplication()
    {}

    HostApplication::~HostApplication()
    {}

    void HostApplication::run(
        DosBoxFramePipeline& dosBoxFramePipeline,
        HostRenderer& hostRenderer,
        DosBoxX::FrameTexture& frameTexture
    )
    {
        bool running =
            true;

        while (running)
        {
            SDL_Event event;

            while (SDL_PollEvent(
                &event
            ))
            {
                if (event.type ==
                    SDL_EVENT_QUIT)
                {
                    running =
                        false;
                }
                if (event.type ==
                    SDL_EVENT_WINDOW_RESIZED)
                {
                    hostRenderer.resize(
                        static_cast<uint32_t>(
                            event.window.data1
                            ),
                        static_cast<uint32_t>(
                            event.window.data2
                            )
                    );
                }
            }

            dosBoxFramePipeline.update();

            hostRenderer.render(
                frameTexture,
                dosBoxFramePipeline.contentWidth(),
                dosBoxFramePipeline.contentHeight()
            );
        }
    }
}