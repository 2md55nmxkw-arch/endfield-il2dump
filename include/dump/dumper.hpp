#ifndef DUMPER_HPP
#define DUMPER_HPP

#include <io/executor.hpp>
#include <io/il2cpp_binary.hpp>
#include <core/metadata.hpp>

#include <fstream>
#include <string>
#include <unordered_set>

class Dumper {
public:
    Dumper( const Metadata & md, const Il2CppBinary & bin, const Executor & ex )
        : md_( md ), bin_( bin ), ex_( ex ) { }

    void DumpAll( const std::string & outputDir );

private:
    const Metadata & md_;
    const Il2CppBinary & bin_;
    const Executor & ex_;

    void WriteImageList( std::ofstream & out );
    void EmitType( std::ofstream & out, const std::string & asmName, int ti, int indent );
    void EmitField( std::ofstream & out, int ti, int j, const std::string & pad );
    void EmitProperty( std::ofstream & out, int ti, int j, const std::string & pad );
    void EmitEvent( std::ofstream & out, int ti, int j, const std::string & pad );
    void EmitMethod( std::ofstream & out, const std::string & asmName, int ti, int j,
                     const std::string & pad );
    void EmitGenericInstances( std::ofstream & out, int methodDefIndex,
                               const MethodDef & m, const TypeDef & td,
                               const std::string & pad );
    std::unordered_set<int> ConsumedMethodIndices( const TypeDef & td );

    void WriteScriptJson( const std::string & path );
    void WriteStringLiteralJson( const std::string & path );
    std::string BuildMethodSignature( const MethodDef & m, const std::string & typeFq );
    std::string BuildGenericInstanceSignature( const MethodDef & m, const TypeDef & td,
                                               const Il2CppBinary::GenericInstance & inst );
};

#endif
