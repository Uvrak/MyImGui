#pragma once

#include <string>

namespace DosBoxX
{
    class Controller;
    class NamedPipeClient;
}

namespace MightAndMagic3
{
    class MM3Launcher
    {
    public:
        MM3Launcher(
            DosBoxX::Controller& controller,
            DosBoxX::NamedPipeClient& namedPipeClient
        );

        void start();
        void update();

        bool isRunning() const;
        bool isFinished() const;

    private:
        enum class State
        {
            Idle,
            Waiting,
            Mount,
            ChangeDrive,
            StartGame,
            Done
        };

        DosBoxX::Controller& m_controller;
        DosBoxX::NamedPipeClient& m_namedPipeClient;

        State m_state =
            State::Idle;

        unsigned long long m_nextStep = 0;

        std::string m_gamePath =
            "C:\\GOG Galaxy\\Games\\Might and Magic 3";
    };
}