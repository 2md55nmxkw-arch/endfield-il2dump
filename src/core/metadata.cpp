#include <core/metadata.hpp>

#include <cstring>
#include <fstream>
#include <iostream>

namespace {

class Cursor {
public:
    Cursor( const std::vector<uint8_t> & buf, size_t pos = 0 )
        : buf_( buf ), pos_( pos ) { }

    size_t Pos( ) const { return pos_; }
    void   SetPos( size_t p ) { pos_ = p; }

    bool Read( void * dst, size_t n ) {
        if ( pos_ + n > buf_.size( ) ) return false;
        std::memcpy( dst, buf_.data( ) + pos_, n );
        pos_ += n;
        return true;
    }

    template <typename T> bool Read( T & out ) {
        return Read( &out, sizeof( T ) );
    }

    void Skip( size_t n ) { pos_ += n; }

private:
    const std::vector<uint8_t> & buf_;
    size_t pos_;
};

template <typename T>
void ReadRaw( const std::vector<uint8_t> & buf, uint32_t off, int32_t size,
              std::vector<T> & out ) {
    if ( size <= 0 || ( size_t ) ( off + size ) > buf.size( ) ) {
        out.clear( );
        return;
    }
    size_t count = size / sizeof( T );
    out.resize( count );
    std::memcpy( out.data( ), buf.data( ) + off, count * sizeof( T ) );
}

}

bool Metadata::Load( const std::string & path ) {
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

    if ( bytes.size( ) < 8 )
        return false;
    uint32_t sanity = 0;
    int32_t  ver    = 0;
    std::memcpy( &sanity, bytes.data( ),     4 );
    std::memcpy( &ver,    bytes.data( ) + 4, 4 );
    if ( sanity != 0xFAB11BAFu ) {
        std::cerr << "error: invalid metadata sanity 0x" << std::hex << sanity
                  << std::dec << " (expected 0xFAB11BAF)\n";
        return false;
    }
    if ( ver < 24 || ver > 31 ) {
        std::cerr << "error: unsupported metadata version " << ver << "\n";
        return false;
    }
    versionMajor = ver;
    version      = ver;

    ReadHeader( );
    DetectVariant( );
    LoadTables( );
    BuildLookups( );
    return true;
}

void Metadata::ReadHeader( ) {
    Cursor c( bytes, 0 );
    auto V = [ this ] ( double minV, double maxV ) {
        return version >= minV && version <= maxV;
    };

    c.Read( header.sanity );
    c.Read( header.version );
    c.Read( header.stringLiteralOffset );
    c.Read( header.stringLiteralSize );
    c.Read( header.stringLiteralDataOffset );
    c.Read( header.stringLiteralDataSize );
    c.Read( header.stringOffset );
    c.Read( header.stringSize );
    c.Read( header.eventsOffset );
    c.Read( header.eventsSize );
    c.Read( header.propertiesOffset );
    c.Read( header.propertiesSize );
    c.Read( header.methodsOffset );
    c.Read( header.methodsSize );
    c.Read( header.parameterDefaultValuesOffset );
    c.Read( header.parameterDefaultValuesSize );
    c.Read( header.fieldDefaultValuesOffset );
    c.Read( header.fieldDefaultValuesSize );
    c.Read( header.fieldAndParameterDefaultValueDataOffset );
    c.Read( header.fieldAndParameterDefaultValueDataSize );
    c.Read( header.fieldMarshaledSizesOffset );
    c.Read( header.fieldMarshaledSizesSize );
    c.Read( header.parametersOffset );
    c.Read( header.parametersSize );
    c.Read( header.fieldsOffset );
    c.Read( header.fieldsSize );
    c.Read( header.genericParametersOffset );
    c.Read( header.genericParametersSize );
    c.Read( header.genericParameterConstraintsOffset );
    c.Read( header.genericParameterConstraintsSize );
    c.Read( header.genericContainersOffset );
    c.Read( header.genericContainersSize );
    c.Read( header.nestedTypesOffset );
    c.Read( header.nestedTypesSize );
    c.Read( header.interfacesOffset );
    c.Read( header.interfacesSize );
    c.Read( header.vtableMethodsOffset );
    c.Read( header.vtableMethodsSize );
    c.Read( header.interfaceOffsetsOffset );
    c.Read( header.interfaceOffsetsSize );
    c.Read( header.typeDefinitionsOffset );
    c.Read( header.typeDefinitionsSize );

    if ( V( 0, 24.1 ) ) {
        c.Read( header.rgctxEntriesOffset );
        c.Read( header.rgctxEntriesCount );
    }

    c.Read( header.imagesOffset );
    c.Read( header.imagesSize );
    c.Read( header.assembliesOffset );
    c.Read( header.assembliesSize );

    if ( V( 19, 24.5 ) ) {
        c.Read( header.metadataUsageListsOffset );
        c.Read( header.metadataUsageListsCount );
        c.Read( header.metadataUsagePairsOffset );
        c.Read( header.metadataUsagePairsCount );
    }

    c.Read( header.fieldRefsOffset );
    c.Read( header.fieldRefsSize );

    c.Read( header.referencedAssembliesOffset );
    c.Read( header.referencedAssembliesSize );

    if ( V( 21, 27.2 ) ) {
        c.Read( header.attributesInfoOffset );
        c.Read( header.attributesInfoCount );
        c.Read( header.attributeTypesOffset );
        c.Read( header.attributeTypesCount );
    }

    if ( V( 29, 100 ) ) {
        c.Read( header.attributeDataOffset );
        c.Read( header.attributeDataSize );
        c.Read( header.attributeDataRangeOffset );
        c.Read( header.attributeDataRangeSize );
    }

    c.Read( header.unresolvedVirtualCallParameterTypesOffset );
    c.Read( header.unresolvedVirtualCallParameterTypesSize );
    c.Read( header.unresolvedVirtualCallParameterRangesOffset );
    c.Read( header.unresolvedVirtualCallParameterRangesSize );
    c.Read( header.windowsRuntimeTypeNamesOffset );
    c.Read( header.windowsRuntimeTypeNamesSize );

    if ( V( 27, 100 ) ) {
        c.Read( header.windowsRuntimeStringsOffset );
        c.Read( header.windowsRuntimeStringsSize );
    }

    c.Read( header.exportedTypeDefinitionsOffset );
    c.Read( header.exportedTypeDefinitionsSize );
}

void Metadata::DetectVariant( ) {
    if ( versionMajor != 24 )
        return;
    if ( header.stringLiteralOffset == 264 )
        version = 24.2;
}

void Metadata::LoadTables( ) {
    auto V = [ this ] ( double minV, double maxV ) {
        return version >= minV && version <= maxV;
    };

    {
        size_t totalBytes = ( size_t ) header.imagesSize;
        size_t imageRecSize = 40;
        if ( totalBytes % imageRecSize != 0 ) {
            if ( totalBytes % 32 == 0 )      imageRecSize = 32;
            else if ( totalBytes % 24 == 0 ) imageRecSize = 24;
        }

        size_t count = totalBytes / imageRecSize;
        images.resize( count );
        for ( size_t i = 0; i < count; ++i ) {
            ImageDef & d = images [ i ];
            Cursor cc( bytes, ( size_t ) header.imagesOffset + i * imageRecSize );
            cc.Read( d.nameIndex );
            cc.Read( d.assemblyIndex );
            cc.Read( d.typeStart );
            cc.Read( d.typeCount );
            if ( imageRecSize >= 32 ) {
                cc.Read( d.exportedTypeStart );
                cc.Read( d.exportedTypeCount );
            }
            cc.Read( d.entryPointIndex );
            cc.Read( d.token );
            if ( imageRecSize >= 40 ) {
                cc.Read( d.customAttributeStart );
                cc.Read( d.customAttributeCount );
            }
        }

        if ( versionMajor == 24 && version == 24 ) {
            for ( const auto & d : images ) {
                if ( d.token != 1 ) {
                    version = 24.1;
                    break;
                }
            }
        }
    }

    {
        size_t anNameSize = 4 + 4;
        bool hasHashIdx = ( version <= 24.3 );
        if ( hasHashIdx ) anNameSize += 4;
        anNameSize += 4 + 4 + 4 + 4 + 4 + 4 + 4 + 4 + 8;

        size_t hdr = 4;
        if ( version >= 24.1 ) hdr += 4;
        if ( version <= 24 )   hdr += 4;
        if ( version >= 20 )   hdr += 8;
        size_t recSize = hdr + anNameSize;

        size_t totalBytes = ( size_t ) header.assembliesSize;
        if ( !recSize ) recSize = totalBytes;
        size_t count = totalBytes / recSize;
        assemblies.resize( count );

        for ( size_t i = 0; i < count; ++i ) {
            AssemblyDef & a = assemblies [ i ];
            Cursor c( bytes, ( size_t ) header.assembliesOffset + i * recSize );
            c.Read( a.imageIndex );
            if ( version >= 24.1 ) c.Read( a.token );
            if ( version <= 24 )   c.Read( a.customAttributeIndex );
            if ( version >= 20 ) {
                c.Read( a.referencedAssemblyStart );
                c.Read( a.referencedAssemblyCount );
            }
            c.Read( a.aname.nameIndex );
            c.Read( a.aname.cultureIndex );
            if ( hasHashIdx ) c.Read( a.aname.hashValueIndex );
            else              a.aname.hashValueIndex = -1;
            c.Read( a.aname.publicKeyIndex );
            c.Read( a.aname.hash_alg );
            c.Read( a.aname.hash_len );
            c.Read( a.aname.flags );
            c.Read( a.aname.major );
            c.Read( a.aname.minor );
            c.Read( a.aname.build );
            c.Read( a.aname.revision );
            c.Read( a.aname.public_key_token );
        }
    }

    {
        size_t recSize = 4 + 4;
        if ( version <= 24 )   recSize += 4;
        recSize += 4;
        if ( version <= 24.5 ) recSize += 4;
        recSize += 4 + 4 + 4;
        if ( version <= 24.1 ) recSize += 4 + 4;
        recSize += 4;
        if ( version <= 22 )                        recSize += 4 + 4;
        if ( version >= 21 && version <= 22 )       recSize += 4 + 4;
        recSize += 4;
        recSize += 8 * 4;
        if ( version >= 29 )                        recSize += 4;
        recSize += 8 * 2;
        recSize += 4;
        recSize += 4;

        size_t totalBytes = ( size_t ) header.typeDefinitionsSize;
        size_t count = recSize ? totalBytes / recSize : 0;
        types.resize( count );

        for ( size_t i = 0; i < count; ++i ) {
            TypeDef & t = types [ i ];
            Cursor c( bytes, ( size_t ) header.typeDefinitionsOffset + i * recSize );
            c.Read( t.nameIndex );
            c.Read( t.namespaceIndex );
            if ( version <= 24 ) c.Read( t.customAttributeIndex );
            else                 t.customAttributeIndex = -1;
            c.Read( t.byvalTypeIndex );
            if ( version <= 24.5 ) c.Read( t.byrefTypeIndex );
            else                   t.byrefTypeIndex = -1;
            c.Read( t.declaringTypeIndex );
            c.Read( t.parentIndex );
            c.Read( t.elementTypeIndex );
            if ( version <= 24.1 ) {
                c.Read( t.rgctxStartIndex );
                c.Read( t.rgctxCount );
            }
            else {
                t.rgctxStartIndex = 0;
                t.rgctxCount = 0;
            }
            c.Read( t.genericContainerIndex );
            if ( version <= 22 )                  c.Skip( 4 + 4 );
            if ( version >= 21 && version <= 22 ) c.Skip( 4 + 4 );
            c.Read( t.flags );
            c.Read( t.fieldStart );
            c.Read( t.methodStart );
            c.Read( t.eventStart );
            c.Read( t.propertyStart );
            c.Read( t.nestedTypesStart );
            c.Read( t.interfacesStart );
            c.Read( t.vtableStart );
            c.Read( t.interfaceOffsetsStart );
            if ( version >= 29 ) c.Skip( 4 );
            c.Read( t.method_count );
            c.Read( t.property_count );
            c.Read( t.field_count );
            c.Read( t.event_count );
            c.Read( t.nested_type_count );
            c.Read( t.vtable_count );
            c.Read( t.interfaces_count );
            c.Read( t.interface_offsets_count );
            c.Read( t.bitfield );
            c.Read( t.token );
        }
    }

    {
        size_t recSize = 4 + 4 + 4;
        if ( version >= 31 ) recSize += 4;
        recSize += 4;
        if ( version <= 24 ) recSize += 4;
        recSize += 4;
        if ( version <= 24.1 ) recSize += 5 * 4;
        recSize += 4;
        recSize += 4 * 2;

        size_t totalBytes = ( size_t ) header.methodsSize;
        size_t count = recSize ? totalBytes / recSize : 0;
        methods.resize( count );

        for ( size_t i = 0; i < count; ++i ) {
            MethodDef & m = methods [ i ];
            Cursor c( bytes, ( size_t ) header.methodsOffset + i * recSize );
            c.Read( m.nameIndex );
            c.Read( m.declaringType );
            c.Read( m.returnType );
            if ( version >= 31 ) c.Read( m.returnParameterToken );
            c.Read( m.parameterStart );
            if ( version <= 24 ) c.Read( m.customAttributeIndex );
            else                 m.customAttributeIndex = -1;
            c.Read( m.genericContainerIndex );
            if ( version <= 24.1 ) {
                c.Read( m.methodIndex );
                c.Read( m.invokerIndex );
                c.Read( m.delegateWrapperIndex );
                c.Read( m.rgctxStartIndex );
                c.Read( m.rgctxCount );
            }
            else {
                m.methodIndex = -1;
            }
            c.Read( m.token );
            c.Read( m.flags );
            c.Read( m.iflags );
            c.Read( m.slot );
            c.Read( m.parameterCount );
        }
    }

    {
        size_t recSize = 4 + 4 + ( version <= 24 ? 4 : 0 ) + 4;
        size_t totalBytes = ( size_t ) header.parametersSize;
        size_t count = recSize ? totalBytes / recSize : 0;
        parameters.resize( count );
        for ( size_t i = 0; i < count; ++i ) {
            ParameterDef & p = parameters [ i ];
            Cursor c( bytes, ( size_t ) header.parametersOffset + i * recSize );
            c.Read( p.nameIndex );
            c.Read( p.token );
            if ( version <= 24 ) c.Read( p.customAttributeIndex );
            else                 p.customAttributeIndex = -1;
            c.Read( p.typeIndex );
        }
    }

    {
        size_t recSize = 4 + 4 + ( version <= 24 ? 4 : 0 ) + 4;
        size_t totalBytes = ( size_t ) header.fieldsSize;
        size_t count = recSize ? totalBytes / recSize : 0;
        fields.resize( count );
        for ( size_t i = 0; i < count; ++i ) {
            FieldDef & d = fields [ i ];
            Cursor c( bytes, ( size_t ) header.fieldsOffset + i * recSize );
            c.Read( d.nameIndex );
            c.Read( d.typeIndex );
            if ( version <= 24 ) c.Read( d.customAttributeIndex );
            else                 d.customAttributeIndex = -1;
            c.Read( d.token );
        }
    }

    ReadRaw<FieldDefaultValue>( bytes, header.fieldDefaultValuesOffset,
                                header.fieldDefaultValuesSize, fieldDefaultValues );
    ReadRaw<ParameterDefaultValue>( bytes, header.parameterDefaultValuesOffset,
                                    header.parameterDefaultValuesSize,
                                    parameterDefaultValues );

    {
        size_t recSize = 4 + 4 + 4 + 4 + ( version <= 24 ? 4 : 0 ) + 4;
        size_t totalBytes = ( size_t ) header.propertiesSize;
        size_t count = recSize ? totalBytes / recSize : 0;
        properties.resize( count );
        for ( size_t i = 0; i < count; ++i ) {
            PropertyDef & p = properties [ i ];
            Cursor c( bytes, ( size_t ) header.propertiesOffset + i * recSize );
            c.Read( p.nameIndex );
            c.Read( p.get );
            c.Read( p.set );
            c.Read( p.attrs );
            if ( version <= 24 ) c.Read( p.customAttributeIndex );
            else                 p.customAttributeIndex = -1;
            c.Read( p.token );
        }
    }

    {
        size_t recSize = 4 + 4 + 4 + 4 + 4 + ( version <= 24 ? 4 : 0 ) + 4;
        size_t totalBytes = ( size_t ) header.eventsSize;
        size_t count = recSize ? totalBytes / recSize : 0;
        events.resize( count );
        for ( size_t i = 0; i < count; ++i ) {
            EventDef & e = events [ i ];
            Cursor c( bytes, ( size_t ) header.eventsOffset + i * recSize );
            c.Read( e.nameIndex );
            c.Read( e.typeIndex );
            c.Read( e.add );
            c.Read( e.remove );
            c.Read( e.raise );
            if ( version <= 24 ) c.Read( e.customAttributeIndex );
            else                 e.customAttributeIndex = -1;
            c.Read( e.token );
        }
    }

    ReadRaw<GenericContainer>( bytes, header.genericContainersOffset,
                               header.genericContainersSize, genericContainers );
    ReadRaw<GenericParameter>( bytes, header.genericParametersOffset,
                               header.genericParametersSize, genericParameters );

    ReadRaw<int32_t>( bytes, header.interfacesOffset, header.interfacesSize,
                      interfaceIndices );
    ReadRaw<int32_t>( bytes, header.nestedTypesOffset, header.nestedTypesSize,
                      nestedTypeIndices );

    ReadRaw<StringLiteral>( bytes, header.stringLiteralOffset,
                            header.stringLiteralSize, stringLiterals );

    if ( version >= 21 && version < 29 ) {
        size_t recSize = ( version >= 24.1 ) ? ( 4 + 4 + 4 ) : ( 4 + 4 );
        size_t totalBytes = ( size_t ) header.attributesInfoCount;
        size_t count = recSize ? totalBytes / recSize : 0;
        attributeTypeRanges.resize( count );
        for ( size_t i = 0; i < count; ++i ) {
            CustomAttributeTypeRange & r = attributeTypeRanges [ i ];
            Cursor c( bytes, ( size_t ) header.attributesInfoOffset + i * recSize );
            if ( version >= 24.1 ) c.Read( r.token );
            else                   r.token = 0;
            c.Read( r.start );
            c.Read( r.count );
        }
        ReadRaw<int32_t>( bytes, header.attributeTypesOffset,
                          header.attributeTypesCount, attributeTypes );
    }
    if ( version >= 29 ) {
        ReadRaw<CustomAttributeDataRange>( bytes, header.attributeDataRangeOffset,
                                           header.attributeDataRangeSize,
                                           attributeDataRanges );
    }
}

void Metadata::BuildLookups( ) {
    fieldDefaultValueByField.reserve( fieldDefaultValues.size( ) );
    for ( const auto & v : fieldDefaultValues )
        fieldDefaultValueByField [ v.fieldIndex ] = v;

    parameterDefaultValueByParam.reserve( parameterDefaultValues.size( ) );
    for ( const auto & v : parameterDefaultValues )
        parameterDefaultValueByParam [ v.parameterIndex ] = v;

    if ( version > 24 ) {
        attributeIndexByImageToken.resize( images.size( ) );
        for ( size_t i = 0; i < images.size( ); ++i ) {
            const auto & img = images [ i ];
            auto & dic = attributeIndexByImageToken [ i ];
            int end = img.customAttributeStart + ( int ) img.customAttributeCount;
            for ( int idx = img.customAttributeStart; idx < end; ++idx ) {
                if ( version >= 29 ) {
                    if ( idx >= 0 && ( size_t ) idx < attributeDataRanges.size( ) )
                        dic [ attributeDataRanges [ idx ].token ] = idx;
                }
                else {
                    if ( idx >= 0 && ( size_t ) idx < attributeTypeRanges.size( ) )
                        dic [ attributeTypeRanges [ idx ].token ] = idx;
                }
            }
        }
    }
}

std::string Metadata::GetString( uint32_t index ) const {
    size_t at = ( size_t ) header.stringOffset + index;
    if ( at >= bytes.size( ) )
        return "";
    std::string out;
    out.reserve( 32 );
    while ( at < bytes.size( ) && bytes [ at ] != 0 ) {
        out.push_back( ( char ) bytes [ at ] );
        ++at;
    }
    return out;
}

std::string Metadata::GetStringLiteral( uint32_t index ) const {
    if ( index >= stringLiterals.size( ) )
        return "";
    const auto & lit = stringLiterals [ index ];
    size_t at = ( size_t ) header.stringLiteralDataOffset + lit.dataIndex;
    if ( at + lit.length > bytes.size( ) )
        return "";
    return std::string( ( const char * ) ( bytes.data( ) + at ), lit.length );
}

bool Metadata::TryGetFieldDefault( int fieldIndex, FieldDefaultValue & out ) const {
    auto it = fieldDefaultValueByField.find( fieldIndex );
    if ( it == fieldDefaultValueByField.end( ) ) return false;
    out = it->second;
    return true;
}

bool Metadata::TryGetParameterDefault( int paramIndex,
                                       ParameterDefaultValue & out ) const {
    auto it = parameterDefaultValueByParam.find( paramIndex );
    if ( it == parameterDefaultValueByParam.end( ) ) return false;
    out = it->second;
    return true;
}

int Metadata::GetCustomAttributeIndex( const ImageDef & img,
                                       int customAttributeIndex,
                                       uint32_t token ) const {
    if ( version > 24 ) {
        size_t i = ( size_t ) ( &img - images.data( ) );
        if ( i >= attributeIndexByImageToken.size( ) ) return -1;
        const auto & dic = attributeIndexByImageToken [ i ];
        auto it = dic.find( token );
        return it == dic.end( ) ? -1 : it->second;
    }
    return customAttributeIndex;
}

const uint8_t * Metadata::DefaultValueData( int dataIndex,
                                            size_t & remaining ) const {
    size_t at = ( size_t ) header.fieldAndParameterDefaultValueDataOffset + dataIndex;
    if ( at >= bytes.size( ) ) {
        remaining = 0;
        return nullptr;
    }
    remaining = bytes.size( ) - at;
    return bytes.data( ) + at;
}
