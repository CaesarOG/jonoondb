#pragma once

#include <cstdint>
#include <sstream>
#include <string>
#include <vector>
#include "constraint.h"
#include "document.h"
#include "enums.h"
#include "exception_utils.h"
#include "index_info_impl.h"
#include "index_stat.h"
#include "indexer.h"
#include "mama_jennies_bitmap.h"
#include "null_helpers.h"
#include "string_utils.h"

namespace jonoondb_api {
class VectorStringIndexer final : public Indexer {
 public:
  VectorStringIndexer(const IndexInfoImpl& indexInfo,
                      const FieldType& fieldType) {
    std::string errorMsg;
    if (indexInfo.GetIndexName().size() == 0) {
      errorMsg = "Argument indexInfo has empty name.";
    } else if (indexInfo.GetColumnName().size() == 0) {
      errorMsg = "Argument indexInfo has empty column name.";
    } else if (indexInfo.GetType() != IndexType::VECTOR) {
      errorMsg =
          "Argument indexInfo can only have IndexType VECTOR for "
          "VectorStringIndexer.";
    } else if (!IsValidFieldType(fieldType)) {
      std::ostringstream ss;
      ss << "Argument fieldType " << GetFieldString(fieldType)
         << " is not valid for VectorStringIndexer.";
      errorMsg = ss.str();
    }

    if (errorMsg.length() > 0) {
      throw InvalidArgumentException(errorMsg, __FILE__, __func__, __LINE__);
    }

    m_fieldNameTokens = StringUtils::Split(indexInfo.GetColumnName(), ".");
    m_indexStat = IndexStat(indexInfo, fieldType);
  }

  static bool IsValidFieldType(FieldType fieldType) {
    return (fieldType == FieldType::STRING);
  }

  void Insert(std::uint64_t documentID, const Document& document) override {
    auto val =
        DocumentUtils::GetStringValue(document, m_subDoc, m_fieldNameTokens);
    assert(m_dataVector.size() == documentID);
    m_dataVector.push_back(val);
  }

  const IndexStat& GetIndexStats() override {
    return m_indexStat;
  }

  std::shared_ptr<MamaJenniesBitmap> Filter(
      const Constraint& constraint) override {
    switch (constraint.op) {
      case jonoondb_api::IndexConstraintOperator::EQUAL:
        return GetBitmapEQ(constraint);
      case jonoondb_api::IndexConstraintOperator::LESS_THAN:
        return GetBitmapLT(constraint);
      case jonoondb_api::IndexConstraintOperator::LESS_THAN_EQUAL:
        return GetBitmapLTE(constraint);
      case jonoondb_api::IndexConstraintOperator::GREATER_THAN:
        return GetBitmapGT(constraint);
      case jonoondb_api::IndexConstraintOperator::GREATER_THAN_EQUAL:
        return GetBitmapGTE(constraint);
      case jonoondb_api::IndexConstraintOperator::MATCH:
        // TODO: Handle this
      default:
        std::ostringstream ss;
        ss << "IndexConstraintOperator type "
           << static_cast<std::int32_t>(constraint.op) << " is not valid.";
        throw JonoonDBException(ss.str(), __FILE__, __func__, __LINE__);
    }
  }

  std::shared_ptr<MamaJenniesBitmap> FilterRange(
      const Constraint& lowerConstraint,
      const Constraint& upperConstraint) override {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    auto lowerVal = GetOperandVal(lowerConstraint);
    auto upperVal = GetOperandVal(upperConstraint);

    if (lowerConstraint.op == IndexConstraintOperator::GREATER_THAN &&
        upperConstraint.op == IndexConstraintOperator::LESS_THAN) {
      for (size_t i = 0; i < m_dataVector.size(); i++) {
        if (m_dataVector[i] > lowerVal && m_dataVector[i] < upperVal &&
            !NullHelpers::IsNull(m_dataVector[i])) {
          bitmap->Add(i);
        }
      }
    } else if (lowerConstraint.op == IndexConstraintOperator::GREATER_THAN &&
               upperConstraint.op == IndexConstraintOperator::LESS_THAN_EQUAL) {
      for (size_t i = 0; i < m_dataVector.size(); i++) {
        if (m_dataVector[i] > lowerVal && m_dataVector[i] <= upperVal &&
            !NullHelpers::IsNull(m_dataVector[i])) {
          bitmap->Add(i);
        }
      }
    } else if (lowerConstraint.op ==
                   IndexConstraintOperator::GREATER_THAN_EQUAL &&
               upperConstraint.op == IndexConstraintOperator::LESS_THAN) {
      for (size_t i = 0; i < m_dataVector.size(); i++) {
        if (m_dataVector[i] >= lowerVal && m_dataVector[i] < upperVal &&
            !NullHelpers::IsNull(m_dataVector[i])) {
          bitmap->Add(i);
        }
      }
    } else if (lowerConstraint.op ==
                   IndexConstraintOperator::GREATER_THAN_EQUAL &&
               upperConstraint.op == IndexConstraintOperator::LESS_THAN_EQUAL) {
      for (size_t i = 0; i < m_dataVector.size(); i++) {
        if (m_dataVector[i] >= lowerVal && m_dataVector[i] <= upperVal &&
            !NullHelpers::IsNull(m_dataVector[i])) {
          bitmap->Add(i);
        }
      }
    }

    return bitmap;
  }

  bool TryGetStringValue(std::uint64_t documentID, std::string& val) override {
    if (documentID < m_dataVector.size()) {
      val = m_dataVector[documentID];
      return true;
    }

    return false;
  }

 private:
  inline std::string GetOperandVal(const Constraint& constraint) {
    std::string val;
    if (constraint.operandType == OperandType::INTEGER) {
      return std::to_string(constraint.operand.int64Val);
    } else if (constraint.operandType == OperandType::DOUBLE) {
      return std::to_string(constraint.operand.doubleVal);
    } else {
      return constraint.strVal;
    }
  }

  std::shared_ptr<MamaJenniesBitmap> GetBitmapEQ(const Constraint& constraint) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    auto val = GetOperandVal(constraint);

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] == val && !NullHelpers::IsNull(m_dataVector[i])) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  std::shared_ptr<MamaJenniesBitmap> GetBitmapLT(const Constraint& constraint) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    auto val = GetOperandVal(constraint);

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] < val && !NullHelpers::IsNull(m_dataVector[i])) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  std::shared_ptr<MamaJenniesBitmap> GetBitmapLTE(
      const Constraint& constraint) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    auto val = GetOperandVal(constraint);

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] <= val && !NullHelpers::IsNull(m_dataVector[i])) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  std::shared_ptr<MamaJenniesBitmap> GetBitmapGT(const Constraint& constraint) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    auto val = GetOperandVal(constraint);

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] > val && !NullHelpers::IsNull(m_dataVector[i])) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  std::shared_ptr<MamaJenniesBitmap> GetBitmapGTE(
      const Constraint& constraint) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    auto val = GetOperandVal(constraint);

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] >= val && !NullHelpers::IsNull(m_dataVector[i])) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  IndexStat m_indexStat;
  std::vector<std::string> m_fieldNameTokens;
  std::vector<std::string> m_dataVector;
  std::unique_ptr<Document> m_subDoc;
};
}  // namespace jonoondb_api
