#ifndef PE_HPP
#define PE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

struct PeSection {
    std::string name;
    uint32_t virtualAddress;
    uint32_t virtualSize;
    uint32_t pointerToRawData;
    uint32_t sizeOfRawData;
    uint32_t characteristics;

    bool IsExec( ) const { return ( characteristics & 0x20000000u ) != 0; }
    bool IsRead( ) const { return ( characteristics & 0x40000000u ) != 0; }
    bool IsWrite( ) const { return ( characteristics & 0x80000000u ) != 0; }
};

class PeImage {
public:
    bool Load( const std::string & path );

    bool Is64( ) const { return is64; }
    uint64_t ImageBase( ) const { return imageBase; }
    uint32_t SizeOfImage( ) const { return sizeOfImage; }
    const std::vector<PeSection> & Sections( ) const { return sections; }
    const std::vector<uint8_t> & Bytes( ) const { return bytes; }

    uint64_t MapVAtoFile( uint64_t va ) const;
    uint64_t MapFileToVA( uint64_t fileOff ) const;

    bool ReadAtFile( uint64_t fileOff, void * dst, size_t size ) const;
    bool ReadVA( uint64_t va, void * dst, size_t size ) const;

    template <typename T> bool ReadVA( uint64_t va, T & out ) const {
        return ReadVA( va, &out, sizeof( T ) );
    }

    bool ReadCStringVA( uint64_t va, std::string & out, size_t maxLen = 4096 ) const;
    const uint8_t * RawAtFile( uint64_t fileOff, size_t size ) const;

private:
    std::vector<uint8_t> bytes;
    std::vector<PeSection> sections;
    uint64_t imageBase = 0;
    uint32_t sizeOfImage = 0;
    bool is64 = false;
};

#endif
