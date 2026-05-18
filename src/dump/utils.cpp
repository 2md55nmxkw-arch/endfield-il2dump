#include <dump/utils.hpp>

#include <direct.h>
#include <fstream>
#include <iostream>

std::string g_outputDir;
std::size_t g_rvaTotal = 0;
std::size_t g_rvaResolved = 0;

static std::string BaseDir( ) {
    return g_outputDir.empty( ) ? std::string( ".\\" ) : ( g_outputDir + "\\" );
}

static std::ofstream & GetLogFile( ) {
    static std::ofstream logFile;
    static bool opened = false;
    if ( !opened ) {
        logFile.open( BaseDir( ) + "dump.log", std::ios::out | std::ios::trunc );
        opened = true;
    }
    return logFile;
}

void Log( const std::string & msg ) {
    std::cout << msg << "\n";
    auto & lf = GetLogFile( );
    if ( lf.is_open( ) ) {
        lf << msg << "\n";
        lf.flush( );
    }
}

void EnsureDirectory( const std::string & path ) { _mkdir( path.c_str( ) ); }
