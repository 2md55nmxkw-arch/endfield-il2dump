#ifndef IL2CPP_BINARY_HPP
#define IL2CPP_BINARY_HPP

#include <core/metadata.hpp>
#include <core/pe.hpp>

#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

enum Il2CppTypeKind : uint32_t {
    TYPE_END = 0x00,
    TYPE_VOID = 0x01,
    TYPE_BOOLEAN = 0x02,
    TYPE_CHAR = 0x03,
    TYPE_I1 = 0x04,
    TYPE_U1 = 0x05,
    TYPE_I2 = 0x06,
    TYPE_U2 = 0x07,
    TYPE_I4 = 0x08,
    TYPE_U4 = 0x09,
    TYPE_I8 = 0x0a,
    TYPE_U8 = 0x0b,
    TYPE_R4 = 0x0c,
    TYPE_R8 = 0x0d,
    TYPE_STRING = 0x0e,
    TYPE_PTR = 0x0f,
    TYPE_BYREF = 0x10,
    TYPE_VALUETYPE = 0x11,
    TYPE_CLASS = 0x12,
    TYPE_VAR = 0x13,
    TYPE_ARRAY = 0x14,
    TYPE_GENERICINST = 0x15,
    TYPE_TYPEDBYREF = 0x16,
    TYPE_I = 0x18,
    TYPE_U = 0x19,
    TYPE_FNPTR = 0x1b,
    TYPE_OBJECT = 0x1c,
    TYPE_SZARRAY = 0x1d,
    TYPE_MVAR = 0x1e,
    TYPE_CMOD_REQD = 0x1f,
    TYPE_CMOD_OPT = 0x20,
    TYPE_INTERNAL = 0x21,
    TYPE_ENUM = 0x55,
    TYPE_IL2CPP_TYPE_INDEX = 0xff,
};

struct Il2CppTypeRec {
    uint64_t data;
    uint32_t attrs;
    uint32_t kind;
    uint32_t num_mods;
    uint32_t byref;
    uint32_t pinned;
    uint32_t valuetype;
};

struct CodeRegistration {
    uint64_t reversePInvokeWrapperCount = 0;
    uint64_t reversePInvokeWrappers = 0;
    uint64_t genericMethodPointersCount = 0;
    uint64_t genericMethodPointers = 0;
    uint64_t genericAdjustorThunks = 0;
    uint64_t invokerPointersCount = 0;
    uint64_t invokerPointers = 0;
    uint64_t customAttributeCount = 0;
    uint64_t customAttributeGenerators = 0;
    uint64_t unresolvedVirtualCallCount = 0;
    uint64_t unresolvedVirtualCallPointers = 0;
    uint64_t interopDataCount = 0;
    uint64_t interopData = 0;
    uint64_t windowsRuntimeFactoryCount = 0;
    uint64_t windowsRuntimeFactoryTable = 0;
    uint64_t codeGenModulesCount = 0;
    uint64_t codeGenModules = 0;
    uint64_t methodPointersCount = 0;
    uint64_t methodPointers = 0;
};

struct MetadataRegistration {
    int64_t genericClassesCount = 0;
    uint64_t genericClasses = 0;
    int64_t genericInstsCount = 0;
    uint64_t genericInsts = 0;
    int64_t genericMethodTableCount = 0;
    uint64_t genericMethodTable = 0;
    int64_t typesCount = 0;
    uint64_t types = 0;
    int64_t methodSpecsCount = 0;
    uint64_t methodSpecs = 0;
    int64_t fieldOffsetsCount = 0;
    uint64_t fieldOffsets = 0;
    int64_t typeDefinitionsSizesCount = 0;
    uint64_t typeDefinitionsSizes = 0;
    uint64_t metadataUsagesCount = 0;
    uint64_t metadataUsages = 0;
};

struct CodeGenModule {
    uint64_t moduleName = 0;
    int64_t methodPointerCount = 0;
    uint64_t methodPointers = 0;
    int64_t adjustorThunkCount = 0;
    uint64_t adjustorThunks = 0;
    uint64_t invokerIndices = 0;
    uint64_t reversePInvokeWrapperCount = 0;
    uint64_t reversePInvokeWrapperIndices = 0;
    int64_t rgctxRangesCount = 0;
    uint64_t rgctxRanges = 0;
    int64_t rgctxsCount = 0;
    uint64_t rgctxs = 0;
    uint64_t debuggerMetadata = 0;
    uint64_t customAttributeCacheGenerator = 0;
    uint64_t moduleInitializer = 0;
    uint64_t staticConstructorTypeIndices = 0;
    uint64_t metadataRegistration = 0;
    uint64_t codeRegistration = 0;
};

class Il2CppBinary {
public:
    Il2CppBinary( PeImage & pe, Metadata & md ) : pe_( pe ), md_( md ) { }

    bool Initialize( );

    enum class ResolveStatus {
        Ok,
        NoModule,
        ZeroToken,
        TokenOutOfRange,
        NullPointer,
    };

    uint64_t GetMethodVA( const std::string & imageName, const MethodDef & method ) const;
    uint64_t ResolveMethodVA( const std::string & imageName, const MethodDef & method,
                              ResolveStatus & status ) const;
    uint64_t VAtoRVA( uint64_t va ) const {
        return va > pe_.ImageBase( ) ? va - pe_.ImageBase( ) : 0;
    }
    uint64_t VAtoFile( uint64_t va ) const { return pe_.MapVAtoFile( va ); }
    uint64_t ImageBase( ) const { return pe_.ImageBase( ); }

    const Il2CppTypeRec * Type( int index ) const {
        if ( index < 0 || ( size_t ) index >= types_.size( ) )
            return nullptr;
        return &types_ [ index ];
    }
    size_t TypeCount( ) const { return types_.size( ); }

    int32_t TypeDefSize( int typeDefIndex ) const {
        if ( typeDefIndex < 0 || ( size_t ) typeDefIndex >= typeSizes_.size( ) )
            return 0;
        return typeSizes_ [ typeDefIndex ];
    }

    int32_t FieldOffset( int typeDefIndex, int fieldIndexInType, bool isValueType,
        bool isStatic ) const;

    const Il2CppTypeRec * TypeFromGenericClassPtr( uint64_t genericClassVA ) const;
    const Il2CppTypeRec * TypeFromVA( uint64_t va ) const;
    bool ReadGenericInstArgs( uint64_t genericClassVA,
        std::vector<const Il2CppTypeRec *> & out ) const;

    struct GenericInstance {
        int methodDefIndex;
        std::vector<const Il2CppTypeRec *> classArgs;
        std::vector<const Il2CppTypeRec *> methodArgs;
        uint64_t va;
    };

    const std::vector<GenericInstance> & InstancesForMethod( int methodDefIndex ) const;

    uint64_t CustomAttributeGeneratorVA( int attributeIndex ) const;

    bool ReadPointersVA( uint64_t arrayVA, size_t count,
        std::vector<uint64_t> & out ) const;

private:
    PeImage & pe_;
    Metadata & md_;

    CodeRegistration code_ { };
    MetadataRegistration metaReg_ { };
    std::vector<Il2CppTypeRec> types_;
    std::unordered_map<uint64_t, int> typeIndexByVA_;
    std::vector<int32_t> typeSizes_;
    std::vector<uint64_t> fieldOffsetTablePtrs_;
    bool fieldOffsetsArePointers_ = true;
    std::vector<uint64_t> customAttributeGenerators_;

    std::unordered_map<std::string, CodeGenModule> codeGenModules_;
    std::unordered_map<std::string, std::vector<uint64_t>> codeGenModuleMethodPointers_;

    std::vector<uint64_t> methodPointers_;

    std::vector<GenericInstance> genericInstances_;
    std::unordered_map<int, std::vector<int>> instancesByMethodDef_;
    std::vector<GenericInstance> emptyInstances_;

    bool LocateRegistrations( uint64_t & codeReg, uint64_t & metaReg );

    bool ReadCodeRegistration( uint64_t codeRegVA );
    bool ReadMetadataRegistration( uint64_t metaRegVA );
    bool ReadTypes( );
    bool ReadFieldOffsets( );
    bool ReadTypeDefSizes( );
    bool ReadCodeGenModules( );
    bool ReadGenericInstances( );

    void DecodeType( const uint8_t buf [ 12 ], Il2CppTypeRec & out ) const;
};

#endif
