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

#include "gcs.h"
#include "order32.h"
#include <algorithm>
#include <assert.h>
#include <crypto/sha1.h>
#include <hash.h>
#include <stdint.h>
#include <string.h>
#include <utilstrencodings.h>

#define BITMASK(n) ((1 << (n)) - 1)

namespace bitcoin {

static uint64_t fastReduction(uint64_t v, uint64_t nHi, uint64_t nLo)
{
    // First, we'll spit the item we need to reduce into its higher and
    // lower bits.
    auto vhi = v >> 32;
    auto vlo = uint64_t(uint32_t(v));

    // Then, we distribute multiplication over each part.
    auto vnphi = vhi * nHi;
    auto vnpmid = vhi * nLo;
    auto npvmid = nHi * vlo;
    auto vnplo = vlo * nLo;

    // We calculate the carry bit.
    auto carry = (uint64_t(uint32_t(vnpmid)) + uint64_t(uint32_t(npvmid)) + (vnplo >> 32)) >> 32;

    // Last, we add the high bits, the middle bits, and the carry.
    v = vnphi + (vnpmid >> 32) + (npvmid >> 32) + carry;

    return v;
}

class BitWriter {
private:
    std::vector<uint8_t>& f;
    uint8_t wCount = 0;

public:
    BitWriter(std::vector<uint8_t>& _f)
        : f(_f)
    {
    }

    void writeBits(int count, uint64_t data)
    {
        data <<= uint64_t(64 - count);

        // handle write byte if count over 8
        while (count >= 8) {
            auto byte = uint8_t(data >> (64 - 8));
            writeOneByte(byte);

            data <<= 8;
            count -= 8;
        }

        // handle write bit
        while (count > 0) {
            auto bi = data >> (64 - 1);
            writeBit(bi == 1);
            data <<= 1;
            count--;
        }
    }

    void writeOneByte(uint8_t data)
    {
        if (wCount == 0) {
            f.push_back(data);
            return;
        }

        auto latestIndex = f.size() - 1;

        f[latestIndex] |= data >> (8 - wCount);
        f.push_back(0);
        latestIndex++;
        f[latestIndex] = data << wCount;
    }

    void writeBit(bool input)
    {
        if (wCount == 0) {
            f.push_back(0);
            wCount = 8;
        }

        auto latestIndex = f.size() - 1;
        if (input) {
            f[latestIndex] |= 1 << (wCount - 1);
        }
        wCount--;
    }
};

class BitReader {
    const std::vector<uint8_t>& _stream;
    size_t _rCount = 8;
    size_t _latestIndex = 0;

public:
    explicit BitReader(const std::vector<uint8_t>& from)
        : _stream(from)
    {
    }

    bool isAtEnd() { return _stream.size() <= _latestIndex; }

    bool readBit(bool& retBit)
    {
        // empty return io.EOF
        if (isAtEnd()) {
            return false;
        }

        // if first byte already empty, move to next byte to retrieval
        if (_rCount == 0) {
            ++_latestIndex;

            if (isAtEnd()) {
                return false;
            }

            _rCount = 8;
        }

        // handle bit retrieval
        retBit = (_stream[_latestIndex] & (1 << (_rCount - 1))) != 0;
        _rCount--;

        return true;
    }

    bool readByte(uint8_t& retByte)
    {
        // empty return io.EOF
        if (isAtEnd()) {
            return false;
        }

        // if first byte already empty, move to next byte to retrieval
        if (_rCount == 0) {
            _latestIndex++;

            if (isAtEnd()) {
                return false;
            }

            _rCount = 8;
        }

        // just remain 8 bit, just return this byte directly
        if (_rCount == 8) {
            retByte = _stream[_latestIndex];
            ++_latestIndex;
            return true;
        }

        // handle byte retrieval
        retByte = _stream[_latestIndex] << (8 - _rCount);
        _latestIndex++;

        // check if we could finish retrieval on next byte
        if (isAtEnd()) {
            return false;
        }

        // handle remain bit on next stream
        retByte |= _stream[_latestIndex] >> _rCount;
        return true;
    }

    bool readBits(int count, uint64_t& retValue)
    {
        retValue = 0;
        // handle byte reading
        while (count >= 8) {
            retValue <<= 8;
            uint8_t byte;
            if (!readByte(byte)) {
                return false;
            }

            retValue |= uint64_t(byte);
            count -= 8;
        }

        while (count > 0) {
            retValue <<= 1;
            bool bit;
            if (!readBit(bit)) {
                return false;
            }

            if (bit) {
                retValue |= 1;
            }

            count--;
        }

        return true;
    }
};

static bool ReadFullUint64(BitReader& reader, int P, uint64_t& result)
{
    uint64_t quotient = 0;

    // Count the 1s until we reach a 0.
    bool c;
    if (!reader.readBit(c)) {
        return false;
    }
    while (c) {
        quotient++;
        if (!reader.readBit(c)) {
            return false;
        }
    }

    // Read P bits.
    uint64_t remainder;
    if (!reader.readBits(P, remainder)) {
        return false;
    }

    // Add the multiple and the remainder.
    result = (quotient << P) + remainder;
    return true;
}

size_t Filter::build()
{
    //    size_t maxN = (1 << 32);
    //    if(_data.size() >= maxN)
    //    {
    //        throw std::runtime_error("N too big");
    //    }

    if (_p > 32) {
        throw std::runtime_error("P is too big");
    }

    _modulusNP = _data.size() * _m;

    auto nphi = _modulusNP >> 32;
    auto nplo = uint64_t(uint32_t(_modulusNP));
    std::vector<uint64_t> values;

    for (auto&& item : _data) {
        uint64_t k0 = *(reinterpret_cast<uint64_t*>(&_key[0]));
        uint64_t k1 = *(reinterpret_cast<uint64_t*>(&_key[KeySize / 2]));
        auto v = bitcoin::CSipHasher(k0, k1).Write(&item[0], item.size()).Finalize();

        values.push_back(fastReduction(v, nphi, nplo));
    }

    std::sort(std::begin(values), std::end(values));

    uint64_t lastValue = 0;
    BitWriter writer(_bytes);
    for (auto&& v : values) {
        // Calculate the difference between this value and the last,
        // modulo P.
        uint64_t remainder = (v - lastValue) & ((uint64_t(1) << _p) - 1);
        uint64_t value = (v - lastValue - remainder) >> _p;
        lastValue = v;
        while (value > 0) {
            writer.writeBit(true);
            value--;
        }
        writer.writeBit(false);

        writer.writeBits(_p, remainder);
    }

    return _data.size();
}

bool Filter::match(std::vector<unsigned char> data)
{
    // Create a filter bitstream.
    const auto& filterData = _bytes;

    BitReader reader(filterData);

    // We take the high and low bits of modulusNP for the multiplication
    // of 2 64-bit integers into a 128-bit integer.
    auto nphi = _modulusNP >> 32;
    auto nplo = uint64_t(uint32_t(_modulusNP));

    // Then we hash our search term with the same parameters as the filter.
    uint64_t k0 = *(reinterpret_cast<uint64_t*>(&_key[0]));
    uint64_t k1 = *(reinterpret_cast<uint64_t*>(&_key[KeySize / 2]));
    auto term = bitcoin::CSipHasher(k0, k1).Write(&data[0], data.size()).Finalize();

    term = fastReduction(term, nphi, nplo);

    // Go through the search filter and look for the desired value.
    uint64_t lastValue = 0;
    while (lastValue < term) {

        // Read the difference between previous and new value from
        // bitstream.
        uint64_t value;
        if (!ReadFullUint64(reader, _p, value)) {
            return false;
        }

        // Add the previous value to it.
        value += lastValue;
        if (value == term) {
            return true;
        }

        lastValue = value;
    }

    return false;
}

bool Filter::match(std::string str)
{
    return match(std::vector<unsigned char>{ str.begin(), str.end() });
}

bool Filter::matchAny(std::vector<std::string> data)
{
    return data.size() > (_modulusNP / _m / 2) ? hashMatchAny(data) : zipMatchAny(data);
}

void Filter::addEntry(std::vector<unsigned char> entry)
{
    _data.emplace_back(entry);
}

void Filter::addEntries(std::vector<std::string> entries)
{
    for (auto&& entry : entries) {
        _data.emplace_back(entry.begin(), entry.end());
    }
}

std::vector<uint8_t> Filter::bytes() const
{
    return _bytes;
}

Filter Filter::WithKeyMP(Filter::key_t key, uint64_t m, unsigned short p)
{
    Filter filter;
    filter._key = key;
    filter._p = p;
    filter._m = m;

    return filter;
}

Filter Filter::FromNMPBytes(
    Filter::key_t key, uint64_t n, uint64_t m, unsigned short p, std::vector<uint8_t> bytes)
{
    Filter filter;
    filter._key = key;
    filter._m = m;
    filter._p = p;
    filter._modulusNP = n * m;
    filter._bytes = bytes;

    return filter;
}

bool Filter::hashMatchAny(const std::vector<std::string>& data)
{
    if (data.empty()) {
        return false;
    }

    // Create a filter bitstream.
    const auto& filterData = _bytes;

    BitReader reader(filterData);

    uint64_t lastValue = 0;
    std::vector<uint64_t> values;
    values.reserve(10000);
    uint64_t value;
    while (ReadFullUint64(reader, _p, value)) {
        lastValue += value;
        values.push_back(lastValue);
    }

    // We take the high and low bits of modulusNP for the multiplication
    // of 2 64-bit integers into a 128-bit integer.
    auto nphi = _modulusNP >> 32;
    auto nplo = uint64_t(uint32_t(_modulusNP));

    // Then we hash our search term with the same parameters as the filter.
    uint64_t k0 = *(reinterpret_cast<uint64_t*>(&_key[0]));
    uint64_t k1 = *(reinterpret_cast<uint64_t*>(&_key[KeySize / 2]));

    // Finally, run through the provided data items, querying the index to
    // determine if the filter contains any elements of interest.
    for (auto&& entry : data) {
        // For each datum, we assign the initial hash to
        // a uint64.
        auto term = bitcoin::CSipHasher(k0, k1)
                        .Write(reinterpret_cast<const unsigned char*>(&entry[0]), entry.size())
                        .Finalize();

        // We'll then reduce the value down to the range
        // of our modulus.
        for (size_t i = 0; i < values.size(); ++i) {
            if (values[i] == fastReduction(term, nphi, nplo)) {
                return true;
            }
        }
    }

    return false;
}

bool Filter::zipMatchAny(const std::vector<std::string>& data)
{
    // Create a filter bitstream.
    const auto& filterData = _bytes;

    BitReader reader(filterData);

    // We take the high and low bits of modulusNP for the multiplication
    // of 2 64-bit integers into a 128-bit integer.
    auto nphi = _modulusNP >> 32;
    auto nplo = uint64_t(uint32_t(_modulusNP));

    std::vector<uint64_t> values(data.size());

    // Then we hash our search term with the same parameters as the filter.
    uint64_t k0 = *(reinterpret_cast<uint64_t*>(&_key[0]));
    uint64_t k1 = *(reinterpret_cast<uint64_t*>(&_key[KeySize / 2]));

    std::transform(std::begin(data), std::end(data), std::begin(values),
        [k0, k1, nphi, nplo](const auto& entry) {
            auto term = bitcoin::CSipHasher(k0, k1)
                            .Write(reinterpret_cast<const unsigned char*>(&entry[0]), entry.size())
                            .Finalize();
            return fastReduction(term, nphi, nplo);
        });

    std::sort(std::begin(values), std::end(values));

    // Go through the search filter and look for the desired value.
    uint64_t lastValue1 = 0;
    uint64_t lastValue2 = values[0];
    size_t i = 1;
    while (lastValue1 != lastValue2) {
        // Check which filter to advance to make sure we're comparing
        // the right values.
        if (lastValue1 > lastValue2) {
            // Advance filter created from search terms or return
            // false if we're at the end because nothing matched.
            if (i < values.size()) {
                lastValue2 = values[i++];
            } else {
                return false;
            }
        } else if (lastValue1 < lastValue2) {
            // Advance filter we're searching or return false if
            // we're at the end because nothing matched.
            uint64_t value;
            if (!ReadFullUint64(reader, _p, value)) {
                return false;
            }

            lastValue1 += value;
        }
    }

    return true;
}
}
