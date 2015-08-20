#include <memory>
#include <string>
#include <boost/tokenizer.hpp>
#include "flatbuffers/idl.h"
#include "flatbuffers_document_schema.h"
#include "enums.h"
#include "status.h"
#include "string_utils.h"
#include "exception_utils.h"
#include "flatbuffers_field.h"
#include "field.h"

using namespace std;
using namespace jonoondb_api;
using namespace flatbuffers;
using namespace boost;

FlatbuffersDocumentSchema::FlatbuffersDocumentSchema(const char* schemaText,
                                                     SchemaType schemaType,
                                                     unique_ptr<Parser> parser)
    : m_schemaText(schemaText),
      m_schemaType(schemaType),
      m_parser(move(parser)) {
}

FlatbuffersDocumentSchema::~FlatbuffersDocumentSchema() {
}

Status FlatbuffersDocumentSchema::Construct(
    const char* schemaText, SchemaType schemaType,
    FlatbuffersDocumentSchema*& documentSchema) {
  if (StringUtils::IsNullOrEmpty(schemaText)) {
    string errorMsg = "Argument schemaText is null or empty.";
    return Status(kStatusInvalidArgumentCode, errorMsg.c_str(),
                  errorMsg.length());
  }

  unique_ptr<Parser> parser(new Parser());
  if (!parser->Parse(schemaText)) {
    ostringstream ss;
    ss << "Flatbuffers parser failed to parse the given schema." << endl
       << schemaText;
    string errorMsg = ss.str();
    return Status(kStatusSchemaParseErrorCode, errorMsg.c_str(),
                  errorMsg.length());
  }

  documentSchema = new FlatbuffersDocumentSchema(schemaText, schemaType,
                                                 move(parser));
  return Status();
}

const char* FlatbuffersDocumentSchema::GetSchemaText() const {
  return m_schemaText.c_str();
}

SchemaType FlatbuffersDocumentSchema::GetSchemaType() const {
  return m_schemaType;
}

Status FlatbuffersDocumentSchema::GetFieldType(const char* fieldName,
                                               FieldType& fieldType) const {
  // The fieldName is dot(.) sperated e.g. Field1.Field2.Field3
  if (StringUtils::IsNullOrEmpty(fieldName)) {
    string errorMsg = "Argument fieldName is null or empty.";
    return Status(kStatusInvalidArgumentCode, errorMsg.c_str(),
                  errorMsg.length());
  }

  char_separator<char> sep(".");
  // TODO: Find a way to avoid the cost of string construction
  string fieldNameStr(fieldName);
  tokenizer<char_separator<char>> tokens(fieldNameStr, sep);
  auto structDef = m_parser->root_struct_def;
  vector<string> tokenVec(tokens.begin(), tokens.end());

  FieldDef* fieldDef = nullptr;
  for (size_t i = 0; i < tokenVec.size() - 1; i++) {
    fieldDef = structDef->fields.Lookup(tokenVec[i]);
    if (fieldDef == nullptr) {
      return ExceptionUtils::GetMissingFieldErrorStatus(tokenVec[i].c_str());
    }
    if (fieldDef->value.type.base_type != BaseType::BASE_TYPE_STRUCT) {
      return ExceptionUtils::GetInvalidStructFieldErrorStatus(
          tokenVec[i].c_str(), fieldName);
    }
    structDef = m_parser->structs_.Lookup(
        fieldDef->value.type.struct_def->name);
  }

  fieldDef = structDef->fields.Lookup(tokenVec[tokenVec.size() - 1]);
  if (fieldDef == nullptr) {
    return ExceptionUtils::GetMissingFieldErrorStatus(fieldName);
  }
  fieldType = FlatbuffersDocumentSchema::MapFlatbuffersToJonoonDBType(
      fieldDef->value.type.base_type);

  return Status();
}

std::size_t FlatbuffersDocumentSchema::GetRootFieldCount() const {
  return m_parser->root_struct_def->fields.vec.size();
}

Status FlatbuffersDocumentSchema::GetRootField(size_t index,
                                               Field*& field) const {
  FlatbuffersField* fbField = dynamic_cast<FlatbuffersField*>(field);
  if (fbField == nullptr) {
    // This means that the passed in doc cannot be casted to FlatbuffersDocument    
    string errorMsg = "Argument field cannot be casted to underlying field "
        "implementation i.e. FlatbuffersField. "
        "Make sure you are creating the val by calling AllocateField call.";
    return Status(kStatusInvalidArgumentCode, errorMsg.c_str(),
                  errorMsg.length());
  }

  if (index > GetRootFieldCount() - 1 || index < 0) {
    string errorMsg = "Index was outside the bounds of the array.";
    return Status(kStatusIndexOutOfBoundErrorCode, errorMsg.c_str(),
                  errorMsg.length());
  }

  fbField->SetFieldDef(m_parser->root_struct_def->fields.vec[index]);
  return Status();
}

Status FlatbuffersDocumentSchema::AllocateField(Field*& field) const {
  try {
    field = new FlatbuffersField();
  } catch (bad_alloc) {
    // Memory allocation failed
    string errorMsg = "Memory allocation failed.";
    return Status(kStatusOutOfMemoryErrorCode, errorMsg.c_str(),
                  errorMsg.length());
  }

  return Status();
}

const StructDef* FlatbuffersDocumentSchema::GetRootStruct() const {
  return m_parser->root_struct_def;
}

const SymbolTable<StructDef>* FlatbuffersDocumentSchema::GetChildStructs() const {
  return &m_parser->structs_;
}

FieldType FlatbuffersDocumentSchema::MapFlatbuffersToJonoonDBType(
    flatbuffers::BaseType flatbuffersType) {
  switch (flatbuffersType) {
    case flatbuffers::BASE_TYPE_NONE:
      return FieldType::BASE_TYPE_UINT8;
    case flatbuffers::BASE_TYPE_UTYPE:
      return FieldType::BASE_TYPE_UINT8;
    case flatbuffers::BASE_TYPE_BOOL:
      return FieldType::BASE_TYPE_UINT8;
    case flatbuffers::BASE_TYPE_CHAR:
      return FieldType::BASE_TYPE_INT8;
    case flatbuffers::BASE_TYPE_UCHAR:
      return FieldType::BASE_TYPE_UINT8;
    case flatbuffers::BASE_TYPE_SHORT:
      return FieldType::BASE_TYPE_INT16;
    case flatbuffers::BASE_TYPE_USHORT:
      return FieldType::BASE_TYPE_UINT16;
    case flatbuffers::BASE_TYPE_INT:
      return FieldType::BASE_TYPE_INT32;
    case flatbuffers::BASE_TYPE_UINT:
      return FieldType::BASE_TYPE_UINT32;
    case flatbuffers::BASE_TYPE_LONG:
      return FieldType::BASE_TYPE_INT64;
    case flatbuffers::BASE_TYPE_ULONG:
      return FieldType::BASE_TYPE_UINT64;
    case flatbuffers::BASE_TYPE_FLOAT:
      return FieldType::BASE_TYPE_FLOAT32;
    case flatbuffers::BASE_TYPE_DOUBLE:
      return FieldType::BASE_TYPE_DOUBLE;
    case flatbuffers::BASE_TYPE_STRING:
      return FieldType::BASE_TYPE_STRING;
    case flatbuffers::BASE_TYPE_VECTOR:
      return FieldType::BASE_TYPE_VECTOR;
    case flatbuffers::BASE_TYPE_STRUCT:
      return FieldType::BASE_TYPE_COMPLEX;
      // case flatbuffers::BASE_TYPE_UNION: break; TODO: Need to handle this type
    default:
      assert(0);  // this should never happen. TODO: Handle it for release case as well. assert only works in debug mode.
  }
}