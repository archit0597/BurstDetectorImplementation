#ifndef BITMAP_H
#define BITMAP_H

#include <cstdint>
#include <cstring>

// The BitMap structure holds the pointer to the bit set and the size of the bit set.
struct BitMap {
    uint8_t* bitSet;
    uint32_t size;

    BitMap(uint32_t length);

    ~BitMap();

    void SetBit(uint32_t index);

    bool GetBit(uint32_t index);

    void ClearBit(uint32_t index);

    void ClearAllBits();
};

BitMap::BitMap(uint32_t length) {
    size = ((length + 7) >> 3);
    bitSet = new uint8_t[size]; 
    memset(bitSet, 0, size * sizeof(uint8_t));
}

BitMap::~BitMap() {
    delete [] bitSet;
}

// Set the bit at the specified index.
void BitMap::SetBit(uint32_t index) {
    uint32_t position = (index >> 3); 
    uint32_t offset = (index & 0x7); 
    bitSet[position] |= (1 << offset);
}

// Get the bit at the specified index.
bool BitMap::GetBit(uint32_t index) {
    uint32_t position = (index >> 3);
    uint32_t offset = (index & 0x7);
    return (bitSet[position] & (1 << offset));
}

// Clear the bit at the specified index.
void BitMap::ClearBit(uint32_t index) {
    uint32_t position = (index >> 3);
    uint32_t offset = (index & 0x7); 
    bitSet[position] &= (~(1 << offset)); 
}

// Clear all the bits in the bit set.
void BitMap::ClearAllBits() {
    memset(bitSet, 0, sizeof(uint8_t) * size);
}

#endif