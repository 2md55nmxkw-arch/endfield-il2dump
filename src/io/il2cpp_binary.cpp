#include <io/il2cpp_binary.hpp>

#include <algorithm>
#include <cstring>
#include <iostream>

namespace {

enum FieldTag {
    F_METHODPTR_COUNT,
    F_METHODPTRS,
    F_REVPINVOKE_COUNT,
    F_REVPINVOKE,
    F_GENERIC_METHODPTRS_COUNT,
    F_GENERIC_METHODPTRS,
    F_GENERIC_ADJUSTOR_THUNKS,
    F_INVOKER_COUNT,
    F_INVOKERS,
    F_CUSTOM_ATTR_COUNT,
    F_CUSTOM_ATTR_GENS,
    F_UNRESOLVED_VC_COUNT,
    F_UNRESOLVED_VC,
    F_UNRESOLVED_INSTANCE,
    F_UNRESOLVED_STATIC,
    F_INTEROP_COUNT,
    F_INTEROP_DATA,
    F_WINRT_FACTORY_COUNT,
    F_WINRT_FACTORY_TABLE,
    F_CODEGEN_MODULES_COUNT,
    F_CODEGEN_MODULES,
};

std::vector<FieldTag> CodeRegLayout( double v ) {
    std::vector<FieldTag> r;
    if ( v <= 24.1 ) {
        r.push_back( F_METHODPTR_COUNT );
        r.push_back( F_METHODPTRS );
    }
    if ( v >= 22 ) {
        r.push_back( F_REVPINVOKE_COUNT );
        r.push_back( F_REVPINVOKE );
    }
    r.push_back( F_GENERIC_METHODPTRS_COUNT );
    r.push_back( F_GENERIC_METHODPTRS );
    if ( v == 24.5 || v >= 27.1 )
        r.push_back( F_GENERIC_ADJUSTOR_THUNKS );
    r.push_back( F_INVOKER_COUNT );
    r.push_back( F_INVOKERS );
    if ( v <= 24.5 ) {
        r.push_back( F_CUSTOM_ATTR_COUNT );
        r.push_back( F_CUSTOM_ATTR_GENS );
    }
    if ( v >= 22 ) {
        r.push_back( F_UNRESOLVED_VC_COUNT );
        r.push_back( F_UNRESOLVED_VC );
    }
    if ( v >= 29.1 ) {
        r.push_back( F_UNRESOLVED_INSTANCE );
        r.push_back( F_UNRESOLVED_STATIC );
    }
    if ( v >= 23 ) {
        r.push_back( F_INTEROP_COUNT );
        r.push_back( F_INTEROP_DATA );
    }
    if ( v >= 24.3 ) {
        r.push_back( F_WINRT_FACTORY_COUNT );
        r.push_back( F_WINRT_FACTORY_TABLE );
    }
    if ( v >= 24.2 ) {
        r.push_back( F_CODEGEN_MODULES_COUNT );
        r.push_back( F_CODEGEN_MODULES );
    }
    return r;
}

void FindAll( const std::vector<uint8_t> & buf, size_t start, size_t end,
              const std::vector<uint8_t> & needle,
              std::vector<size_t> & hits ) {
    if ( needle.empty( ) || end <= start )
        return;
    for ( size_t i = start; i + needle.size( ) <= end; ++i ) {
        if ( std::memcmp( buf.data( ) + i, needle.data( ), needle.size( ) ) == 0 )
            hits.push_back( i );
    }
}

}

bool Il2CppBinary::ReadCodeRegistration( uint64_t codeRegVA ) {
    auto layout = CodeRegLayout( md_.Version( ) );
    std::vector<uint64_t> ptrs;
    if ( !ReadPointersVA( codeRegVA, layout.size( ), ptrs ) ) {
        std::cerr << "error: failed to read CodeRegistration\n";
        return false;
    }
    for ( size_t i = 0; i < layout.size( ); ++i ) {
        uint64_t v = ptrs [ i ];
        switch ( layout [ i ] ) {
        case F_METHODPTR_COUNT:           code_.methodPointersCount = v; break;
        case F_METHODPTRS:                code_.methodPointers = v; break;
        case F_REVPINVOKE_COUNT:          code_.reversePInvokeWrapperCount = v; break;
        case F_REVPINVOKE:                code_.reversePInvokeWrappers = v; break;
        case F_GENERIC_METHODPTRS_COUNT:  code_.genericMethodPointersCount = v; break;
        case F_GENERIC_METHODPTRS:        code_.genericMethodPointers = v; break;
        case F_GENERIC_ADJUSTOR_THUNKS:   code_.genericAdjustorThunks = v; break;
        case F_INVOKER_COUNT:             code_.invokerPointersCount = v; break;
        case F_INVOKERS:                  code_.invokerPointers = v; break;
        case F_CUSTOM_ATTR_COUNT:         code_.customAttributeCount = v; break;
        case F_CUSTOM_ATTR_GENS:          code_.customAttributeGenerators = v; break;
        case F_UNRESOLVED_VC_COUNT:       code_.unresolvedVirtualCallCount = v; break;
        case F_UNRESOLVED_VC:             code_.unresolvedVirtualCallPointers = v; break;
        case F_INTEROP_COUNT:             code_.interopDataCount = v; break;
        case F_INTEROP_DATA:              code_.interopData = v; break;
        case F_WINRT_FACTORY_COUNT:       code_.windowsRuntimeFactoryCount = v; break;
        case F_WINRT_FACTORY_TABLE:       code_.windowsRuntimeFactoryTable = v; break;
        case F_CODEGEN_MODULES_COUNT:     code_.codeGenModulesCount = v; break;
        case F_CODEGEN_MODULES:           code_.codeGenModules = v; break;
        default: break;
        }
    }
    return true;
}

bool Il2CppBinary::ReadMetadataRegistration( uint64_t metaRegVA ) {
    size_t fieldCount = ( md_.Version( ) >= 19 ) ? 16 : 14;
    std::vector<uint64_t> p;
    if ( !ReadPointersVA( metaRegVA, fieldCount, p ) ) {
        std::cerr << "error: failed to read MetadataRegistration\n";
        return false;
    }
    metaReg_.genericClassesCount       = ( int64_t ) p [ 0 ];
    metaReg_.genericClasses            = p [ 1 ];
    metaReg_.genericInstsCount         = ( int64_t ) p [ 2 ];
    metaReg_.genericInsts              = p [ 3 ];
    metaReg_.genericMethodTableCount   = ( int64_t ) p [ 4 ];
    metaReg_.genericMethodTable        = p [ 5 ];
    metaReg_.typesCount                = ( int64_t ) p [ 6 ];
    metaReg_.types                     = p [ 7 ];
    metaReg_.methodSpecsCount          = ( int64_t ) p [ 8 ];
    metaReg_.methodSpecs               = p [ 9 ];
    metaReg_.fieldOffsetsCount         = ( int64_t ) p [ 10 ];
    metaReg_.fieldOffsets              = p [ 11 ];
    metaReg_.typeDefinitionsSizesCount = ( int64_t ) p [ 12 ];
    metaReg_.typeDefinitionsSizes      = p [ 13 ];
    if ( fieldCount >= 16 ) {
        metaReg_.metadataUsagesCount = p [ 14 ];
        metaReg_.metadataUsages      = p [ 15 ];
    }
    return true;
}

void Il2CppBinary::DecodeType( const uint8_t buf [ 12 ], Il2CppTypeRec & out ) const {
    uint64_t datapoint = 0;
    uint32_t bits = 0;
    std::memcpy( &datapoint, buf, 8 );
    std::memcpy( &bits, buf + 8, 4 );

    out.data  = datapoint;
    out.attrs = bits & 0xFFFFu;
    out.kind  = ( bits >> 16 ) & 0xFFu;
    if ( md_.Version( ) >= 27.2 ) {
        out.num_mods  = ( bits >> 24 ) & 0x1F;
        out.byref     = ( bits >> 29 ) & 1;
        out.pinned    = ( bits >> 30 ) & 1;
        out.valuetype = bits >> 31;
    }
    else {
        out.num_mods  = ( bits >> 24 ) & 0x3F;
        out.byref     = ( bits >> 30 ) & 1;
        out.pinned    = bits >> 31;
        out.valuetype = 0;
    }
}

bool Il2CppBinary::ReadTypes( ) {
    if ( !metaReg_.types || metaReg_.typesCount <= 0 )
        return false;
    std::vector<uint64_t> typePtrs;
    if ( !ReadPointersVA( metaReg_.types, ( size_t ) metaReg_.typesCount, typePtrs ) )
        return false;

    types_.resize( typePtrs.size( ) );
    typeIndexByVA_.reserve( typePtrs.size( ) * 2 );
    for ( size_t i = 0; i < typePtrs.size( ); ++i ) {
        uint8_t buf [ 12 ] = { 0 };
        if ( !pe_.ReadVA( typePtrs [ i ], buf, sizeof( buf ) ) )
            continue;
        DecodeType( buf, types_ [ i ] );
        typeIndexByVA_ [ typePtrs [ i ] ] = ( int ) i;
    }
    return true;
}

bool Il2CppBinary::ReadFieldOffsets( ) {
    if ( !metaReg_.fieldOffsets || metaReg_.fieldOffsetsCount <= 0 )
        return false;
    fieldOffsetsArePointers_ = md_.Version( ) > 21;
    if ( fieldOffsetsArePointers_ ) {
        return ReadPointersVA( metaReg_.fieldOffsets,
                               ( size_t ) metaReg_.fieldOffsetsCount,
                               fieldOffsetTablePtrs_ );
    }
    fieldOffsetTablePtrs_.resize( ( size_t ) metaReg_.fieldOffsetsCount );
    std::vector<uint32_t> tmp( ( size_t ) metaReg_.fieldOffsetsCount );
    if ( !pe_.ReadVA( metaReg_.fieldOffsets, tmp.data( ),
                      tmp.size( ) * sizeof( uint32_t ) ) )
        return false;
    for ( size_t i = 0; i < tmp.size( ); ++i )
        fieldOffsetTablePtrs_ [ i ] = tmp [ i ];
    return true;
}

bool Il2CppBinary::ReadTypeDefSizes( ) {
    if ( !metaReg_.typeDefinitionsSizes || metaReg_.typeDefinitionsSizesCount <= 0 )
        return false;
    std::vector<uint64_t> ptrs;
    if ( !ReadPointersVA( metaReg_.typeDefinitionsSizes,
                          ( size_t ) metaReg_.typeDefinitionsSizesCount, ptrs ) )
        return false;
    typeSizes_.assign( ptrs.size( ), 0 );
    for ( size_t i = 0; i < ptrs.size( ); ++i ) {
        if ( !ptrs [ i ] ) continue;
        uint32_t sz = 0;
        pe_.ReadVA( ptrs [ i ], sz );
        typeSizes_ [ i ] = ( int32_t ) sz;
    }
    return true;
}

bool Il2CppBinary::ReadCodeGenModules( ) {
    if ( md_.Version( ) < 24.2 || !code_.codeGenModules ||
         code_.codeGenModulesCount == 0 ) {
        if ( code_.methodPointers && code_.methodPointersCount )
            ReadPointersVA( code_.methodPointers,
                            ( size_t ) code_.methodPointersCount,
                            methodPointers_ );
        return true;
    }

    std::vector<uint64_t> modulePtrs;
    if ( !ReadPointersVA( code_.codeGenModules, ( size_t ) code_.codeGenModulesCount,
                          modulePtrs ) )
        return false;

    auto v = md_.Version( );
    size_t fieldCount = 13;
    if ( v >= 27 && v <= 27.2 ) fieldCount += 1;
    if ( v >= 27 )              fieldCount += 4;

    for ( size_t i = 0; i < modulePtrs.size( ); ++i ) {
        std::vector<uint64_t> fields;
        if ( !ReadPointersVA( modulePtrs [ i ], fieldCount, fields ) )
            continue;

        CodeGenModule m;
        size_t k = 0;
        m.moduleName         = fields [ k++ ];
        m.methodPointerCount = ( int64_t ) fields [ k++ ];
        m.methodPointers     = fields [ k++ ];
        if ( v == 24.5 || v >= 27.1 ) {
            m.adjustorThunkCount = ( int64_t ) fields [ k++ ];
            m.adjustorThunks     = fields [ k++ ];
        }
        m.invokerIndices               = fields [ k++ ];
        m.reversePInvokeWrapperCount   = fields [ k++ ];
        m.reversePInvokeWrapperIndices = fields [ k++ ];
        m.rgctxRangesCount             = ( int64_t ) fields [ k++ ];
        m.rgctxRanges                  = fields [ k++ ];
        m.rgctxsCount                  = ( int64_t ) fields [ k++ ];
        m.rgctxs                       = fields [ k++ ];
        if ( k < fields.size( ) )
            m.debuggerMetadata = fields [ k++ ];

        std::string name;
        pe_.ReadCStringVA( m.moduleName, name );

        std::vector<uint64_t> mp;
        if ( m.methodPointers && m.methodPointerCount > 0 )
            ReadPointersVA( m.methodPointers, ( size_t ) m.methodPointerCount, mp );

        codeGenModules_ [ name ] = m;
        codeGenModuleMethodPointers_ [ name ] = std::move( mp );
    }
    return true;
}

uint64_t Il2CppBinary::GetMethodVA( const std::string & imageName,
                                    const MethodDef & method ) const {
    ResolveStatus s;
    return ResolveMethodVA( imageName, method, s );
}

uint64_t Il2CppBinary::ResolveMethodVA( const std::string & imageName,
                                        const MethodDef & method,
                                        ResolveStatus & status ) const {
    if ( md_.Version( ) >= 24.2 ) {
        auto it = codeGenModuleMethodPointers_.find( imageName );
        if ( it == codeGenModuleMethodPointers_.end( ) ) {
            status = ResolveStatus::NoModule;
            return 0;
        }
        const auto & ptrs = it->second;
        uint32_t idx = method.token & 0x00FFFFFFu;
        if ( idx == 0 ) {
            status = ResolveStatus::ZeroToken;
            return 0;
        }
        size_t i = idx - 1;
        if ( i >= ptrs.size( ) ) {
            status = ResolveStatus::TokenOutOfRange;
            return 0;
        }
        uint64_t va = ptrs [ i ];
        if ( !va ) {
            status = ResolveStatus::NullPointer;
            return 0;
        }
        status = ResolveStatus::Ok;
        return va;
    }

    if ( method.methodIndex < 0 ||
         ( size_t ) method.methodIndex >= methodPointers_.size( ) ) {
        status = ResolveStatus::TokenOutOfRange;
        return 0;
    }
    uint64_t va = methodPointers_ [ method.methodIndex ];
    if ( !va ) {
        status = ResolveStatus::NullPointer;
        return 0;
    }
    status = ResolveStatus::Ok;
    return va;
}

const std::vector<Il2CppBinary::GenericInstance> &
Il2CppBinary::InstancesForMethod( int methodDefIndex ) const {
    auto it = instancesByMethodDef_.find( methodDefIndex );
    if ( it == instancesByMethodDef_.end( ) )
        return emptyInstances_;
    static thread_local std::vector<GenericInstance> tmp;
    tmp.clear( );
    tmp.reserve( it->second.size( ) );
    for ( int i : it->second )
        tmp.push_back( genericInstances_ [ i ] );
    return tmp;
}

int32_t Il2CppBinary::FieldOffset( int typeDefIndex, int fieldIndexInType,
                                   bool isValueType, bool isStatic ) const {
    if ( typeDefIndex < 0 ||
         ( size_t ) typeDefIndex >= fieldOffsetTablePtrs_.size( ) )
        return -1;
    if ( !fieldOffsetsArePointers_ )
        return -1;
    uint64_t va = fieldOffsetTablePtrs_ [ typeDefIndex ];
    if ( !va ) return -1;
    uint32_t off = 0;
    if ( !pe_.ReadVA( va + 4ull * ( uint32_t ) fieldIndexInType, off ) )
        return -1;
    int32_t signed_off = ( int32_t ) off;
    if ( signed_off > 0 && isValueType && !isStatic )
        signed_off -= 16;
    return signed_off;
}

const Il2CppTypeRec * Il2CppBinary::TypeFromGenericClassPtr( uint64_t va ) const {
    uint64_t typePtr = 0;
    if ( md_.Version( ) >= 27 ) {
        if ( !pe_.ReadVA( va, typePtr ) )
            return nullptr;
    }
    else {
        return nullptr;
    }
    return TypeFromVA( typePtr );
}

const Il2CppTypeRec * Il2CppBinary::TypeFromVA( uint64_t va ) const {
    auto it = typeIndexByVA_.find( va );
    if ( it == typeIndexByVA_.end( ) ) return nullptr;
    return &types_ [ it->second ];
}

bool Il2CppBinary::ReadGenericInstArgs(
    uint64_t genericClassVA,
    std::vector<const Il2CppTypeRec *> & out ) const {
    out.clear( );
    if ( md_.Version( ) < 27 )
        return false;
    uint64_t classInstVA = 0;
    if ( !pe_.ReadVA( genericClassVA + 8, classInstVA ) )
        return false;
    if ( !classInstVA )
        return false;
    int64_t  argc = 0;
    uint64_t argv = 0;
    if ( !pe_.ReadVA( classInstVA, argc ) )
        return false;
    if ( !pe_.ReadVA( classInstVA + 8, argv ) )
        return false;
    if ( argc <= 0 || argc > 64 )
        return false;
    std::vector<uint64_t> ptrs;
    if ( !ReadPointersVA( argv, ( size_t ) argc, ptrs ) )
        return false;
    out.reserve( ptrs.size( ) );
    for ( uint64_t p : ptrs )
        out.push_back( TypeFromVA( p ) );
    return true;
}

uint64_t Il2CppBinary::CustomAttributeGeneratorVA( int attributeIndex ) const {
    if ( attributeIndex < 0 ||
         ( size_t ) attributeIndex >= customAttributeGenerators_.size( ) )
        return 0;
    return customAttributeGenerators_ [ attributeIndex ];
}

bool Il2CppBinary::ReadPointersVA( uint64_t va, size_t count,
                                   std::vector<uint64_t> & out ) const {
    out.assign( count, 0 );
    if ( !va || !count )
        return true;
    return pe_.ReadVA( va, out.data( ), count * sizeof( uint64_t ) );
}

bool Il2CppBinary::LocateRegistrations( uint64_t & codeReg, uint64_t & metaReg ) {
    auto vaOfFile = [ & ] ( size_t f ) { return pe_.MapFileToVA( f ); };

    std::vector<PeSection> dataSections;
    for ( const auto & s : pe_.Sections( ) ) {
        if ( s.IsRead( ) && !s.IsExec( ) )
            dataSections.push_back( s );
    }

    static const std::vector<uint8_t> needle = {
        'm','s','c','o','r','l','i','b','.','d','l','l',0
    };

    std::vector<uint64_t> mscorlibVAs;
    for ( const auto & s : dataSections ) {
        std::vector<size_t> hits;
        FindAll( pe_.Bytes( ), s.pointerToRawData,
                 s.pointerToRawData + s.sizeOfRawData, needle, hits );
        for ( auto h : hits ) {
            uint64_t va = vaOfFile( h );
            if ( va ) mscorlibVAs.push_back( va );
        }
    }
    if ( mscorlibVAs.empty( ) ) {
        std::cerr << "error: mscorlib.dll string not found\n";
        return false;
    }

    auto findRefs = [ & ] ( uint64_t target, std::vector<uint64_t> & out ) {
        for ( const auto & s : dataSections ) {
            size_t end = s.pointerToRawData + s.sizeOfRawData;
            for ( size_t p = s.pointerToRawData; p + 8 <= end; p += 8 ) {
                uint64_t v = 0;
                std::memcpy( &v, pe_.Bytes( ).data( ) + p, 8 );
                if ( v == target ) {
                    uint64_t va = vaOfFile( p );
                    if ( va ) out.push_back( va );
                }
            }
        }
    };

    int imageCount = ( int ) md_.Images( ).size( );
    if ( imageCount <= 0 )
        return false;

    bool found = false;
    for ( uint64_t mscorVa : mscorlibVAs ) {
        std::vector<uint64_t> refs1;
        findRefs( mscorVa, refs1 );
        for ( uint64_t ref1 : refs1 ) {
            std::vector<uint64_t> refs2;
            findRefs( ref1, refs2 );
            for ( uint64_t ref2 : refs2 ) {
                if ( md_.Version( ) >= 27 ) {
                    for ( int i = imageCount - 1; i >= 0 && !found; --i ) {
                        std::vector<uint64_t> refs3;
                        findRefs( ref2 - ( uint64_t ) i * 8, refs3 );
                        for ( uint64_t ref3 : refs3 ) {
                            uint64_t probe = 0;
                            if ( !pe_.ReadVA( ref3 - 8, probe ) ) continue;
                            if ( probe == ( uint64_t ) imageCount ) {
                                if ( md_.Version( ) >= 29 )
                                    codeReg = ref3 - 8 * 14;
                                else
                                    codeReg = ref3 - 8 * 13;
                                found = true;
                                break;
                            }
                        }
                    }
                }
                else {
                    for ( int i = 0; i < imageCount && !found; ++i ) {
                        std::vector<uint64_t> refs3;
                        findRefs( ref2 - ( uint64_t ) i * 8, refs3 );
                        if ( !refs3.empty( ) ) {
                            codeReg = refs3.front( ) - 8 * 13;
                            found = true;
                        }
                    }
                }
                if ( found ) break;
            }
            if ( found ) break;
        }
        if ( found ) break;
    }
    if ( !found ) {
        std::cerr << "error: CodeRegistration scan failed\n";
        return false;
    }

    int typeDefsCount = ( int ) md_.Types( ).size( );
    if ( typeDefsCount <= 0 )
        return false;

    auto looksLikeMetaReg = [ & ] ( uint64_t baseVA ) -> bool {
        std::vector<uint64_t> p;
        if ( !ReadPointersVA( baseVA, 16, p ) ) return false;
        int64_t typesCount = ( int64_t ) p [ 6 ];
        if ( typesCount <= 0 || typesCount > 4'000'000 ) return false;
        if ( !pe_.MapVAtoFile( p [ 7 ] ) ) return false;
        std::vector<uint64_t> sample;
        size_t n = ( size_t ) std::min<int64_t>( 4, typesCount );
        if ( !ReadPointersVA( p [ 7 ], n, sample ) ) return false;
        for ( uint64_t v : sample )
            if ( !pe_.MapVAtoFile( v ) )
                return false;
        if ( ( int64_t ) p [ 12 ] != typeDefsCount ) return false;
        return true;
    };

    bool metaFound = false;
    for ( const auto & s : dataSections ) {
        size_t pos = s.pointerToRawData;
        size_t end = s.pointerToRawData + s.sizeOfRawData;
        while ( pos + 8 <= end ) {
            int64_t v = 0;
            std::memcpy( &v, pe_.Bytes( ).data( ) + pos, 8 );
            if ( v == ( int64_t ) typeDefsCount ) {
                uint64_t cand = vaOfFile( pos ) - 8 * 10;
                if ( looksLikeMetaReg( cand ) ) {
                    metaReg = cand;
                    metaFound = true;
                    break;
                }
            }
            pos += 8;
        }
        if ( metaFound ) break;
    }
    if ( !metaFound ) {
        std::cerr << "error: MetadataRegistration scan failed\n";
        return false;
    }
    return true;
}

bool Il2CppBinary::Initialize( ) {
    uint64_t codeRegVA = 0, metaRegVA = 0;
    if ( !LocateRegistrations( codeRegVA, metaRegVA ) )
        return false;

    std::cout << "CodeRegistration     : 0x" << std::hex << codeRegVA
              << std::dec << "\n";
    std::cout << "MetadataRegistration : 0x" << std::hex << metaRegVA
              << std::dec << "\n";

    if ( !ReadCodeRegistration( codeRegVA ) )     return false;
    if ( !ReadMetadataRegistration( metaRegVA ) ) return false;
    if ( !ReadTypes( ) )                          return false;
    ReadFieldOffsets( );
    ReadTypeDefSizes( );
    ReadCodeGenModules( );

    if ( md_.Version( ) >= 21 && md_.Version( ) < 27 &&
         code_.customAttributeGenerators ) {
        ReadPointersVA( code_.customAttributeGenerators,
                        ( size_t ) code_.customAttributeCount,
                        customAttributeGenerators_ );
    }
    ReadGenericInstances( );
    return true;
}

bool Il2CppBinary::ReadGenericInstances( ) {
    if ( !metaReg_.methodSpecs || metaReg_.methodSpecsCount <= 0 )
        return false;
    if ( !code_.genericMethodPointers || code_.genericMethodPointersCount <= 0 )
        return false;

    bool isV27 = md_.Version( ) >= 27;
    size_t recSize = isV27 ? 24 : 16;
    size_t specCount = ( size_t ) metaReg_.methodSpecsCount;

    std::vector<uint8_t> specBuf( specCount * recSize );
    if ( !pe_.ReadVA( metaReg_.methodSpecs, specBuf.data( ), specBuf.size( ) ) )
        return false;

    std::vector<uint64_t> ptrs;
    if ( !ReadPointersVA( code_.genericMethodPointers,
                          ( size_t ) code_.genericMethodPointersCount, ptrs ) )
        return false;

    auto readInst = [ this ] ( uint64_t instVA,
                               std::vector<const Il2CppTypeRec *> & out ) {
        out.clear( );
        if ( !instVA )
            return;
        int64_t  argc = 0;
        uint64_t argv = 0;
        if ( !pe_.ReadVA( instVA, argc ) )       return;
        if ( !pe_.ReadVA( instVA + 8, argv ) )   return;
        if ( argc <= 0 || argc > 64 )            return;
        std::vector<uint64_t> p;
        if ( !ReadPointersVA( argv, ( size_t ) argc, p ) ) return;
        out.reserve( p.size( ) );
        for ( uint64_t v : p )
            out.push_back( TypeFromVA( v ) );
    };

    genericInstances_.clear( );
    instancesByMethodDef_.clear( );
    genericInstances_.reserve( specCount );

    for ( size_t i = 0; i < specCount; ++i ) {
        const uint8_t * p = specBuf.data( ) + i * recSize;
        int32_t methodDefIdx = 0;
        std::memcpy( &methodDefIdx, p, 4 );

        if ( methodDefIdx < 0 ||
             ( size_t ) methodDefIdx >= md_.Methods( ).size( ) )
            continue;

        uint64_t classInstVA  = 0;
        uint64_t methodInstVA = 0;
        if ( isV27 ) {
            std::memcpy( &classInstVA,  p + 8,  8 );
            std::memcpy( &methodInstVA, p + 16, 8 );
        }

        GenericInstance inst { };
        inst.methodDefIndex = methodDefIdx;
        inst.va = ( i < ptrs.size( ) ) ? ptrs [ i ] : 0;
        if ( isV27 ) {
            readInst( classInstVA,  inst.classArgs );
            readInst( methodInstVA, inst.methodArgs );
        }

        int slot = ( int ) genericInstances_.size( );
        genericInstances_.push_back( std::move( inst ) );
        instancesByMethodDef_ [ methodDefIdx ].push_back( slot );
    }
    return true;
}
