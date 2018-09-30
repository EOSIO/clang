#pragma once

#include "clang/AST/DeclCXX.h"
#include "clang/AST/Expr.h"
#include "clang/Basic/Builtins.h"
#include <string>

namespace eosio { namespace cdt {

bool is_vector_type( const clang::QualType& type ) {
   if ( auto tst = llvm::dyn_cast<clang::TemplateSpecializationType>(type.getTypePtr()) )
      if ( type.getBaseTypeIdentifier()->getName().str() == "__vector_base" )
         return true;
   return false;
}

clang::QualType get_vector_type( const clang::QualType& type ) {
   if ( auto tst = llvm::dyn_cast<clang::TemplateSpecializationType>(type.getTypePtr()) )
      return tst->getArg(0).getAsType();
}

template <typename Lambda>
std::string get_base_type_name( const clang::QualType& type, Lambda&& error_handler ) {
   clang::QualType newType = type;
   bool is_a_vector = is_vector_type(type);
   if ( is_a_vector )
      newType = get_vector_type(type); 
   if (is_vector_type(newType)) {
      std::cout << "Error, can't have nested vectors\n";
      error_handler();
   }
   std::string type_str = newType.getNonReferenceType().getAsString();
   int i = type_str.length()-1;
   for (; i > 0; i--)
      if (type_str[i] == ':' || type_str[i] == ' ') {
         return type_str.substr(i+1); 
      }

   return type_str;
}

template <typename Lambda>
std::string translate_type( const clang::QualType& t, Lambda&& error_handler ) {
   std::map<std::string, std::string> translation_table =
   {
      {"_Bool", "bool"},
      
      {"unsigned __int128", "uint128"},
      {"__int128", "int128"},
      {"uint128_t", "uint128"},
      {"int128_t", "int128"},

      {"unsigned long long", "uint64"},
      {"long long", "int64"},
      {"uint64_t", "uint64"},
      {"int64_t", "int64"},

      {"unsigned long", "uint32"},
      {"long", "int32"},
      {"unsigned int", "uint32"},
      {"int", "int32"},
      {"uint32_t", "uint32"},
      {"int32_t", "int32"},

      {"unsigned short", "uint16"},
      {"short", "int16"},
      {"uint16_t", "uint16"},
      {"int16_t", "int16"},

      {"unsigned char", "uint8"},
      {"char", "int8"},
      {"uint8_t", "uint8"},
      {"int8_t", "int8"},

      {"float",  "float32"},
      {"double", "float64"},
      {"long double", "float128"},
      
      {"account_name", "name"},
      {"permission_name", "name"},
      {"table_name", "name"},
      {"scope_name", "name"},
      {"action_name", "name"},
      {"symbol_type", "symbol"}
   }; 
   std::string base_name = get_base_type_name(t, error_handler);
   auto ret = translation_table[get_base_type_name(t, error_handler)];
   if (ret == "")
      return t.getNonReferenceType().getAsString();
   return ret;
}

bool is_name_type( const std::string& t ) {
   static const std::set<std::string> name_types = { "name", 
                                                     "account_name",
                                                     "permission_name",
                                                     "table_name",
                                                     "scope_name",
                                                     "action_name" };
   return name_types.count(t) >= 1;
}

bool is_builtin_type( const std::string& t ) {
   static const std::set<std::string> builtins =
   {
      "bool",
      "int8",
      "uint8",
      "int16",
      "uint16",
      "int32",
      "uint32",
      "int64",
      "uint64",
      "int128",
      "uint128",
      "varint32",
      "varuint32",
      "float32",
      "float64",
      "float128",
      "time_point",
      "time_point_sec",
      "block_timestamp_type",
      "name",
      "bytes",
      "string",
      "checksum160",
      "checksum256",
      "checksum512",
      "public_key",
      "signature",
      "symbol",
      "symbol_code",
      "asset",
      "extended_asset"
   };
   return builtins.count(t) >= 1;
}

bool is_cxx_record( const clang::QualType& t ) {
   return t.getTypePtr()->isRecordType();
}


template <typename Lambda>
std::string get_type( const clang::QualType& t, Lambda&& error_handler ) {
   std::string type = translate_type( t, error_handler );
   if ( !is_builtin_type( type ) && !is_name_type( type ) )
      type = get_base_type_name( t, error_handler );
   return type;
}

template <typename Lambda>
std::string get_type_alias( const clang::QualType& t, Lambda&& error_handler ) {
   if (is_name_type(get_base_type_name(t, error_handler)))
      return "name";
   return get_type(clang::QualType(t).getCanonicalType(), error_handler);
}

template <typename Lambda>
bool is_aliasing( const clang::QualType& t, Lambda&& error_handler ) {
   if (is_name_type(get_base_type_name(t, error_handler)))
      return true;
   return get_base_type_name(t, error_handler).compare(get_type_alias(t,error_handler)) != 0;
}

}} // ns eosio::cdt
