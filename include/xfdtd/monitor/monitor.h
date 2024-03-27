#ifndef _XFDTD_CORE_MONITOR_H_
#define _XFDTD_CORE_MONITOR_H_

#include <xfdtd/calculation_param/calculation_param.h>
#include <xfdtd/divider/divider.h>
#include <xfdtd/electromagnetic_field/electromagnetic_field.h>
#include <xfdtd/exception/exception.h>
#include <xfdtd/grid_space/grid_space.h>
#include <xfdtd/parallel/mpi_config.h>
#include <xfdtd/parallel/mpi_support.h>
#include <xfdtd/parallel/parallelized_config.h>
#include <xfdtd/shape/shape.h>

#include <memory>
#include <string>
#include <vector>
#include <xtensor/xarray.hpp>

namespace xfdtd {

class XFDTDMonitorException : public XFDTDException {
 public:
  explicit XFDTDMonitorException(
      std::string message = "XFDTD Monitor Exception")
      : XFDTDException(std::move(message)) {}
};

class Monitor {
 public:
  explicit Monitor(std::unique_ptr<Shape> shape, std::string name = "monitor",
                   std::string output_dir = "xfdtd_output");

  Monitor(const Monitor&) = delete;

  Monitor(Monitor&&) noexcept = default;

  Monitor& operator=(const Monitor&) = delete;

  Monitor& operator=(Monitor&&) noexcept = default;

  virtual ~Monitor() = default;

  virtual void init(std::shared_ptr<const GridSpace> grid_space,
                    std::shared_ptr<const CalculationParam> calculation_param,
                    std::shared_ptr<const EMF> emf) = 0;

  virtual void update() = 0;

  const std::unique_ptr<Shape>& shape() const;

  const std::string& name() const;

  const std::string& outputDir() const;

  const xt::xarray<double>& data() const;

  std::unique_ptr<Shape>& shape();

  xt::xarray<double>& data();

  void setName(std::string name);

  void setOutputDir(std::string output_dir);

  virtual void output();

  virtual void initTimeDependentVariable();

  GridBox globalGridBox() const;

  GridBox nodeGridBox() const;

  const MpiConfig& monitorMpiConfig() const;

  virtual std::string toString() const;

  virtual auto initParallelizedConfig() -> void;

  auto globalTask() const -> Divider::IndexTask;

  auto nodeTask() const -> Divider::IndexTask;

 protected:
  void defaultInit(std::shared_ptr<const GridSpace> grid_space,
                   std::shared_ptr<const CalculationParam> calculation_param,
                   std::shared_ptr<const EMF> emf);

  const GridSpace* gridSpacePtr() const;

  const CalculationParam* calculationParamPtr() const;

  const EMF* emfPtr() const;

  auto nodeGridBox() -> GridBox&;

  auto nodeTask() -> Divider::IndexTask&;

  MpiConfig& monitorMpiConfig();

  virtual auto gatherData() -> void;

  auto setGlobalTask(Divider::IndexTask task) -> void;

  auto setNodeTask(Divider::IndexTask task) -> void;

  auto mpiBlock() -> MpiSupport::Block&;

  auto mpiBlock() const -> const MpiSupport::Block&;

  auto mpiBlockArray() -> std::vector<MpiSupport::Block>&;

  auto mpiBlockArray() const -> const std::vector<MpiSupport::Block>&;

 private:
  std::unique_ptr<Shape> _shape;
  std::string _name;
  std::string _output_dir;
  xt::xarray<double> _data;

  std::shared_ptr<const GridSpace> _grid_space;
  std::shared_ptr<const CalculationParam> _calculation_param;
  std::shared_ptr<const EMF> _emf;

  GridBox _global_grid_box;
  GridBox _node_grid_box;
  Divider::IndexTask _global_task;
  Divider::IndexTask _node_task;

  MpiConfig _monitor_mpi_config;
  MpiSupport::Block _block;
  std::vector<MpiSupport::Block::Profile> _profiles;
  std::vector<MpiSupport::Block> _blocks_mpi;
};

}  // namespace xfdtd

#endif  // _XFDTD_CORE_MONITOR_H_
