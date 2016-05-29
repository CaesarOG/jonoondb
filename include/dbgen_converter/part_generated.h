// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_PART_DBGEN_CONVERTER_H_
#define FLATBUFFERS_GENERATED_PART_DBGEN_CONVERTER_H_

#include "flatbuffers/flatbuffers.h"

#include "customer_generated.h"
#include "lineitem_generated.h"
#include "nation_generated.h"
#include "orders_generated.h"

namespace dbgen_converter {
struct CUSTOMER;
}  // namespace dbgen_converter
namespace dbgen_converter {
struct LINEITEM;
}  // namespace dbgen_converter
namespace dbgen_converter {
struct NATION;
}  // namespace dbgen_converter
namespace dbgen_converter {
struct ORDERS;
}  // namespace dbgen_converter

namespace dbgen_converter {

struct PART;

struct PART FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  int32_t P_PARTKEY() const { return GetField<int32_t>(4, 0); }
  const flatbuffers::String *P_NAME() const { return GetPointer<const flatbuffers::String *>(6); }
  const flatbuffers::String *P_MFGR() const { return GetPointer<const flatbuffers::String *>(8); }
  const flatbuffers::String *P_BRAND() const { return GetPointer<const flatbuffers::String *>(10); }
  const flatbuffers::String *P_TYPE() const { return GetPointer<const flatbuffers::String *>(12); }
  int32_t P_SIZE() const { return GetField<int32_t>(14, 0); }
  const flatbuffers::String *P_CONTAINER() const { return GetPointer<const flatbuffers::String *>(16); }
  double P_RETAILPRICE() const { return GetField<double>(18, 0); }
  const flatbuffers::String *P_COMMENT() const { return GetPointer<const flatbuffers::String *>(20); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, 4 /* P_PARTKEY */) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 6 /* P_NAME */) &&
           verifier.Verify(P_NAME()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 8 /* P_MFGR */) &&
           verifier.Verify(P_MFGR()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 10 /* P_BRAND */) &&
           verifier.Verify(P_BRAND()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 12 /* P_TYPE */) &&
           verifier.Verify(P_TYPE()) &&
           VerifyField<int32_t>(verifier, 14 /* P_SIZE */) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 16 /* P_CONTAINER */) &&
           verifier.Verify(P_CONTAINER()) &&
           VerifyField<double>(verifier, 18 /* P_RETAILPRICE */) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 20 /* P_COMMENT */) &&
           verifier.Verify(P_COMMENT()) &&
           verifier.EndTable();
  }
};

struct PARTBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_P_PARTKEY(int32_t P_PARTKEY) { fbb_.AddElement<int32_t>(4, P_PARTKEY, 0); }
  void add_P_NAME(flatbuffers::Offset<flatbuffers::String> P_NAME) { fbb_.AddOffset(6, P_NAME); }
  void add_P_MFGR(flatbuffers::Offset<flatbuffers::String> P_MFGR) { fbb_.AddOffset(8, P_MFGR); }
  void add_P_BRAND(flatbuffers::Offset<flatbuffers::String> P_BRAND) { fbb_.AddOffset(10, P_BRAND); }
  void add_P_TYPE(flatbuffers::Offset<flatbuffers::String> P_TYPE) { fbb_.AddOffset(12, P_TYPE); }
  void add_P_SIZE(int32_t P_SIZE) { fbb_.AddElement<int32_t>(14, P_SIZE, 0); }
  void add_P_CONTAINER(flatbuffers::Offset<flatbuffers::String> P_CONTAINER) { fbb_.AddOffset(16, P_CONTAINER); }
  void add_P_RETAILPRICE(double P_RETAILPRICE) { fbb_.AddElement<double>(18, P_RETAILPRICE, 0); }
  void add_P_COMMENT(flatbuffers::Offset<flatbuffers::String> P_COMMENT) { fbb_.AddOffset(20, P_COMMENT); }
  PARTBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  PARTBuilder &operator=(const PARTBuilder &);
  flatbuffers::Offset<PART> Finish() {
    auto o = flatbuffers::Offset<PART>(fbb_.EndTable(start_, 9));
    return o;
  }
};

inline flatbuffers::Offset<PART> CreatePART(flatbuffers::FlatBufferBuilder &_fbb,
   int32_t P_PARTKEY = 0,
   flatbuffers::Offset<flatbuffers::String> P_NAME = 0,
   flatbuffers::Offset<flatbuffers::String> P_MFGR = 0,
   flatbuffers::Offset<flatbuffers::String> P_BRAND = 0,
   flatbuffers::Offset<flatbuffers::String> P_TYPE = 0,
   int32_t P_SIZE = 0,
   flatbuffers::Offset<flatbuffers::String> P_CONTAINER = 0,
   double P_RETAILPRICE = 0,
   flatbuffers::Offset<flatbuffers::String> P_COMMENT = 0) {
  PARTBuilder builder_(_fbb);
  builder_.add_P_RETAILPRICE(P_RETAILPRICE);
  builder_.add_P_COMMENT(P_COMMENT);
  builder_.add_P_CONTAINER(P_CONTAINER);
  builder_.add_P_SIZE(P_SIZE);
  builder_.add_P_TYPE(P_TYPE);
  builder_.add_P_BRAND(P_BRAND);
  builder_.add_P_MFGR(P_MFGR);
  builder_.add_P_NAME(P_NAME);
  builder_.add_P_PARTKEY(P_PARTKEY);
  return builder_.Finish();
}

inline const dbgen_converter::PART *GetPART(const void *buf) { return flatbuffers::GetRoot<dbgen_converter::PART>(buf); }

inline bool VerifyPARTBuffer(flatbuffers::Verifier &verifier) { return verifier.VerifyBuffer<dbgen_converter::PART>(); }

inline void FinishPARTBuffer(flatbuffers::FlatBufferBuilder &fbb, flatbuffers::Offset<dbgen_converter::PART> root) { fbb.Finish(root); }

}  // namespace dbgen_converter

#endif  // FLATBUFFERS_GENERATED_PART_DBGEN_CONVERTER_H_