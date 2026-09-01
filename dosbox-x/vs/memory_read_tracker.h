#pragma once

#include "mem.h"

#include <vector>
#include <utility>
#include <array>
#include <cstdint>

namespace MemoryReadTracker
{
    struct RegisterSnapshot
    {
        uint16_t ax = 0;
        uint16_t bx = 0;
        uint16_t cx = 0;
        uint16_t dx = 0;

        uint16_t si = 0;
        uint16_t di = 0;
        uint16_t bp = 0;
        uint16_t sp = 0;

        uint16_t ds = 0;
        uint16_t es = 0;
        uint16_t ss = 0;
    };

    struct RuntimeInstruction
    {
        LinearPt address = 0;

        uint16_t cs = 0;
        uint16_t ip = 0;

        RegisterSnapshot registers;

        std::array<uint8_t, 16>
            bytes{};

        std::array<uint8_t, 32>
            stackBytes{};
    };

    void record(
        LinearPt address
    );

    void record(
        LinearPt address,
        LinearPt instructionAddress
    );

    void setTransitionTarget(
        LinearPt address
    );

    LinearPt transitionTarget();

    void start();
    void stop();
    void clear();

    bool active();

    void startTransitionTracking();

    void stopTransitionTracking();

    bool transitionTrackingIsActive();

    void clearTransitionTracking();

    void recordTransitionInstruction(
        LinearPt instructionAddress,
        uint16_t cs,
        uint16_t ip,
        const RegisterSnapshot& registers
    );

    std::vector<LinearPt>
        addresses();

    std::vector<std::pair<LinearPt, LinearPt>>
        instructions();

    void setCurrentInstructionContext(
        LinearPt instructionAddress,
        uint16_t cs,
        uint16_t ip,
        const RegisterSnapshot& registers,
        LinearPt stackAddress
    );

    void recordInstruction(
        LinearPt instructionAddress,
        uint16_t cs,
        uint16_t ip,
        const RegisterSnapshot& registers,
        const std::array<uint8_t, 16>& instructionBytes,
        const std::array<uint8_t, 32>& stackBytes
    );

    std::vector<std::pair<LinearPt, LinearPt>>
        instructionTransitions();

    std::vector<std::pair<uint16_t, uint16_t>>
        instructionTransitionContexts();

    std::vector<std::array<uint8_t, 16>>
        instructionTransitionBytes();

    std::vector<std::vector<RuntimeInstruction>>
        instructionTransitionHistories();

    std::vector<RuntimeInstruction>
        instructionTransitionNextInstructions();

    void setReadTraceTarget(
        LinearPt address
    );

    LinearPt readTraceTarget();

    bool readTraceActive();

    bool readTraceArmed();

    std::vector<RuntimeInstruction>
        readTrace();

    void setExecutionCaptureTarget(
        LinearPt address
    );

    LinearPt executionCaptureTarget();

    void clearExecutionCapture();

    bool executionCaptureHit();

    RuntimeInstruction executionCapture();

    void setMemoryWriteWatchTarget(
        LinearPt address
    );

    LinearPt memoryWriteWatchTarget();

    RuntimeInstruction
        memoryWriteWatchCapture();

    uint8_t memoryWriteWatchValue();

    void clearMemoryWriteWatch();

    bool memoryWriteWatchHit();

    void recordMemoryWrite(
        LinearPt address,
        uint8_t value
    );
}
