#pragma once

#include <memory>
#include "enums.h"

namespace jonoondb_api {
// Forward Declaration
class BufferImpl;
class Document;
class DocumentSchema;

class DocumentFactory {
 public:
  static std::unique_ptr<Document> CreateDocument(
      const DocumentSchema& documentSchema, const BufferImpl& buffer);

 private:
  DocumentFactory() = delete;
  DocumentFactory(const DocumentFactory&) = delete;
  DocumentFactory(DocumentFactory&&) = delete;
};
}  // namespace jonoondb_api
