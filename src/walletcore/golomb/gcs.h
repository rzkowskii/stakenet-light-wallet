// This is free and unencumbered software released into the public domain.

// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.

// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.

// For more information, please refer to <http://unlicense.org/>

#ifndef GCS_H
#define GCS_H

#include <array>
#include <iostream>
#include <stdint.h>
#include <string>
#include <vector>

namespace bitcoin {

class Filter {
public:
    static constexpr size_t KeySize = 16;
    using key_t = std::array<uint8_t, KeySize>;

    size_t build(); // returns N
    bool match(std::vector<unsigned char> data);
    bool match(std::string str);
    bool matchAny(std::vector<std::string> entries);
    void addEntry(std::vector<unsigned char> entry);
    void addEntries(std::vector<std::string> entries);
    std::vector<uint8_t> bytes() const;

    static Filter WithKeyMP(key_t key, uint64_t m, unsigned short p);
    static Filter FromNMPBytes(key_t key, uint64_t n, uint64_t m, unsigned short p, std::vector<uint8_t> bytes);

private:
    bool hashMatchAny(const std::vector<std::string>& data);
    bool zipMatchAny(const std::vector<std::string>& data);

private:
    key_t _key;
    uint8_t _p;
    uint64_t _m;
    uint64_t _modulusNP;
    std::vector<std::vector<unsigned char>> _data;
    std::vector<uint8_t> _bytes;
};
}

#endif /* GCS_H */
