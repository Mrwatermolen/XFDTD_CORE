#include "updator/dispersive_material_updator.h"

#include <xfdtd/material/dispersive_material.h>
#include <xfdtd/util/fdtd_basic.h>

#include <memory>
#include <utility>

#include "updator/dispersive_material_update_method/dispersive_material_update_method.h"
#include "updator/update_scheme.h"

namespace xfdtd {

LinearDispersiveMaterialUpdator::LinearDispersiveMaterialUpdator(
    std::vector<std::shared_ptr<Material>> material_arr,
    std::shared_ptr<const GridSpace> grid_space,
    std::shared_ptr<const CalculationParam> calculation_param,
    std::shared_ptr<EMF> emf, IndexTask task)
    : BasicUpdator3D(std::move(grid_space), std::move(calculation_param),
                     std::move(emf), task) {
  init(std::move(material_arr));
}

auto LinearDispersiveMaterialUpdator::init(
    std::vector<std::shared_ptr<Material>> material_arr) -> void {
  if (material_arr.empty()) {
    throw XFDTDLinearDispersiveMaterialException{"Material array is empty"};
  }

  _map.resize(material_arr.size(), -1);

  for (Index i{0}; i < material_arr.size(); ++i) {
    const auto& m = material_arr[i];
    if (!m->dispersion()) {
      continue;
    }

    auto dispersive_material =
        std::dynamic_pointer_cast<LinearDispersiveMaterial>(m);
    if (dispersive_material == nullptr) {
      continue;
    }

    _update_methods.emplace_back(dispersive_material->updateMethod()->clone());
    _update_methods.back()->initUpdate(_grid_space.get(), _calculation_param,
                                       _emf, i, task());
    _map[i] = _update_methods.size() - 1;
  }
}

auto LinearDispersiveMaterialUpdator::updateE() -> void {
  const auto task = this->task();

  const auto& cexe{_calculation_param->fdtdCoefficient()->cexe()};
  const auto& cexhy{_calculation_param->fdtdCoefficient()->cexhy()};
  const auto& cexhz{_calculation_param->fdtdCoefficient()->cexhz()};
  const auto& ceye{_calculation_param->fdtdCoefficient()->ceye()};
  const auto& ceyhz{_calculation_param->fdtdCoefficient()->ceyhz()};
  const auto& ceyhx{_calculation_param->fdtdCoefficient()->ceyhx()};
  const auto& ceze{_calculation_param->fdtdCoefficient()->ceze()};
  const auto& cezhx{_calculation_param->fdtdCoefficient()->cezhx()};
  const auto& cezhy{_calculation_param->fdtdCoefficient()->cezhy()};

  const auto& hx{_emf->hx()};
  const auto& hy{_emf->hy()};
  const auto& hz{_emf->hz()};
  auto& ex{_emf->ex()};
  auto& ey{_emf->ey()};
  auto& ez{_emf->ez()};

  auto is = basic::GridStructure::exFDTDUpdateXStart(task.xRange().start());
  auto ie = basic::GridStructure::exFDTDUpdateXEnd(task.xRange().end());
  // auto js = basic::GridStructure::exFDTDUpdateYStart(task.yRange().start());
  auto js = task.yRange().start() == 0 ? 1 : task.yRange().start();
  auto je = basic::GridStructure::exFDTDUpdateYEnd(task.yRange().end());
  // auto ks = basic::GridStructure::exFDTDUpdateZStart(task.zRange().start());
  auto ks = task.zRange().start() == 0 ? 1 : task.zRange().start();
  auto ke = basic::GridStructure::exFDTDUpdateZEnd(task.zRange().end());
  for (std::size_t i{is}; i < ie; ++i) {
    for (std::size_t j{js}; j < je; ++j) {
      for (std::size_t k{ks}; k < ke; ++k) {
        auto m_index =
            _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

        if (m_index == -1 || _map[m_index] == -1) {
          ex(i, j, k) = eNext(cexe(i, j, k), ex(i, j, k), cexhy(i, j, k),
                              hy(i, j, k), hy(i, j, k - 1), cexhz(i, j, k),
                              hz(i, j, k), hz(i, j - 1, k));
          continue;
        }

        _update_methods[_map[m_index]]->updateEx(i, j, k);
      }
    }
  }

  // is = basic::GridStructure::eyFDTDUpdateXStart(task.xRange().start());
  is = task.xRange().start() == 0 ? 1 : task.xRange().start();
  ie = basic::GridStructure::eyFDTDUpdateXEnd(task.xRange().end());
  js = basic::GridStructure::eyFDTDUpdateYStart(task.yRange().start());
  je = basic::GridStructure::eyFDTDUpdateYEnd(task.yRange().end());
  // ks = basic::GridStructure::eyFDTDUpdateZStart(task.zRange().start());
  ks = task.zRange().start() == 0 ? 1 : task.zRange().start();
  ke = basic::GridStructure::eyFDTDUpdateZEnd(task.zRange().end());
  for (std::size_t i{is}; i < ie; ++i) {
    for (std::size_t j{js}; j < je; ++j) {
      for (std::size_t k{ks}; k < ke; ++k) {
        auto m_index =
            _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

        if (m_index == -1 || _map[m_index] == -1) {
          ey(i, j, k) = eNext(ceye(i, j, k), ey(i, j, k), ceyhz(i, j, k),
                              hz(i, j, k), hz(i - 1, j, k), ceyhx(i, j, k),
                              hx(i, j, k), hx(i, j, k - 1));
          continue;
        }

        _update_methods[_map[m_index]]->updateEy(i, j, k);
      }
    }
  }

  // is = basic::GridStructure::ezFDTDUpdateXStart(task.xRange().start());
  is = task.xRange().start() == 0 ? 1 : task.xRange().start();
  ie = basic::GridStructure::ezFDTDUpdateXEnd(task.xRange().end());
  // js = basic::GridStructure::ezFDTDUpdateYStart(task.yRange().start());
  js = task.yRange().start() == 0 ? 1 : task.yRange().start();
  je = basic::GridStructure::ezFDTDUpdateYEnd(task.yRange().end());
  ks = basic::GridStructure::ezFDTDUpdateZStart(task.zRange().start());
  ke = basic::GridStructure::ezFDTDUpdateZEnd(task.zRange().end());
  for (std::size_t i{is}; i < ie; ++i) {
    for (std::size_t j{js}; j < je; ++j) {
      for (std::size_t k{ks}; k < ke; ++k) {
        auto m_index =
            _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

        if (m_index == -1 || _map[m_index] == -1) {
          ez(i, j, k) = eNext(ceze(i, j, k), ez(i, j, k), cezhx(i, j, k),
                              hx(i, j, k), hx(i, j - 1, k), cezhy(i, j, k),
                              hy(i, j, k), hy(i - 1, j, k));
          continue;
        }

        _update_methods[_map[m_index]]->updateEz(i, j, k);
      }
    }
  }

  // updateEEdge();
}

// auto LinearDispersiveMaterialUpdator::updateEEdge() -> void {
//   const auto is = task().xRange().start();
//   const auto ie = task().xRange().end();
//   const auto js = task().yRange().start();
//   const auto je = task().yRange().end();
//   const auto ks = task().zRange().start();
//   const auto ke = task().zRange().end();

//   const auto& cexe{_calculation_param->fdtdCoefficient()->cexe()};
//   const auto& cexhy{_calculation_param->fdtdCoefficient()->cexhy()};
//   const auto& cexhz{_calculation_param->fdtdCoefficient()->cexhz()};
//   const auto& ceye{_calculation_param->fdtdCoefficient()->ceye()};
//   const auto& ceyhz{_calculation_param->fdtdCoefficient()->ceyhz()};
//   const auto& ceyhx{_calculation_param->fdtdCoefficient()->ceyhx()};
//   const auto& ceze{_calculation_param->fdtdCoefficient()->ceze()};
//   const auto& cezhx{_calculation_param->fdtdCoefficient()->cezhx()};
//   const auto& cezhy{_calculation_param->fdtdCoefficient()->cezhy()};

//   const auto& hx{_emf->hx()};
//   const auto& hy{_emf->hy()};
//   const auto& hz{_emf->hz()};
//   auto& ex{_emf->ex()};
//   auto& ey{_emf->ey()};
//   auto& ez{_emf->ez()};

//   bool contain_xn_edge = containXNEdge();
//   bool contain_yn_edge = containYNEdge();
//   bool contain_zn_edge = containZNEdge();

//   if (!contain_yn_edge && !contain_zn_edge) {
//     auto j = js;
//     auto k = ks;
//     for (std::size_t i{is}; i < ie; ++i) {
//       auto m_index = _grid_space->gridWithMaterial()(i, j, k)->materialIndex();
//       if (m_index == -1 || _map[m_index] == -1) {
//         ex(i, j, k) = eNext(cexe(i, j, k), ex(i, j, k), cexhy(i, j, k),
//                             hy(i, j, k), hy(i, j, k - 1), cexhz(i, j, k),
//                             hz(i, j, k), hz(i, j - 1, k));
//         continue;
//       }

//       _update_methods[_map[m_index]]->updateEx(i, j, k);
//     }
//   }

//   if (!contain_xn_edge && !contain_zn_edge) {
//     auto i = is;
//     auto k = ks;
//     for (std::size_t j{js}; j < je; ++j) {
//       auto m_index = _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

//       if (m_index == -1 || _map[m_index] == -1) {
//         ey(i, j, k) = eNext(ceye(i, j, k), ey(i, j, k), ceyhz(i, j, k),
//                             hz(i, j, k), hz(i - 1, j, k), ceyhx(i, j, k),
//                             hx(i, j, k), hx(i, j, k - 1));
//         continue;
//       }

//       _update_methods[_map[m_index]]->updateEy(i, j, k);
//     }
//   }

//   if (!contain_xn_edge && !contain_yn_edge) {
//     auto i = is;
//     auto j = js;
//     for (std::size_t k{ks}; k < ke; ++k) {
//       auto m_index = _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

//       if (m_index == -1 || _map[m_index] == -1) {
//         ez(i, j, k) = eNext(ceze(i, j, k), ez(i, j, k), cezhx(i, j, k),
//                             hx(i, j, k), hx(i, j - 1, k), cezhy(i, j, k),
//                             hy(i, j, k), hy(i - 1, j, k));
//         continue;
//       }

//       _update_methods[_map[m_index]]->updateEz(i, j, k);
//     }
//   }

//   if (!contain_xn_edge) {
//     auto i = is;
//     for (std::size_t j{js}; j < je; ++j) {
//       for (std::size_t k{ks + 1}; k < ke; ++k) {
//         auto m_index =
//             _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

//         if (m_index == -1 || _map[m_index] == -1) {
//           ey(i, j, k) = eNext(ceye(i, j, k), ey(i, j, k), ceyhz(i, j, k),
//                               hz(i, j, k), hz(i - 1, j, k), ceyhx(i, j, k),
//                               hx(i, j, k), hx(i, j, k - 1));
//           continue;
//         }

//         _update_methods[_map[m_index]]->updateEy(i, j, k);
//       }
//     }
//     for (std::size_t j{js + 1}; j < je; ++j) {
//       for (std::size_t k{ks}; k < ke; ++k) {
//         auto m_index =
//             _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

//         if (m_index == -1 || _map[m_index] == -1) {
//           ez(i, j, k) = eNext(ceze(i, j, k), ez(i, j, k), cezhx(i, j, k),
//                               hx(i, j, k), hx(i, j - 1, k), cezhy(i, j, k),
//                               hy(i, j, k), hy(i - 1, j, k));
//           continue;
//         }

//         _update_methods[_map[m_index]]->updateEz(i, j, k);
//       }
//     }
//   }

//   if (!contain_yn_edge) {
//     auto j = js;
//     for (std::size_t i{is + 1}; i < ie; ++i) {
//       for (std::size_t k{ks}; k < ke; ++k) {
//         auto m_index =
//             _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

//         if (m_index == -1 || _map[m_index] == -1) {
//           ez(i, j, k) = eNext(ceze(i, j, k), ez(i, j, k), cezhx(i, j, k),
//                               hx(i, j, k), hx(i, j - 1, k), cezhy(i, j, k),
//                               hy(i, j, k), hy(i - 1, j, k));
//           continue;
//         }

//         _update_methods[_map[m_index]]->updateEz(i, j, k);
//       }
//     }
//     for (std::size_t i{is}; i < ie; ++i) {
//       for (std::size_t k{ks + 1}; k < ke; ++k) {
//         auto m_index =
//             _grid_space->gridWithMaterial()(i, j, k)->materialIndex();
//         if (m_index == -1 || _map[m_index] == -1) {
//           ex(i, j, k) = eNext(cexe(i, j, k), ex(i, j, k), cexhy(i, j, k),
//                               hy(i, j, k), hy(i, j, k - 1), cexhz(i, j, k),
//                               hz(i, j, k), hz(i, j - 1, k));
//           continue;
//         }

//         _update_methods[_map[m_index]]->updateEx(i, j, k);
//       }
//     }
//   }

//   if (!contain_zn_edge) {
//     auto k = ks;
//     for (std::size_t i{is}; i < ie; ++i) {
//       for (std::size_t j{js + 1}; j < je; ++j) {
//         auto m_index =
//             _grid_space->gridWithMaterial()(i, j, k)->materialIndex();
//         if (m_index == -1 || _map[m_index] == -1) {
//           ex(i, j, k) = eNext(cexe(i, j, k), ex(i, j, k), cexhy(i, j, k),
//                               hy(i, j, k), hy(i, j, k - 1), cexhz(i, j, k),
//                               hz(i, j, k), hz(i, j - 1, k));
//           continue;
//         }

//         _update_methods[_map[m_index]]->updateEx(i, j, k);
//       }
//     }
//     for (std::size_t i{is + 1}; i < ie; ++i) {
//       for (std::size_t j{js}; j < je; ++j) {
//         auto m_index =
//             _grid_space->gridWithMaterial()(i, j, k)->materialIndex();

//         if (m_index == -1 || _map[m_index] == -1) {
//           ey(i, j, k) = eNext(ceye(i, j, k), ey(i, j, k), ceyhz(i, j, k),
//                               hz(i, j, k), hz(i - 1, j, k), ceyhx(i, j, k),
//                               hx(i, j, k), hx(i, j, k - 1));
//           continue;
//         }

//         _update_methods[_map[m_index]]->updateEy(i, j, k);
//       }
//     }
//   }
// }

LinearDispersiveMaterialUpdator1D::LinearDispersiveMaterialUpdator1D(
    const std::vector<std::shared_ptr<Material>>& material_arr,
    std::shared_ptr<const GridSpace> grid_space,
    std::shared_ptr<const CalculationParam> calculation_param,
    std::shared_ptr<EMF> emf, IndexTask task)
    : BasicUpdatorTEM(std::move(grid_space), std::move(calculation_param),
                      std::move(emf), task) {
  init(material_arr);
}

auto LinearDispersiveMaterialUpdator1D::init(
    const std::vector<std::shared_ptr<Material>>& material_arr) -> void {
  if (material_arr.empty()) {
    throw XFDTDLinearDispersiveMaterialException{"Material array is empty"};
  }

  _map.resize(material_arr.size(), -1);

  for (Index i{0}; i < material_arr.size(); ++i) {
    const auto& m = material_arr[i];
    if (!m->dispersion()) {
      continue;
    }

    auto dispersive_material =
        std::dynamic_pointer_cast<LinearDispersiveMaterial>(m);
    if (dispersive_material == nullptr) {
      continue;
    }

    _update_methods.emplace_back(dispersive_material->updateMethod()->clone());
    _update_methods.back()->initUpdate(_grid_space.get(), _calculation_param,
                                       _emf, i, task());
    _map[i] = _update_methods.size() - 1;
  }
}

auto LinearDispersiveMaterialUpdator1D::updateE() -> void {
  const auto ks =
      basic::GridStructure::exFDTDUpdateZStart(task().zRange().start());
  const auto ke = basic::GridStructure::exFDTDUpdateZEnd(task().zRange().end());

  const auto& cexe{_calculation_param->fdtdCoefficient()->cexe()};
  const auto& cexhy{_calculation_param->fdtdCoefficient()->cexhy()};

  const auto& hy{_emf->hy()};
  auto& ex{_emf->ex()};

  for (Index k{ks}; k < ke; ++k) {
    auto m_index = _grid_space->gridWithMaterial()(0, 0, k)->materialIndex();

    if (m_index == -1 || _map[m_index] == -1) {
      ex(0, 0, k) = eNext(cexe(0, 0, k), ex(0, 0, k), cexhy(0, 0, k),
                          hy(0, 0, k), hy(0, 0, k - 1), 0.0, 0.0, 0.0);
      continue;
    }

    _update_methods[_map[m_index]]->updateTEM(0, 0, k);
  }

  // updateEEdge();
}

// auto LinearDispersiveMaterialUpdator1D::updateEEdge() -> void {
//   if (containZNEdge()) {
//     return;
//   }

//   const auto ks = task().zRange().start();

//   const auto& cexe{_calculation_param->fdtdCoefficient()->cexe()};
//   const auto& cexhy{_calculation_param->fdtdCoefficient()->cexhy()};

//   const auto& hy{_emf->hy()};
//   auto& ex{_emf->ex()};
//   auto k = ks;

//   auto m_index = _grid_space->gridWithMaterial()(0, 0, k)->materialIndex();

//   if (m_index == -1 || _map[m_index] == -1) {
//     ex(0, 0, k) = eNext(cexe(0, 0, k), ex(0, 0, k), cexhy(0, 0, k), hy(0, 0, k),
//                         hy(0, 0, k - 1), 0.0, 0.0, 0.0);
//     return;
//   }

//   _update_methods[_map[m_index]]->updateTEM(0, 0, k);
// }

}  // namespace xfdtd
