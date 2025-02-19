#pragma once

#include <cmath>
#include <cstdint>
#include <limits>
#include <memory>
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
#include "string_utils.h"

namespace jonoondb_api {
template <typename T>
class VectorIntegerIndexer final : public Indexer {
 public:
  VectorIntegerIndexer(const IndexInfoImpl& indexInfo,
                       const FieldType& fieldType) {
    std::string errorMsg;
    if (indexInfo.GetIndexName().size() == 0) {
      errorMsg = "Argument indexInfo has empty name.";
    } else if (indexInfo.GetColumnName().size() == 0) {
      errorMsg = "Argument indexInfo has empty column name.";
    } else if (indexInfo.GetType() != IndexType::VECTOR) {
      errorMsg =
          "Argument indexInfo can only have IndexType VECTOR for "
          "VectorIntegerIndexer.";
    } else if (!IsValidFieldType(fieldType)) {
      std::ostringstream ss;
      ss << "Argument fieldType " << GetFieldString(fieldType)
         << " is not valid for VectorIntegerIndexer.";
      errorMsg = ss.str();
    }

    if (errorMsg.length() > 0) {
      throw InvalidArgumentException(errorMsg, __FILE__, __func__, __LINE__);
    }

    m_fieldNameTokens = StringUtils::Split(indexInfo.GetColumnName(), ".");
    m_indexStat = IndexStat(indexInfo, fieldType);
  }

  static bool IsValidFieldType(FieldType fieldType) {
    return (fieldType == FieldType::INT8 || fieldType == FieldType::INT16 ||
            fieldType == FieldType::INT32 || fieldType == FieldType::INT64);
  }

  void Insert(std::uint64_t documentID, const Document& document) override {
    auto val =
        DocumentUtils::GetIntegerValue(document, m_subDoc, m_fieldNameTokens);
    assert(m_dataVector.size() == documentID);
    assert(val <= std::numeric_limits<T>::max());
    assert(val >= std::numeric_limits<T>::min());
    // We create this class with int32_t as T for int32 and smaller types
    // So we should never get a overflow situation for int32_t. However it is
    // technically possible to abuse this situation hence adding the asserts
    // above to catch any misuse atleast in debug build
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
        return GetBitmapLT(constraint, false);
      case jonoondb_api::IndexConstraintOperator::LESS_THAN_EQUAL:
        return GetBitmapLT(constraint, true);
      case jonoondb_api::IndexConstraintOperator::GREATER_THAN:
        return GetBitmapGT(constraint, false);
      case jonoondb_api::IndexConstraintOperator::GREATER_THAN_EQUAL:
        return GetBitmapGT(constraint, true);
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
    std::int64_t lowerVal, upperVal;

    if (lowerConstraint.operandType == OperandType::DOUBLE) {
      lowerVal =
          static_cast<int64_t>(std::floor(lowerConstraint.operand.doubleVal));
    } else {
      if (lowerConstraint.op == IndexConstraintOperator::GREATER_THAN_EQUAL) {
        // Subtracting 1 here allows us to use > operator in the loop below
        lowerVal = lowerConstraint.operand.int64Val - 1;
      } else {
        lowerVal = lowerConstraint.operand.int64Val;
      }
    }

    if (upperConstraint.operandType == OperandType::DOUBLE) {
      upperVal =
          static_cast<int64_t>(std::ceil(upperConstraint.operand.doubleVal));
    } else {
      if (upperConstraint.op == IndexConstraintOperator::LESS_THAN_EQUAL) {
        // Adding 1 here allows us to use < operator in the loop below
        upperVal = upperConstraint.operand.int64Val + 1;
      } else {
        upperVal = upperConstraint.operand.int64Val;
      }
    }

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] > lowerVal && m_dataVector[i] < upperVal) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  bool TryGetIntegerValue(std::uint64_t documentID,
                          std::int64_t& val) override {
    if (documentID < m_dataVector.size()) {
      val = m_dataVector[documentID];
      return true;
    }

    return false;
  }

  bool TryGetIntegerVector(const gsl::span<std::uint64_t>& documentIDs,
                           std::vector<std::int64_t>& values) override {
    assert(documentIDs.size() == values.size());
    for (auto i = 0; i < documentIDs.size(); i++) {
      if (documentIDs[i] >= m_dataVector.size()) {
        return false;
      }
      values[i] = m_dataVector[documentIDs[i]];
    }

    return true;
  }

 private:
  std::shared_ptr<MamaJenniesBitmap> GetBitmapEQ(const Constraint& constraint) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    if (constraint.operandType == OperandType::INTEGER) {
      for (size_t i = 0; i < m_dataVector.size(); i++) {
        if (m_dataVector[i] == constraint.operand.int64Val) {
          bitmap->Add(i);
        }
      }
    } else if (constraint.operandType == OperandType::DOUBLE) {
      // Check if double has no fractional part. If it has fractional part
      // then it can't be equal to any integer
      std::int64_t intVal =
          static_cast<std::int64_t>(constraint.operand.doubleVal);
      if (constraint.operand.doubleVal == intVal) {
        for (size_t i = 0; i < m_dataVector.size(); i++) {
          if (m_dataVector[i] == intVal) {
            bitmap->Add(i);
          }
        }
      }
    }

    // In all other cases the operand cannot be equal. The cases are:
    // Operand is a string value, this should not happen because the query
    // should fail before reaching this point
    return bitmap;
  }

  std::shared_ptr<MamaJenniesBitmap> GetBitmapLT(const Constraint& constraint,
                                                 bool orEqual) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    int64_t valToCmp;
    if (constraint.operandType == OperandType::DOUBLE) {
      valToCmp = static_cast<int64_t>(std::ceil(constraint.operand.doubleVal));
    } else {
      if (orEqual) {
        // Adding 1 here allows us to use < operator in the loop below
        valToCmp = constraint.operand.int64Val + 1;
      } else {
        valToCmp = constraint.operand.int64Val;
      }
    }

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] < valToCmp) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  std::shared_ptr<MamaJenniesBitmap> GetBitmapGT(const Constraint& constraint,
                                                 bool orEqual) {
    auto bitmap = std::make_shared<MamaJenniesBitmap>();
    int64_t valToCmp;
    if (constraint.operandType == OperandType::DOUBLE) {
      valToCmp = static_cast<int64_t>(std::floor(constraint.operand.doubleVal));
    } else {
      if (orEqual) {
        // Subtracting 1 here allows us to use > operator in the loop below
        valToCmp = constraint.operand.int64Val - 1;
      } else {
        valToCmp = constraint.operand.int64Val;
      }
    }

    for (size_t i = 0; i < m_dataVector.size(); i++) {
      if (m_dataVector[i] > valToCmp) {
        bitmap->Add(i);
      }
    }

    return bitmap;
  }

  IndexStat m_indexStat;
  std::vector<std::string> m_fieldNameTokens;
  std::vector<T> m_dataVector;
  std::unique_ptr<Document> m_subDoc;
};
}  // namespace jonoondb_api
