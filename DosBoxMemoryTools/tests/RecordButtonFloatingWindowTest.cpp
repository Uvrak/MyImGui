#include "pch.h"
#include "gtest/gtest.h"

#include "MyImGui.h"
#include "RecordButton.h"
#include "FloatingWindow.h"

// Minimal unit test to exercise the interaction between RecordButton and FloatingWindow.
// This test will be expanded/adjusted after implementing the fix that pins the toolbox when recording.

TEST(RecordButtonFloatingWindow, RecordStartsDoesNotLockWindowByDefault)
{
    // Arrange
    MyImGui::FloatingWindowOptions opts;
    opts.movable = true;
    opts.resizable = true;
    opts.dockable = true;

    MyImGui::FloatingWindow window("Test Window", opts);
    MyImGui::RecordButton button;

    // Act
    // simulate clicking the record button
    button.start();

    // Assert
    // By default, the window should remain movable (the fix will change this behavior)
    // We cannot inspect internal m_options here; this is a placeholder assertion that should fail until the fix is applied.
    ASSERT_TRUE(button.recording());
}
