#ifndef __XFDTD_CORE_TFSF_CORRECTOR_H__
#define __XFDTD_CORE_TFSF_CORRECTOR_H__

#include <xfdtd/common/index_task.h>
#include <xfdtd/common/type_define.h>

#include "waveform_source/waveform_source_corrector.h"
#include "xfdtd/coordinate_system/coordinate_system.h"
#include "xfdtd/electromagnetic_field/electromagnetic_field.h"
namespace xfdtd {

class TFSFCorrector : public WaveformSourceCorrector {
 public:
  TFSFCorrector(IndexTask task, IndexTask node_task, IndexTask global_task,
                GridSpace* grid_space, CalculationParam* calculation_param,
                EMF* emf, const Array1D<Real>* waveform, Index node_offset_i,
                Index node_offset_j, Index node_offset_k,
                const Array1D<Real>* projection_x_int,
                const Array1D<Real>* projection_y_int,
                const Array1D<Real>* projection_z_int,
                const Array1D<Real>* projection_x_half,
                const Array1D<Real>* projection_y_half,
                const Array1D<Real>* projection_z_half, Array2D<Real>* e_inc,
                Array2D<Real>* h_inc, Real cax, Real cbx, Real cay, Real cby,
                Real caz, Real cbz, Real transform_e_x = 0.0,
                Real transform_e_y = 0.0, Real transform_e_z = 0.0,
                Real transform_h_x = 0.0, Real transform_h_y = 0.0,
                Real transform_h_z = 0.0)
      : WaveformSourceCorrector{task,       node_task,         global_task,
                                grid_space, calculation_param, emf,
                                waveform},
        _node_offset_i{node_offset_i},
        _node_offset_j{node_offset_j},
        _node_offset_k{node_offset_k},
        _projection_x_int{projection_x_int},
        _projection_y_int{projection_y_int},
        _projection_z_int{projection_z_int},
        _projection_x_half{projection_x_half},
        _projection_y_half{projection_y_half},
        _projection_z_half{projection_z_half},
        _e_inc{e_inc},
        _h_inc{h_inc},
        _cax{cax},
        _cbx{cbx},
        _cay{cay},
        _cby{cby},
        _caz{caz},
        _cbz{cbz},
        _transform_e_x{transform_e_x},
        _transform_e_y{transform_e_y},
        _transform_e_z{transform_e_z},
        _transform_h_x{transform_h_x},
        _transform_h_y{transform_h_y},
        _transform_h_z{transform_h_z} {
    auto is = this->task().xRange().start();
    auto js = this->task().yRange().start();
    auto ks = this->task().zRange().start();
    auto ie = this->task().xRange().end();
    auto je = this->task().yRange().end();
    auto ke = this->task().zRange().end();

    _xn = is == globalTask().xRange().start();
    _xp = ie == globalTask().xRange().end();
    _yn = js == globalTask().yRange().start();
    _yp = je == globalTask().yRange().end();
    _zn = ks == globalTask().zRange().start();
    _zp = ke == globalTask().zRange().end();
  }

  ~TFSFCorrector() override = default;

  template <Axis::Direction direction>
  auto checkUpdate() const -> bool {
    if constexpr (direction == Axis::Direction::XN) {
      return _xn;
    } else if constexpr (direction == Axis::Direction::XP) {
      return _xp;
    } else if constexpr (direction == Axis::Direction::YN) {
      return _yn;
    } else if constexpr (direction == Axis::Direction::YP) {
      return _yp;
    } else if constexpr (direction == Axis::Direction::ZN) {
      return _zn;
    } else if constexpr (direction == Axis::Direction::ZP) {
      return _zp;
    }
  }

  template <Axis::Direction direction, EMF::Attribute attribute,
            Axis::XYZ field_xyz>
  auto correctTFSF(IndexTask task_in_global, Index offset_i, Index offset_j,
                   Index offset_k) {
    if (!checkUpdate<direction>()) {
      return;
    }

    constexpr auto xyz = Axis::fromDirectionToXYZ<direction>();
    constexpr auto dual_xyz_a = Axis::tangentialAAxis<xyz>();
    constexpr auto dual_xyz_b = Axis::tangentialBAxis<xyz>();

    static_assert(field_xyz == dual_xyz_a || field_xyz == dual_xyz_b,
                  "Field XYZ must be dual");

    // xyz \times field_xyz
    // constant auto dual_field_xyz = Axis::cross<xyz, field_xyz>();
    constexpr auto dual_field_xyz =
        (field_xyz == dual_xyz_a) ? dual_xyz_b : dual_xyz_a;

    constexpr auto dual_attribute = EMF::dualAttribute(attribute);

    auto [as, bs, cs] = transform::xYZToABC<Index, xyz>(
        task_in_global.xRange().start(), task_in_global.yRange().start(),
        task_in_global.zRange().start());
    auto [ae, be, ce] = transform::xYZToABC<Index, xyz>(
        task_in_global.xRange().end(), task_in_global.yRange().end(),
        task_in_global.zRange().end());

    // EA: [as, ae), [bs, be+1), [c]
    // EB: [as, ae+1), [bs, be), [c]
    // HA: [as, ae+1), [bs, be), [c]
    // HB: [as, ae), [bs, be+1), [c]
    constexpr auto offset_a =
        ((attribute == EMF::Attribute::E && field_xyz == dual_xyz_b) ||
         (attribute == EMF::Attribute::H && field_xyz == dual_xyz_a))
            ? 1
            : 0;
    constexpr auto offset_b = offset_a == 1 ? 0 : 1;
    // ae += offset_a;
    // be += offset_b;
    if constexpr (offset_a == 1) {
      if (ae == getNodeGlobalEnd<dual_xyz_a>()) {
        ae += offset_a;
      }
    }

    if constexpr (offset_b == 1) {
      if (be == getNodeGlobalEnd<dual_xyz_b>()) {
        be += offset_b;
      }
    }

    cs = (Axis::directionNegative<direction>()) ? cs : ce;
    ce = cs + 1;

    // E in total field need add incident for forward H.
    // H in scattered field need deduct incident for backward E.
    constexpr auto compensate_flag = 1;
    // E = E + c_h * \times H
    // H = H - c_e * \times E
    constexpr auto equation_flag = (attribute == EMF::Attribute::E) ? 1 : -1;
    // EA: \times H = ( \partial H_c / \partial b - \partial H_b / \partial c )
    // EB: \times H = ( \partial H_a / \partial c - \partial H_c / \partial a )
    constexpr auto different_flag = (field_xyz == dual_xyz_a) ? -1 : 1;
    // different is (index) - (index - 1)
    // Direction in negative: -
    // Direction in positive: +
    constexpr auto direction_flag =
        Axis::directionNegative<direction>() ? -1 : 1;
    // OK. we get coefficient flag
    constexpr auto coefficient_flag =
        compensate_flag * equation_flag * different_flag * direction_flag;

    if constexpr (direction == Axis::Direction::XN &&
                  attribute == EMF::Attribute::E && field_xyz == Axis::XYZ::Z) {
      static_assert(coefficient_flag == -1, "Coefficient flag error");
    }

    // Global index
    auto [is, js, ks] = transform::aBCToXYZ<Index, xyz>(as, bs, cs);
    auto [ie, je, ke] = transform::aBCToXYZ<Index, xyz>(ae, be, ce);

    const auto t = calculationParam()->timeParam()->currentTimeStep();

    auto emf = this->emf();
    for (Index i{is}; i < ie; ++i) {
      for (Index j{js}; j < je; ++j) {
        for (Index k{ks}; k < ke; ++k) {
          const auto i_node = i - offset_i;
          const auto j_node = j - offset_j;
          const auto k_node = k - offset_k;

          auto [a, b, c] = transform::xYZToABC<Index, xyz>(i, j, k);

          if constexpr (Axis::directionNegative<direction>()) {
            if constexpr (attribute == EMF::Attribute::E) {
              auto [i_dual, j_dual, k_dual] =
                  transform::aBCToXYZ<Index, xyz>(a, b, c - 1);
              const auto dual_incident_field_v =
                  getInc<dual_attribute, dual_field_xyz>(t, i_dual, j_dual,
                                                         k_dual);
              const auto coefficient =
                  getCoefficient<xyz, attribute>() * coefficient_flag;

              auto&& field_v =
                  emf->field<attribute, field_xyz>()(i_node, j_node, k_node);
              field_v += dual_incident_field_v * coefficient;
            } else {
              const auto dual_incident_field_v =
                  getInc<dual_attribute, dual_field_xyz>(t, i, j, k);

              const auto coefficient =
                  getCoefficient<xyz, attribute>() * coefficient_flag;

              auto [ii, jj, kk] = transform::aBCToXYZ<Index, xyz>(a, b, c - 1);
              const auto ii_node = ii - offset_i;
              const auto jj_node = jj - offset_j;
              const auto kk_node = kk - offset_k;
              auto&& field_v =
                  emf->field<attribute, field_xyz>()(ii_node, jj_node, kk_node);
              field_v += dual_incident_field_v * coefficient;
            }

            continue;
          }

          auto&& field_v =
              emf->field<attribute, field_xyz>()(i_node, j_node, k_node);

          const auto dual_incident_field_v =
              getInc<dual_attribute, dual_field_xyz>(t, i, j, k);

          const auto coefficient =
              getCoefficient<xyz, attribute>() * coefficient_flag;

          field_v += dual_incident_field_v * coefficient;
        }
      }
    }
  };

  template <EMF::Attribute attribute, Axis::XYZ xyz>
  auto getInc(Index t, Index i, Index j, Index k) const {
    if constexpr (attribute == EMF::Attribute::E) {
      if constexpr (xyz == Axis::XYZ::X) {
        return exInc(t, i, j, k);
      } else if constexpr (xyz == Axis::XYZ::Y) {
        return eyInc(t, i, j, k);
      } else if constexpr (xyz == Axis::XYZ::Z) {
        return ezInc(t, i, j, k);
      }
    } else {
      if constexpr (xyz == Axis::XYZ::X) {
        return hxInc(t, i, j, k);
      } else if constexpr (xyz == Axis::XYZ::Y) {
        return hyInc(t, i, j, k);
      } else if constexpr (xyz == Axis::XYZ::Z) {
        return hzInc(t, i, j, k);
      }
    }
  }

  template <Axis::XYZ xyz, EMF::Attribute attribute>
  auto getCoefficient() const {
    if constexpr (attribute == EMF::Attribute::E) {
      if constexpr (xyz == Axis::XYZ::X) {
        return cax();
      } else if constexpr (xyz == Axis::XYZ::Y) {
        return cay();
      } else if constexpr (xyz == Axis::XYZ::Z) {
        return caz();
      }
    } else {
      if constexpr (xyz == Axis::XYZ::X) {
        return cbx();
      } else if constexpr (xyz == Axis::XYZ::Y) {
        return cby();
      } else if constexpr (xyz == Axis::XYZ::Z) {
        return cbz();
      }
    }
  }

  auto exInc(Index t, Index i, Index j, Index k) const -> Real;

  auto eyInc(Index t, Index i, Index j, Index k) const -> Real;

  auto ezInc(Index t, Index i, Index j, Index k) const -> Real;

  auto hxInc(Index t, Index i, Index j, Index k) const -> Real;

  auto hyInc(Index t, Index i, Index j, Index k) const -> Real;

  auto hzInc(Index t, Index i, Index j, Index k) const -> Real;

  Real cax() const { return _cax; }

  Real cay() const { return _cay; }

  Real caz() const { return _caz; }

  Real cbx() const { return _cbx; }

  Real cby() const { return _cby; }

  Real cbz() const { return _cbz; }

  auto globalStartI() const { return globalTask().xRange().start(); }

  auto globalStartJ() const { return globalTask().yRange().start(); }

  auto globalStartK() const { return globalTask().zRange().start(); }

  auto globalEndI() const { return globalTask().xRange().end(); }

  auto globalEndJ() const { return globalTask().yRange().end(); }

  auto globalEndK() const { return globalTask().zRange().end(); }

  template <Axis::XYZ xyz>
  auto getNodeGlobalStart() const {
    if constexpr (xyz == Axis::XYZ::X) {
      return nodeTask().xRange().start() + _node_offset_i;
    } else if constexpr (xyz == Axis::XYZ::Y) {
      return nodeTask().yRange().start() + _node_offset_j;
    } else if constexpr (xyz == Axis::XYZ::Z) {
      return nodeTask().zRange().start() + _node_offset_k;
    }
  }

  template <Axis::XYZ xyz>
  auto getNodeGlobalEnd() const {
    if constexpr (xyz == Axis::XYZ::X) {
      return nodeTask().xRange().end() + _node_offset_i;
    } else if constexpr (xyz == Axis::XYZ::Y) {
      return nodeTask().yRange().end() + _node_offset_j;
    } else if constexpr (xyz == Axis::XYZ::Z) {
      return nodeTask().zRange().end() + _node_offset_k;
    }
  }

  std::string toString() const override;

 protected:
  Index _node_offset_i, _node_offset_j, _node_offset_k;

 private:
  bool _xn{false}, _xp{false}, _yn{false}, _yp{false}, _zn{false}, _zp{false};

  const Array1D<Real>* _projection_x_int;
  const Array1D<Real>* _projection_y_int;
  const Array1D<Real>* _projection_z_int;
  const Array1D<Real>* _projection_x_half;
  const Array1D<Real>* _projection_y_half;
  const Array1D<Real>* _projection_z_half;

  // IFA
  Array2D<Real>*_e_inc, *_h_inc;
  const Real _cax, _cbx, _cay, _cby, _caz, _cbz;
  const Real _transform_e_x, _transform_e_y, _transform_e_z, _transform_h_x,
      _transform_h_y, _transform_h_z;
};

class TFSF1DCorrector : public TFSFCorrector {
 public:
  TFSF1DCorrector(IndexTask task, IndexTask node_task, IndexTask global_task,
                  GridSpace* grid_space, CalculationParam* calculation_param,
                  EMF* emf, const Array1D<Real>* waveform, Index node_offset_k,
                  const Array1D<Real>* projection_x_int,
                  const Array1D<Real>* projection_y_int,
                  const Array1D<Real>* projection_z_int,
                  const Array1D<Real>* projection_x_half,
                  const Array1D<Real>* projection_y_half,
                  const Array1D<Real>* projection_z_half, Array2D<Real>* e_inc,
                  Array2D<Real>* h_inc, Real cax, Real cbx, Real cay, Real cby,
                  Real caz, Real cbz, Real transform_e_x, Real transform_e_y,
                  Real transform_e_z, Real transform_h_x, Real transform_h_y,
                  Real transform_h_z, bool forward)
      : TFSFCorrector{task,
                      node_task,
                      global_task,
                      grid_space,
                      calculation_param,
                      emf,
                      waveform,
                      0,
                      0,
                      node_offset_k,
                      projection_x_int,
                      projection_y_int,
                      projection_z_int,
                      projection_x_half,
                      projection_y_half,
                      projection_z_half,
                      e_inc,
                      h_inc,
                      cax,
                      cbx,
                      cay,
                      cby,
                      caz,
                      cbz,
                      transform_e_x,
                      transform_e_y,
                      transform_e_z,
                      transform_h_x,
                      transform_h_y,
                      transform_h_z},
        _forward{forward} {};

  ~TFSF1DCorrector() override = default;

  void correctE() override;

  void correctH() override;

 private:
  const bool _forward;
};

class TFSF2DCorrector : public TFSFCorrector {
 public:
  TFSF2DCorrector(IndexTask task, IndexTask node_task, IndexTask global_task,
                  GridSpace* grid_space, CalculationParam* calculation_param,
                  EMF* emf, const Array1D<Real>* waveform, Index node_offset_i,
                  Index node_offset_j, const Array1D<Real>* projection_x_int,
                  const Array1D<Real>* projection_y_int,
                  const Array1D<Real>* projection_z_int,
                  const Array1D<Real>* projection_x_half,
                  const Array1D<Real>* projection_y_half,
                  const Array1D<Real>* projection_z_half, Array2D<Real>* e_inc,
                  Array2D<Real>* h_inc, Real cax, Real cbx, Real cay, Real cby,
                  Real caz, Real cbz, Real transform_e_x, Real transform_e_y,
                  Real transform_e_z, Real transform_h_x, Real transform_h_y,
                  Real transform_h_z)
      : TFSFCorrector{task,
                      node_task,
                      global_task,
                      grid_space,
                      calculation_param,
                      emf,
                      waveform,
                      node_offset_i,
                      node_offset_j,
                      0,
                      projection_x_int,
                      projection_y_int,
                      projection_z_int,
                      projection_x_half,
                      projection_y_half,
                      projection_z_half,
                      e_inc,
                      h_inc,
                      cax,
                      cbx,
                      cay,
                      cby,
                      caz,
                      cbz,
                      transform_e_x,
                      transform_e_y,
                      transform_e_z,
                      transform_h_x,
                      transform_h_y,
                      transform_h_z} {};

  ~TFSF2DCorrector() override = default;

  void correctE() override;

  void correctH() override;

 private:
};

class TFSF3DCorrector : public TFSFCorrector {
 public:
  using TFSFCorrector::TFSFCorrector;

  ~TFSF3DCorrector() override = default;

  void correctE() override;

  void correctH() override;

 private:
};

}  // namespace xfdtd

#endif  // __XFDTD_CORE_TFSF_CORRECTOR_H__
