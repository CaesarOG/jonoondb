#include "jonoondb_api/flatbuffers_field.h"
#include "jonoondb_api/flatbuffers_document_schema.h"
#include "jonoondb_api/jonoondb_exceptions.h"

using namespace flatbuffers;
using namespace jonoondb_api;
using namespace std;

// Todo find a more optimized way to avoid string copy
// flatbuffers api is returning the string as a copy
const std::string FlatbuffersField::GetName() const {
  return m_fieldDef->name()->str();
}

FieldType FlatbuffersField::GetType() const {
  if (m_fieldDef->type()->base_type() == reflection::BaseType::Vector &&
      (m_fieldDef->type()->element() == reflection::BaseType::Byte ||
       m_fieldDef->type()->element() == reflection::BaseType::UByte)) {
    return FieldType::BLOB;
  }

  return FlatbuffersDocumentSchema::MapFlatbuffersToJonoonDBType(
      m_fieldDef->type()->base_type());
}

FieldType FlatbuffersField::GetElementType() const {
  assert(m_fieldDef->type()->base_type() == reflection::BaseType::Vector);
  return FlatbuffersDocumentSchema::MapFlatbuffersToJonoonDBType(
      m_fieldDef->type()->element());
}

size_t FlatbuffersField::GetSubFieldCount() const {
  if (m_fieldDef->type()->base_type() == reflection::BaseType::Obj) {
    return m_schema->objects()
        ->Get(m_fieldDef->type()->index())
        ->fields()
        ->size();
  }

  return 0;
}

void FlatbuffersField::GetSubField(size_t index, Field*& field) const {
  FlatbuffersField* fbField = dynamic_cast<FlatbuffersField*>(field);
  if (fbField == nullptr) {
    // This means that the passed in doc cannot be casted to FlatbuffersDocument
    string errorMsg =
        "Argument field cannot be casted to underlying field "
        "implementation i.e. FlatbuffersField. "
        "Make sure you are creating the val by calling AllocateField call.";
    throw InvalidArgumentException(errorMsg, __FILE__, __func__, __LINE__);
  }

  if (index > GetSubFieldCount() - 1) {
    throw IndexOutOfBoundException("Index was outside the bounds of the array.",
                                   __FILE__, __func__, __LINE__);
  }

  auto obj = m_schema->objects()->Get(m_fieldDef->type()->index());
  fbField->SetMembers(const_cast<reflection::Field*>(obj->fields()->Get(index)),
                      m_schema);
}

Field* FlatbuffersField::AllocateField() const {
  return new FlatbuffersField();
}

void FlatbuffersField::Dispose() {
  m_fieldDef = nullptr;
  m_schema = nullptr;
  delete this;
}

void FlatbuffersField::SetMembers(reflection::Field* val,
                                  reflection::Schema* valSch) {
  m_fieldDef = val;
  m_schema = valSch;
}
