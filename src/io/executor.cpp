#include <io/executor.hpp>

#include <cstdint>
#include <cstdio>
#include <cstring>

namespace {

const char * PrimitiveName( uint32_t kind ) {
    switch ( kind ) {
    case TYPE_VOID:       return "void";
    case TYPE_BOOLEAN:    return "bool";
    case TYPE_CHAR:       return "char";
    case TYPE_I1:         return "sbyte";
    case TYPE_U1:         return "byte";
    case TYPE_I2:         return "short";
    case TYPE_U2:         return "ushort";
    case TYPE_I4:         return "int";
    case TYPE_U4:         return "uint";
    case TYPE_I8:         return "long";
    case TYPE_U8:         return "ulong";
    case TYPE_R4:         return "float";
    case TYPE_R8:         return "double";
    case TYPE_STRING:     return "string";
    case TYPE_TYPEDBYREF: return "TypedReference";
    case TYPE_I:          return "IntPtr";
    case TYPE_U:          return "UIntPtr";
    case TYPE_OBJECT:     return "object";
    default:              return nullptr;
    }
}

int32_t ReadCompressedInt32( const uint8_t * & p, const uint8_t * end, bool & ok ) {
    ok = false;
    if ( p >= end ) return 0;
    uint8_t first = *p++;
    if ( first == 0xFF ) { ok = true; return -1; }
    if ( ( first & 0x80 ) == 0 ) { ok = true; return first; }
    if ( ( first & 0xC0 ) == 0x80 ) {
        if ( p + 1 > end ) return 0;
        uint8_t b = *p++;
        ok = true;
        return ( ( first & 0x3F ) << 8 ) | b;
    }
    if ( ( first & 0xE0 ) == 0xC0 ) {
        if ( p + 3 > end ) return 0;
        uint8_t b1 = *p++, b2 = *p++, b3 = *p++;
        ok = true;
        return ( ( first & 0x1F ) << 24 ) | ( b1 << 16 ) | ( b2 << 8 ) | b3;
    }
    if ( first == 0xF0 ) {
        if ( p + 4 > end ) return 0;
        int32_t v;
        std::memcpy( &v, p, 4 );
        p += 4;
        ok = true;
        return v;
    }
    return 0;
}

uint32_t ReadCompressedUInt32( const uint8_t * & p, const uint8_t * end, bool & ok ) {
    ok = false;
    if ( p >= end ) return 0;
    uint8_t first = *p++;
    if ( ( first & 0x80 ) == 0 ) { ok = true; return first; }
    if ( ( first & 0xC0 ) == 0x80 ) {
        if ( p + 1 > end ) return 0;
        uint8_t b = *p++;
        ok = true;
        return ( ( uint32_t ) ( first & 0x3F ) << 8 ) | b;
    }
    if ( ( first & 0xE0 ) == 0xC0 ) {
        if ( p + 3 > end ) return 0;
        uint8_t b1 = *p++, b2 = *p++, b3 = *p++;
        ok = true;
        return ( ( uint32_t ) ( first & 0x1F ) << 24 ) | ( b1 << 16 ) | ( b2 << 8 ) | b3;
    }
    if ( first == 0xF0 ) {
        if ( p + 4 > end ) return 0;
        uint32_t v;
        std::memcpy( &v, p, 4 );
        p += 4;
        ok = true;
        return v;
    }
    return 0;
}

}

std::string Executor::GetTypeName( const Il2CppTypeRec & t, bool addNamespace,
                                   bool nested ) const {
    switch ( t.kind ) {
    case TYPE_SZARRAY: {
        const auto * el = bin_.TypeFromVA( t.data );
        return el ? GetTypeName( *el, addNamespace, false ) + "[]"
                  : std::string( "object[]" );
    }
    case TYPE_ARRAY: {
        const auto * el = bin_.TypeFromVA( t.data );
        return el ? GetTypeName( *el, addNamespace, false ) + "[]"
                  : std::string( "object[]" );
    }
    case TYPE_PTR: {
        const auto * el = bin_.TypeFromVA( t.data );
        return el ? GetTypeName( *el, addNamespace, false ) + "*"
                  : std::string( "void*" );
    }
    case TYPE_VAR:
    case TYPE_MVAR: {
        if ( ( int64_t ) t.data >= 0 &&
             ( size_t ) t.data < md_.GenericParameters( ).size( ) ) {
            const auto & gp = md_.GenericParameters( ) [ t.data ];
            return md_.GetString( gp.nameIndex );
        }
        return "T";
    }
    case TYPE_CLASS:
    case TYPE_VALUETYPE: {
        if ( ( int64_t ) t.data < 0 ||
             ( size_t ) t.data >= md_.Types( ).size( ) )
            return "object";
        const TypeDef & td = md_.Types( ) [ t.data ];
        return GetTypeDefName( td, addNamespace, !nested );
    }
    case TYPE_GENERICINST: {
        const auto * inner = bin_.TypeFromGenericClassPtr( t.data );
        if ( !inner ) return "object";
        std::string base;
        if ( inner->kind == TYPE_CLASS || inner->kind == TYPE_VALUETYPE ) {
            if ( ( int64_t ) inner->data >= 0 &&
                 ( size_t ) inner->data < md_.Types( ).size( ) ) {
                const TypeDef & td = md_.Types( ) [ inner->data ];
                base = GetTypeDefName( td, addNamespace, false );
            }
        }
        if ( base.empty( ) )
            base = GetTypeName( *inner, addNamespace, true );

        std::vector<const Il2CppTypeRec *> args;
        bin_.ReadGenericInstArgs( t.data, args );
        if ( args.empty( ) )
            return base;
        std::string out = base + "<";
        for ( size_t i = 0; i < args.size( ); ++i ) {
            if ( i ) out += ", ";
            out += args [ i ] ? GetTypeName( *args [ i ], addNamespace, false )
                              : std::string( "object" );
        }
        out += ">";
        return out;
    }
    default: {
        const char * n = PrimitiveName( t.kind );
        return n ? std::string( n ) : std::string( "object" );
    }
    }
}

std::string Executor::GetTypeDefName( const TypeDef & td, bool addNamespace,
                                      bool genericParameters ) const {
    std::string prefix;
    if ( td.declaringTypeIndex != -1 &&
         ( size_t ) td.declaringTypeIndex < bin_.TypeCount( ) ) {
        const auto * decl = bin_.Type( td.declaringTypeIndex );
        if ( decl )
            prefix = GetTypeName( *decl, addNamespace, true ) + ".";
    }
    else if ( addNamespace ) {
        std::string ns = md_.GetString( td.namespaceIndex );
        if ( !ns.empty( ) )
            prefix = ns + ".";
    }
    std::string name = md_.GetString( td.nameIndex );
    if ( td.genericContainerIndex >= 0 &&
         ( size_t ) td.genericContainerIndex < md_.GenericContainers( ).size( ) ) {
        auto pos = name.find( '`' );
        if ( pos != std::string::npos )
            name.erase( pos );
        if ( genericParameters ) {
            const auto & gc = md_.GenericContainers( ) [ td.genericContainerIndex ];
            name += GetGenericContainerParams( gc );
        }
    }
    return prefix + name;
}

std::string Executor::GetGenericContainerParams( const GenericContainer & gc ) const {
    if ( gc.type_argc <= 0 )
        return "";
    std::string out = "<";
    for ( int i = 0; i < gc.type_argc; ++i ) {
        if ( i ) out += ", ";
        int idx = gc.genericParameterStart + i;
        if ( idx < 0 || ( size_t ) idx >= md_.GenericParameters( ).size( ) )
            out += "T";
        else
            out += md_.GetString( md_.GenericParameters( ) [ idx ].nameIndex );
    }
    out += ">";
    return out;
}

bool Executor::TryFormatDefaultValue( int typeIndex, int dataIndex,
                                      std::string & out ) const {
    if ( typeIndex < 0 || ( size_t ) typeIndex >= bin_.TypeCount( ) )
        return false;
    const auto * t = bin_.Type( typeIndex );
    if ( !t )
        return false;
    size_t remaining = 0;
    const uint8_t * p = md_.DefaultValueData( dataIndex, remaining );
    if ( !p )
        return false;
    const uint8_t * end = p + remaining;
    char buf [ 64 ];

    auto readN = [ & ] ( void * dst, size_t n ) -> bool {
        if ( ( size_t ) ( end - p ) < n ) return false;
        std::memcpy( dst, p, n );
        p += n;
        return true;
    };

    bool v29plus = md_.Version( ) >= 29;

    switch ( t->kind ) {
    case TYPE_BOOLEAN: {
        uint8_t v;
        if ( !readN( &v, 1 ) ) return false;
        out = v ? "true" : "false";
        return true;
    }
    case TYPE_U1: {
        uint8_t v;
        if ( !readN( &v, 1 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%u", v );
        out = buf;
        return true;
    }
    case TYPE_I1: {
        int8_t v;
        if ( !readN( &v, 1 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%d", v );
        out = buf;
        return true;
    }
    case TYPE_CHAR: {
        uint16_t v;
        if ( !readN( &v, 2 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "'\\x%x'", v );
        out = buf;
        return true;
    }
    case TYPE_U2: {
        uint16_t v;
        if ( !readN( &v, 2 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%u", v );
        out = buf;
        return true;
    }
    case TYPE_I2: {
        int16_t v;
        if ( !readN( &v, 2 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%d", v );
        out = buf;
        return true;
    }
    case TYPE_U4: {
        uint32_t v;
        if ( v29plus ) {
            bool ok;
            v = ReadCompressedUInt32( p, end, ok );
            if ( !ok ) return false;
        }
        else if ( !readN( &v, 4 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%u", v );
        out = buf;
        return true;
    }
    case TYPE_I4: {
        int32_t v;
        if ( v29plus ) {
            bool ok;
            v = ReadCompressedInt32( p, end, ok );
            if ( !ok ) return false;
        }
        else if ( !readN( &v, 4 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%d", v );
        out = buf;
        return true;
    }
    case TYPE_U8: {
        uint64_t v;
        if ( !readN( &v, 8 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%lluUL", ( unsigned long long ) v );
        out = buf;
        return true;
    }
    case TYPE_I8: {
        int64_t v;
        if ( !readN( &v, 8 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%lldL", ( long long ) v );
        out = buf;
        return true;
    }
    case TYPE_R4: {
        float v;
        if ( !readN( &v, 4 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%gf", v );
        out = buf;
        return true;
    }
    case TYPE_R8: {
        double v;
        if ( !readN( &v, 8 ) ) return false;
        std::snprintf( buf, sizeof( buf ), "%g", v );
        out = buf;
        return true;
    }
    case TYPE_STRING: {
        int32_t len;
        if ( v29plus ) {
            bool ok;
            len = ReadCompressedInt32( p, end, ok );
            if ( !ok ) return false;
            if ( len == -1 ) { out = "null"; return true; }
        }
        else if ( !readN( &len, 4 ) ) return false;

        if ( len < 0 || ( size_t ) ( end - p ) < ( size_t ) len )
            return false;
        std::string s( ( const char * ) p, ( size_t ) len );
        std::string esc = "\"";
        for ( char c : s ) {
            switch ( c ) {
            case '\\': esc += "\\\\"; break;
            case '"':  esc += "\\\""; break;
            case '\n': esc += "\\n";  break;
            case '\r': esc += "\\r";  break;
            case '\t': esc += "\\t";  break;
            default:
                if ( ( unsigned char ) c < 0x20 ) {
                    char tmp [ 8 ];
                    std::snprintf( tmp, sizeof( tmp ), "\\x%02X",
                                   ( unsigned char ) c );
                    esc += tmp;
                }
                else {
                    esc += c;
                }
            }
        }
        esc += "\"";
        out = std::move( esc );
        return true;
    }
    default:
        return false;
    }
}
