#ifndef __XFDTD_CORE_DRUDE_ADE_METHOD_H__
#define __XFDTD_CORE_DRUDE_ADE_METHOD_H__

#include <xfdtd/common/type_define.h>
#include <xfdtd/material/dispersive_material.h>

#include <memory>

#include "updator/dispersive_material_update_method/dispersive_material_update_method.h"

namespace xfdtd {

class DrudeADEMethod : public LinearDispersiveMaterialUpdateMethod {
 public:
  static auto k(const DrudeEqDecision& eq, Real dt) -> Array1D<Real>;

  static auto beta(const DrudeEqDecision& eq, Real dt) -> Array1D<Real>;

 public:
  DrudeADEMethod(Real epsilon_inf, std::shared_ptr<DrudeEqDecision> drude_eq);

  ~DrudeADEMethod() override;

  auto clone() const
      -> std::unique_ptr<LinearDispersiveMaterialUpdateMethod> override {
    return std::make_unique<DrudeADEMethod>(*this);
  }

  auto init(Real dt) -> void override;

  auto correctCoeff(Index i, Index j, Index k, const GridSpace* grid_space,
                    CalculationParam* calculation_param) const -> void override;

  auto initUpdate(const GridSpace* grid_space,
                  std::shared_ptr<const CalculationParam> calculation_param,
                  std::shared_ptr<EMF> emf, Index m_index,
                  const IndexTask& task) -> void override;

  auto updateEx(Index i, Index j, Index k) -> void override;

  auto updateEy(Index i, Index j, Index k) -> void override;

  auto updateEz(Index i, Index j, Index k) -> void override;

  auto updateTEM(Index i, Index j, Index k) -> void override;

 private:
  std::shared_ptr<DrudeEqDecision> _drude_eq;

  Real _sum_beta;

  Array1D<Real> _coeff_j_sum_j, _coeff_j_j, _coeff_j_e;

  Real _coeff_e_j_sum;

  Array4D<Real> _jx, _jy, _jz;

  inline auto a(const auto& epsilon_inf, const auto& epsilon_0,
                const auto& sum_beta, const auto& dt, const auto& sigma) const {
    return (2 * epsilon_0 * epsilon_inf - dt * sum_beta - dt * sigma) /
           (2 * epsilon_0 * epsilon_inf - dt * sum_beta + dt * sigma);
  }

  inline auto b(const auto& epsilon_inf, const auto& epsilon_0,
                const auto& sum_beta, const auto& dt, const auto& sigma) const {
    return (2 * dt) /
           (2 * epsilon_0 * epsilon_inf - dt * sum_beta + dt * sigma);
  }

  auto calculateJSum(Index i, Index j, Index k,
                     const Array4D<Real>& j_arr) const -> Real;

  auto updateJ(Index i, Index j, Index k, Real e_next, Real e_cur,
               Array4D<Real>& j_arr) -> void;
};

}  // namespace xfdtd

#endif  // __XFDTD_CORE_DRUDE_ADE_METHOD_H__
