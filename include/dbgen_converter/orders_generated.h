// automatically generated by the FlatBuffers compiler, do not modify

#ifndef FLATBUFFERS_GENERATED_ORDERS_DBGEN_CONVERTER_H_
#define FLATBUFFERS_GENERATED_ORDERS_DBGEN_CONVERTER_H_

#include "flatbuffers/flatbuffers.h"

#include "customer_generated.h"
#include "lineitem_generated.h"
#include "nation_generated.h"

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

struct ORDERS FLATBUFFERS_FINAL_CLASS : private flatbuffers::Table {
  int32_t O_ORDERKEY() const { return GetField<int32_t>(4, 0); }
  int32_t O_CUSTKEY() const { return GetField<int32_t>(6, 0); }
  const flatbuffers::String *O_ORDERSTATUS() const { return GetPointer<const flatbuffers::String *>(8); }
  double O_TOTALPRICE() const { return GetField<double>(10, 0); }
  const flatbuffers::String *O_ORDERDATE() const { return GetPointer<const flatbuffers::String *>(12); }
  const flatbuffers::String *O_ORDERPRIORITY() const { return GetPointer<const flatbuffers::String *>(14); }
  const flatbuffers::String *O_CLERK() const { return GetPointer<const flatbuffers::String *>(16); }
  int32_t O_SHIPPRIORITY() const { return GetField<int32_t>(18, 0); }
  const flatbuffers::String *O_COMMENT() const { return GetPointer<const flatbuffers::String *>(20); }
  bool Verify(flatbuffers::Verifier &verifier) const {
    return VerifyTableStart(verifier) &&
           VerifyField<int32_t>(verifier, 4 /* O_ORDERKEY */) &&
           VerifyField<int32_t>(verifier, 6 /* O_CUSTKEY */) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 8 /* O_ORDERSTATUS */) &&
           verifier.Verify(O_ORDERSTATUS()) &&
           VerifyField<double>(verifier, 10 /* O_TOTALPRICE */) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 12 /* O_ORDERDATE */) &&
           verifier.Verify(O_ORDERDATE()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 14 /* O_ORDERPRIORITY */) &&
           verifier.Verify(O_ORDERPRIORITY()) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 16 /* O_CLERK */) &&
           verifier.Verify(O_CLERK()) &&
           VerifyField<int32_t>(verifier, 18 /* O_SHIPPRIORITY */) &&
           VerifyField<flatbuffers::uoffset_t>(verifier, 20 /* O_COMMENT */) &&
           verifier.Verify(O_COMMENT()) &&
           verifier.EndTable();
  }
};

struct ORDERSBuilder {
  flatbuffers::FlatBufferBuilder &fbb_;
  flatbuffers::uoffset_t start_;
  void add_O_ORDERKEY(int32_t O_ORDERKEY) { fbb_.AddElement<int32_t>(4, O_ORDERKEY, 0); }
  void add_O_CUSTKEY(int32_t O_CUSTKEY) { fbb_.AddElement<int32_t>(6, O_CUSTKEY, 0); }
  void add_O_ORDERSTATUS(flatbuffers::Offset<flatbuffers::String> O_ORDERSTATUS) { fbb_.AddOffset(8, O_ORDERSTATUS); }
  void add_O_TOTALPRICE(double O_TOTALPRICE) { fbb_.AddElement<double>(10, O_TOTALPRICE, 0); }
  void add_O_ORDERDATE(flatbuffers::Offset<flatbuffers::String> O_ORDERDATE) { fbb_.AddOffset(12, O_ORDERDATE); }
  void add_O_ORDERPRIORITY(flatbuffers::Offset<flatbuffers::String> O_ORDERPRIORITY) { fbb_.AddOffset(14, O_ORDERPRIORITY); }
  void add_O_CLERK(flatbuffers::Offset<flatbuffers::String> O_CLERK) { fbb_.AddOffset(16, O_CLERK); }
  void add_O_SHIPPRIORITY(int32_t O_SHIPPRIORITY) { fbb_.AddElement<int32_t>(18, O_SHIPPRIORITY, 0); }
  void add_O_COMMENT(flatbuffers::Offset<flatbuffers::String> O_COMMENT) { fbb_.AddOffset(20, O_COMMENT); }
  ORDERSBuilder(flatbuffers::FlatBufferBuilder &_fbb) : fbb_(_fbb) { start_ = fbb_.StartTable(); }
  ORDERSBuilder &operator=(const ORDERSBuilder &);
  flatbuffers::Offset<ORDERS> Finish() {
    auto o = flatbuffers::Offset<ORDERS>(fbb_.EndTable(start_, 9));
    return o;
  }
};

inline flatbuffers::Offset<ORDERS> CreateORDERS(flatbuffers::FlatBufferBuilder &_fbb,
   int32_t O_ORDERKEY = 0,
   int32_t O_CUSTKEY = 0,
   flatbuffers::Offset<flatbuffers::String> O_ORDERSTATUS = 0,
   double O_TOTALPRICE = 0,
   flatbuffers::Offset<flatbuffers::String> O_ORDERDATE = 0,
   flatbuffers::Offset<flatbuffers::String> O_ORDERPRIORITY = 0,
   flatbuffers::Offset<flatbuffers::String> O_CLERK = 0,
   int32_t O_SHIPPRIORITY = 0,
   flatbuffers::Offset<flatbuffers::String> O_COMMENT = 0) {
  ORDERSBuilder builder_(_fbb);
  builder_.add_O_TOTALPRICE(O_TOTALPRICE);
  builder_.add_O_COMMENT(O_COMMENT);
  builder_.add_O_SHIPPRIORITY(O_SHIPPRIORITY);
  builder_.add_O_CLERK(O_CLERK);
  builder_.add_O_ORDERPRIORITY(O_ORDERPRIORITY);
  builder_.add_O_ORDERDATE(O_ORDERDATE);
  builder_.add_O_ORDERSTATUS(O_ORDERSTATUS);
  builder_.add_O_CUSTKEY(O_CUSTKEY);
  builder_.add_O_ORDERKEY(O_ORDERKEY);
  return builder_.Finish();
}

inline const dbgen_converter::ORDERS *GetORDERS(const void *buf) { return flatbuffers::GetRoot<dbgen_converter::ORDERS>(buf); }

inline bool VerifyORDERSBuffer(flatbuffers::Verifier &verifier) { return verifier.VerifyBuffer<dbgen_converter::ORDERS>(); }

inline void FinishORDERSBuffer(flatbuffers::FlatBufferBuilder &fbb, flatbuffers::Offset<dbgen_converter::ORDERS> root) { fbb.Finish(root); }

}  // namespace dbgen_converter

#endif  // FLATBUFFERS_GENERATED_ORDERS_DBGEN_CONVERTER_H_