#include <xfdtd/common/constant.h>
#include <xfdtd/material/dispersive_material.h>
#include <xfdtd/parallel/mpi_support.h>

#include "dispersive_sphere_scatter.h"

int main(int argc, char *argv[]) {
  int id = 1;
  if (argc != 2) {
    id = 1;
  }

  if (argc == 2) {
    id = std::stoi(argv[1]);
    if (id < 0 || 2 <= id) {
      id = 1;
    }
  }

  {
    {
      testCase(xfdtd::DebyeMedium::makeDebyeMedium(
                   "debye_medium", 2, {7}, {2e-9 / (2 * xfdtd::constant::PI)}),
               id);
    }

    xfdtd::MpiSupport::instance().barrier();

    {
      testCase(xfdtd::MLorentzMaterial::makeMLorentz(
                   "debye_m_lor", 2, {5}, {0}, {1},
                   {2e-9 / (2 * xfdtd::constant::PI)}, {0}),
               id);
    }
  }

  // testCase(xfdtd::DebyeMedium::makeDebyeMedium(
  //              "water", 5.285, {80.074}, {9.352e-12}),
  //          id, 100e9);
}
