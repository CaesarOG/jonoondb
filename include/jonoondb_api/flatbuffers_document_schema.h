#pragma once

#include <cstdint>
#include <string>
#include "document_schema.h"
#include "flatbuffers/idl.h"

namespace jonoondb_api {
// Forward declaration
enum class FieldType : std::int8_t;
enum class SchemaType : std::int32_t;

class FlatbuffersDocumentSchema final : public DocumentSchema {
 public:
  FlatbuffersDocumentSchema(const FlatbuffersDocumentSchema&) = delete;
  FlatbuffersDocumentSchema(FlatbuffersDocumentSchema&&) = delete;
  static FieldType MapFlatbuffersToJonoonDBType(
      reflection::BaseType flatbuffersType);
  ~FlatbuffersDocumentSchema() override;
  FlatbuffersDocumentSchema(std::string binarySchema);
  const std::string& GetSchemaText() const override;
  SchemaType GetSchemaType() const override;
  FieldType GetFieldType(const std::string& fieldName) const override;
  std::size_t GetRootFieldCount() const override;
  void GetRootField(size_t index, Field*& field) const override;
  Field* AllocateField() const override;

 private:
  std::string m_binarySchema;
  reflection::Schema* m_schema;
};
}  // namespace jonoondb_api
