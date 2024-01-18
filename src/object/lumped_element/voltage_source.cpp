#include <xfdtd/object/lumped_element/voltage_source.h>

#include <cstddef>
#include <utility>
#include <xtensor.hpp>

namespace xfdtd {

VoltageSource::VoltageSource(std::string name, std::unique_ptr<Cube> cube,
                             Axis::Direction direction, double resistance,
                             std::unique_ptr<Waveform> waveform,
                             std::unique_ptr<Material> material)
    : LumpedElement{std::move(name), std::move(cube),
                    Axis::formDirectionToXYZ(direction), std::move(material)},
      _direction{direction},
      _resistance{resistance},
      _waveform{std::move(waveform)} {
  if (_resistance == 0) {
    _resistance = 1e-20;
  }
}

std::string VoltageSource::toString() const {
  // return "VoltageSource{name: " + name() + ", shape: " + shape()->toString()
  // +
  //        ", material: " + material()->toString() +
  //        ", direction: " + Axis::toString(_direction) +
  //        ", resistance: " + std::to_string(_resistance) +
  //        ", waveform: " + _waveform->toString() + "}";
  return "";
}

Axis::Direction VoltageSource::direction() const { return _direction; }

double VoltageSource::resistance() const { return _resistance; }

const std::unique_ptr<Waveform>& VoltageSource::waveform() const {
  return _waveform;
}

void VoltageSource::init(std::shared_ptr<const GridSpace> grid_space,
                         std::shared_ptr<CalculationParam> calculation_param,
                         std::shared_ptr<EMF> emf) {
  LumpedElement::init(std::move(grid_space), std::move(calculation_param),
                      std::move(emf));

  auto rf{[](double r, std::size_t na, std::size_t nb, std::size_t nc) {
    return r * na * nb / nc;
  }};
  auto vf{[](double v, std::size_t nc) { return v / nc; }};
  auto dx_dy_dz{[](const xt::xarray<double>& x, const xt::xarray<double>& y,
                   const xt::xarray<double>& z, auto&& x_range, auto&& y_range,
                   auto&& z_range) {
    return xt::meshgrid(xt::view(x, x_range), xt::view(y, y_range),
                        xt::view(z, z_range));
  }};

  _resistance_factor = rf(_resistance, nodeCountSubAxisA(), nodeCountSubAxisB(),
                          nodeCountMainAxis());
  _voltage_amplitude_factor = vf(_waveform->amplitude(), nodeCountMainAxis());

  auto g{gridSpacePtr()};

  if (xyz() == Axis::XYZ::X) {
    if (direction() == Axis::Direction::XN) {
      _voltage_amplitude_factor *= -1;
    }

    auto [dx, dy, dz] = dx_dy_dz(g->eSizeX(), g->hSizeY(), g->hSizeZ(),
                                 rangeX(), rangeY(), rangeZ());

    _da = dy;
    _db = dz;
    _dc = dx;
  }

  if (xyz() == Axis::XYZ::Y) {
    if (direction() == Axis::Direction::YN) {
      _voltage_amplitude_factor *= -1;
    }

    auto [dx, dy, dz] = dx_dy_dz(g->hSizeX(), g->eSizeY(), g->hSizeZ(),
                                 rangeX(), rangeY(), rangeZ());

    _da = dz;
    _db = dx;
    _dc = dy;
  }

  if (xyz() == Axis::XYZ::Z) {
    if (direction() == Axis::Direction::ZN) {
      _voltage_amplitude_factor *= -1;
    }

    auto [dx, dy, dz] = dx_dy_dz(g->hSizeX(), g->hSizeY(), g->eSizeZ(),
                                 rangeX(), rangeY(), rangeZ());

    _da = dx;
    _db = dy;
    _dc = dz;
  }

  _waveform->init(calculationParamPtr()->timeParam()->hTime());
}

void VoltageSource::correctUpdateCoefficient() {
  auto calc_param{calculationParamPtr()};
  auto dt{calc_param->timeParam()->dt()};
  _alpha = _da * _db * _resistance_factor;
  _beta = dt * _dc / _alpha;

  auto func{[this, dt](auto& cece, auto& cecha, auto& cechb, const auto& eps,
                       const auto& sigma) {
    auto range_x{rangeX()};
    auto range_y{rangeY()};
    auto range_z{rangeZ()};
    auto cece_view{xt::view(cece, range_x, range_y, range_z)};
    auto cecha_view{xt::view(cecha, range_x, range_y, range_z)};
    auto cechb_view{xt::view(cechb, range_x, range_y, range_z)};
    const auto eps_view{xt::view(eps, range_x, range_y, range_z)};
    const auto sigma_view{xt::view(sigma, range_x, range_y, range_z)};

    cece_view = (2 * eps_view - dt * sigma_view - _beta) /
                (2 * eps_view + dt * sigma_view + _beta);
    cecha_view = -2 * dt / ((2 * eps_view + dt * sigma_view + _beta) * _db);
    cechb_view = 2 * dt / ((2 * eps_view + dt * sigma_view + _beta) * _da);
    _coff_v = -2 * dt / ((2 * eps_view + dt * sigma_view + _beta) * _alpha) *
              _voltage_amplitude_factor;
  }};

  if (xyz() == Axis::XYZ::X) {
    auto& cexe{calc_param->fdtdCoefficient()->cexe()};
    auto& cexhy{calc_param->fdtdCoefficient()->cexhy()};
    auto& cexhz{calc_param->fdtdCoefficient()->cexhz()};
    auto& eps_x{calc_param->materialParam()->epsX()};
    auto& sigma_e_x{calc_param->materialParam()->sigmaEX()};
    func(cexe, cexhy, cexhz, eps_x, sigma_e_x);
    return;
  }

  if (xyz() == Axis::XYZ::Y) {
    auto& ceye{calc_param->fdtdCoefficient()->ceye()};
    auto& ceyhz{calc_param->fdtdCoefficient()->ceyhz()};
    auto& ceyhx{calc_param->fdtdCoefficient()->ceyhx()};
    auto& eps_y{calc_param->materialParam()->epsY()};
    auto& sigma_e_y{calc_param->materialParam()->sigmaEY()};
    func(ceye, ceyhz, ceyhx, eps_y, sigma_e_y);
    return;
  }

  if (xyz() == Axis::XYZ::Z) {
    auto& ceze{calc_param->fdtdCoefficient()->ceze()};
    auto& cezhx{calc_param->fdtdCoefficient()->cezhx()};
    auto& cezhy{calc_param->fdtdCoefficient()->cezhy()};
    auto& eps_z{calc_param->materialParam()->epsZ()};
    auto& sigma_e_z{calc_param->materialParam()->sigmaEZ()};
    func(ceze, cezhx, cezhy, eps_z, sigma_e_z);
    return;
  }
}

void VoltageSource::correctE() {
  auto e_view{
      xt::view(fieldMainAxis(EMF::Attribute::E), rangeX(), rangeY(), rangeZ())};
  e_view +=
      _coff_v *
      _waveform->value()[calculationParamPtr()->timeParam()->currentTimeStep()];
}

void VoltageSource::correctH() {}

}  // namespace xfdtd