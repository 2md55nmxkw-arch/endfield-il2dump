#ifndef METADATA_HPP
#define METADATA_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

struct Il2CppGlobalMetadataHeader {
    uint32_t sanity;
    int32_t version;
    uint32_t stringLiteralOffset;
    int32_t stringLiteralSize;
    uint32_t stringLiteralDataOffset;
    int32_t stringLiteralDataSize;
    uint32_t stringOffset;
    int32_t stringSize;
    uint32_t eventsOffset;
    int32_t eventsSize;
    uint32_t propertiesOffset;
    int32_t propertiesSize;
    uint32_t methodsOffset;
    int32_t methodsSize;
    uint32_t parameterDefaultValuesOffset;
    int32_t parameterDefaultValuesSize;
    uint32_t fieldDefaultValuesOffset;
    int32_t fieldDefaultValuesSize;
    uint32_t fieldAndParameterDefaultValueDataOffset;
    int32_t fieldAndParameterDefaultValueDataSize;
    int32_t fieldMarshaledSizesOffset;
    int32_t fieldMarshaledSizesSize;
    uint32_t parametersOffset;
    int32_t parametersSize;
    uint32_t fieldsOffset;
    int32_t fieldsSize;
    uint32_t genericParametersOffset;
    int32_t genericParametersSize;
    uint32_t genericParameterConstraintsOffset;
    int32_t genericParameterConstraintsSize;
    uint32_t genericContainersOffset;
    int32_t genericContainersSize;
    uint32_t nestedTypesOffset;
    int32_t nestedTypesSize;
    uint32_t interfacesOffset;
    int32_t interfacesSize;
    uint32_t vtableMethodsOffset;
    int32_t vtableMethodsSize;
    int32_t interfaceOffsetsOffset;
    int32_t interfaceOffsetsSize;
    uint32_t typeDefinitionsOffset;
    int32_t typeDefinitionsSize;
    uint32_t rgctxEntriesOffset;
    int32_t rgctxEntriesCount;
    uint32_t imagesOffset;
    int32_t imagesSize;
    uint32_t assembliesOffset;
    int32_t assembliesSize;
    uint32_t metadataUsageListsOffset;
    int32_t metadataUsageListsCount;
    uint32_t metadataUsagePairsOffset;
    int32_t metadataUsagePairsCount;
    uint32_t fieldRefsOffset;
    int32_t fieldRefsSize;
    int32_t referencedAssembliesOffset;
    int32_t referencedAssembliesSize;
    uint32_t attributesInfoOffset;
    int32_t attributesInfoCount;
    uint32_t attributeTypesOffset;
    int32_t attributeTypesCount;
    uint32_t attributeDataOffset;
    int32_t attributeDataSize;
    uint32_t attributeDataRangeOffset;
    int32_t attributeDataRangeSize;
    int32_t unresolvedVirtualCallParameterTypesOffset;
    int32_t unresolvedVirtualCallParameterTypesSize;
    int32_t unresolvedVirtualCallParameterRangesOffset;
    int32_t unresolvedVirtualCallParameterRangesSize;
    int32_t windowsRuntimeTypeNamesOffset;
    int32_t windowsRuntimeTypeNamesSize;
    int32_t windowsRuntimeStringsOffset;
    int32_t windowsRuntimeStringsSize;
    int32_t exportedTypeDefinitionsOffset;
    int32_t exportedTypeDefinitionsSize;
};

struct AssemblyName {
    uint32_t nameIndex;
    uint32_t cultureIndex;
    int32_t hashValueIndex;
    uint32_t publicKeyIndex;
    uint32_t hash_alg;
    int32_t hash_len;
    uint32_t flags;
    int32_t major, minor, build, revision;
    uint8_t public_key_token [ 8 ];
};

struct AssemblyDef {
    int32_t imageIndex;
    uint32_t token;
    int32_t customAttributeIndex;
    int32_t referencedAssemblyStart;
    int32_t referencedAssemblyCount;
    AssemblyName aname;
};

struct ImageDef {
    uint32_t nameIndex;
    int32_t assemblyIndex;
    int32_t typeStart;
    uint32_t typeCount;
    int32_t exportedTypeStart;
    uint32_t exportedTypeCount;
    int32_t entryPointIndex;
    uint32_t token;
    int32_t customAttributeStart;
    uint32_t customAttributeCount;
};

struct TypeDef {
    uint32_t nameIndex;
    uint32_t namespaceIndex;
    int32_t customAttributeIndex;
    int32_t byvalTypeIndex;
    int32_t byrefTypeIndex;
    int32_t declaringTypeIndex;
    int32_t parentIndex;
    int32_t elementTypeIndex;
    int32_t rgctxStartIndex;
    int32_t rgctxCount;
    int32_t genericContainerIndex;
    uint32_t flags;
    int32_t fieldStart;
    int32_t methodStart;
    int32_t eventStart;
    int32_t propertyStart;
    int32_t nestedTypesStart;
    int32_t interfacesStart;
    int32_t vtableStart;
    int32_t interfaceOffsetsStart;
    uint16_t method_count;
    uint16_t property_count;
    uint16_t field_count;
    uint16_t event_count;
    uint16_t nested_type_count;
    uint16_t vtable_count;
    uint16_t interfaces_count;
    uint16_t interface_offsets_count;
    uint32_t bitfield;
    uint32_t token;

    bool IsValueType( ) const { return ( bitfield & 0x1 ) != 0; }
    bool IsEnum( ) const { return ( ( bitfield >> 1 ) & 0x1 ) != 0; }
};

struct MethodDef {
    uint32_t nameIndex;
    int32_t declaringType;
    int32_t returnType;
    int32_t returnParameterToken;
    int32_t parameterStart;
    int32_t customAttributeIndex;
    int32_t genericContainerIndex;
    int32_t methodIndex;
    int32_t invokerIndex;
    int32_t delegateWrapperIndex;
    int32_t rgctxStartIndex;
    int32_t rgctxCount;
    uint32_t token;
    uint16_t flags;
    uint16_t iflags;
    uint16_t slot;
    uint16_t parameterCount;
};

struct ParameterDef {
    uint32_t nameIndex;
    uint32_t token;
    int32_t customAttributeIndex;
    int32_t typeIndex;
};

struct FieldDef {
    uint32_t nameIndex;
    int32_t typeIndex;
    int32_t customAttributeIndex;
    uint32_t token;
};

struct FieldDefaultValue {
    int32_t fieldIndex;
    int32_t typeIndex;
    int32_t dataIndex;
};

struct ParameterDefaultValue {
    int32_t parameterIndex;
    int32_t typeIndex;
    int32_t dataIndex;
};

struct PropertyDef {
    uint32_t nameIndex;
    int32_t get;
    int32_t set;
    uint32_t attrs;
    int32_t customAttributeIndex;
    uint32_t token;
};

struct EventDef {
    uint32_t nameIndex;
    int32_t typeIndex;
    int32_t add;
    int32_t remove;
    int32_t raise;
    int32_t customAttributeIndex;
    uint32_t token;
};

struct GenericContainer {
    int32_t ownerIndex;
    int32_t type_argc;
    int32_t is_method;
    int32_t genericParameterStart;
};

struct GenericParameter {
    int32_t ownerIndex;
    uint32_t nameIndex;
    int16_t constraintsStart;
    int16_t constraintsCount;
    uint16_t num;
    uint16_t flags;
};

struct StringLiteral {
    uint32_t length;
    int32_t dataIndex;
};

struct CustomAttributeTypeRange {
    uint32_t token;
    int32_t start;
    int32_t count;
};

struct CustomAttributeDataRange {
    uint32_t token;
    uint32_t startOffset;
};

class Metadata {
public:
    bool Load( const std::string & path );

    double Version( ) const { return version; }
    const Il2CppGlobalMetadataHeader & Header( ) const { return header; }
    const std::vector<uint8_t> & Bytes( ) const { return bytes; }

    const std::vector<AssemblyDef> & Assemblies( ) const { return assemblies; }
    const std::vector<ImageDef> & Images( ) const { return images; }
    const std::vector<TypeDef> & Types( ) const { return types; }
    const std::vector<MethodDef> & Methods( ) const { return methods; }
    const std::vector<ParameterDef> & Parameters( ) const { return parameters; }
    const std::vector<FieldDef> & Fields( ) const { return fields; }
    const std::vector<PropertyDef> & Properties( ) const { return properties; }
    const std::vector<EventDef> & Events( ) const { return events; }
    const std::vector<GenericContainer> & GenericContainers( ) const { return genericContainers; }
    const std::vector<GenericParameter> & GenericParameters( ) const { return genericParameters; }
    const std::vector<int32_t> & InterfaceIndices( ) const { return interfaceIndices; }
    const std::vector<int32_t> & NestedTypeIndices( ) const { return nestedTypeIndices; }
    const std::vector<StringLiteral> & StringLiterals( ) const { return stringLiterals; }
    const std::vector<CustomAttributeTypeRange> & AttributeTypeRanges( ) const { return attributeTypeRanges; }
    const std::vector<int32_t> & AttributeTypes( ) const { return attributeTypes; }
    const std::vector<CustomAttributeDataRange> & AttributeDataRanges( ) const { return attributeDataRanges; }

    std::string GetString( uint32_t index ) const;
    std::string GetStringLiteral( uint32_t index ) const;

    bool TryGetFieldDefault( int fieldIndex, FieldDefaultValue & out ) const;
    bool TryGetParameterDefault( int paramIndex, ParameterDefaultValue & out ) const;

    int GetCustomAttributeIndex( const ImageDef & img, int customAttributeIndex,
        uint32_t token ) const;

    const uint8_t * DefaultValueData( int dataIndex, size_t & remaining ) const;

private:
    std::vector<uint8_t> bytes;
    Il2CppGlobalMetadataHeader header { };
    double version = 0;
    int versionMajor = 0;

    std::vector<AssemblyDef> assemblies;
    std::vector<ImageDef> images;
    std::vector<TypeDef> types;
    std::vector<MethodDef> methods;
    std::vector<ParameterDef> parameters;
    std::vector<FieldDef> fields;
    std::vector<PropertyDef> properties;
    std::vector<EventDef> events;
    std::vector<GenericContainer> genericContainers;
    std::vector<GenericParameter> genericParameters;
    std::vector<int32_t> interfaceIndices;
    std::vector<int32_t> nestedTypeIndices;
    std::vector<StringLiteral> stringLiterals;
    std::vector<CustomAttributeTypeRange> attributeTypeRanges;
    std::vector<int32_t> attributeTypes;
    std::vector<CustomAttributeDataRange> attributeDataRanges;
    std::vector<FieldDefaultValue> fieldDefaultValues;
    std::vector<ParameterDefaultValue> parameterDefaultValues;

    std::unordered_map<int32_t, FieldDefaultValue> fieldDefaultValueByField;
    std::unordered_map<int32_t, ParameterDefaultValue> parameterDefaultValueByParam;
    std::vector<std::unordered_map<uint32_t, int>> attributeIndexByImageToken;

    void ReadHeader( );
    void DetectVariant( );
    void LoadTables( );
    void BuildLookups( );
};

#endif
