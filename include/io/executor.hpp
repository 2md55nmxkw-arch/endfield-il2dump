#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include <io/il2cpp_binary.hpp>
#include <core/metadata.hpp>

#include <string>

class Executor {
public:
    Executor( const Metadata & md, const Il2CppBinary & bin )
        : md_( md ), bin_( bin ) { }

    std::string GetTypeName( const Il2CppTypeRec & t, bool addNamespace,
        bool nested ) const;
    std::string GetTypeDefName( const TypeDef & td, bool addNamespace,
        bool genericParameters ) const;
    std::string GetGenericContainerParams( const GenericContainer & gc ) const;

    bool TryFormatDefaultValue( int typeIndex, int dataIndex,
        std::string & out ) const;

private:
    const Metadata & md_;
    const Il2CppBinary & bin_;
};

#endif
