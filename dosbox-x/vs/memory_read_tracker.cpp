#include "memory_read_tracker.h"

#include <unordered_set>
#include <vector>
#include <algorithm>
#include <atomic>
#include <mutex>
#include <utility>
#include <functional>
#include <array>
#include <cstdint>


#include <cstdio>
#include <windows.h>

#include <deque>

namespace
{
    struct TransitionHistoryEntry
    {
        LinearPt address = 0;

        uint16_t cs = 0;
        uint16_t ip = 0;

        MemoryReadTracker::RegisterSnapshot
            registers;
    };

    constexpr size_t transitionHistoryCapacity = 16;

    std::array<
        TransitionHistoryEntry,
        transitionHistoryCapacity
    >
        transitionHistory{};

    size_t transitionHistorySize = 0;
    size_t transitionHistoryWriteIndex = 0;

    std::atomic<bool> trackingActive{ false };

    std::atomic<bool>
        transitionTrackingActive{ false };

    std::unordered_set<LinearPt>
        readAddresses;

    LinearPt previousInstructionAddress = 0;

    uint16_t previousCS = 0;
    uint16_t previousIP = 0;

    bool hasPreviousInstruction = false;

    LinearPt transitionTargetAddress = 0xEC2B;

    std::atomic<LinearPt>
        executionCaptureTargetAddress{ 0 };

    bool hasExecutionCapture = false;

    MemoryReadTracker::RuntimeInstruction
        capturedExecutionInstruction;

    struct MemoryWriteCapture
    {
        MemoryReadTracker::RuntimeInstruction
            instruction;

        uint8_t writeValue = 0;
    };

    std::deque<MemoryWriteCapture>
        capturedMemoryWriteInstructions;

    LinearPt
        currentStackAddress = 0;

    std::atomic<LinearPt>
        memoryWriteWatchTargetAddress{ 0 };

    bool
        hasMemoryWriteWatchHit = false;

    uint8_t
        capturedMemoryWriteValue = 0;

    MemoryReadTracker::RuntimeInstruction
        capturedMemoryWriteInstruction{};

    MemoryReadTracker::RuntimeInstruction
        currentRuntimeInstruction{};

    constexpr size_t instructionByteCount = 16;
    constexpr size_t instructionHistoryCount = 16;

    std::array<uint8_t, instructionByteCount>
        previousInstructionBytes{};

    std::deque<MemoryReadTracker::RuntimeInstruction>
        recentInstructions;

    LinearPt readTraceTargetAddress = 0;

    bool readTraceRunning = false;
    bool readTraceArmedState = false;

    constexpr size_t readTraceInstructionCount = 1000;

    std::deque<MemoryReadTracker::RuntimeInstruction>
        readTraceInstructions;

    struct InstructionTransition
    {
        LinearPt previousAddress = 0;
        LinearPt currentAddress = 0;

        uint16_t previousCS = 0;
        uint16_t previousIP = 0;

        std::array<uint8_t, instructionByteCount>
            previousBytes{};

        std::vector<MemoryReadTracker::RuntimeInstruction>
            history;

        MemoryReadTracker::RuntimeInstruction
            nextInstruction;

        bool hasNextInstruction = false;
    };

    std::vector<InstructionTransition>
        recordedInstructionTransitions;

    bool waitingForTransitionNextInstruction = false;

    size_t pendingTransitionIndex = 0;

    struct ReadInstructionHash
    {
        size_t operator()(
            const std::pair<LinearPt, LinearPt>& value
            ) const noexcept
        {
            const size_t h1 =
                std::hash<LinearPt>{}(
                    value.first
                    );

            const size_t h2 =
                std::hash<LinearPt>{}(
                    value.second
                    );

            return h1 ^
                (h2 << 1);
        }
    };

    std::unordered_set<
        std::pair<LinearPt, LinearPt>,
        ReadInstructionHash
    >
        readInstructions;
    
    std::mutex
        readAddressesMutex;
}

void MemoryReadTracker::record(
    LinearPt address
)
{
    const bool tracking =
        trackingActive.load();

    if(!tracking)
    {
        return;
    }

    constexpr LinearPt rangeStart = 0x2BF00;
    constexpr LinearPt rangeEnd = 0x2C100;

    if(address < rangeStart ||
        address > rangeEnd)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readAddresses.insert(
        address
    );
}

void MemoryReadTracker::record(
    LinearPt address,
    LinearPt instructionAddress
)
{
    const bool tracking =
        trackingActive.load();

    if(!tracking)
    {
        return;
    }

    constexpr LinearPt rangeStart = 0x2BF00;
    constexpr LinearPt rangeEnd = 0x2C100;

    if(address < rangeStart ||
        address > rangeEnd)
    {
        return;
    }

    // TEMP PERFORMANCE TEST
    return;

    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readAddresses.insert(
        address
    );

    /*
    readInstructions.emplace(
        address,
        instructionAddress
    );*/
}

void MemoryReadTracker::start()
{
    {
        std::lock_guard<std::mutex> lock(
            readAddressesMutex
        );

        readAddresses.clear();
        readInstructions.clear();

        recordedInstructionTransitions.clear();
        recentInstructions.clear();

        previousInstructionAddress = 0;
        hasPreviousInstruction = false;

        waitingForTransitionNextInstruction = false;
        pendingTransitionIndex = 0;
    }

    readTraceTargetAddress =
        0x2BF35;

    readTraceRunning =
        false;

    readTraceInstructions.clear();

    trackingActive = true;
}

void MemoryReadTracker::stop()
{
    trackingActive = false;
}

void MemoryReadTracker::clear()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readAddresses.clear();
    readInstructions.clear();

    recordedInstructionTransitions.clear();
    recentInstructions.clear();

    previousInstructionAddress = 0;
    hasPreviousInstruction = false;

    waitingForTransitionNextInstruction = false;
    pendingTransitionIndex = 0;
}

bool MemoryReadTracker::active()
{
    return trackingActive;
}

void MemoryReadTracker::startTransitionTracking()
{
    transitionTrackingActive =
        true;
}

void MemoryReadTracker::stopTransitionTracking()
{
    transitionTrackingActive =
        false;
}

bool MemoryReadTracker::transitionTrackingIsActive()
{
    return transitionTrackingActive;
}

void MemoryReadTracker::clearTransitionTracking()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    recordedInstructionTransitions.clear();
    recentInstructions.clear();

    transitionHistorySize = 0;
    transitionHistoryWriteIndex = 0;

    previousInstructionAddress = 0;
    previousCS = 0;
    previousIP = 0;

    previousInstructionBytes =
    {};

    hasPreviousInstruction =
        false;

    waitingForTransitionNextInstruction =
        false;

    pendingTransitionIndex =
        0;
}

void MemoryReadTracker::recordTransitionInstruction(
    LinearPt instructionAddress,
    uint16_t cs,
    uint16_t ip,
    const RegisterSnapshot& registers
)
{
    if(!transitionTrackingActive.load())
    {
        return;
    }

    RuntimeInstruction runtimeInstruction;

    runtimeInstruction.address =
        instructionAddress;

    runtimeInstruction.cs =
        cs;

    runtimeInstruction.ip =
        ip;

    runtimeInstruction.registers =
        registers;

    // bytes und stackBytes bleiben zunächst 0.

    if(waitingForTransitionNextInstruction &&
        pendingTransitionIndex <
        recordedInstructionTransitions.size())
    {
        InstructionTransition& transition =
            recordedInstructionTransitions[
                pendingTransitionIndex
            ];

        transition.nextInstruction =
            runtimeInstruction;

        transition.hasNextInstruction =
            true;

        waitingForTransitionNextInstruction =
            false;
    }

    if(instructionAddress ==
        transitionTargetAddress &&
        hasPreviousInstruction)
    {
        InstructionTransition transition;

        transition.history.reserve(
            transitionHistorySize
        );

        const size_t historyStart =
            transitionHistorySize <
            transitionHistoryCapacity
            ? 0
            : transitionHistoryWriteIndex;

        for(size_t i = 0;
            i < transitionHistorySize;
            ++i)
        {
            const size_t index =
                (historyStart + i) %
                transitionHistoryCapacity;

            const TransitionHistoryEntry& entry =
                transitionHistory[index];

            RuntimeInstruction historyInstruction;

            historyInstruction.address =
                entry.address;

            historyInstruction.cs =
                entry.cs;

            historyInstruction.ip =
                entry.ip;

            historyInstruction.registers =
                entry.registers;

            for(size_t byteIndex = 0;
                byteIndex < historyInstruction.bytes.size();
                ++byteIndex)
            {
                historyInstruction.bytes[
                    byteIndex
                ] =
                    mem_readb(
                        entry.address +
                        static_cast<LinearPt>(
                            byteIndex
                            )
                    );
            }

            transition.history.push_back(
                historyInstruction
            );
        }
        transition.previousAddress =
            previousInstructionAddress;

        transition.currentAddress =
            instructionAddress;

        transition.previousCS =
            previousCS;

        transition.previousIP =
            previousIP;

        for(size_t byteIndex = 0;
            byteIndex < transition.previousBytes.size();
            ++byteIndex)
        {
            transition.previousBytes[
                byteIndex
            ] =
                mem_readb(
                    previousInstructionAddress +
                    static_cast<LinearPt>(
                        byteIndex
                        )
                );
        }

        recordedInstructionTransitions.push_back(
            std::move(
                transition
            )
        );

        pendingTransitionIndex =
            recordedInstructionTransitions.size() - 1;

        waitingForTransitionNextInstruction =
            true;
    }

    TransitionHistoryEntry& historyEntry =
        transitionHistory[
            transitionHistoryWriteIndex
        ];

    historyEntry.address =
        instructionAddress;

    historyEntry.cs =
        cs;

    historyEntry.ip =
        ip;

    historyEntry.registers =
        registers;

    transitionHistoryWriteIndex =
        (transitionHistoryWriteIndex + 1) %
        transitionHistoryCapacity;

    if(transitionHistorySize <
        transitionHistoryCapacity)
    {
        ++transitionHistorySize;
    }

    previousInstructionAddress =
        instructionAddress;

    previousCS =
        cs;

    previousIP =
        ip;

    hasPreviousInstruction =
        true;
}

std::vector<LinearPt>
MemoryReadTracker::addresses()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<LinearPt> result(
        readAddresses.begin(),
        readAddresses.end()
    );

    std::sort(
        result.begin(),
        result.end()
    );

    return result;
}

std::vector<std::pair<LinearPt, LinearPt>>
MemoryReadTracker::instructions()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return std::vector<
        std::pair<LinearPt, LinearPt>
    >(
        readInstructions.begin(),
        readInstructions.end()
    );
}

void MemoryReadTracker::setCurrentInstructionContext(
    LinearPt instructionAddress,
    uint16_t cs,
    uint16_t ip,
    const RegisterSnapshot& registers,
    LinearPt stackAddress
)
{
    currentRuntimeInstruction.address =
        instructionAddress;

    currentRuntimeInstruction.cs =
        cs;

    currentRuntimeInstruction.ip =
        ip;

    currentRuntimeInstruction.registers =
        registers;

    currentStackAddress =
        stackAddress;
}

void MemoryReadTracker::recordInstruction(
    LinearPt instructionAddress,
    uint16_t cs,
    uint16_t ip,
    const RegisterSnapshot& registers,
    const std::array<uint8_t, 16>& instructionBytes,
    const std::array<uint8_t, 32>& stackBytes
)
{
    const bool tracking =
        trackingActive.load();

    const LinearPt executionTarget =
        executionCaptureTargetAddress.load();

    const LinearPt memoryWriteTarget =
        memoryWriteWatchTargetAddress.load();

    if(!tracking &&
        executionTarget == 0 &&
        memoryWriteTarget == 0 &&
        !readTraceRunning)
    {
        return;
    }

    MemoryReadTracker::RuntimeInstruction
        runtimeInstruction;

    runtimeInstruction.address =
        instructionAddress;

    runtimeInstruction.cs =
        cs;

    runtimeInstruction.ip =
        ip;

    runtimeInstruction.registers =
        registers;

    runtimeInstruction.bytes =
        instructionBytes;

    runtimeInstruction.stackBytes =
        stackBytes;

    currentRuntimeInstruction =
        runtimeInstruction;

    if(!tracking &&
        executionTarget == 0 &&
        !readTraceRunning)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    if(executionTarget != 0 &&
        instructionAddress ==
        executionTarget &&
        !hasExecutionCapture)
    {
        capturedExecutionInstruction =
            runtimeInstruction;

        hasExecutionCapture =
            true;
    }

    if(readTraceRunning)
    {
        const bool repeatedInstruction =
            !readTraceInstructions.empty() &&
            readTraceInstructions.back().address ==
            runtimeInstruction.address &&
            readTraceInstructions.back().bytes ==
            runtimeInstruction.bytes;

        if(!repeatedInstruction)
        {
            readTraceInstructions.push_back(
                runtimeInstruction
            );

            while(readTraceInstructions.size() >
                readTraceInstructionCount)
            {
                readTraceInstructions.pop_front();
            }
        }

        if(readTraceTargetAddress != 0 &&
            instructionAddress ==
            readTraceTargetAddress)
        {
            readTraceRunning =
                false;

            readTraceArmedState =
                false;

            readTraceTargetAddress =
                0;
        }
    }
}

std::vector<std::pair<LinearPt, LinearPt>>
MemoryReadTracker::instructionTransitions()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<std::pair<LinearPt, LinearPt>>
        result;

    result.reserve(
        recordedInstructionTransitions.size()
    );

    for(const InstructionTransition& transition :
        recordedInstructionTransitions)
    {
        result.emplace_back(
            transition.previousAddress,
            transition.currentAddress
        );
    }

    return result;
}

std::vector<std::pair<uint16_t, uint16_t>>
MemoryReadTracker::instructionTransitionContexts()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<std::pair<uint16_t, uint16_t>>
        result;

    result.reserve(
        recordedInstructionTransitions.size()
    );

    for(const InstructionTransition& transition :
        recordedInstructionTransitions)
    {
        result.emplace_back(
            transition.previousCS,
            transition.previousIP
        );
    }

    return result;
}

std::vector<std::array<uint8_t, 16>>
MemoryReadTracker::instructionTransitionBytes()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<std::array<uint8_t, 16>>
        result;

    result.reserve(
        recordedInstructionTransitions.size()
    );

    for(const InstructionTransition& transition :
        recordedInstructionTransitions)
    {
        result.push_back(
            transition.previousBytes
        );
    }

    return result;
}

void MemoryReadTracker::setTransitionTarget(
    LinearPt address
)
{
    transitionTargetAddress =
        address;
}

LinearPt MemoryReadTracker::transitionTarget()
{
    return transitionTargetAddress;
}

std::vector<
    std::vector<MemoryReadTracker::RuntimeInstruction>
>
MemoryReadTracker::instructionTransitionHistories()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<
        std::vector<MemoryReadTracker::RuntimeInstruction>
    >
        result;

    result.reserve(
        recordedInstructionTransitions.size()
    );

    for(const InstructionTransition& transition :
        recordedInstructionTransitions)
    {
        result.push_back(
            transition.history
        );
    }

    return result;
}

std::vector<MemoryReadTracker::RuntimeInstruction>
MemoryReadTracker::instructionTransitionNextInstructions()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<RuntimeInstruction>
        result;

    result.reserve(
        recordedInstructionTransitions.size()
    );

    for(const InstructionTransition& transition :
        recordedInstructionTransitions)
    {
        if(transition.hasNextInstruction)
        {
            result.push_back(
                transition.nextInstruction
            );
        }
        else
        {
            result.push_back(
                RuntimeInstruction{}
            );
        }
    }

    return result;
}

void MemoryReadTracker::setReadTraceTarget(
    LinearPt address
)
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    readTraceTargetAddress =
        address;

    readTraceArmedState =
        address != 0;

    readTraceRunning =
        address != 0;

    readTraceInstructions.clear();
}

LinearPt MemoryReadTracker::readTraceTarget()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return readTraceTargetAddress;
}

bool MemoryReadTracker::readTraceActive()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return readTraceRunning;
}

bool MemoryReadTracker::readTraceArmed()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return readTraceArmedState;
}

std::vector<MemoryReadTracker::RuntimeInstruction>
MemoryReadTracker::readTrace()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    std::vector<MemoryReadTracker::RuntimeInstruction>
        result(
            readTraceInstructions.begin(),
            readTraceInstructions.end()
        );

    for(RuntimeInstruction& instruction :
        result)
    {
        for(size_t byteIndex = 0;
            byteIndex < instruction.bytes.size();
            ++byteIndex)
        {
            instruction.bytes[
                byteIndex
            ] =
                mem_readb(
                    instruction.address +
                    static_cast<LinearPt>(
                        byteIndex
                        )
                );
        }
    }

    return result;
}

void MemoryReadTracker::setExecutionCaptureTarget(
    LinearPt address
)
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    capturedExecutionInstruction =
        RuntimeInstruction{};

    hasExecutionCapture =
        false;

    executionCaptureTargetAddress.store(
        address
    );
}

LinearPt MemoryReadTracker::executionCaptureTarget()
{
    return executionCaptureTargetAddress.load();
}

void MemoryReadTracker::clearExecutionCapture()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    capturedExecutionInstruction =
        RuntimeInstruction{};

    hasExecutionCapture =
        false;

    executionCaptureTargetAddress.store(
        0
    );
}

bool MemoryReadTracker::executionCaptureHit()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return hasExecutionCapture;
}

MemoryReadTracker::RuntimeInstruction
MemoryReadTracker::executionCapture()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return capturedExecutionInstruction;
}

void MemoryReadTracker::setMemoryWriteWatchTarget(
    LinearPt address
)
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    memoryWriteWatchTargetAddress.store(
        address
    );

    char debugText[128];

    std::snprintf(
        debugText,
        sizeof(debugText),
        "MEMWRITE TARGET SET: 0x%zX\n",
        static_cast<size_t>(
            address
            )
    );

    OutputDebugStringA(
        debugText
    );

    hasMemoryWriteWatchHit =
        false;

    capturedMemoryWriteValue =
        0;

    capturedMemoryWriteInstructions.clear();
}

LinearPt MemoryReadTracker::memoryWriteWatchTarget()
{
    return memoryWriteWatchTargetAddress.load();
}

bool MemoryReadTracker::memoryWriteWatchHit()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return hasMemoryWriteWatchHit;
}

MemoryReadTracker::RuntimeInstruction
MemoryReadTracker::memoryWriteWatchCapture()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return capturedMemoryWriteInstruction;
}

size_t MemoryReadTracker::memoryWriteWatchCaptureCount()
{
    {
        std::lock_guard<std::mutex> lock(
            readAddressesMutex
        );

        return capturedMemoryWriteInstructions.size();
    }
}

MemoryReadTracker::RuntimeInstruction
MemoryReadTracker::memoryWriteWatchCapture(
    size_t index
)
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    if(index >=
        capturedMemoryWriteInstructions.size())
    {
        return RuntimeInstruction{};
    }

    return capturedMemoryWriteInstructions[
        index
    ].instruction;
}

uint8_t MemoryReadTracker::
memoryWriteWatchCaptureValue(
    size_t index
)
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    if(index >=
        capturedMemoryWriteInstructions.size())
    {
        return 0;
    }

    return capturedMemoryWriteInstructions[
        index
    ].writeValue;
}

uint8_t MemoryReadTracker::
memoryWriteWatchValue()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    return capturedMemoryWriteValue;
}

void MemoryReadTracker::clearMemoryWriteWatch()
{
    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    memoryWriteWatchTargetAddress.store(
        0
    );

    hasMemoryWriteWatchHit =
        false;

    capturedMemoryWriteValue =
        0;

    capturedMemoryWriteInstructions.clear();

}

void MemoryReadTracker::recordMemoryWrite(
    LinearPt address,
    uint8_t value
)
{
    const LinearPt targetAddress =
        memoryWriteWatchTargetAddress.load();

    if(targetAddress != 0 &&
        address >= targetAddress - 16 &&
        address <= targetAddress + 16)
    {
        char debugText[256];

        std::snprintf(
            debugText,
            sizeof(debugText),
            "MEMWRITE near target: address=0x%zX target=0x%zX value=%u\n",
            static_cast<size_t>(address),
            static_cast<size_t>(targetAddress),
            static_cast<unsigned int>(value)
        );

        OutputDebugStringA(
            debugText
        );
    }

    if(targetAddress == 0 ||
        address != targetAddress)
    {
        return;
    }

    std::lock_guard<std::mutex> lock(
        readAddressesMutex
    );

    capturedMemoryWriteValue =
        value;

    capturedMemoryWriteInstruction =
        currentRuntimeInstruction;

    for(size_t i = 0;
        i < capturedMemoryWriteInstruction.stackBytes.size();
        ++i)
    {
        capturedMemoryWriteInstruction.stackBytes[i] =
            mem_readb(
                currentStackAddress +
                static_cast<LinearPt>(i)
            );
    }

    capturedMemoryWriteInstructions.push_back(
        MemoryWriteCapture{
            capturedMemoryWriteInstruction,
            value
        }
    );

    hasMemoryWriteWatchHit =
        true;
}

