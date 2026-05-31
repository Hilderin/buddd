#pragma once

#include <cstdint>

namespace buddd::engine {

/// Engine-level key code enum mapping physical key positions.
/// Values are fixed to SDL scancode positions (not language-dependent keycodes).
/// Underlying type is uint8_t (max 256 values, extensible).
enum class KeyCode : uint8_t {
    Unknown = 0,

    // Letters (A=4 ... Z=29)
    A = 4,  B = 5,  C = 6,  D = 7,  E = 8,  F = 9,  G = 10,
    H = 11, I = 12, J = 13, K = 14, L = 15, M = 16,
    N = 17, O = 18, P = 19, Q = 20, R = 21, S = 22,
    T = 23, U = 24, V = 25, W = 26, X = 27, Y = 28, Z = 29,

    // Digits (top row, Digit1=30 ... Digit0=39)
    Digit1 = 30, Digit2 = 31, Digit3 = 32, Digit4 = 33, Digit5 = 34,
    Digit6 = 35, Digit7 = 36, Digit8 = 37, Digit9 = 38, Digit0 = 39,

    // Common control keys (SDL values)
    Enter     = 40,
    Escape    = 41,
    Backspace = 42,
    Tab       = 43,
    Space     = 44,

    // Punctuation and symbol keys (SDL values)
    Minus       = 45,  // - and _
    Equals      = 46,  // = and +
    BracketLeft = 47,  // [ and {
    BracketRight = 48, // ] and }
    Backslash   = 49,  // \ and |
    Semicolon   = 51,  // ; and :
    Quote       = 52,  // ' and "
    Grave       = 53,  // ` and ~ (key above Tab)
    Comma       = 54,  // , and <
    Period      = 55,  // . and >
    Slash       = 56,  // / and ?
    CapsLock    = 57,

    // Function keys (F1=58 ... F12=69)
    F1 = 58,  F2 = 59,  F3 = 60,  F4 = 61,  F5 = 62,
    F6 = 63,  F7 = 64,  F8 = 65,  F9 = 66,  F10 = 67,
    F11 = 68, F12 = 69,

    // Navigation & editing (SDL values)
    Delete = 76,
    Right  = 79, Left = 80, Down = 81, Up = 82,
    Insert = 93,

    // Modifier keys (left and right distinguished, SDL values)
    ControlLeft  = 224, ShiftLeft  = 225, AltLeft  = 226, SuperLeft  = 227,
    ControlRight = 228, ShiftRight = 229, AltRight = 230, SuperRight = 231,

    // Sentinel value for array sizing — must remain last.
    _Count
};

} // namespace buddd::engine
