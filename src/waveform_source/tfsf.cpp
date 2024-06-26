#include <xfdtd/common/constant.h>
#include <xfdtd/common/index_task.h>
#include <xfdtd/grid_space/grid_space.h>
#include <xfdtd/waveform_source/tfsf.h>

#include <cmath>
#include <cstdlib>

#include "xfdtd/common/type_define.h"
#include "xfdtd/coordinate_system/coordinate_system.h"

namespace xfdtd {

TFSF::TFSF(std::size_t x, std::size_t y, std::size_t z, Real theta, Real phi,
           Real psi, std::unique_ptr<Waveform> waveform)
    : WaveformSource{std::move(waveform)},
      _x{x},
      _y{y},
      _z{z},
      _theta{theta},
      _phi{phi},
      _psi{psi},
      _sin_theta{std::sin(theta)},
      _cos_theta{std::cos(theta)},
      _sin_phi{std::sin(phi)},
      _cos_phi{std::cos(phi)},
      _sin_psi{std::sin(psi)},
      _cos_psi{std::cos(psi)} {
  initTransform();
}

void TFSF::correctMaterialSpace() {}

void TFSF::correctUpdateCoefficient() {}

void TFSF::initTimeDependentVariable() {
  waveform()->init(calculationParamPtr()->timeParam()->eTime() -
                   calculationParamPtr()->timeParam()->dt());
}

void TFSF::updateWaveformSource() {
  auto x{_e_inc(_e_inc.size() - 2)};
  auto y{_e_inc(_e_inc.size() - 1)};
  _e_inc(0) = waveform()->value()(
      calculationParamPtr()->timeParam()->currentTimeStep());
  for (std::size_t i{1}; i < _e_inc.size() - 1; ++i) {
    _e_inc(i) = _ceie * _e_inc(i) + _ceihi * (_h_inc(i) - _h_inc(i - 1));
  }

  _e_inc(_e_inc.size() - 1) = -_a +
                              _abc_coff_0 * (_e_inc(_e_inc.size() - 2) + _b) +
                              _abc_coff_1 * (x + y);
  _a = x;
  _b = y;

  for (std::size_t i{0}; i < _h_inc.size(); ++i) {
    _h_inc(i) = _chih * _h_inc(i) + _chiei * (_e_inc(i + 1) - _e_inc(i));
  }

  _ex_inc = _transform_e.x() * _e_inc;
  _ey_inc = _transform_e.y() * _e_inc;
  _ez_inc = _transform_e.z() * _e_inc;
  _hx_inc = _transform_h.x() * _h_inc;
  _hy_inc = _transform_h.y() * _h_inc;
  _hz_inc = _transform_h.z() * _h_inc;
}

std::size_t TFSF::x() const { return _x; }

std::size_t TFSF::y() const { return _y; }

std::size_t TFSF::z() const { return _z; }

Real TFSF::theta() const { return _theta; }

Real TFSF::phi() const { return _phi; }

Real TFSF::psi() const { return _psi; }

Real TFSF::sinTheta() const { return _sin_theta; }

Real TFSF::cosTheta() const { return _cos_theta; }

Real TFSF::sinPhi() const { return _sin_phi; }

Real TFSF::cosPhi() const { return _cos_phi; }

Real TFSF::sinPsi() const { return _sin_psi; }

Real TFSF::cosPsi() const { return _cos_psi; }

Vector TFSF::k() const { return _k; }

GridBox TFSF::globalBox() const { return _global_box; }

IndexTask TFSF::taskXN() const {
  const auto is = globalBox().origin().i();
  const auto ie = globalBox().end().i();
  const auto js = globalBox().origin().j();
  const auto je = globalBox().end().j();
  const auto ks = globalBox().origin().k();
  const auto ke = globalBox().end().k();

  return makeIndexTask(makeIndexRange(is, is + 1), makeIndexRange(js, je),
                       makeIndexRange(ks, ke));
}

IndexTask TFSF::taskXP() const {
  const auto is = globalBox().origin().i();
  const auto ie = globalBox().end().i();
  const auto js = globalBox().origin().j();
  const auto je = globalBox().end().j();
  const auto ks = globalBox().origin().k();
  const auto ke = globalBox().end().k();

  return makeIndexTask(makeIndexRange(ie, ie + 1), makeIndexRange(js, je),
                       makeIndexRange(ks, ke));
}

IndexTask TFSF::taskYN() const {
  const auto is = globalBox().origin().i();
  const auto ie = globalBox().end().i();
  const auto js = globalBox().origin().j();
  const auto je = globalBox().end().j();
  const auto ks = globalBox().origin().k();
  const auto ke = globalBox().end().k();

  return makeIndexTask(makeIndexRange(is, ie), makeIndexRange(js, js + 1),
                       makeIndexRange(ks, ke));
}

IndexTask TFSF::taskYP() const {
  const auto is = globalBox().origin().i();
  const auto ie = globalBox().end().i();
  const auto js = globalBox().origin().j();
  const auto je = globalBox().end().j();
  const auto ks = globalBox().origin().k();
  const auto ke = globalBox().end().k();

  return makeIndexTask(makeIndexRange(is, ie), makeIndexRange(je, je + 1),
                       makeIndexRange(ks, ke));
}

IndexTask TFSF::taskZN() const {
  const auto is = globalBox().origin().i();
  const auto ie = globalBox().end().i();
  const auto js = globalBox().origin().j();
  const auto je = globalBox().end().j();
  const auto ks = globalBox().origin().k();
  const auto ke = globalBox().end().k();

  return makeIndexTask(makeIndexRange(is, ie), makeIndexRange(js, je),
                       makeIndexRange(ks, ks + 1));
}

IndexTask TFSF::taskZP() const {
  const auto is = globalBox().origin().i();
  const auto ie = globalBox().end().i();
  const auto js = globalBox().origin().j();
  const auto je = globalBox().end().j();
  const auto ks = globalBox().origin().k();
  const auto ke = globalBox().end().k();

  return makeIndexTask(makeIndexRange(is, ie), makeIndexRange(js, je),
                       makeIndexRange(ke, ke + 1));
}

IndexTask TFSF::globalEyTaskXN() const {
  auto task = taskXN();
  return makeTask(
      task.xRange(), task.yRange(),
      makeIndexRange(task.zRange().start(), task.zRange().end() + 1));
}

IndexTask TFSF::globalEzTaskXN() const {
  auto task = taskXN();
  return makeTask(
      task.xRange(),
      makeIndexRange(task.yRange().start(), task.yRange().end() + 1),
      task.zRange());
}

IndexTask TFSF::globalEyTaskXP() const {
  auto task = taskXP();
  return makeTask(
      task.xRange(), task.yRange(),
      makeIndexRange(task.zRange().start(), task.zRange().end() + 1));
}

IndexTask TFSF::globalEzTaskXP() const {
  auto task = taskXP();
  return makeIndexTask(
      task.xRange(),
      makeIndexRange(task.yRange().start(), task.yRange().end() + 1),
      task.zRange());
}

IndexTask TFSF::globalEzTaskYN() const {
  auto task = taskYN();
  return makeIndexTask(
      makeIndexRange(task.xRange().start(), task.xRange().end() + 1),
      task.yRange(), task.zRange());
}

IndexTask TFSF::globalExTaskYN() const {
  auto task = taskYN();
  return makeIndexTask(
      task.xRange(), task.yRange(),
      makeIndexRange(task.zRange().start(), task.zRange().end() + 1));
}

IndexTask TFSF::globalEzTaskYP() const {
  auto task = taskYP();
  return makeIndexTask(
      makeIndexRange(task.xRange().start(), task.xRange().end() + 1),
      task.yRange(), task.zRange());
}

IndexTask TFSF::globalExTaskYP() const {
  auto task = taskYP();
  return makeIndexTask(
      task.xRange(), task.yRange(),
      makeIndexRange(task.zRange().start(), task.zRange().end() + 1));
}

IndexTask TFSF::globalExTaskZN() const {
  auto task = taskZN();
  return makeIndexTask(
      task.xRange(),
      makeIndexRange(task.yRange().start(), task.yRange().end() + 1),
      task.zRange());
}

IndexTask TFSF::globalEyTaskZN() const {
  auto task = taskZN();
  return makeIndexTask(
      makeIndexRange(task.xRange().start(), task.xRange().end() + 1),
      task.yRange(), task.zRange());
}

IndexTask TFSF::globalExTaskZP() const {
  auto task = taskZP();
  return makeIndexTask(
      task.xRange(),
      makeIndexRange(task.yRange().start(), task.yRange().end() + 1),
      task.zRange());
}

IndexTask TFSF::globalEyTaskZP() const {
  auto task = taskZP();
  return makeIndexTask(
      makeIndexRange(task.xRange().start(), task.xRange().end() + 1),
      task.yRange(), task.zRange());
}

void TFSF::defaultInit(std::shared_ptr<GridSpace> grid_space,
                       std::shared_ptr<CalculationParam> calculation_param,
                       std::shared_ptr<EMF> emf) {
  WaveformSource::defaultInit(std::move(grid_space),
                              std::move(calculation_param), std::move(emf));
  if (gridSpacePtr()->type() != GridSpace::Type::UNIFORM) {
    throw std::runtime_error("TFSF only supports uniform grid space");
  }

  auto global_grid_space = gridSpacePtr()->globalGridSpace();
  if (global_grid_space == nullptr) {
    throw std::runtime_error("TFSF need global grid space");
  }

  auto origin_x{global_grid_space->box().origin().i() + x()};
  auto origin_y{global_grid_space->box().origin().j() + y()};
  auto origin_z{global_grid_space->box().origin().k() + z()};
  auto size_x{global_grid_space->box().size().i() - 2 * x()};
  auto size_y{global_grid_space->box().size().j() - 2 * y()};
  auto size_z{global_grid_space->box().size().k() - 2 * z()};

  _global_box =
      GridBox{Grid{origin_x, origin_y, origin_z}, Grid{size_x, size_y, size_z}};

  _ratio_delta =
      1 / std::sqrt(std::pow(sinTheta(), 4) *
                        (std::pow(cosPhi(), 4) + std::pow(sinPhi(), 4)) +
                    std::pow(cosTheta(), 4));
  _auxiliary_size =
      std::ceil(_ratio_delta *
                (std::sqrt(pow(size_x, 2) + pow(size_y, 2) + pow(size_z, 2)))) +
      4 + 1;

  calculateProjection();

  _e_inc = xt::zeros<Real>({_auxiliary_size});
  _ex_inc = xt::zeros<Real>({_auxiliary_size});
  _ey_inc = xt::zeros<Real>({_auxiliary_size});
  _ez_inc = xt::zeros<Real>({_auxiliary_size});
  _h_inc = xt::zeros<Real>({_auxiliary_size - 1});
  _hx_inc = xt::zeros<Real>({_auxiliary_size - 1});
  _hy_inc = xt::zeros<Real>({_auxiliary_size - 1});
  _hz_inc = xt::zeros<Real>({_auxiliary_size - 1});
  _a = 0;
  _b = 0;

  if (gridSpacePtr()->dimension() == GridSpace::Dimension::ONE) {
    _scaled_dl = gridSpacePtr()->basedDz() / _ratio_delta;
  }
  if (gridSpacePtr()->dimension() == GridSpace::Dimension::TWO) {
    _scaled_dl = gridSpacePtr()->basedDx() / _ratio_delta;
  }
  if (gridSpacePtr()->dimension() == GridSpace::Dimension::THREE) {
    _scaled_dl = gridSpacePtr()->basedDz() / _ratio_delta;
  }

  _ceie = 1;
  _chih = 1;
  _ceihi = -(calculationParamPtr()->timeParam()->dt() /
             (constant::EPSILON_0 * _scaled_dl));
  _chiei = -(calculationParamPtr()->timeParam()->dt() /
             (constant::MU_0 * _scaled_dl));
  _abc_coff_0 =
      (constant::C_0 * calculationParamPtr()->timeParam()->dt() - _scaled_dl) /
      (constant::C_0 * calculationParamPtr()->timeParam()->dt() + _scaled_dl);
  _abc_coff_1 =
      2 * _scaled_dl /
      (constant::C_0 * calculationParamPtr()->timeParam()->dt() + _scaled_dl);
}

Real TFSF::exInc(std::size_t i, std::size_t j, std::size_t k) {
  i = i - globalBox().origin().i() + 1;
  j = j - globalBox().origin().j();
  k = k - globalBox().origin().k();
  auto projection{_projection_x_half(i) + _projection_y_int(j) +
                  _projection_z_int(k)};
  auto index{static_cast<std::size_t>(projection)};
  auto weight{projection - index};
  return (1 - weight) * _ex_inc(index) + weight * _ex_inc(index + 1);
}

Real TFSF::eyInc(std::size_t i, std::size_t j, std::size_t k) {
  i = i - globalBox().origin().i();
  j = j - globalBox().origin().j() + 1;
  k = k - globalBox().origin().k();
  auto projection{_projection_x_int(i) + _projection_y_half(j) +
                  _projection_z_int(k)};
  auto index{static_cast<std::size_t>(projection)};
  auto weight{projection - index};
  return (1 - weight) * _ey_inc(index) + weight * _ey_inc(index + 1);
}

Real TFSF::ezInc(std::size_t i, std::size_t j, std::size_t k) {
  i = i - globalBox().origin().i();
  j = j - globalBox().origin().j();
  k = k - globalBox().origin().k() + 1;
  auto projection{_projection_x_int(i) + _projection_y_int(j) +
                  _projection_z_half(k)};
  auto index{static_cast<std::size_t>(projection)};
  auto weight{projection - index};
  return (1 - weight) * _ez_inc(index) + weight * _ez_inc(index + 1);
}

Real TFSF::hxInc(std::size_t i, std::size_t j, std::size_t k) {
  i = i - globalBox().origin().i();
  j = j - globalBox().origin().j() + 1;
  k = k - globalBox().origin().k() + 1;
  auto projection{_projection_x_int(i) + _projection_y_half(j) +
                  _projection_z_half(k) - 0.5};
  auto index{static_cast<std::size_t>(projection)};
  auto weight{projection - index};
  return (1 - weight) * _hx_inc(index) + weight * _hx_inc(index + 1);
}

Real TFSF::hyInc(std::size_t i, std::size_t j, std::size_t k) {
  i = i - globalBox().origin().i() + 1;
  j = j - globalBox().origin().j();
  k = k - globalBox().origin().k() + 1;
  auto projection{_projection_x_half(i) + _projection_y_int(j) +
                  _projection_z_half(k) - 0.5};
  auto index{static_cast<std::size_t>(projection)};
  auto weight{projection - index};
  return (1 - weight) * _hy_inc(index) + weight * _hy_inc(index + 1);
}

Real TFSF::hzInc(std::size_t i, std::size_t j, std::size_t k) {
  i = i - globalBox().origin().i() + 1;
  j = j - globalBox().origin().j() + 1;
  k = k - globalBox().origin().k();
  auto projection{_projection_x_half(i) + _projection_y_half(j) +
                  _projection_z_int(k) - 0.5};
  auto index{static_cast<std::size_t>(projection)};
  auto weight{projection - index};
  return (1 - weight) * _hz_inc(index) + weight * _hz_inc(index + 1);
}

Real TFSF::cax() {
  return calculationParamPtr()->timeParam()->dt() /
         (constant::EPSILON_0 * gridSpacePtr()->basedDx());
}

Real TFSF::cay() {
  return calculationParamPtr()->timeParam()->dt() /
         (constant::EPSILON_0 * gridSpacePtr()->basedDy());
}

Real TFSF::caz() {
  return calculationParamPtr()->timeParam()->dt() /
         (constant::EPSILON_0 * gridSpacePtr()->basedDz());
}

Real TFSF::cbx() {
  return calculationParamPtr()->timeParam()->dt() /
         (constant::MU_0 * gridSpacePtr()->basedDx());
}

Real TFSF::cby() {
  return calculationParamPtr()->timeParam()->dt() /
         (constant::MU_0 * gridSpacePtr()->basedDy());
}

Real TFSF::cbz() {
  return calculationParamPtr()->timeParam()->dt() /
         (constant::MU_0 * gridSpacePtr()->basedDz());
}

static auto intersectionTask(const GridBox& valid_box,
                             const IndexTask& my_task) {
  auto valid_task =
      makeTask(makeIndexRange(valid_box.origin().i(), valid_box.end().i()),
               makeIndexRange(valid_box.origin().j(), valid_box.end().j()),
               makeIndexRange(valid_box.origin().k(), valid_box.end().k()));
  auto intersection = taskIntersection(my_task, valid_task);
  return intersection;
}

IndexTask TFSF::nodeEyTaskXN(const IndexTask& task) const {
  const auto& my_task = globalEyTaskXN();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box = gridSpace()->validGridBoxEy();
  auto global_valid_box =
      GridBox{valid_box.origin() + offset, valid_box.size()};

  auto intersection_task = intersectionTask(global_valid_box, my_task);

  return intersection_task.has_value()
             ? intersection_task.value()
             : makeIndexTask(my_task.xRange(), makeIndexRange(1, 0),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeEzTaskXN(const IndexTask& task) const {
  const auto& my_task = globalEzTaskXN();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box = gridSpace()->validGridBoxEz();
  auto global_valid_box =
      GridBox{valid_box.origin() + offset, valid_box.size()};

  auto intersection_task = intersectionTask(global_valid_box, my_task);

  return intersection_task.has_value()
             ? intersection_task.value()
             : makeIndexTask(my_task.xRange(), makeIndexRange(1, 0),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeEyTaskXP(const IndexTask& task) const {
  const auto& my_task = globalEyTaskXP();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box = gridSpace()->validGridBoxEy();
  auto global_valid_box =
      GridBox{valid_box.origin() + offset, valid_box.size()};

  auto intersection_task = intersectionTask(global_valid_box, my_task);

  return intersection_task.has_value()
             ? intersection_task.value()
             : makeIndexTask(my_task.xRange(), makeIndexRange(1, 0),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeEzTaskXP(const IndexTask& task) const {
  const auto& my_task = globalEzTaskXP();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box = gridSpace()->validGridBoxEz();
  auto global_valid_box =
      GridBox{valid_box.origin() + offset, valid_box.size()};

  auto intersection_task = intersectionTask(global_valid_box, my_task);

  return intersection_task.has_value()
             ? intersection_task.value()
             : makeIndexTask(my_task.xRange(), makeIndexRange(1, 0),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeExTaskYN(const IndexTask& task) const {
  const auto& my_task = globalExTaskYN();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box = gridSpace()->validGridBoxEx();
  auto global_valid_box =
      GridBox{valid_box.origin() + offset, valid_box.size()};

  auto intersection_task = intersectionTask(global_valid_box, my_task);

  return intersection_task.has_value()
             ? intersection_task.value()
             : makeIndexTask(makeIndexRange(1, 0), my_task.yRange(),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeEzTaskYN(const IndexTask& task) const {
  const auto& my_task = globalEzTaskYN();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box = gridSpace()->validGridBoxEz();
  auto global_valid_box =
      GridBox{valid_box.origin() + offset, valid_box.size()};

  auto intersection_task = intersectionTask(global_valid_box, my_task);

  return intersection_task.has_value()
             ? intersection_task.value()
             : makeIndexTask(makeIndexRange(1, 0), my_task.yRange(),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeExTaskYP(const IndexTask& task) const {
  const auto& my_total_global_task_yp = globalExTaskYP();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box_ex = gridSpace()->validGridBoxEx();
  auto global_valid_box_ex =
      GridBox{valid_box_ex.origin() + offset, valid_box_ex.size()};

  auto intersection_ex =
      intersectionTask(global_valid_box_ex, my_total_global_task_yp);

  return intersection_ex.has_value()
             ? intersection_ex.value()
             : makeIndexTask(makeIndexRange(1, 0),
                             my_total_global_task_yp.yRange(),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeEzTaskYP(const IndexTask& task) const {
  const auto& my_total_global_task_yp = globalEzTaskYP();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box_ez = gridSpace()->validGridBoxEz();
  auto global_valid_box_ez =
      GridBox{valid_box_ez.origin() + offset, valid_box_ez.size()};

  auto intersection_ez =
      intersectionTask(global_valid_box_ez, my_total_global_task_yp);

  return intersection_ez.has_value()
             ? intersection_ez.value()
             : makeIndexTask(makeIndexRange(1, 0),
                             my_total_global_task_yp.yRange(),
                             makeIndexRange(1, 0));
}

IndexTask TFSF::nodeExTaskZN(const IndexTask& task) const {
  const auto& my_total_global_task_zn = globalExTaskZN();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box_ex = gridSpace()->validGridBoxEx();
  auto global_valid_box_ex =
      GridBox{valid_box_ex.origin() + offset, valid_box_ex.size()};

  auto intersection_ex =
      intersectionTask(global_valid_box_ex, my_total_global_task_zn);

  return intersection_ex.has_value()
             ? intersection_ex.value()
             : makeIndexTask(makeIndexRange(1, 0), makeIndexRange(1, 0),
                             my_total_global_task_zn.zRange());
}

IndexTask TFSF::nodeEyTaskZN(const IndexTask& task) const {
  const auto& my_total_global_task_zn = globalEyTaskZN();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box_ey = gridSpace()->validGridBoxEy();
  auto global_valid_box_ey =
      GridBox{valid_box_ey.origin() + offset, valid_box_ey.size()};

  auto intersection_ey =
      intersectionTask(global_valid_box_ey, my_total_global_task_zn);

  return intersection_ey.has_value()
             ? intersection_ey.value()
             : makeIndexTask(makeIndexRange(1, 0), makeIndexRange(1, 0),
                             my_total_global_task_zn.zRange());
}

IndexTask TFSF::nodeExTaskZP(const IndexTask& task) const {
  const auto& my_total_global_task_zp = globalExTaskZP();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box_ex = gridSpace()->validGridBoxEx();
  auto global_valid_box_ex =
      GridBox{valid_box_ex.origin() + offset, valid_box_ex.size()};

  auto intersection_ex =
      intersectionTask(global_valid_box_ex, my_total_global_task_zp);

  return intersection_ex.has_value()
             ? intersection_ex.value()
             : makeIndexTask(makeIndexRange(1, 0), makeIndexRange(1, 0),
                             my_total_global_task_zp.zRange());
}

IndexTask TFSF::nodeEyTaskZP(const IndexTask& task) const {
  const auto& my_total_global_task_zp = globalEyTaskZP();
  const auto& offset = gridSpace()->globalBox().origin();
  const auto& valid_box_ey = gridSpace()->validGridBoxEy();
  auto global_valid_box_ey =
      GridBox{valid_box_ey.origin() + offset, valid_box_ey.size()};

  auto intersection_ey =
      intersectionTask(global_valid_box_ey, my_total_global_task_zp);

  return intersection_ey.has_value()
             ? intersection_ey.value()
             : makeIndexTask(makeIndexRange(1, 0), makeIndexRange(1, 0),
                             my_total_global_task_zp.zRange());
}

void TFSF::initTransform() {
  auto sin_theta{sinTheta()};
  auto cos_theta{cosTheta()};
  auto sin_phi{sinPhi()};
  auto cos_phi{cosPhi()};
  auto sin_psi{sinPsi()};
  auto cos_psi{cosPsi()};

  _k = Vector{sin_theta * cos_phi, sin_theta * sin_phi, cos_theta};

  auto a = Vector{-_sin_phi, _cos_theta * _cos_phi, _sin_theta * _cos_phi};
  auto b = Vector{_cos_phi, _cos_theta * _sin_phi, _sin_theta * _sin_phi};
  auto c = Vector{0, -_sin_theta, _cos_theta};

  _k_e = Vector{sin_psi, cos_psi, 0};

  _transform_e = Vector{
      a.dot(_k_e),
      b.dot(_k_e),
      c.dot(_k_e),
  };

  _transform_h = _k.cross(_transform_e);

  // _transform_e = xt::linalg::dot(_rotation_matrix, _k_e.data());
  // _transform_h = xt::linalg::cross(_k.data(), _transform_e);
}

void TFSF::calculateProjection() {
  auto get_int{[](std::size_t n, Real inc_point, Real tfsf_origin, Real k,
                  Real ratio_delta) {
    Array1D<Real> arr = xt::zeros<Real>({n});
    for (std::size_t i = 0; i < n; ++i) {
      arr(i) = (i + tfsf_origin - inc_point) * k * ratio_delta;
    }
    return arr;
  }};
  auto get_half{[](std::size_t n, Real tfsf_origin, Real inc_point, Real k,
                   Real ratio_delta) {
    Array1D<Real> arr = xt::zeros<Real>({n});
    for (std::size_t i = 0; i < n; ++i) {
      arr(i) = (i + tfsf_origin - inc_point - 0.5) * k * ratio_delta;
    }
    return arr;
  }};

  const auto& box = globalBox();

  auto nx{box.size().i()};
  auto ny{box.size().j()};
  auto nz{box.size().k()};

  auto tfsf_origin_x{box.origin().i()};
  auto tfsf_origin_y{box.origin().j()};
  auto tfsf_origin_z{box.origin().k()};

  auto temp_point{calculateInjectPostion()};
  auto extra_dis{2 * _k.data() / _ratio_delta};
  auto inc_point_x{temp_point.i() - extra_dis(0)};
  auto inc_point_y{temp_point.j() - extra_dis(1)};
  auto inc_point_z{temp_point.k() - extra_dis(2)};

  _projection_x_int =
      get_int(nx + 1, inc_point_x, tfsf_origin_x, k().x(), _ratio_delta);
  _projection_y_int =
      get_int(ny + 1, inc_point_y, tfsf_origin_y, k().y(), _ratio_delta);
  _projection_z_int =
      get_int(nz + 1, inc_point_z, tfsf_origin_z, k().z(), _ratio_delta);
  _projection_x_half =
      get_half(nx + 2, tfsf_origin_x, inc_point_x, k().x(), _ratio_delta);
  _projection_y_half =
      get_half(ny + 2, tfsf_origin_y, inc_point_y, k().y(), _ratio_delta);
  _projection_z_half =
      get_half(nz + 2, tfsf_origin_z, inc_point_z, k().z(), _ratio_delta);
}

Grid TFSF::calculateInjectPostion() {
  const auto& box = globalBox();
  if (0 <= _k.x() && 0 <= _k.y() && 0 <= _k.z()) {
    return box.origin();
  }

  if (0 <= _k.x() && 0 <= _k.y() && _k.z() < 0) {
    return {box.origin().i(), box.origin().j(), box.end().k()};
  }

  if (0 <= _k.x() && _k.y() < 0 && 0 <= _k.z()) {
    return {box.origin().i(), box.end().j(), box.origin().k()};
  }

  if (0 <= _k.x() && _k.y() < 0 && _k.z() < 0) {
    return {box.origin().i(), box.end().j(), box.end().k()};
  }

  if (_k.x() < 0 && 0 <= _k.y() && 0 <= _k.z()) {
    return {box.end().i(), box.origin().j(), box.origin().k()};
  }

  if (_k.x() < 0 && 0 <= _k.y() && _k.z() < 0) {
    return {box.end().i(), box.origin().j(), box.end().k()};
  }

  if (_k.x() < 0 && _k.y() < 0 && 0 <= _k.z()) {
    return {box.end().i(), box.end().j(), box.origin().k()};
  }

  if (_k.x() < 0 && _k.y() < 0 && _k.z() < 0) {
    return box.end();
  }

  throw std::runtime_error("TFSF::calculateInjectPostion() failed");
}

}  // namespace xfdtd
