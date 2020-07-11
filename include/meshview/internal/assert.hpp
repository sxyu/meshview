#pragma once

#define _MESHVIEW_ASSERT(x) do { \
    if (!(x)) { \
    std::cerr << "meshview assertion FAILED: \"" << #x << "\" (" << (bool)(x) << \
        ")\n  at " << __FILE__ << " line " << __LINE__ <<"\n"; \
    std::exit(1); \
}} while(0)
#define _MESHVIEW_ASSERT_EQ(x, y) do {\
    if ((x) != (y)) { \
    std::cerr << "meshview assertion FAILED: " << #x << \
        " == " << #y << " (" << (x) << " != " << (y) << \
        ")\n  at " << __FILE__ << " line " << __LINE__ <<"\n"; \
    std::exit(1); \
}} while(0)
#define _MESHVIEW_ASSERT_NE(x, y) do {\
    if ((x) == (y)) { \
    std::cerr << "meshview assertion FAILED: " << #x << \
        " != " << #y << " (" << (x) << " == " << (y) << \
        ")\n  at " << __FILE__ << " line " << __LINE__ <<"\n"; \
    std::exit(1); \
}} while(0)
#define _MESHVIEW_ASSERT_LE(x, y) do {\
    if ((x) > (y)) { \
    std::cerr << "meshview assertion FAILED: " << #x << \
        " <= " << #y << " (" << (x) << " > " << (y) << \
        ")\n  at " << __FILE__ << " line " << __LINE__ <<"\n"; \
    std::exit(1); \
}} while(0)
#define _MESHVIEW_ASSERT_LT(x, y) do {\
    if ((x) >= (y)) { \
    std::cerr << "meshview assertion FAILED: " << #x << \
        " < " << #y << " (" << (x) << " >= " << (y) << \
        ")\n  at " << __FILE__ << " line " << __LINE__ <<"\n"; \
    std::exit(1); \
}} while(0)
