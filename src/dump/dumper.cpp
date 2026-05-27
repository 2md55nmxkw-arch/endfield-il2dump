#include <dump/dumper.hpp>
#include <dump/utils.hpp>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

namespace {

constexpr uint32_t TA_VISIBILITY_MASK    = 0x00000007;
constexpr uint32_t TA_NOT_PUBLIC         = 0x00000000;
constexpr uint32_t TA_PUBLIC             = 0x00000001;
constexpr uint32_t TA_NESTED_PUBLIC      = 0x00000002;
constexpr uint32_t TA_NESTED_PRIVATE     = 0x00000003;
constexpr uint32_t TA_NESTED_FAMILY      = 0x00000004;
constexpr uint32_t TA_NESTED_ASSEMBLY    = 0x00000005;
constexpr uint32_t TA_NESTED_FAM_AND_ASM = 0x00000006;
constexpr uint32_t TA_NESTED_FAM_OR_ASM  = 0x00000007;
constexpr uint32_t TA_INTERFACE          = 0x00000020;
constexpr uint32_t TA_ABSTRACT           = 0x00000080;
constexpr uint32_t TA_SEALED             = 0x00000100;
constexpr uint32_t TA_SERIALIZABLE       = 0x00002000;

constexpr uint32_t MA_ACCESS_MASK    = 0x0007;
constexpr uint32_t MA_PRIVATE        = 0x0001;
constexpr uint32_t MA_FAM_AND_ASSEM  = 0x0002;
constexpr uint32_t MA_ASSEM          = 0x0003;
constexpr uint32_t MA_FAMILY         = 0x0004;
constexpr uint32_t MA_FAM_OR_ASSEM   = 0x0005;
constexpr uint32_t MA_PUBLIC         = 0x0006;
constexpr uint32_t MA_STATIC         = 0x0010;
constexpr uint32_t MA_FINAL          = 0x0020;
constexpr uint32_t MA_VIRTUAL        = 0x0040;
constexpr uint32_t MA_NEW_SLOT       = 0x0100;
constexpr uint32_t MA_ABSTRACT       = 0x0400;
constexpr uint32_t MA_PINVOKEIMPL    = 0x2000;

constexpr uint32_t MIA_CODETYPE_MASK = 0x0003;
constexpr uint32_t MIA_NATIVE        = 0x0001;
constexpr uint32_t MIA_RUNTIME       = 0x0003;
constexpr uint32_t MIA_INTERNALCALL  = 0x1000;

constexpr uint32_t FA_ACCESS_MASK = 0x0007;
constexpr uint32_t FA_PRIVATE     = 0x0001;
constexpr uint32_t FA_FAM_AND_ASM = 0x0002;
constexpr uint32_t FA_ASSEMBLY    = 0x0003;
constexpr uint32_t FA_FAMILY      = 0x0004;
constexpr uint32_t FA_FAM_OR_ASM  = 0x0005;
constexpr uint32_t FA_PUBLIC      = 0x0006;
constexpr uint32_t FA_STATIC      = 0x0010;
constexpr uint32_t FA_INITONLY    = 0x0020;
constexpr uint32_t FA_LITERAL     = 0x0040;

constexpr uint32_t PARAM_IN  = 0x0001;
constexpr uint32_t PARAM_OUT = 0x0002;

std::string TypeVisibility( uint32_t flags ) {
    switch ( flags & TA_VISIBILITY_MASK ) {
    case TA_PUBLIC:
    case TA_NESTED_PUBLIC:        return "public ";
    case TA_NESTED_PRIVATE:       return "private ";
    case TA_NESTED_FAMILY:        return "protected ";
    case TA_NESTED_ASSEMBLY:
    case TA_NESTED_FAM_AND_ASM:
    case TA_NOT_PUBLIC:           return "internal ";
    case TA_NESTED_FAM_OR_ASM:    return "protected internal ";
    }
    return "internal ";
}

std::string FieldVisibility( uint32_t flags ) {
    switch ( flags & FA_ACCESS_MASK ) {
    case FA_PUBLIC:      return "public ";
    case FA_PRIVATE:     return "private ";
    case FA_FAMILY:      return "protected ";
    case FA_ASSEMBLY:
    case FA_FAM_AND_ASM: return "internal ";
    case FA_FAM_OR_ASM:  return "protected internal ";
    }
    return "private ";
}

std::string MethodVisibility( uint32_t flags ) {
    switch ( flags & MA_ACCESS_MASK ) {
    case MA_PUBLIC:        return "public ";
    case MA_PRIVATE:       return "private ";
    case MA_FAMILY:        return "protected ";
    case MA_ASSEM:
    case MA_FAM_AND_ASSEM: return "internal ";
    case MA_FAM_OR_ASSEM:  return "protected internal ";
    }
    return "private ";
}

std::string MethodModifiers( const MethodDef & m ) {
    std::string s = MethodVisibility( m.flags );
    if ( m.flags & MA_STATIC )
        s += "static ";
    if ( m.flags & MA_ABSTRACT ) {
        s += "abstract ";
        if ( ( m.flags & MA_NEW_SLOT ) == 0 )
            s += "override ";
    }
    else if ( m.flags & MA_FINAL ) {
        if ( ( m.flags & MA_NEW_SLOT ) == 0 )
            s += "sealed override ";
    }
    else if ( m.flags & MA_VIRTUAL ) {
        s += ( m.flags & MA_NEW_SLOT ) ? "virtual " : "override ";
    }
    if ( m.flags & MA_PINVOKEIMPL )
        s += "extern ";
    return s;
}

std::string FieldModifiers( uint32_t flags ) {
    std::string s = FieldVisibility( flags );
    if ( flags & FA_LITERAL ) {
        s += "const ";
    }
    else {
        if ( flags & FA_STATIC )
            s += "static ";
        if ( flags & FA_INITONLY )
            s += "readonly ";
    }
    return s;
}

std::string ClassDeclaration( const TypeDef & td ) {
    std::string s = TypeVisibility( td.flags );
    bool isInterface = ( td.flags & TA_INTERFACE ) != 0;
    bool isAbstract  = ( td.flags & TA_ABSTRACT ) != 0;
    bool isSealed    = ( td.flags & TA_SEALED ) != 0;

    if ( isAbstract && isSealed )
        s += "static ";
    else if ( !isInterface && isAbstract )
        s += "abstract ";
    else if ( !td.IsValueType( ) && !td.IsEnum( ) && isSealed )
        s += "sealed ";

    if ( isInterface )            s += "interface ";
    else if ( td.IsEnum( ) )      s += "enum ";
    else if ( td.IsValueType( ) ) s += "struct ";
    else                          s += "class ";
    return s;
}

std::string MethodNoCodeReason( const MethodDef & m,
                                 Il2CppBinary::ResolveStatus status ) {
    if ( m.flags & MA_ABSTRACT )                          return "abstract";
    if ( m.flags & MA_PINVOKEIMPL )                       return "extern (P/Invoke)";
    if ( m.iflags & MIA_INTERNALCALL )                    return "extern (icall)";
    if ( ( m.iflags & MIA_CODETYPE_MASK ) == MIA_NATIVE ) return "native";
    if ( ( m.iflags & MIA_CODETYPE_MASK ) == MIA_RUNTIME )return "runtime";
    if ( m.genericContainerIndex >= 0 )                   return "generic def";
    switch ( status ) {
    case Il2CppBinary::ResolveStatus::NullPointer:     return "stripped";
    case Il2CppBinary::ResolveStatus::ZeroToken:       return "zero token";
    case Il2CppBinary::ResolveStatus::TokenOutOfRange: return "token out of range";
    case Il2CppBinary::ResolveStatus::NoModule:        return "no codegen module";
    default:                                            return "not resolved";
    }
}

std::string Hex( uint64_t v, int width = 0 ) {
    char buf [ 32 ];
    if ( width > 0 )
        std::snprintf( buf, sizeof( buf ), "0x%0*llX", width,
                       ( unsigned long long ) v );
    else
        std::snprintf( buf, sizeof( buf ), "0x%llX",
                       ( unsigned long long ) v );
    return buf;
}

std::string EscapeJson( const std::string & s ) {
    std::string r;
    r.reserve( s.size( ) + 4 );
    for ( unsigned char c : s ) {
        switch ( c ) {
        case '"':  r += "\\\""; break;
        case '\\': r += "\\\\"; break;
        case '\n': r += "\\n";  break;
        case '\r': r += "\\r";  break;
        case '\t': r += "\\t";  break;
        case '\b': r += "\\b";  break;
        case '\f': r += "\\f";  break;
        default:
            if ( c < 0x20 ) {
                char tmp [ 8 ];
                std::snprintf( tmp, sizeof( tmp ), "\\u%04X", c );
                r += tmp;
            }
            else {
                r += ( char ) c;
            }
        }
    }
    return r;
}

bool IsReservedWindowsName( const std::string & s ) {
    static const char * names [ ] = {
        "CON", "PRN", "AUX", "NUL",
        "COM1","COM2","COM3","COM4","COM5","COM6","COM7","COM8","COM9",
        "LPT1","LPT2","LPT3","LPT4","LPT5","LPT6","LPT7","LPT8","LPT9"
    };
    std::string up;
    up.reserve( s.size( ) );
    for ( char c : s ) up += ( char ) std::toupper( ( unsigned char ) c );
    for ( const char * n : names )
        if ( up == n ) return true;
    return false;
}

std::string SanitizePathPart( const std::string & s ) {
    std::string r;
    r.reserve( s.size( ) );
    for ( unsigned char c : s ) {
        if ( c < 0x20 || c == '<' || c == '>' || c == ':' || c == '"' ||
             c == '/' || c == '\\' || c == '|' || c == '?' || c == '*' )
            r += '_';
        else
            r += ( char ) c;
    }
    while ( !r.empty( ) && ( r.back( ) == ' ' || r.back( ) == '.' ) )
        r.pop_back( );
    if ( r.empty( ) )
        r = "_";
    if ( IsReservedWindowsName( r ) )
        r += "_";
    if ( r.size( ) > 120 )
        r.resize( 120 );
    return r;
}

std::string SanitizeFileBase( const std::string & name ) {
    std::string s = name;
    auto pos = s.find( '`' );
    if ( pos != std::string::npos ) {
        s.erase( 0, 0 );
        std::string clean;
        clean.reserve( s.size( ) );
        for ( char c : s ) {
            if ( c == '`' ) { clean += '_'; continue; }
            clean += c;
        }
        s = std::move( clean );
    }
    return SanitizePathPart( s );
}

std::vector<std::string> SplitNamespace( const std::string & ns ) {
    std::vector<std::string> out;
    std::string cur;
    for ( char c : ns ) {
        if ( c == '.' ) {
            if ( !cur.empty( ) ) { out.push_back( cur ); cur.clear( ); }
        }
        else cur += c;
    }
    if ( !cur.empty( ) ) out.push_back( cur );
    return out;
}

std::string AssemblyDirName( const std::string & imageName ) {
    std::string s = imageName;
    if ( s.size( ) >= 4 ) {
        std::string tail = s.substr( s.size( ) - 4 );
        for ( char & c : tail ) c = ( char ) std::tolower( ( unsigned char ) c );
        if ( tail == ".dll" || tail == ".exe" )
            s.resize( s.size( ) - 4 );
    }
    return SanitizePathPart( s );
}

}

void Dumper::DumpAll( const std::string & outputDir ) {
    EnsureDirectory( outputDir );

    std::string dumpPath   = outputDir + "\\dump.cs";
    std::string scriptPath = outputDir + "\\script.json";
    std::string stringPath = outputDir + "\\stringliteral.json";
    std::string typesDir   = outputDir + "\\types";
    std::string idaPyPath  = outputDir + "\\ida_rename.py";
    std::string idaIdcPath = outputDir + "\\ida_rename.idc";

    std::ofstream out( dumpPath, std::ios::out | std::ios::trunc );
    if ( !out.is_open( ) ) {
        Log( "error: cannot open " + dumpPath );
        return;
    }
    WriteImageList( out );
    for ( const auto & img : md_.Images( ) ) {
        std::string asmName = md_.GetString( img.nameIndex );
        int typeEnd = img.typeStart + ( int ) img.typeCount;
        for ( int ti = img.typeStart; ti < typeEnd; ++ti ) {
            const TypeDef & td = md_.Types( ) [ ti ];
            if ( td.declaringTypeIndex != -1 )
                continue;
            EmitType( out, asmName, ti, 0 );
        }
    }
    out.close( );

    WriteScriptJson( scriptPath );
    WriteStringLiteralJson( stringPath );
    WritePerTypeTree( typesDir );
    WriteIdaPythonScript( idaPyPath, "script.json" );
    WriteIdaIdcScript( idaIdcPath, "script.json" );

    Log( "" );
    Log( "dump.cs            -> " + dumpPath );
    Log( "script.json        -> " + scriptPath );
    Log( "stringliteral.json -> " + stringPath );
    Log( "types/             -> " + typesDir );
    Log( "ida_rename.py      -> " + idaPyPath );
    Log( "ida_rename.idc     -> " + idaIdcPath );
    Log( "RVA resolved: " + std::to_string( g_rvaResolved ) + " / " +
         std::to_string( g_rvaTotal ) );
}

void Dumper::WriteImageList( std::ofstream & out ) {
    const auto & images = md_.Images( );
    for ( size_t i = 0; i < images.size( ); ++i ) {
        out << "// Image " << i << ": "
            << md_.GetString( images [ i ].nameIndex )
            << " - " << images [ i ].typeCount << "\n";
    }
}

void Dumper::EmitType( std::ofstream & out, const std::string & asmName,
                       int ti, int indent ) {
    const TypeDef & td = md_.Types( ) [ ti ];
    std::string pad( ( size_t ) indent, '\t' );

    out << "\n" << pad << "// Namespace: "
        << md_.GetString( td.namespaceIndex ) << "\n";

    if ( td.flags & TA_SERIALIZABLE )
        out << pad << "[Serializable]\n";

    out << pad << ClassDeclaration( td )
        << ex_.GetTypeDefName( td, false, true );

    bool isInterface = ( td.flags & TA_INTERFACE ) != 0;
    std::vector<std::string> bases;
    if ( !td.IsEnum( ) && !td.IsValueType( ) && !isInterface &&
         td.parentIndex >= 0 ) {
        const auto * pt = bin_.Type( td.parentIndex );
        if ( pt ) {
            std::string pn = ex_.GetTypeName( *pt, false, false );
            if ( pn != "object" )
                bases.push_back( pn );
        }
    }
    for ( int j = 0; j < td.interfaces_count; ++j ) {
        int idx = td.interfacesStart + j;
        if ( idx < 0 || ( size_t ) idx >= md_.InterfaceIndices( ).size( ) )
            continue;
        int typeIdx = md_.InterfaceIndices( ) [ idx ];
        const auto * it = bin_.Type( typeIdx );
        if ( it )
            bases.push_back( ex_.GetTypeName( *it, false, false ) );
    }
    if ( !bases.empty( ) ) {
        out << " : ";
        for ( size_t j = 0; j < bases.size( ); ++j ) {
            if ( j ) out << ", ";
            out << bases [ j ];
        }
    }

    out << " // TypeDefIndex: " << ti;
    int32_t sz = bin_.TypeDefSize( ti );
    if ( sz > 0 )
        out << ", Size: " << Hex( ( uint64_t ) sz );
    out << "\n" << pad << "{\n";

    if ( td.field_count > 0 ) {
        out << pad << "\t// Fields\n";
        for ( int j = 0; j < td.field_count; ++j )
            EmitField( out, ti, j, pad );
    }

    if ( td.property_count > 0 ) {
        out << "\n" << pad << "\t// Properties\n";
        for ( int j = 0; j < td.property_count; ++j )
            EmitProperty( out, ti, j, pad );
    }

    if ( td.event_count > 0 ) {
        out << "\n" << pad << "\t// Events\n";
        for ( int j = 0; j < td.event_count; ++j )
            EmitEvent( out, ti, j, pad );
    }

    if ( td.method_count > 0 ) {
        std::unordered_set<int> consumed = ConsumedMethodIndices( td );
        bool wroteHeader = false;
        for ( int j = 0; j < td.method_count; ++j ) {
            if ( consumed.count( j ) )
                continue;
            if ( !wroteHeader ) {
                out << "\n" << pad << "\t// Methods\n";
                wroteHeader = true;
            }
            EmitMethod( out, asmName, ti, j, pad );
        }
    }

    if ( td.nested_type_count > 0 ) {
        for ( int j = 0; j < td.nested_type_count; ++j ) {
            int idx = td.nestedTypesStart + j;
            if ( idx < 0 || ( size_t ) idx >= md_.NestedTypeIndices( ).size( ) )
                continue;
            int nested = md_.NestedTypeIndices( ) [ idx ];
            if ( nested < 0 || ( size_t ) nested >= md_.Types( ).size( ) )
                continue;
            EmitType( out, asmName, nested, indent + 1 );
        }
    }

    out << pad << "}\n";
}

void Dumper::EmitField( std::ofstream & out, int ti, int j,
                        const std::string & pad ) {
    const TypeDef & td = md_.Types( ) [ ti ];
    int fi = td.fieldStart + j;
    if ( fi < 0 || ( size_t ) fi >= md_.Fields( ).size( ) )
        return;
    const FieldDef & fd = md_.Fields( ) [ fi ];
    const Il2CppTypeRec * ft = bin_.Type( fd.typeIndex );
    if ( !ft )
        return;
    uint32_t fflags = ft->attrs;
    bool isStatic = ( fflags & FA_STATIC ) != 0;
    bool isLiteral = ( fflags & FA_LITERAL ) != 0;

    out << pad << "\t" << FieldModifiers( fflags )
        << ex_.GetTypeName( *ft, false, false ) << " "
        << md_.GetString( fd.nameIndex );

    FieldDefaultValue fdv;
    if ( md_.TryGetFieldDefault( fi, fdv ) && fdv.dataIndex != -1 ) {
        std::string lit;
        if ( ex_.TryFormatDefaultValue( fdv.typeIndex, fdv.dataIndex, lit ) )
            out << " = " << lit;
    }

    if ( !isLiteral ) {
        int32_t off = bin_.FieldOffset( ti, j, td.IsValueType( ), isStatic );
        out << "; // " << Hex( ( uint64_t ) off ) << "\n";
    }
    else {
        out << ";\n";
    }
}

void Dumper::EmitProperty( std::ofstream & out, int ti, int j,
                           const std::string & pad ) {
    const TypeDef & td = md_.Types( ) [ ti ];
    int pi = td.propertyStart + j;
    if ( pi < 0 || ( size_t ) pi >= md_.Properties( ).size( ) )
        return;
    const PropertyDef & p = md_.Properties( ) [ pi ];

    const MethodDef * gm = nullptr;
    const MethodDef * sm = nullptr;
    if ( p.get >= 0 && p.get < td.method_count ) {
        int mi = td.methodStart + p.get;
        if ( mi >= 0 && ( size_t ) mi < md_.Methods( ).size( ) )
            gm = &md_.Methods( ) [ mi ];
    }
    if ( p.set >= 0 && p.set < td.method_count ) {
        int mi = td.methodStart + p.set;
        if ( mi >= 0 && ( size_t ) mi < md_.Methods( ).size( ) )
            sm = &md_.Methods( ) [ mi ];
    }

    std::string typeName = "object";
    std::string mods;
    if ( gm ) {
        mods = MethodModifiers( *gm );
        const auto * rt = bin_.Type( gm->returnType );
        if ( rt ) typeName = ex_.GetTypeName( *rt, false, false );
    }
    else if ( sm ) {
        mods = MethodModifiers( *sm );
        int firstPi = sm->parameterStart;
        if ( firstPi >= 0 && ( size_t ) firstPi < md_.Parameters( ).size( ) ) {
            const auto * pt = bin_.Type( md_.Parameters( ) [ firstPi ].typeIndex );
            if ( pt ) typeName = ex_.GetTypeName( *pt, false, false );
        }
    }

    out << pad << "\t" << mods << typeName << " "
        << md_.GetString( p.nameIndex ) << " { ";
    if ( gm ) out << "get; ";
    if ( sm ) out << "set; ";
    out << "}\n";
}

void Dumper::EmitEvent( std::ofstream & out, int ti, int j,
                        const std::string & pad ) {
    const TypeDef & td = md_.Types( ) [ ti ];
    int ei = td.eventStart + j;
    if ( ei < 0 || ( size_t ) ei >= md_.Events( ).size( ) )
        return;
    const EventDef & e = md_.Events( ) [ ei ];
    const auto * et = bin_.Type( e.typeIndex );
    std::string typeName = et ? ex_.GetTypeName( *et, false, false ) : "object";

    std::string mods;
    if ( e.add >= 0 && e.add < td.method_count ) {
        int mi = td.methodStart + e.add;
        if ( mi >= 0 && ( size_t ) mi < md_.Methods( ).size( ) )
            mods = MethodModifiers( md_.Methods( ) [ mi ] );
    }

    out << pad << "\t" << mods << "event " << typeName << " "
        << md_.GetString( e.nameIndex ) << ";\n";
}

std::unordered_set<int> Dumper::ConsumedMethodIndices( const TypeDef & td ) {
    std::unordered_set<int> s;
    for ( int j = 0; j < td.property_count; ++j ) {
        int pi = td.propertyStart + j;
        if ( pi < 0 || ( size_t ) pi >= md_.Properties( ).size( ) )
            continue;
        const PropertyDef & p = md_.Properties( ) [ pi ];
        if ( p.get >= 0 && p.get < td.method_count ) s.insert( p.get );
        if ( p.set >= 0 && p.set < td.method_count ) s.insert( p.set );
    }
    for ( int j = 0; j < td.event_count; ++j ) {
        int ei = td.eventStart + j;
        if ( ei < 0 || ( size_t ) ei >= md_.Events( ).size( ) )
            continue;
        const EventDef & e = md_.Events( ) [ ei ];
        if ( e.add    >= 0 && e.add    < td.method_count ) s.insert( e.add );
        if ( e.remove >= 0 && e.remove < td.method_count ) s.insert( e.remove );
        if ( e.raise  >= 0 && e.raise  < td.method_count ) s.insert( e.raise );
    }
    return s;
}

void Dumper::EmitMethod( std::ofstream & out, const std::string & asmName,
                         int ti, int j, const std::string & pad ) {
    const TypeDef & td = md_.Types( ) [ ti ];
    int mi = td.methodStart + j;
    if ( mi < 0 || ( size_t ) mi >= md_.Methods( ).size( ) )
        return;
    const MethodDef & m = md_.Methods( ) [ mi ];

    Il2CppBinary::ResolveStatus status;
    uint64_t va  = bin_.ResolveMethodVA( asmName, m, status );
    uint64_t rva = bin_.VAtoRVA( va );
    uint64_t off = va ? bin_.VAtoFile( va ) : 0;

    ++g_rvaTotal;
    if ( rva ) ++g_rvaResolved;

    out << pad << "\t// RVA: ";
    if ( rva ) {
        out << Hex( rva, 8 ) << " Offset: " << Hex( off, 8 )
            << " VA: " << Hex( va, 16 );
    }
    else {
        out << "-1 Offset: -1 // " << MethodNoCodeReason( m, status );
    }
    if ( m.slot != ( uint16_t ) -1 )
        out << " Slot: " << m.slot;
    out << "\n";

    const auto * rt = bin_.Type( m.returnType );
    bool retByref = rt ? ( rt->byref != 0 ) : false;
    std::string retName = rt ? ex_.GetTypeName( *rt, false, false )
                             : std::string( "object" );

    std::string mname = md_.GetString( m.nameIndex );
    std::string line = MethodModifiers( m );
    if ( retByref ) line += "ref ";
    line += retName + " " + mname;

    if ( m.genericContainerIndex >= 0 &&
         ( size_t ) m.genericContainerIndex < md_.GenericContainers( ).size( ) ) {
        line += ex_.GetGenericContainerParams(
            md_.GenericContainers( ) [ m.genericContainerIndex ] );
    }
    line += "(";
    for ( int k = 0; k < m.parameterCount; ++k ) {
        if ( k > 0 ) line += ", ";
        int ppi = m.parameterStart + k;
        if ( ppi < 0 || ( size_t ) ppi >= md_.Parameters( ).size( ) ) {
            line += "object arg" + std::to_string( k );
            continue;
        }
        const ParameterDef & p = md_.Parameters( ) [ ppi ];
        const auto * pt = bin_.Type( p.typeIndex );
        std::string ptName = pt ? ex_.GetTypeName( *pt, false, false )
                                : std::string( "object" );
        std::string prefix;
        if ( pt && pt->byref ) {
            bool inP   = ( pt->attrs & PARAM_IN ) != 0;
            bool outP  = ( pt->attrs & PARAM_OUT ) != 0;
            if      ( outP && !inP ) prefix = "out ";
            else if ( inP && !outP ) prefix = "in ";
            else                     prefix = "ref ";
        }
        line += prefix + ptName + " " + md_.GetString( p.nameIndex );

        ParameterDefaultValue pdv;
        if ( md_.TryGetParameterDefault( ppi, pdv ) && pdv.dataIndex != -1 ) {
            std::string lit;
            if ( ex_.TryFormatDefaultValue( pdv.typeIndex, pdv.dataIndex, lit ) )
                line += " = " + lit;
        }
    }
    line += ")";

    bool noBody = ( m.flags & MA_ABSTRACT ) ||
                  ( m.flags & MA_PINVOKEIMPL ) ||
                  ( m.iflags & MIA_INTERNALCALL ) ||
                  ( ( td.flags & TA_INTERFACE ) != 0 );

    out << pad << "\t" << line << ( noBody ? ";\n" : " { }\n" );

    EmitGenericInstances( out, mi, m, td, pad );
}

void Dumper::EmitGenericInstances( std::ofstream & out, int methodDefIndex,
                                   const MethodDef & m, const TypeDef & td,
                                   const std::string & pad ) {
    const auto & list = bin_.InstancesForMethod( methodDefIndex );
    if ( list.empty( ) )
        return;

    bool typeIsGeneric = td.genericContainerIndex >= 0;
    bool methodIsGeneric = m.genericContainerIndex >= 0;
    if ( !typeIsGeneric && !methodIsGeneric )
        return;

    std::string typeName = ex_.GetTypeDefName( td, false, false );
    std::string methodName = md_.GetString( m.nameIndex );

    for ( const auto & inst : list ) {
        if ( !inst.va )
            continue;

        std::string sig = typeName;
        if ( !inst.classArgs.empty( ) ) {
            sig += "<";
            for ( size_t i = 0; i < inst.classArgs.size( ); ++i ) {
                if ( i ) sig += ", ";
                sig += inst.classArgs [ i ]
                    ? ex_.GetTypeName( *inst.classArgs [ i ], false, false )
                    : std::string( "object" );
            }
            sig += ">";
        }
        sig += "." + methodName;
        if ( !inst.methodArgs.empty( ) ) {
            sig += "<";
            for ( size_t i = 0; i < inst.methodArgs.size( ); ++i ) {
                if ( i ) sig += ", ";
                sig += inst.methodArgs [ i ]
                    ? ex_.GetTypeName( *inst.methodArgs [ i ], false, false )
                    : std::string( "object" );
            }
            sig += ">";
        }

        uint64_t rva = bin_.VAtoRVA( inst.va );
        out << pad << "\t//   RVA: " << Hex( rva, 8 )
            << " VA: " << Hex( inst.va, 16 )
            << "  " << sig << "\n";
    }
}

void Dumper::WriteScriptJson( const std::string & path ) {
    std::ofstream out( path, std::ios::out | std::ios::trunc );
    if ( !out.is_open( ) ) {
        Log( "error: cannot open " + path );
        return;
    }

    out << "{\n  \"Addresses\": [],\n  \"ScriptMethod\": [\n";
    bool first = true;
    auto emit = [ & ] ( uint64_t va, const std::string & sig ) {
        if ( !va ) return;
        if ( !first ) out << ",\n";
        first = false;
        out << "    { \"Address\": " << va
            << ", \"Name\": \"" << EscapeJson( sig ) << "\" }";
    };

    for ( const auto & img : md_.Images( ) ) {
        std::string asmName = md_.GetString( img.nameIndex );
        int typeEnd = img.typeStart + ( int ) img.typeCount;
        for ( int ti = img.typeStart; ti < typeEnd; ++ti ) {
            const TypeDef & td = md_.Types( ) [ ti ];
            std::string typeFq = ex_.GetTypeDefName( td, true, true );
            std::string ns = md_.GetString( td.namespaceIndex );
            if ( !ns.empty( ) && typeFq.find( ns ) != 0 )
                typeFq = ns + "." + typeFq;
            for ( int j = 0; j < td.method_count; ++j ) {
                int mi = td.methodStart + j;
                if ( mi < 0 || ( size_t ) mi >= md_.Methods( ).size( ) )
                    continue;
                const MethodDef & m = md_.Methods( ) [ mi ];
                emit( bin_.GetMethodVA( asmName, m ),
                      BuildMethodSignature( m, typeFq ) );

                const auto & insts = bin_.InstancesForMethod( mi );
                for ( const auto & inst : insts ) {
                    if ( !inst.va ) continue;
                    emit( inst.va,
                          BuildGenericInstanceSignature( m, td, inst ) );
                }
            }
        }
    }
    out << "\n  ],\n";
    out << "  \"ScriptString\": [],\n";
    out << "  \"ScriptMetadata\": [],\n";
    out << "  \"ScriptMetadataMethod\": []\n";
    out << "}\n";
}

std::string Dumper::BuildGenericInstanceSignature(
    const MethodDef & m, const TypeDef & td,
    const Il2CppBinary::GenericInstance & inst ) {
    const auto * rt = bin_.Type( m.returnType );
    std::string ret = rt ? ex_.GetTypeName( *rt, true, false )
                         : std::string( "object" );

    std::string typeFq = ex_.GetTypeDefName( td, true, false );
    std::string ns = md_.GetString( td.namespaceIndex );
    if ( !ns.empty( ) && typeFq.find( ns ) != 0 )
        typeFq = ns + "." + typeFq;
    if ( !inst.classArgs.empty( ) ) {
        typeFq += "<";
        for ( size_t i = 0; i < inst.classArgs.size( ); ++i ) {
            if ( i ) typeFq += ", ";
            typeFq += inst.classArgs [ i ]
                ? ex_.GetTypeName( *inst.classArgs [ i ], true, false )
                : std::string( "object" );
        }
        typeFq += ">";
    }

    std::string mname = md_.GetString( m.nameIndex );
    if ( !inst.methodArgs.empty( ) ) {
        mname += "<";
        for ( size_t i = 0; i < inst.methodArgs.size( ); ++i ) {
            if ( i ) mname += ", ";
            mname += inst.methodArgs [ i ]
                ? ex_.GetTypeName( *inst.methodArgs [ i ], true, false )
                : std::string( "object" );
        }
        mname += ">";
    }

    std::string s = ret + " " + typeFq + "$$" + mname + "(";
    for ( int k = 0; k < m.parameterCount; ++k ) {
        if ( k ) s += ", ";
        int ppi = m.parameterStart + k;
        if ( ppi < 0 || ( size_t ) ppi >= md_.Parameters( ).size( ) ) {
            s += "object";
            continue;
        }
        const auto * pt = bin_.Type( md_.Parameters( ) [ ppi ].typeIndex );
        s += pt ? ex_.GetTypeName( *pt, true, false ) : std::string( "object" );
    }
    s += ")";
    return s;
}

std::string Dumper::BuildMethodSignature( const MethodDef & m,
                                          const std::string & typeFq ) {
    const auto * rt = bin_.Type( m.returnType );
    std::string ret = rt ? ex_.GetTypeName( *rt, true, false )
                         : std::string( "object" );
    std::string s = ret + " " + typeFq + "$$" + md_.GetString( m.nameIndex ) + "(";
    for ( int k = 0; k < m.parameterCount; ++k ) {
        if ( k ) s += ", ";
        int ppi = m.parameterStart + k;
        if ( ppi < 0 || ( size_t ) ppi >= md_.Parameters( ).size( ) ) {
            s += "object";
            continue;
        }
        const auto * pt = bin_.Type( md_.Parameters( ) [ ppi ].typeIndex );
        s += pt ? ex_.GetTypeName( *pt, true, false ) : std::string( "object" );
    }
    s += ")";
    return s;
}

void Dumper::WriteStringLiteralJson( const std::string & path ) {
    std::ofstream out( path, std::ios::out | std::ios::trunc );
    if ( !out.is_open( ) ) {
        Log( "error: cannot open " + path );
        return;
    }
    out << "[\n";
    const auto & lits = md_.StringLiterals( );
    for ( size_t i = 0; i < lits.size( ); ++i ) {
        std::string v = md_.GetStringLiteral( ( uint32_t ) i );
        out << "  { \"Index\": " << i
            << ", \"Value\": \"" << EscapeJson( v ) << "\" }";
        if ( i + 1 < lits.size( ) ) out << ",";
        out << "\n";
    }
    out << "]\n";
}

void Dumper::WritePerTypeTree( const std::string & rootDir ) {
    namespace fs = std::filesystem;
    std::error_code ec;
    fs::create_directories( fs::u8path( rootDir ), ec );

    std::unordered_map<std::string, std::unordered_map<std::string, int>> usedNames;

    for ( const auto & img : md_.Images( ) ) {
        std::string asmName = md_.GetString( img.nameIndex );
        std::string asmDir = AssemblyDirName( asmName );

        int typeEnd = img.typeStart + ( int ) img.typeCount;
        for ( int ti = img.typeStart; ti < typeEnd; ++ti ) {
            const TypeDef & td = md_.Types( ) [ ti ];
            if ( td.declaringTypeIndex != -1 )
                continue;

            std::string ns = md_.GetString( td.namespaceIndex );
            std::vector<std::string> nsParts = SplitNamespace( ns );

            fs::path dir = fs::u8path( rootDir ) / fs::u8path( asmDir );
            for ( const auto & p : nsParts )
                dir /= fs::u8path( SanitizePathPart( p ) );
            fs::create_directories( dir, ec );

            std::string base = SanitizeFileBase( md_.GetString( td.nameIndex ) );
            std::string key = dir.u8string( );
            int & cnt = usedNames[ key ][ base ];
            std::string fileName = base;
            if ( cnt > 0 ) {
                char suf[ 16 ];
                std::snprintf( suf, sizeof( suf ), "_%d", cnt );
                fileName += suf;
            }
            ++cnt;
            fileName += ".cs";

            fs::path filePath = dir / fs::u8path( fileName );
            std::ofstream f( filePath.u8string( ), std::ios::out | std::ios::trunc );
            if ( !f.is_open( ) )
                continue;

            f << "// Assembly: " << asmName << "\n";
            f << "// TypeDefIndex: " << ti << "\n";
            if ( !ns.empty( ) ) {
                f << "namespace " << ns << "\n{\n";
                EmitType( f, asmName, ti, 1 );
                f << "}\n";
            }
            else {
                EmitType( f, asmName, ti, 0 );
            }
        }
    }
}

void Dumper::WriteIdaPythonScript( const std::string & path, const std::string & scriptJsonPath ) {
    std::ofstream out( path, std::ios::out | std::ios::trunc );
    if ( !out.is_open( ) ) {
        Log( "error: cannot open " + path );
        return;
    }
    out << "import idaapi\n";
    out << "import idc\n";
    out << "import ida_name\n";
    out << "import ida_funcs\n";
    out << "import ida_kernwin\n";
    out << "import ida_idaapi\n";
    out << "import json\n";
    out << "import os\n";
    out << "import re\n";
    out << "\n";
    out << "DEFAULT_JSON = " << "\"" << scriptJsonPath << "\"" << "\n";
    out << "\n";
    out << "_safe = re.compile(r\"[^A-Za-z0-9_]\")\n";
    out << "\n";
    out << "def sanitize(name):\n";
    out << "    name = name.replace(\"::\", \"_\")\n";
    out << "    name = _safe.sub(\"_\", name)\n";
    out << "    if not name:\n";
    out << "        name = \"sub\"\n";
    out << "    if name[0].isdigit():\n";
    out << "        name = \"_\" + name\n";
    out << "    if len(name) > 511:\n";
    out << "        name = name[:511]\n";
    out << "    return name\n";
    out << "\n";
    out << "def pick_path():\n";
    out << "    here = os.path.dirname(idaapi.get_input_file_path() or \"\")\n";
    out << "    cand = os.path.join(here, DEFAULT_JSON)\n";
    out << "    if os.path.isfile(cand):\n";
    out << "        return cand\n";
    out << "    return ida_kernwin.ask_file(False, \"script.json\", \"Select script.json\")\n";
    out << "\n";
    out << "def main():\n";
    out << "    p = pick_path()\n";
    out << "    if not p:\n";
    out << "        return\n";
    out << "    with open(p, \"r\", encoding=\"utf-8\") as f:\n";
    out << "        data = json.load(f)\n";
    out << "    base = idaapi.get_imagebase()\n";
    out << "    methods = data.get(\"ScriptMethod\", []) or []\n";
    out << "    ok = 0\n";
    out << "    fail = 0\n";
    out << "    used = {}\n";
    out << "    flags = ida_name.SN_NOWARN | ida_name.SN_NOCHECK | ida_name.SN_FORCE\n";
    out << "    for i, m in enumerate(methods):\n";
    out << "        addr = m.get(\"Address\")\n";
    out << "        sig = m.get(\"Name\", \"\")\n";
    out << "        if not addr or not sig:\n";
    out << "            continue\n";
    out << "        ea = base + (addr & 0xFFFFFFFF) if addr < base else addr\n";
    out << "        if ea == ida_idaapi.BADADDR:\n";
    out << "            continue\n";
    out << "        f = ida_funcs.get_func(ea)\n";
    out << "        if not f:\n";
    out << "            ida_funcs.add_func(ea)\n";
    out << "            f = ida_funcs.get_func(ea)\n";
    out << "        target = f.start_ea if f else ea\n";
    out << "        nm = sanitize(sig)\n";
    out << "        n = used.get(nm, 0)\n";
    out << "        used[nm] = n + 1\n";
    out << "        final = nm if n == 0 else \"%s_%d\" % (nm, n)\n";
    out << "        if ida_name.set_name(target, final, flags):\n";
    out << "            idc.set_cmt(target, sig, 0)\n";
    out << "            ok += 1\n";
    out << "        else:\n";
    out << "            fail += 1\n";
    out << "        if (i + 1) % 5000 == 0:\n";
    out << "            print(\"[il2dump] processed %d / %d\" % (i + 1, len(methods)))\n";
    out << "    print(\"[il2dump] done. renamed=%d failed=%d total=%d\" % (ok, fail, len(methods)))\n";
    out << "\n";
    out << "main()\n";
}

void Dumper::WriteIdaIdcScript( const std::string & path, const std::string & scriptJsonPath ) {
    std::ofstream out( path, std::ios::out | std::ios::trunc );
    if ( !out.is_open( ) ) {
        Log( "error: cannot open " + path );
        return;
    }
    out << "#include <idc.idc>\n";
    out << "\n";
    out << "static sanitize(s) {\n";
    out << "    auto i, c, r;\n";
    out << "    r = \"\";\n";
    out << "    for ( i = 0; i < strlen(s); i = i + 1 ) {\n";
    out << "        c = substr(s, i, i + 1);\n";
    out << "        if ( (c >= \"A\" && c <= \"Z\") || (c >= \"a\" && c <= \"z\") ||\n";
    out << "             (c >= \"0\" && c <= \"9\") || c == \"_\" )\n";
    out << "            r = r + c;\n";
    out << "        else\n";
    out << "            r = r + \"_\";\n";
    out << "    }\n";
    out << "    if ( strlen(r) == 0 ) r = \"sub\";\n";
    out << "    if ( strlen(r) > 480 ) r = substr(r, 0, 480);\n";
    out << "    return r;\n";
    out << "}\n";
    out << "\n";
    out << "static main() {\n";
    out << "    auto path, fp, line, all, n, ok, fail, base;\n";
    out << "    auto addrPos, namePos, addrEnd, nameEnd, addrStr, nameStr, addr, ea, nm;\n";
    out << "    path = ask_file(0, \"" << scriptJsonPath << "\", \"Select script.json\");\n";
    out << "    if ( path == \"\" ) return;\n";
    out << "    fp = fopen(path, \"r\");\n";
    out << "    if ( fp == 0 ) { msg(\"cannot open script.json\\n\"); return; }\n";
    out << "    all = \"\";\n";
    out << "    while ( (line = readstr(fp)) != -1 )\n";
    out << "        all = all + line;\n";
    out << "    fclose(fp);\n";
    out << "    base = get_imagebase();\n";
    out << "    n = 0; ok = 0; fail = 0;\n";
    out << "    addrPos = 0;\n";
    out << "    while ( 1 ) {\n";
    out << "        addrPos = strstr(all, \"\\\"Address\\\":\", addrPos);\n";
    out << "        if ( addrPos < 0 ) break;\n";
    out << "        addrPos = addrPos + 10;\n";
    out << "        addrEnd = strstr(all, \",\", addrPos);\n";
    out << "        if ( addrEnd < 0 ) break;\n";
    out << "        addrStr = substr(all, addrPos, addrEnd);\n";
    out << "        namePos = strstr(all, \"\\\"Name\\\":\", addrEnd);\n";
    out << "        if ( namePos < 0 ) break;\n";
    out << "        namePos = strstr(all, \"\\\"\", namePos + 7) + 1;\n";
    out << "        nameEnd = strstr(all, \"\\\"\", namePos);\n";
    out << "        while ( nameEnd > 0 && substr(all, nameEnd - 1, nameEnd) == \"\\\\\" )\n";
    out << "            nameEnd = strstr(all, \"\\\"\", nameEnd + 1);\n";
    out << "        if ( nameEnd < 0 ) break;\n";
    out << "        nameStr = substr(all, namePos, nameEnd);\n";
    out << "        addrPos = nameEnd;\n";
    out << "        addr = atol(addrStr);\n";
    out << "        if ( addr == 0 ) continue;\n";
    out << "        ea = addr;\n";
    out << "        if ( addr < base ) ea = base + addr;\n";
    out << "        nm = sanitize(nameStr);\n";
    out << "        if ( set_name(ea, nm, SN_NOWARN | SN_NOCHECK) ) {\n";
    out << "            set_cmt(ea, nameStr, 0);\n";
    out << "            ok = ok + 1;\n";
    out << "        }\n";
    out << "        else fail = fail + 1;\n";
    out << "        n = n + 1;\n";
    out << "        if ( (n % 5000) == 0 ) msg(\"[il2dump] %d processed\\n\", n);\n";
    out << "    }\n";
    out << "    msg(\"[il2dump] done. renamed=%d failed=%d total=%d\\n\", ok, fail, n);\n";
    out << "}\n";
}
