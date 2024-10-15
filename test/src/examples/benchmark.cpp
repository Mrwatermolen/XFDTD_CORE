#include <cstdio>
#include <unistd.h>

#include <filesystem>
#include <iostream>
#include <memory>
#include <xtensor/xnpy.hpp>

#include "argparse.hpp"
#include "xfdtd/boundary/pml.h"
#include "xfdtd/coordinate_system/coordinate_system.h"
#include "xfdtd/material/material.h"
#include "xfdtd/object/object.h"
#include "xfdtd/parallel/mpi_support.h"
#include "xfdtd/parallel/parallelized_config.h"
#include "xfdtd/shape/cube.h"
#include "xfdtd/simulation/simulation.h"

void dielectricSphereScatter(int argc, char* argv[]) {
  // auto cpus = sysconf(_SC_NPROCESSORS_ONLN);
  // printf("cpu: %ld\n", cpus);

  // int cpu_id = 4;
  // cpu_set_t mask;
  // CPU_ZERO(&mask);
  // CPU_SET(cpu_id, &mask);
  // if (sched_setaffinity(0, sizeof(mask), &mask) == -1) {
  //   printf("Could not set CPU affinity\n");
  // } else {
  //   printf("CPU affinity set to CPU %d\n", cpu_id);
  // }
  // press any key to continue
  // std::cin.get();
  // xfdtd::MpiSupport::setMpiParallelDim(2, 2, 1);

  auto program = argparse::ArgumentParser("dielectricSphereScatter");
  program.add_argument("-t", "--time_steps")
      .help("Number of time steps")
      .default_value(4800)
      .action([](const std::string& value) { return std::stoi(value); });

  program.add_argument("-d", "--dl")
      .help("Grid resolution")
      .default_value(5e-3)
      .action([](const std::string& value) { return std::stod(value); });

  // for thread config
  program.add_argument("-t_c", "--thread_config")
      .help("Thread configuration")
      .default_value(std::vector<int>{1, 1, 2})
      .nargs(3)
      .scan<'i', int>();

  program.add_argument("--with_pml")
      .help("With PML")
      .default_value(false)
      .implicit_value(true);

  try {
    program.parse_args(argc, argv);
  } catch (const std::runtime_error& err) {
    std::cout << err.what() << std::endl;
    std::cout << program;
    exit(0);
  }

  auto time_steps = program.get<int>("-t");
  auto dl = program.get<double>("-d");
  auto thread_config = program.get<std::vector<int>>("-t_c");
  auto with_pml = program.get<bool>("--with_pml");
  std::cout << "time_steps: " << time_steps << std::endl;
  std::cout << "dl: " << dl << std::endl;
  std::cout << "thread_config: " << thread_config[0] << " " << thread_config[1]
            << " " << thread_config[2] << std::endl;
  std::cout << "with_pml: " << std::boolalpha << with_pml << std::endl;

  using namespace std::string_view_literals;
  constexpr auto data_path_str = "./data/dielectric_sphere_scatter"sv;
  const auto data_path = std::filesystem::path{data_path_str};

  auto domain{std::make_shared<xfdtd::Object>(
      "domain",
      std::make_unique<xfdtd::Cube>(xfdtd::Vector{-0.175, -0.175, -0.175},
                                    xfdtd::Vector{0.35, 0.35, 0.35}),
      xfdtd::Material::createAir())};

  auto s{
      xfdtd::Simulation{dl, dl, dl, 0.9,
                        xfdtd::ThreadConfig{thread_config[0], thread_config[1],
                                            thread_config[2]}}};
  s.addObject(domain);
  if (with_pml) {
    s.addBoundary(std::make_shared<xfdtd::PML>(8, xfdtd::Axis::Direction::XN));
    s.addBoundary(std::make_shared<xfdtd::PML>(8, xfdtd::Axis::Direction::XP));
    s.addBoundary(std::make_shared<xfdtd::PML>(8, xfdtd::Axis::Direction::YN));
    s.addBoundary(std::make_shared<xfdtd::PML>(8, xfdtd::Axis::Direction::YP));
    s.addBoundary(std::make_shared<xfdtd::PML>(8, xfdtd::Axis::Direction::ZN));
    s.addBoundary(std::make_shared<xfdtd::PML>(8, xfdtd::Axis::Direction::ZP));
  }
  s.run(time_steps);

  if (!s.isRoot()) {
    return;
  }
}

int main(int argc, char* argv[]) {
  xfdtd::MpiSupport::setMpiParallelDim(2, 1, 1);
  xfdtd::MpiSupport::instance(argc, argv);
  dielectricSphereScatter(argc, argv);
  return 0;
}
