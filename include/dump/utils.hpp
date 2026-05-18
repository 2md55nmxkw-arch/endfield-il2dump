#ifndef UTILS_HPP
#define UTILS_HPP

#include <cstdint>
#include <string>

extern std::string g_outputDir;
extern std::size_t g_rvaTotal;
extern std::size_t g_rvaResolved;

void Log( const std::string & msg );
void EnsureDirectory( const std::string & path );

#endif
