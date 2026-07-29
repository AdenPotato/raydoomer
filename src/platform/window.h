#pragma once

namespace platform {

/// Opens the game window and runs until the user closes it.
///
/// @remarks
/// This is the scaffold's minimal proof that the cross-compiled binary reaches
/// a window on the target platform. The real platform seam - interfaces over
/// window, input, audio, filesystem, and clock, mockable from tests - is ADE-24.
/// Until then this is the only function in the layer.
///
/// @param width  Window width in pixels.
/// @param height Window height in pixels.
/// @param title  Window title, shown in the title bar.
/// @returns 0 on a clean exit, non-zero if the window could not be created.
int runWindow(int width, int height, const char* title);

} // namespace platform
