#ifndef _XFDTD_CORE_CURRENT_MONITOR_H_
#define _XFDTD_CORE_CURRENT_MONITOR_H_

#include <xfdtd/monitor/time_monitor.h>

namespace xfdtd {

class CurrentMonitor : public TimeMonitor {
 public:
  CurrentMonitor(std::string name, std::unique_ptr<Cube> cube,
                 Axis::Direction direction, std::string output_dir);

  CurrentMonitor(const CurrentMonitor&) = delete;

  CurrentMonitor(CurrentMonitor&&) noexcept = default;

  CurrentMonitor& operator=(const CurrentMonitor&) = delete;

  CurrentMonitor& operator=(CurrentMonitor&&) noexcept = default;

  ~CurrentMonitor() override = default;

  void init(std::shared_ptr<const GridSpace> grid_space,
            std::shared_ptr<const CalculationParam> calculation_param,
            std::shared_ptr<const EMF> emf) override;

  void update() override;

  void initTimeDependentVariable() override;

  auto initParallelizedConfig() -> void override;

  auto gatherData() -> void override;

  auto valid() const -> bool override;

  auto toString() const -> std::string override;

 private:
  Axis::Direction _direction;
  std::size_t _is, _ie, _js, _je, _ks, _ke;
  Array1D<Real> _da, _db;
  Real _positive;

  Array1D<Real> _node_data;
  IndexRange _ha_range_bn, _ha_range_bp, _hb_range_an, _hb_range_ap;
};

}  // namespace xfdtd

#endif  // _XFDTD_CORE_CURRENT_MONITOR_H_
