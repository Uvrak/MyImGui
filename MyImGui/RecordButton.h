#pragma once

namespace MyImGui
{
    class RecordButton
    {
    public:
        bool draw(
            const char* recordingLabel = "Stop"
        );

        bool recording() const;

        void start();
        void stop();

    private:
        bool m_recording = false;
    };
}