#include <core/pe.hpp>

#include <cstdio>
#include <cstring>
#include <fstream>

namespace {

#pragma pack( push, 1 )
struct DosHeader {
    uint16_t e_magic;
    uint16_t e_cblp;
    uint16_t e_cp;
    uint16_t e_crlc;
    uint16_t e_cparhdr;
    uint16_t e_minalloc;
    uint16_t e_maxalloc;
    uint16_t e_ss;
    uint16_t e_sp;
    uint16_t e_csum;
    uint16_t e_ip;
    uint16_t e_cs;
    uint16_t e_lfarlc;
    uint16_t e_ovno;
    uint16_t e_res [ 4 ];
    uint16_t e_oemid;
    uint16_t e_oeminfo;
    uint16_t e_res2 [ 10 ];
    uint32_t e_lfanew;
};

struct CoffFileHeader {
    uint16_t Machine;
    uint16_t NumberOfSections;
    uint32_t TimeDateStamp;
    uint32_t PointerToSymbolTable;
    uint32_t NumberOfSymbols;
    uint16_t SizeOfOptionalHeader;
    uint16_t Characteristics;
};

struct PeSectionRaw {
    char     Name [ 8 ];
    uint32_t VirtualSize;
    uint32_t VirtualAddress;
    uint32_t SizeOfRawData;
    uint32_t PointerToRawData;
    uint32_t PointerToRelocations;
    uint32_t PointerToLinenumbers;
    uint16_t NumberOfRelocations;
    uint16_t NumberOfLinenumbers;
    uint32_t Characteristics;
};
#pragma pack( pop )

}

bool PeImage::Load( const std::string & path ) {
    std::ifstream f( path, std::ios::binary | std::ios::ate );
    if ( !f )
        return false;
    auto end = f.tellg( );
    if ( end <= 0 )
        return false;

    bytes.resize( ( size_t ) end );
    f.seekg( 0 );
    f.read( ( char * ) bytes.data( ), bytes.size( ) );
    if ( !f )
        return false;

    if ( bytes.size( ) < sizeof( DosHeader ) )
        return false;

    DosHeader dos;
    std::memcpy( &dos, bytes.data( ), sizeof( dos ) );
    if ( dos.e_magic != 0x5A4D )
        return false;
    if ( dos.e_lfanew + 24 > bytes.size( ) )
        return false;

    uint32_t peSig = 0;
    std::memcpy( &peSig, bytes.data( ) + dos.e_lfanew, 4 );
    if ( peSig != 0x00004550 )
        return false;

    CoffFileHeader fh;
    std::memcpy( &fh, bytes.data( ) + dos.e_lfanew + 4, sizeof( fh ) );

    size_t optStart = dos.e_lfanew + 4 + sizeof( fh );
    if ( optStart + 2 > bytes.size( ) )
        return false;

    uint16_t magic = 0;
    std::memcpy( &magic, bytes.data( ) + optStart, 2 );

    if ( magic == 0x10b ) {
        is64 = false;
        if ( optStart + 60 > bytes.size( ) )
            return false;
        uint32_t base32 = 0;
        std::memcpy( &base32, bytes.data( ) + optStart + 28, 4 );
        imageBase = base32;
        std::memcpy( &sizeOfImage, bytes.data( ) + optStart + 56, 4 );
    }
    else if ( magic == 0x20b ) {
        is64 = true;
        if ( optStart + 60 > bytes.size( ) )
            return false;
        std::memcpy( &imageBase, bytes.data( ) + optStart + 24, 8 );
        std::memcpy( &sizeOfImage, bytes.data( ) + optStart + 56, 4 );
    }
    else {
        return false;
    }

    size_t sectStart = optStart + fh.SizeOfOptionalHeader;
    if ( sectStart + fh.NumberOfSections * sizeof( PeSectionRaw ) > bytes.size( ) )
        return false;

    sections.reserve( fh.NumberOfSections );
    for ( uint16_t i = 0; i < fh.NumberOfSections; ++i ) {
        PeSectionRaw raw;
        std::memcpy( &raw, bytes.data( ) + sectStart + i * sizeof( raw ),
                     sizeof( raw ) );
        PeSection s;
        char nameBuf [ 9 ] = { 0 };
        std::memcpy( nameBuf, raw.Name, 8 );
        s.name             = nameBuf;
        s.virtualAddress   = raw.VirtualAddress;
        s.virtualSize      = raw.VirtualSize;
        s.pointerToRawData = raw.PointerToRawData;
        s.sizeOfRawData    = raw.SizeOfRawData;
        s.characteristics  = raw.Characteristics;
        sections.emplace_back( std::move( s ) );
    }
    return true;
}

uint64_t PeImage::MapVAtoFile( uint64_t va ) const {
    if ( va < imageBase )
        return 0;
    uint64_t rva = va - imageBase;
    for ( const auto & s : sections ) {
        uint32_t span = s.virtualSize < s.sizeOfRawData ? s.virtualSize
                                                        : s.sizeOfRawData;
        if ( rva >= s.virtualAddress &&
             rva < ( uint64_t ) s.virtualAddress + span ) {
            return ( uint64_t ) s.pointerToRawData + ( rva - s.virtualAddress );
        }
    }
    return 0;
}

uint64_t PeImage::MapFileToVA( uint64_t fileOff ) const {
    for ( const auto & s : sections ) {
        uint32_t span = s.virtualSize < s.sizeOfRawData ? s.virtualSize
                                                        : s.sizeOfRawData;
        if ( fileOff >= s.pointerToRawData &&
             fileOff < ( uint64_t ) s.pointerToRawData + span ) {
            return imageBase + s.virtualAddress + ( fileOff - s.pointerToRawData );
        }
    }
    return 0;
}

bool PeImage::ReadAtFile( uint64_t fileOff, void * dst, size_t size ) const {
    if ( fileOff + size > bytes.size( ) )
        return false;
    std::memcpy( dst, bytes.data( ) + fileOff, size );
    return true;
}

bool PeImage::ReadVA( uint64_t va, void * dst, size_t size ) const {
    uint64_t f = MapVAtoFile( va );
    if ( !f )
        return false;
    return ReadAtFile( f, dst, size );
}

bool PeImage::ReadCStringVA( uint64_t va, std::string & out, size_t maxLen ) const {
    out.clear( );
    uint64_t f = MapVAtoFile( va );
    if ( !f )
        return false;
    for ( size_t i = 0; i < maxLen && f + i < bytes.size( ); ++i ) {
        char c = ( char ) bytes [ ( size_t ) f + i ];
        if ( c == 0 )
            return true;
        out.push_back( c );
    }
    return true;
}

const uint8_t * PeImage::RawAtFile( uint64_t fileOff, size_t size ) const {
    if ( fileOff + size > bytes.size( ) )
        return nullptr;
    return bytes.data( ) + fileOff;
}
