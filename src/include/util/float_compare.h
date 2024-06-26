#ifndef _XFDTD_CORE_FLOAT_COMPARE_H_
#define _XFDTD_CORE_FLOAT_COMPARE_H_

#include <xfdtd/common/type_define.h>
#include <xfdtd/exception/exception.h>

#include <cstdlib>

namespace xfdtd {

enum class FloatCompareOperator {
  Equal,
  NotEqual,
  Less,
  LessEqual,
  Greater,
  GreaterEqual,
};

inline bool floatCompareEqual(Real lhs, Real rhs, Real epsilon = 1e-8) {
  return std::abs(lhs - rhs) <= epsilon;
}

inline bool floatCompare(Real lhs, Real rhs, FloatCompareOperator op,
                         Real epsilon = 1e-8) {
  switch (op) {
    case FloatCompareOperator::Equal:
      return floatCompareEqual(lhs, rhs, epsilon);
    case FloatCompareOperator::NotEqual:
      return !floatCompareEqual(lhs, rhs, epsilon);
    case FloatCompareOperator::Less:
      return lhs < rhs;
    case FloatCompareOperator::LessEqual:
      return floatCompareEqual(lhs, rhs, epsilon) ? true : lhs < rhs;
    case FloatCompareOperator::Greater:
      return lhs > rhs;
    case FloatCompareOperator::GreaterEqual:
      return floatCompareEqual(lhs, rhs, epsilon) ? true : lhs > rhs;
    default:
      throw XFDTDException("Invalid FloatCompareOperator");
  }
}

}  // namespace xfdtd

#endif  // _XFDTD_CORE_FLOAT_COMPARE_H_
