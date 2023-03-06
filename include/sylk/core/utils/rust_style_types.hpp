//
// Created by August on 21-2-23.
//

#pragma once

// NOTE: These type aliases exist merely for the sake of comfort
// Despite the name, they're not intended to function as 'int8_t' style cstdint types would
// I simply like the way Rust types are written, but C++ requires too much compatibility with systems to do things that way
// Those systems, however, are not my targets; so this is fine.

using i8  = signed char;
using i16 = signed short int;
using i32 = signed int;
using i64 = signed long int;

using u8  = unsigned char;
using u16 = unsigned short int;
using u32 = unsigned int;
using u64 = unsigned long int;

using f32 = float;
using f64 = double;
