
#include <memory>
#include <xtensor.hpp>
#include <xtensor/xnpy.hpp>

#include "divider/divider.h"
#include "xfdtd/coordinate_system/coordinate_system.h"
#include "xfdtd/material/material.h"
#include "xfdtd/monitor/voltage_monitor.h"
#include "xfdtd/object/lumped_element/capacitor.h"
#include "xfdtd/object/lumped_element/inductor.h"
#include "xfdtd/object/lumped_element/pec_plane.h"
#include "xfdtd/object/lumped_element/voltage_source.h"
#include "xfdtd/object/object.h"
#include "xfdtd/shape/cube.h"
#include "xfdtd/simulation/simulation.h"
#include "xfdtd/waveform/waveform.h"

constexpr double SIZE{1e-3};

constexpr static double ORIGIN_A{-1 * SIZE};
constexpr static double ORIGIN_B{-1 * SIZE};
constexpr static double ORIGIN_C{-1 * SIZE};

constexpr static double LENGTH_A{6 * SIZE};
constexpr static double LENGTH_B{4 * SIZE};
constexpr static double LENGTH_C{4 * SIZE};

constexpr static double PLANE_O_A{0 * SIZE};
constexpr static double PLANE_O_B{0 * SIZE};
constexpr static double PLANE_O_C{0 * SIZE};

constexpr static double PLANE_L_A{1 * SIZE};
constexpr static double PLANE_L_B{1 * SIZE};
constexpr static double PLANE_L_C{0 * SIZE};

constexpr static double PLANE1_O_A{0 * SIZE};
constexpr static double PLANE1_O_B{0 * SIZE};
constexpr static double PLANE1_O_C{2 * SIZE};

constexpr static double PLANE1_L_A{1 * SIZE};
constexpr static double PLANE1_L_B{1 * SIZE};
constexpr static double PLANE1_L_C{0 * SIZE};

constexpr static double V_SOURCE_O_A{0 * SIZE};
constexpr static double V_SOURCE_O_B{0 * SIZE};
constexpr static double V_SOURCE_O_C{0 * SIZE};
constexpr static double V_SOURCE_L_A{0 * SIZE};
constexpr static double V_SOURCE_L_B{1 * SIZE};
constexpr static double V_SOURCE_L_C{2 * SIZE};

constexpr static double INDUCTOR_O_A{1 * SIZE};
constexpr static double INDUCTOR_O_B{0 * SIZE};
constexpr static double INDUCTOR_O_C{0 * SIZE};
constexpr static double INDUCTOR_L_A{0 * SIZE};
constexpr static double INDUCTOR_L_B{1 * SIZE};
constexpr static double INDUCTOR_L_C{1 * SIZE};

constexpr static double CAPACITOR_O_A{1 * SIZE};
constexpr static double CAPACITOR_O_B{0 * SIZE};
constexpr static double CAPACITOR_O_C{1 * SIZE};
constexpr static double CAPACITOR_L_A{0 * SIZE};
constexpr static double CAPACITOR_L_B{1 * SIZE};
constexpr static double CAPACITOR_L_C{1 * SIZE};

constexpr static double V_MONITOR_O_A{1 * SIZE};
constexpr static double V_MONITOR_O_B{0 * SIZE};
constexpr static double V_MONITOR_O_C{0 * SIZE};
constexpr static double V_MONITOR_L_A{0 * SIZE};
constexpr static double V_MONITOR_L_B{1 * SIZE};
constexpr static double V_MONITOR_L_C{2 * SIZE};

void rlcCircuit(int num_thread) {
  // Y : B C A
  auto domain{std::make_shared<xfdtd::Object>(
      "domain",
      std::make_unique<xfdtd::Cube>(
          xfdtd::Vector{ORIGIN_A, ORIGIN_B, ORIGIN_C},
          xfdtd::Vector{LENGTH_A, LENGTH_B, LENGTH_C}),
      xfdtd::Material::createAir())};

  auto plane{std::make_shared<xfdtd::PecPlane>(
      "plane", std::make_unique<xfdtd::Cube>(
                   xfdtd::Vector{PLANE_O_A, PLANE_O_B, PLANE_O_C},
                   xfdtd::Vector{PLANE_L_A, PLANE_L_B, PLANE_L_C}))};
  auto plane1{std::make_shared<xfdtd::PecPlane>(
      "plane1", std::make_unique<xfdtd::Cube>(
                    xfdtd::Vector{PLANE1_O_A, PLANE1_O_B, PLANE1_O_C},
                    xfdtd::Vector{PLANE1_L_A, PLANE1_L_B, PLANE1_L_C}))};

  constexpr double bandwidth{4e9};
  constexpr double tau{0.996 / bandwidth};
  constexpr double t_0{4.5 * tau};
  auto v_source{std::make_shared<xfdtd::VoltageSource>(
      "v_source",
      std::make_unique<xfdtd::Cube>(
          xfdtd::Vector{V_SOURCE_O_A, V_SOURCE_O_B, V_SOURCE_O_C},
          xfdtd::Vector{V_SOURCE_L_A, V_SOURCE_L_B, V_SOURCE_L_C}),
      xfdtd::Axis::Direction::ZP, 50,
      std::make_unique<xfdtd::Waveform>(
          xfdtd::Waveform::cosineModulatedGaussian(tau, t_0, 2e9)))};

  auto inductor{std::make_shared<xfdtd::Inductor>(
      "inductor",
      std::make_unique<xfdtd::Cube>(
          xfdtd::Vector{INDUCTOR_O_A, INDUCTOR_O_B, INDUCTOR_O_C},
          xfdtd::Vector{INDUCTOR_L_A, INDUCTOR_L_B, INDUCTOR_L_C}),
      xfdtd::Axis::XYZ::Z, 1e-8)};

  auto capacitor{std::make_shared<xfdtd::Capacitor>(
      "capacitor",
      std::make_unique<xfdtd::Cube>(
          xfdtd::Vector{CAPACITOR_O_A, CAPACITOR_O_B, CAPACITOR_O_C},
          xfdtd::Vector{CAPACITOR_L_A, CAPACITOR_L_B, CAPACITOR_L_C}),
      xfdtd::Axis::XYZ::Z, 1e-11)};

  auto v_monitor{std::make_shared<xfdtd::VoltageMonitor>(
      "v_monitor",
      std::make_unique<xfdtd::Cube>(
          xfdtd::Vector{V_MONITOR_O_A, V_MONITOR_O_B, V_MONITOR_O_C},
          xfdtd::Vector{V_MONITOR_L_A, V_MONITOR_L_B, V_MONITOR_L_C}),
      xfdtd::Axis::Direction::ZP, "./data/rlc_circuit")};

  auto s{xfdtd::Simulation{SIZE, SIZE, SIZE, 0.98, num_thread,
                           xfdtd::Divider::Type::X}};
  s.addObject(domain);
  s.addObject(plane);
  s.addObject(plane1);
  s.addObject(v_source);
  s.addObject(inductor);
  s.addObject(capacitor);
  s.addMonitor(v_monitor);
  s.run(2000);

  v_monitor->output();
  xt::dump_npy("./data/rlc_circuit/source.npy", v_source->waveform()->value());
  xt::dump_npy("./data/rlc_circuit/time.npy",
               s.calculationParam()->timeParam()->hTime());
}

int main(int argc, char *argv[]) {
  int num_thread = 1;
  if (argc > 1) {
    num_thread = std::stoi(argv[1]);
  }
  rlcCircuit(num_thread);
}
