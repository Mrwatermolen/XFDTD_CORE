#include <memory>
#include <xtensor/xnpy.hpp>

#include "xfdtd/boundary/pml.h"
#include "xfdtd/coordinate_system/coordinate_system.h"
#include "xfdtd/material/material.h"
#include "xfdtd/monitor/field_monitor.h"
#include "xfdtd/monitor/movie_monitor.h"
#include "xfdtd/nffft/nffft.h"
#include "xfdtd/object/object.h"
#include "xfdtd/shape/cube.h"
#include "xfdtd/shape/sphere.h"
#include "xfdtd/simulation/simulation.h"
#include "xfdtd/waveform/waveform.h"
#include "xfdtd/waveform_source/tfsf_3d.h"
void dielectricSphereScatter() {
  constexpr double dl{7.5e-3};

  auto domain{std::make_shared<xfdtd::Object>(
      "domain",
      std::make_unique<xfdtd::Cube>(xfdtd::Vector{-40 * dl, -40 * dl, -40 * dl},
                                    xfdtd::Vector{80 * dl, 80 * dl, 80 * dl}),
      xfdtd::Material::createAir())};

  auto dielectric_sphere{std::make_shared<xfdtd::Object>(
      "dielectric_sphere_",
      std::make_unique<xfdtd::Sphere>(xfdtd::Vector{0, 0, 0}, 0.1),
      std::make_unique<xfdtd::Material>(
          "a", xfdtd::ElectroMagneticProperty{3, 2, 0, 0}))};

  constexpr auto l_min{dl * 20};
  constexpr auto tau{l_min / 6e8};
  constexpr auto t_0{4.5 * tau};
  auto tfsf{std::make_shared<xfdtd::TFSF3D>(
      20, 20, 20, 0, 0, 0,
      std::make_unique<xfdtd::Waveform>(xfdtd::Waveform::gaussian(tau, t_0)))};

  auto nffft_fd{std::make_shared<xfdtd::NFFFT>(
      15, 15, 15, xt::xarray<double>{1e9}, "./data/dielectric_sphere_scatter")};

  auto movie_xz{std::make_shared<xfdtd::MovieMonitor>(
      std::make_unique<xfdtd::FieldMonitor>(
          std::make_unique<xfdtd::Cube>(
              xfdtd::Vector{-40 * dl, -0 * dl, -40 * dl},
              xfdtd::Vector{80 * dl, 1 * dl, 80 * dl}),
          xfdtd::Axis::XYZ::Y, xfdtd::EMF::Field::EZ, "", ""),
      20, "movie_xz", "./data/dielectric_sphere_scatter/movie_xz")};

  auto movie_yz{std::make_shared<xfdtd::MovieMonitor>(
      std::make_unique<xfdtd::FieldMonitor>(
          std::make_unique<xfdtd::Cube>(
              xfdtd::Vector{-40 * dl, -40 * dl, -40 * dl},
              xfdtd::Vector{1 * dl, 80 * dl, 80 * dl}),
          xfdtd::Axis::XYZ::X, xfdtd::EMF::Field::EZ, "", ""),
      20, "movie_yz", "./data/dielectric_sphere_scatter/movie_yz")};

  auto s{xfdtd::Simulation{dl, dl, dl, 0.9}};
  s.addObject(domain);
  s.addObject(dielectric_sphere);
  s.addWaveformSource(tfsf);
  s.addNF2FF(nffft_fd);
  s.addBoundary(std::make_shared<xfdtd::PML>(10, xfdtd::Axis::Direction::XN));
  s.addBoundary(std::make_shared<xfdtd::PML>(10, xfdtd::Axis::Direction::XP));
  s.addBoundary(std::make_shared<xfdtd::PML>(10, xfdtd::Axis::Direction::YN));
  s.addBoundary(std::make_shared<xfdtd::PML>(10, xfdtd::Axis::Direction::YP));
  s.addBoundary(std::make_shared<xfdtd::PML>(10, xfdtd::Axis::Direction::ZN));
  s.addBoundary(std::make_shared<xfdtd::PML>(10, xfdtd::Axis::Direction::ZP));
//   s.addMonitor(movie_xz);
//   s.addMonitor(movie_yz);
  s.run(2000);

  nffft_fd->processFarField(
      xfdtd::constant::PI * 0.5,
      xt::linspace<double>(-xfdtd::constant::PI, xfdtd::constant::PI, 360),
      "xy");
  nffft_fd->processFarField(
      xt::linspace<double>(-xfdtd::constant::PI, xfdtd::constant::PI, 360), 0,
      "xz");
  nffft_fd->processFarField(
      xt::linspace<double>(-xfdtd::constant::PI, xfdtd::constant::PI, 360),
      xfdtd::constant::PI * 0.5, "yz");

  auto time{tfsf->waveform()->time()};
  auto incident_wave_data{tfsf->waveform()->value()};
  xt::dump_npy("./data/dielectric_sphere_scatter/time.npy", time);
  xt::dump_npy("./data/dielectric_sphere_scatter/incident_wave.npy",
               incident_wave_data);
}

int main() {
  dielectricSphereScatter();
  return 0;
}