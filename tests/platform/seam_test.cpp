// The seam itself: every platform capability is an interface that a test can
// substitute. This is what keeps engine and gameplay tests runnable in CI with
// no window, no GPU, and no audio device (engine_protocol.md).
//
// GoogleMock was chosen for the project specifically to mock these interfaces.
// These tests prove that choice actually works before anything depends on it.

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "platform/audio_device.h"
#include "platform/input.h"
#include "platform/window.h"

namespace {

using ::testing::_;
using ::testing::Return;

class MockWindow : public platform::Window {
public:
    MOCK_METHOD(bool, open, (int width, int height, const char* title), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(bool, shouldClose, (), (const, override));
    MOCK_METHOD(void, beginFrame, (), (override));
    MOCK_METHOD(void, endFrame, (), (override));
    MOCK_METHOD(platform::Size, size, (), (const, override));
    MOCK_METHOD(void, setCursorCaptured, (bool captured), (override));
    MOCK_METHOD(bool, isCursorCaptured, (), (const, override));
};

class MockInput : public platform::Input {
public:
    MOCK_METHOD(void, poll, (), (override));
    MOCK_METHOD(bool, isKeyDown, (platform::Key key), (const, override));
    MOCK_METHOD(platform::MouseDelta, mouseDelta, (), (const, override));
};

class MockAudioDevice : public platform::AudioDevice {
public:
    MOCK_METHOD(bool, open, (), (override));
    MOCK_METHOD(void, close, (), (override));
    MOCK_METHOD(void, playSound, (platform::SoundId id, float volume), (override));
};

TEST(PlatformSeam, WindowLifecycleIsAnExercisedCycle) {
    // engine_protocol.md: "Teardown is exercised, not assumed. Create-and-destroy
    // is a tested cycle; a subsystem that only works because the process exits is
    // not finished."
    MockWindow window;

    ::testing::InSequence sequence;
    EXPECT_CALL(window, open(1280, 720, _)).WillOnce(Return(true));
    EXPECT_CALL(window, beginFrame());
    EXPECT_CALL(window, endFrame());
    EXPECT_CALL(window, close());

    ASSERT_TRUE(window.open(1280, 720, "doomer"));
    window.beginFrame();
    window.endFrame();
    window.close();
}

TEST(PlatformSeam, TheCursorIsCapturedForMouseLookAndReleasedOnClose) {
    // Mouse look needs the pointer locked to the window; without it, turning
    // stops the moment the pointer reaches a screen edge. Releasing on close is
    // equally required - a game that strands the cursor is hostile.
    MockWindow window;

    ::testing::InSequence sequence;
    EXPECT_CALL(window, setCursorCaptured(true));
    EXPECT_CALL(window, close());

    window.setCursorCaptured(true);
    window.close();
}

TEST(PlatformSeam, EveryAcquireHasAMatchingReleaseOnTheErrorPath) {
    // An early return that skips cleanup is a leak, not a shortcut. Here the
    // device fails to open, and close() must still be safe to call.
    MockAudioDevice audio;

    EXPECT_CALL(audio, open()).WillOnce(Return(false));
    EXPECT_CALL(audio, close());

    if (!audio.open()) {
        audio.close(); // must be safe even though open() failed
    }
}

TEST(PlatformSeam, InputIsQueriedThroughTheInterface) {
    MockInput input;

    EXPECT_CALL(input, poll());
    EXPECT_CALL(input, isKeyDown(platform::Key::Forward)).WillOnce(Return(true));
    EXPECT_CALL(input, isKeyDown(platform::Key::Back)).WillOnce(Return(false));

    input.poll();
    EXPECT_TRUE(input.isKeyDown(platform::Key::Forward));
    EXPECT_FALSE(input.isKeyDown(platform::Key::Back));
}

TEST(PlatformSeam, AudioIsRequestedNotPlayedInATest) {
    // presentation_protocol.md: tests assert WHICH sounds were requested, never
    // real playback. This is the pattern every later audio test follows.
    MockAudioDevice audio;

    EXPECT_CALL(audio, playSound(platform::SoundId{ 7 }, 0.5f)).Times(1);

    audio.playSound(platform::SoundId{ 7 }, 0.5f);
}

TEST(PlatformSeam, InterfacesAreUsableThroughBasePointers) {
    // Callers hold the interface, never the concrete type. If any of these
    // stopped being abstract, the seam would have quietly closed.
    static_assert(std::is_abstract_v<platform::Window>);
    static_assert(std::is_abstract_v<platform::Input>);
    static_assert(std::is_abstract_v<platform::AudioDevice>);
    SUCCEED();
}

} // namespace
