#include "index_stat.h"

using namespace jonoondb_api;

IndexStat::IndexStat() : m_fieldType(FieldType::INT32) {}

IndexStat::IndexStat(const IndexInfoImpl& indexInfo, FieldType fieldType)
    : m_indexInfo(indexInfo), m_fieldType(fieldType) {}

const IndexInfoImpl& IndexStat::GetIndexInfo() const {
  return m_indexInfo;
}

FieldType IndexStat::GetFieldType() const {
  return m_fieldType;
}