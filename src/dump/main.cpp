#include <dump/dumper.hpp>
#include <io/executor.hpp>
#include <io/il2cpp_binary.hpp>
#include <core/metadata.hpp>
#include <core/pe.hpp>
#include <dump/utils.hpp>

#include <cstdio>
#include <filesystem>
#include <iostream>
#include <string>
#include <windows.h>

namespace fs = std::filesystem;

static void PrintUsage( const char * argv0 ) {
    std::cout
        << "il2cpp-dumper [options]\n"
        << "  " << argv0 << " <GameAssembly.dll> <global-metadata.dat> [output-dir]\n"
        << "  " << argv0 << " --game-dir <path> [-o output-dir]\n\n"
        << "  -o, --output <dir>   output directory (default: cwd)\n"
        << "      --game-dir <dir> auto-discover both files in <dir>\n"
        << "  -h, --help           show this help\n";
}

static bool FindByGameDir( const fs::path & root, fs::path & dll,
                           fs::path & meta ) {
    fs::path d = root / "GameAssembly.dll";
    if ( fs::exists( d ) )
        dll = d;

    if ( fs::exists( root ) && fs::is_directory( root ) ) {
        for ( const auto & e : fs::directory_iterator( root ) ) {
            if ( !e.is_directory( ) )
                continue;
            auto name = e.path( ).filename( ).string( );
            if ( name.size( ) > 5 &&
                 name.compare( name.size( ) - 5, 5, "_Data" ) == 0 ) {
                fs::path m = e.path( ) / "il2cpp_data" / "Metadata" /
                             "global-metadata.dat";
                if ( fs::exists( m ) ) {
                    meta = m;
                    if ( dll.empty( ) ) {
                        fs::path alt = e.path( ) / "Plugins" / "x86_64" /
                                       "GameAssembly.dll";
                        if ( fs::exists( alt ) )
                            dll = alt;
                    }
                    break;
                }
            }
        }
    }
    return !dll.empty( ) && !meta.empty( );
}

static std::string WideToUtf8( const wchar_t * w ) {
    if ( !w || !*w )
        return {};
    int sz = WideCharToMultiByte( CP_UTF8, 0, w, -1, nullptr, 0, nullptr, nullptr );
    if ( sz <= 1 )
        return {};
    std::string s( ( size_t ) ( sz - 1 ), '\0' );
    WideCharToMultiByte( CP_UTF8, 0, w, -1, &s [ 0 ], sz - 1, nullptr, nullptr );
    return s;
}

int wmain( int argc, wchar_t * wargv [ ] ) {
    SetConsoleOutputCP( CP_UTF8 );

    std::vector<std::string> argv;
    argv.reserve( argc );
    for ( int i = 0; i < argc; ++i )
        argv.push_back( WideToUtf8( wargv [ i ] ) );

    std::string dllPath, metaPath, outDir, gameDir;
    for ( int i = 1; i < ( int ) argv.size( ); ++i ) {
        const std::string & a = argv [ i ];
        if ( a == "-h" || a == "--help" ) {
            PrintUsage( argv [ 0 ].c_str( ) );
            return 0;
        }
        if ( a == "-o" || a == "--output" ) {
            if ( i + 1 < ( int ) argv.size( ) )
                outDir = argv [ ++i ];
            continue;
        }
        if ( a == "--game-dir" ) {
            if ( i + 1 < ( int ) argv.size( ) )
                gameDir = argv [ ++i ];
            continue;
        }
        if ( dllPath.empty( ) )       dllPath = a;
        else if ( metaPath.empty( ) ) metaPath = a;
        else if ( outDir.empty( ) )   outDir = a;
    }

    fs::path dll, meta;
    if ( !dllPath.empty( ) )  dll  = dllPath;
    if ( !metaPath.empty( ) ) meta = metaPath;
    if ( !gameDir.empty( ) )  FindByGameDir( gameDir, dll, meta );
    if ( dll.empty( ) || meta.empty( ) )
        FindByGameDir( fs::current_path( ), dll, meta );

    if ( dll.empty( ) || meta.empty( ) ) {
        std::cerr << "error: could not find GameAssembly.dll and/or "
                     "global-metadata.dat\n\n";
        PrintUsage( argv [ 0 ].c_str( ) );
        return 1;
    }

    if ( outDir.empty( ) )
        outDir = fs::current_path( ).string( );
    g_outputDir = outDir;
    EnsureDirectory( outDir );

    Log( "GameAssembly.dll    : " + dll.string( ) );
    Log( "global-metadata.dat : " + meta.string( ) );
    Log( "Output              : " + outDir );

    PeImage pe;
    if ( !pe.Load( dll.string( ) ) || !pe.Is64( ) ) {
        std::cerr << "error: failed to load 64-bit PE\n";
        return 2;
    }
    char ibuf [ 32 ];
    std::snprintf( ibuf, sizeof( ibuf ), "0x%llX",
                   ( unsigned long long ) pe.ImageBase( ) );
    Log( std::string( "ImageBase           : " ) + ibuf );

    Metadata md;
    if ( !md.Load( meta.string( ) ) ) {
        std::cerr << "error: failed to parse metadata\n";
        return 3;
    }
    Log( "Metadata version    : " + std::to_string( md.Version( ) ) );
    Log( "Types               : " + std::to_string( md.Types( ).size( ) ) );
    Log( "Methods             : " + std::to_string( md.Methods( ).size( ) ) );
    Log( "Images              : " + std::to_string( md.Images( ).size( ) ) );

    Il2CppBinary bin( pe, md );
    if ( !bin.Initialize( ) ) {
        std::cerr << "error: could not initialise IL2CPP binary state\n";
        return 4;
    }

    Executor ex( md, bin );
    Dumper d( md, bin, ex );
    d.DumpAll( outDir );
    return 0;
}
