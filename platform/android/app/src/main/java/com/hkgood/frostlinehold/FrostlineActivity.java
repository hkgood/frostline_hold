package com.hkgood.frostlinehold;

import org.libsdl.app.SDLActivity;

/**
 * The game: SDL's activity running the shell in libmain.so.
 *
 * SDL is linked into libmain.so (platform/pc/CMakeLists.txt builds it
 * static), so there is no separate libSDL3.so to load.
 */
public class FrostlineActivity extends SDLActivity {
    @Override
    protected String[] getLibraries() {
        return new String[] { "main" };
    }
}
