#include "Controller.h"
#include "NamedPipeClient.h"
#include "FrameReader.h"

#include "HostWindow.h"
#include "HostApplication.h"
#include "HostWindowState.h"
#include "HostRenderer.h"
#include "FrameTexture.h"
#include "DosBoxFramePipeline.h"
#include "Process.h"

int main()
{
    GridBuilderHost::HostWindowState hostWindowState;
    GridBuilderHost::HostWindow hostWindow;
    GridBuilderHost::HostRenderer hostRenderer;
    GridBuilderHost::HostApplication hostApplication;

    DosBoxX::Controller dosBoxController;
    DosBoxX::Process dosBoxProcess;

    DosBoxX::NamedPipeClient dosBoxPipeClient(
        R"(\\.\pipe\GridBuilderDOSBox)"
    );

    DosBoxX::FrameReader dosBoxFrameReader;

    if (!hostWindow.initialize(
        hostWindowState
    ))
    {
        return 1;
    }

    if (!hostRenderer.initialize(
        hostWindow.nativeHandle()
    ))
    {
        return 1;
    }

    DosBoxX::FrameTexture dosBoxFrameTexture(
        hostRenderer.device(),
        hostRenderer.context()
    );

    GridBuilderHost::DosBoxFramePipeline dosBoxFramePipeline(
        dosBoxFrameReader,
        dosBoxFrameTexture
    );

    if (!dosBoxProcess.start(
        L"C:\\Projects\\MyImGui\\dosbox-x\\bin\\x64\\Debug SDL2\\dosbox-x.exe"
    ))
    {
        return 1;
    }

    hostApplication.run(
        dosBoxFramePipeline,
        hostRenderer,
        dosBoxFrameTexture
    );

    hostWindow.saveState(
        hostWindowState
    );

    return 0;
}