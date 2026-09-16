#include "MM3Launcher.h"

#include <Windows.h>

#include "Controller.h"
#include "NamedPipeClient.h"

namespace MightAndMagic3
{
    MM3Launcher::MM3Launcher(
        DosBoxX::Controller& controller,
        DosBoxX::NamedPipeClient& namedPipeClient
    )
        :
        m_controller(
            controller
        ),
        m_namedPipeClient(
            namedPipeClient
        )
    {}

    void MM3Launcher::start()
    {
        m_state =
            State::Waiting;

        m_nextStep =
            GetTickCount64() + 2000;
    }

    void MM3Launcher::update()
    {
        if (m_state == State::Idle ||
            m_state == State::Done)
        {
            return;
        }

        const ULONGLONG now =
            GetTickCount64();

        if (now < m_nextStep)
        {
            return;
        }

        switch (m_state)
        {
        case State::Waiting:
            m_controller.setKeyboardLayout(
                m_namedPipeClient,
                DosBoxX::KeyboardLayout::German
            );

            m_state =
                State::Mount;

            m_nextStep =
                now + 2000;

            break;

        case State::Mount:
        {
            const std::string command =
                "MOUNT C \"" +
                m_gamePath +
                "\"";

            m_namedPipeClient.send("TYPE_TEXT:" + command + "\r");

            m_state =
                State::WaitForMount;

            m_nextStep =
                now + 500;

            break;
        }

        case State::WaitForMount:
        {
            std::string driveStatus;
            if(m_namedPipeClient.request("MM3_DRIVE_STATUS", driveStatus) &&
                (driveStatus == "M" || driveStatus == "C"))
            {
                m_state = State::ChangeDrive;
                m_nextStep = now + 500;
            }
            else
            {
                m_nextStep = now + 500;
            }
            break;
        }

        case State::ChangeDrive:
            m_namedPipeClient.send("TYPE_TEXT:C:\r");

            m_state =
                State::WaitForDrive;

            m_nextStep =
                now + 1000;

            break;

        case State::WaitForDrive:
        {
            std::string driveStatus;
            if(m_namedPipeClient.request("MM3_DRIVE_STATUS", driveStatus) &&
                driveStatus == "C")
            {
                m_state = State::StartGame;
                m_nextStep = now + 500;
            }
            else
            {
                m_state = State::ChangeDrive;
                m_nextStep = now + 500;
            }

            break;
        }

        case State::StartGame:
            m_namedPipeClient.send("TYPE_TEXT:MM3\r");

            m_state =
                State::Done;

            break;

        default:
            break;
        }
    }

    bool MM3Launcher::isRunning() const
    {
        return
            m_state != State::Idle &&
            m_state != State::Done;
    }

    bool MM3Launcher::isFinished() const
    {
        return
            m_state == State::Done;
    }
}
