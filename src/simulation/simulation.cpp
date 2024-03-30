#include <xfdtd/boundary/pml.h>
#include <xfdtd/calculation_param/calculation_param.h>
#include <xfdtd/coordinate_system/coordinate_system.h>
#include <xfdtd/divider/divider.h>
#include <xfdtd/grid_space/grid_space.h>
#include <xfdtd/grid_space/grid_space_generator.h>
#include <xfdtd/material/dispersive_material.h>
#include <xfdtd/monitor/monitor.h>
#include <xfdtd/nffft/nffft.h>
#include <xfdtd/object/lumped_element/pec_plane.h>
#include <xfdtd/parallel/mpi_support.h>
#include <xfdtd/parallel/parallelized_config.h>
#include <xfdtd/simulation/simulation.h>
#include <xfdtd/waveform_source/waveform_source.h>

#include <chrono>
#include <cstdlib>
#include <memory>
#include <sstream>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>
#include <xtensor/xnpy.hpp>

#include "corrector/corrector.h"
#include "domain/domain.h"
#include "updator/basic_updator.h"
#include "updator/dispersive_material_updator.h"
#include "updator/updator.h"

namespace xfdtd {

Simulation::Simulation(double dx, double dy, double dz, double cfl,
                       ThreadConfig thread_config)
    : _dx{dx},
      _dy{dy},
      _dz{dz},
      _cfl{cfl},
      _thread_config{std::move(thread_config)},
      _barrier(_thread_config.size()) {}

Simulation::~Simulation() = default;

bool Simulation::isRoot() const { return MpiSupport::instance().isRoot(); }

int Simulation::myRank() const { return MpiSupport::instance().rank(); }

int Simulation::numNode() const { return MpiSupport::instance().size(); }

int Simulation::numThread() const { return _thread_config.size(); }

void Simulation::addObject(std::shared_ptr<xfdtd::Object> object) {
  _objects.emplace_back(std::move(object));
}

void Simulation::addWaveformSource(
    std::shared_ptr<WaveformSource> waveform_source) {
  _waveform_sources.emplace_back(std::move(waveform_source));
}

void Simulation::addBoundary(std::shared_ptr<Boundary> boundary) {
  _boundaries.emplace_back(std::move(boundary));
}

void Simulation::addMonitor(std::shared_ptr<Monitor> monitor) {
  _monitors.emplace_back(std::move(monitor));
}

void Simulation::addNetwork(std::shared_ptr<Network> network) {
  _networks.emplace_back(std::move(network));
}

void Simulation::addNF2FF(std::shared_ptr<NFFFT> nffft) {
  _nfffts.emplace_back(std::move(nffft));
}

const std::shared_ptr<CalculationParam>& Simulation::calculationParam() const {
  return _calculation_param;
}

const std::shared_ptr<GridSpace>& Simulation::gridSpace() const {
  return _grid_space;
}

const std::shared_ptr<EMF>& Simulation::emf() const { return _emf; }

void Simulation::run(std::size_t time_step) {
  _start_time = std::chrono::high_resolution_clock::now();
  if (isRoot()) {
    std::cout << "Simulation start...\n";
  }

  init();
  _calculation_param->timeParam()->setTimeParamRunRange(time_step);
  // do final check
  for (auto&& o : _objects) {
    o->initTimeDependentVariable();
  }
  for (auto&& w : _waveform_sources) {
    w->initTimeDependentVariable();
  }
  for (auto&& n : _nfffts) {
    n->initTimeDependentVariable();
  }
  for (auto&& m : _monitors) {
    m->initTimeDependentVariable();
  }

  if (isRoot()) {
    // global grid space and thread config
    {
      std::stringstream ss;
      ss << "\n";
      ss << "Global grid space: \n";
      ss << _global_grid_space->toString();
      ss << "\n";

      ss << "\n";
      ss << _thread_config.toString() << "\n";
      std::cout << ss.str() << std::flush;
    }
  }

  // MpiSupport::instance().barrier();

  // {
  //   std::stringstream ss;
  //   ss << "\n";
  //   ss << "Rank: " << myRank() << "\n";
  //   ss << _grid_space->toString();
  //   std::cout << ss.str();
  //   MpiSupport::instance().barrier();
  // }

  // {
  //   std::stringstream ss;
  //   ss << "\n";
  //   ss << "Rank: " << myRank() << "\n";
  //   ss << MpiSupport::instance().config().toString() << "\n";
  //   std::cout << ss.str();
  //   MpiSupport::instance().barrier();
  // }

  // {
  // std::stringstream ss;
  // ss << "\n";
  // for (const auto& d : _domains) {
  //   ss << "Rank: " << myRank() << "\n";
  //   ss << d->toString() << "\n";
  //   std::cout << ss.str() << std::flush;
  //   MpiSupport::instance().barrier();
  //   ss.str("");
  // }
  // }

  // {
  //   std::stringstream ss;
  //   ss << "\n";
  //   for (const auto& m : _monitors) {
  //     auto v = std::dynamic_pointer_cast<CurrentMonitor>(m);
  //     if (v == nullptr) {
  //       continue;
  //     }

  //     if (!v->valid()) {
  //       continue;
  //     }

  //     if (!MpiSupport::instance().isRoot()) {
  //       std::this_thread::sleep_for(std::chrono::milliseconds(100));
  //     }

  //     ss << "Rank: " << myRank() << "\n";
  //     ss << v->toString() << "\n";
  //     std::cout << ss.str();
  //     ss.str("");
  //   }
  // }

  // auto& mpi_support = MpiSupport::instance();
  // auto rank_str = std::to_string(mpi_support.rank());
  // std::filesystem::path check_data = "./data/check_mpi_" + rank_str;

  // if (!std::filesystem::exists(check_data)) {
  //   std::filesystem::create_directory(check_data);
  // }

  // xt::dump_npy(check_data / "cexe.npy",
  //              _calculation_param->fdtdCoefficient()->cexe());
  // xt::dump_npy(check_data / "cexhy.npy",
  //              _calculation_param->fdtdCoefficient()->cexhy());
  // xt::dump_npy(check_data / "cexhz.npy",
  //              _calculation_param->fdtdCoefficient()->cexhz());
  // xt::dump_npy(check_data / "ceye.npy",
  //              _calculation_param->fdtdCoefficient()->ceye());
  // xt::dump_npy(check_data / "ceyhx.npy",
  //              _calculation_param->fdtdCoefficient()->ceyhx());
  // xt::dump_npy(check_data / "ceyhz.npy",
  //              _calculation_param->fdtdCoefficient()->ceyhz());
  // xt::dump_npy(check_data / "ceze.npy",
  //              _calculation_param->fdtdCoefficient()->ceze());
  // xt::dump_npy(check_data / "cezhy.npy",
  //              _calculation_param->fdtdCoefficient()->cezhy());
  // xt::dump_npy(check_data / "cezhx.npy",
  //              _calculation_param->fdtdCoefficient()->cezhx());

  // exit(0);

  {
    std::vector<std::thread> threads;
    for (std::size_t i = 1; i < _domains.size(); ++i) {
      threads.emplace_back([&domain = _domains[i]]() { domain->run(); });
    }

    struct ThreadGuard {
      ~ThreadGuard() {
        for (auto& t : _threads) {
          if (t.joinable()) {
            t.join();
          }
        }
      }

      std::vector<std::thread> _threads;
      std::chrono::high_resolution_clock::time_point _start_time;
    } tg{
        ._threads = std::move(threads),
    };

    _domains[0]->run();
  }

  MpiSupport::instance().barrier();

  _end_time = std::chrono::high_resolution_clock::now();
  if (isRoot()) {
    std::cout << "\n"
              << "Elapsed time: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(
                     _end_time - _start_time)
                     .count()
              << " ms"
              << " or "
              << std::chrono::duration_cast<std::chrono::seconds>(_end_time -
                                                                  _start_time)
                     .count()
              << " s"
              << " or "
              << std::chrono::duration_cast<std::chrono::minutes>(_end_time -
                                                                  _start_time)
                     .count()
              << " m."
              << "\n";
  }
}

void Simulation::init() {
  if (_objects.empty()) {
    throw XFDTDSimulationException("No object is added");
  }

  // First: generate grid space
  generateGridSpace();

  globalGridSpaceDecomposition();

  generateEMF();

  _calculation_param = std::make_shared<CalculationParam>();
  _calculation_param->setTimeParam(makeTimeParam());

  // Third: init all the objects
  for (const auto& o : _objects) {
    o->init(_grid_space, _calculation_param, _emf);
  }
  for (const auto& b : _boundaries) {
    b->init(_grid_space, _calculation_param, _emf);
  }
  for (const auto& s : _waveform_sources) {
    s->init(_grid_space, _calculation_param, _emf);
  }

  generateMaterialSpace();

  generateFDTDUpdateCoefficient();

  // init monitor
  for (auto&& m : _monitors) {
    m->init(_grid_space, _calculation_param, _emf);
  }
  for (const auto& n : _networks) {
    n->init(_grid_space, _calculation_param, _emf);
  }
  for (const auto& n : _nfffts) {
    n->init(_grid_space, _calculation_param, _emf);
  }

  generateDomain();

  for (auto&& m : _monitors) {
    m->initParallelizedConfig();
  }

  MpiSupport::instance().generateSlice(
      _grid_space->sizeX(), _grid_space->sizeY(), _grid_space->sizeZ());
}

void Simulation::generateDomain() {
  if (std::thread::hardware_concurrency() < numThread()) {
    std::stringstream ss;
    ss << "The number of threads is too large : " << numThread() << ". Set to "
       << std::thread::hardware_concurrency() << "\n";
    throw XFDTDSimulationException(ss.str());
  }

  auto num_thread = numThread();
  auto type = _thread_config.dividerType();

  Divider::IndexTask problem = Divider::makeTask(
      Divider::makeRange<std::size_t>(0, _grid_space->sizeX()),
      Divider::makeRange<std::size_t>(0, _grid_space->sizeY()),
      Divider::makeRange<std::size_t>(0, _grid_space->sizeZ()));

  auto tasks = std::vector<Divider::IndexTask>{Divider::makeTask(
      Divider::makeRange<std::size_t>(0, _grid_space->sizeX()),
      Divider::makeRange<std::size_t>(0, _grid_space->sizeY()),
      Divider::makeRange<std::size_t>(0, _grid_space->sizeZ()))};

  tasks = Divider::divide(problem, type, _thread_config.numX(),
                          _thread_config.numY(), _thread_config.numZ());

  // Corrector
  /*  IMPORTANT: the corrector can't be parallelized in thread model, the
  problem is here: tangential electric field in the thread edge is corrected by
  two threads. Example: Divider type is X, the number of threads is 2, total nx
  is 100. Thread 0 get [0, 50), and thread 1 get [50, 100). The tangential
  electric field at x = 50 is corrected by both thread 0 and thread 1. Think
  about the case pml. Ez(x = 50) is contain in pml of thread 0 and pml of
  thread 1.
  NOTE(franzero): If the corrector is parallelized, the corrector has to be
  implemented that function correctEdge(). Just like updator, let thread 0 to
  update 50, and thread 1 to update 51-100. I suggest that it's not necessary to
  parallelize the corrector in the thread model, because the corrector is
  usually much smaller than the problem size.
  */
  std::vector<std::unique_ptr<Corrector>> correctors;

  for (auto&& w : _waveform_sources) {
    auto c = w->generateCorrector(problem);
    if (c == nullptr) {
      continue;
    }

    correctors.emplace_back(std::move(c));
  }

  for (auto&& o : _objects) {
    auto c = o->generateCorrector(problem);
    if (c == nullptr) {
      continue;
    }

    correctors.emplace_back(std::move(c));
  }

  for (auto&& b : _boundaries) {
    auto c = b->generateDomainCorrector(problem);
    if (c == nullptr) {
      continue;
    }

    correctors.emplace_back(std::move(c));
  }

  bool master = true;
  std::size_t id = {0};
  for (const auto& t : tasks) {
    auto updator = makeUpdator(t);

    if (id == _thread_config.root()) {
      _domains.emplace_back(std::make_unique<Domain>(
          id, t, _grid_space, _calculation_param, _emf, std::move(updator),
          _waveform_sources, std::move(correctors), _monitors, _nfffts,
          _barrier, master));
      master = false;
    } else {
      _domains.emplace_back(
          std::make_unique<Domain>(id, t, _grid_space, _calculation_param, _emf,
                                   std::move(updator), _barrier, false));
    }

    ++id;
  }

  if (master) {
    throw XFDTDSimulationException("Master domain is not created");
  }
}

void Simulation::generateGridSpace() {
  std::vector<const Shape*> shapes;
  shapes.reserve(_objects.size());
  for (const auto& o : _objects) {
    shapes.emplace_back(o->shape().get());
  }

  _global_grid_space =
      GridSpaceGenerator::generateUniformGridSpace(shapes, _dx, _dy, _dz);
  for (const auto& b : _boundaries) {
    if (auto pml = dynamic_cast<PML*>(b.get()); pml != nullptr) {
      auto direction = pml->direction();
      auto num = pml->thickness();
      auto main_axis = pml->mainAxis();

      auto space_dim = _global_grid_space->dimension();
      if (space_dim == GridSpace::Dimension::ONE && main_axis != Axis::XYZ::Z) {
        throw XFDTDSimulationException("PML has to be in Z direction");
      }

      if (space_dim == GridSpace::Dimension::TWO && main_axis == Axis::XYZ::Z) {
        throw XFDTDSimulationException("PML has to be in X or Y direction");
      }

      if (num < 0) {
        continue;
      }

      switch (main_axis) {
        case xfdtd::Axis::XYZ::X: {
          _global_grid_space->extendGridSpace(direction, num, _dx);
          break;
        }
        case xfdtd::Axis::XYZ::Y: {
          _global_grid_space->extendGridSpace(direction, num, _dy);
          break;
        }
        case xfdtd::Axis::XYZ::Z: {
          _global_grid_space->extendGridSpace(direction, num, _dz);
          break;
        }
        default:
          throw XFDTDSimulationException("Invalid main axis");
      }
    }
  }

  _global_grid_space->correctGridSpace();
}

void Simulation::generateEMF() {
  _emf = std::make_unique<EMF>();
  const auto nx = _grid_space->sizeX();
  const auto ny = _grid_space->sizeY();
  const auto nz = _grid_space->sizeZ();

  // make a function OK?
  _emf->allocateEx(nx, ny + 1, nz + 1);
  _emf->allocateEy(nx + 1, ny, nz + 1);
  _emf->allocateEz(nx + 1, ny + 1, nz);

  _emf->allocateHx(nx + 1, ny, nz);
  _emf->allocateHy(nx, ny + 1, nz);
  _emf->allocateHz(nx, ny, nz + 1);
}

void Simulation::globalGridSpaceDecomposition() {
  auto my_mpi_rank = myRank();
  // auto mpi_size = numNode();
  auto node_divider_type = MpiSupport::instance().config().dividerType();
  auto divide_nx = MpiSupport::instance().config().numX();
  auto divide_ny = MpiSupport::instance().config().numY();
  auto divide_nz = MpiSupport::instance().config().numZ();

  auto global_problem = Divider::makeIndexTask(
      Divider::makeRange<std::size_t>(0, _global_grid_space->sizeX()),
      Divider::makeRange<std::size_t>(0, _global_grid_space->sizeY()),
      Divider::makeRange<std::size_t>(0, _global_grid_space->sizeZ()));

  auto global_task = Divider::divide(global_problem, node_divider_type,
                                     divide_nx, divide_ny, divide_nz);
  auto my_task = global_task[my_mpi_rank];

  auto overlap_offset =
      std::tuple<std::size_t, std::size_t, std::size_t>{0, 0, 0};

  switch (node_divider_type) {
    case Divider::Type::X: {
      overlap_offset =
          std::tuple<std::size_t, std::size_t, std::size_t>{1, 0, 0};
      break;
    }
    case Divider::Type::Y: {
      overlap_offset =
          std::tuple<std::size_t, std::size_t, std::size_t>{0, 1, 0};
      break;
    }
    case Divider::Type::Z: {
      overlap_offset =
          std::tuple<std::size_t, std::size_t, std::size_t>{0, 0, 1};
      break;
    }
    case Divider::Type::XY: {
      overlap_offset =
          std::tuple<std::size_t, std::size_t, std::size_t>{1, 1, 0};
      break;
    }
    case Divider::Type::XZ: {
      overlap_offset =
          std::tuple<std::size_t, std::size_t, std::size_t>{1, 0, 1};
      break;
    }
    case Divider::Type::YZ: {
      overlap_offset =
          std::tuple<std::size_t, std::size_t, std::size_t>{0, 1, 1};
      break;
    }
    default:
      throw XFDTDSimulationException("Invalid node divider type");
  }

  auto x_start = (my_task.xRange().start() == 0)
                     ? 0
                     : my_task.xRange().start() - std::get<0>(overlap_offset);
  auto y_start = (my_task.yRange().start() == 0)
                     ? 0
                     : my_task.yRange().start() - std::get<1>(overlap_offset);
  auto z_start = (my_task.zRange().start() == 0)
                     ? 0
                     : my_task.zRange().start() - std::get<2>(overlap_offset);
  auto x_end = (my_task.xRange().end() == _global_grid_space->sizeX())
                   ? _global_grid_space->sizeX()
                   : my_task.xRange().end() + std::get<0>(overlap_offset);
  auto y_end = (my_task.yRange().end() == _global_grid_space->sizeY())
                   ? _global_grid_space->sizeY()
                   : my_task.yRange().end() + std::get<1>(overlap_offset);
  auto z_end = (my_task.zRange().end() == _global_grid_space->sizeZ())
                   ? _global_grid_space->sizeZ()
                   : my_task.zRange().end() + std::get<2>(overlap_offset);

  /*   Following is the explanation of the overlap. IMPORTANT: condition in no
    overlap with the boundary. If node divider type is X, the number of EzNode
    and EyNode in X axis is nx + 3; front() means -1, back() means nx + 1.
    (front()) (0) (1) (2) ... (nx - 1) (nx) (back())
    Note: Don't update front() and back(). (0) is the overlap with the previous
    calculation node. And (nx) is the overlap with the next calculation
    node. */
  auto my_task_with_overlap =
      Divider::makeIndexTask(Divider::makeRange<std::size_t>(x_start, x_end),
                             Divider::makeRange<std::size_t>(y_start, y_end),
                             Divider::makeRange<std::size_t>(z_start, z_end));

  _grid_space = _global_grid_space->subGridSpace(
      my_task_with_overlap._x_range.start(),
      my_task_with_overlap._y_range.start(),
      my_task_with_overlap._z_range.start(),
      my_task_with_overlap._x_range.end(), my_task_with_overlap._y_range.end(),
      my_task_with_overlap._z_range.end());

  _grid_space->setGlobalGridSpace(_global_grid_space);
}

void Simulation::generateMaterialSpace() {
  _grid_space->generateMaterialGrid(_grid_space->sizeX(), _grid_space->sizeY(),
                                    _grid_space->sizeZ());
  auto material_param = makeMaterialParam();
  material_param->allocate(_grid_space->sizeX(), _grid_space->sizeY(),
                           _grid_space->sizeZ());
  _calculation_param->setMaterialParam(std::move(material_param));
  correctMaterialSpace();
}

void Simulation::generateFDTDUpdateCoefficient() {
  _calculation_param->calculateCoefficient(_grid_space.get());
  correctUpdateCoefficient();
}

void Simulation::correctMaterialSpace() {
  std::size_t m_index = {0};
  for (auto&& o : _objects) {
    if (std::dynamic_pointer_cast<PecPlane>(o) != nullptr) {
      continue;
    }

    o->correctMaterialSpace(m_index);
    _calculation_param->materialParam()->addMaterial(o->material());
    ++m_index;
  }

  for (auto&& b : _boundaries) {
    b->correctMaterialSpace();
  }

  for (auto&& s : _waveform_sources) {
    s->correctMaterialSpace();
  }

  _calculation_param->generateMaterialSpaceParam(_grid_space.get());
  for (auto&& o : _objects) {
    if (std::dynamic_pointer_cast<PecPlane>(o) == nullptr) {
      continue;
    }
    // for pec plane
    o->correctMaterialSpace(-1);
  }
}

void Simulation::correctUpdateCoefficient() {
  for (auto&& o : _objects) {
    o->correctUpdateCoefficient();
  }

  for (auto&& b : _boundaries) {
    b->correctUpdateCoefficient();
  }

  for (auto&& s : _waveform_sources) {
    s->correctUpdateCoefficient();
  }
}

std::unique_ptr<TimeParam> Simulation::makeTimeParam() {
  auto time_param = std::make_unique<TimeParam>(_cfl);
  switch (_grid_space->dimension()) {
    case GridSpace::Dimension::ONE: {
      time_param->setDt(TimeParam::calculateDt(_cfl, _grid_space->minDz()));
      break;
    }
    case GridSpace::Dimension::TWO: {
      time_param->setDt(TimeParam::calculateDt(_cfl, _grid_space->minDx(),
                                               _grid_space->minDy()));
      break;
    }
    case GridSpace::Dimension::THREE: {
      time_param->setDt(TimeParam::calculateDt(_cfl, _grid_space->minDx(),
                                               _grid_space->minDy(),
                                               _grid_space->minDz()));
      break;
    }
    default:
      throw XFDTDSimulationException("Invalid dimension");
  }

  return time_param;
}

std::unique_ptr<MaterialParam> Simulation::makeMaterialParam() {
  auto material_param = std::make_unique<MaterialParam>();
  auto nx = _grid_space->sizeX();
  auto ny = _grid_space->sizeY();
  auto nz = _grid_space->sizeZ();
  material_param->allocate(nx, ny, nz);
  return material_param;
}

std::unique_ptr<Updator> Simulation::makeUpdator(
    const Divider::IndexTask& task) {
  bool dispersion = false;
  for (const auto& m : _calculation_param->materialParam()->materialArray()) {
    if (m->dispersion()) {
      dispersion = true;
      break;
    }
  }

  if (!dispersion) {
    if (_grid_space->dimension() == GridSpace::Dimension::THREE) {
      return std::make_unique<BasicUpdator3D>(_grid_space, _calculation_param,
                                              _emf, task);
    }
    if (_grid_space->dimension() == GridSpace::Dimension::TWO) {
      return std::make_unique<BasicUpdatorTE>(_grid_space, _calculation_param,
                                              _emf, task);
    }
    throw XFDTDSimulationException("Invalid dimension");
  }

  // Contains linear dispersive material
  if (dispersion && _grid_space->dimension() == GridSpace::Dimension::THREE) {
    for (const auto& m : _calculation_param->materialParam()->materialArray()) {
      if (!m->dispersion()) {
        continue;
      }

      auto dispersion_material =
          std::dynamic_pointer_cast<LinearDispersiveMaterial>(m);
      if (!dispersion_material) {
        break;
      }

      if (auto lorentz_material =
              std::dynamic_pointer_cast<LorentzMedium>(dispersion_material);
          lorentz_material != nullptr) {
        return std::make_unique<LorentzADEUpdator>(
            _grid_space, _calculation_param, _emf, task);
      }

      if (auto drude_material =
              std::dynamic_pointer_cast<DrudeMedium>(dispersion_material);
          drude_material != nullptr) {
        return std::make_unique<DrudeADEUpdator>(
            _grid_space, _calculation_param, _emf, task);
      }

      if (auto debye_material =
              std::dynamic_pointer_cast<DebyeMedium>(dispersion_material);
          debye_material != nullptr) {
        return std::make_unique<DebyeADEUpdator>(
            _grid_space, _calculation_param, _emf, task);
      }
    }
  }

  throw XFDTDSimulationException("don't support this type of updator yet.");
}

}  // namespace xfdtd
