#ifndef _XFDTD_CORE_NETWORK_H_
#define _XFDTD_CORE_NETWORK_H_

#include <xfdtd/common/type_define.h>
#include <xfdtd/exception/exception.h>
#include <xfdtd/network/port.h>

#include <complex>
#include <cstddef>
#include <memory>
#include <set>
#include <unordered_map>
#include <vector>
namespace xfdtd {

class XFDTDNetworkException : public XFDTDException {
 public:
  explicit XFDTDNetworkException(
      const std::string& message = "XFDTD Network Exception")
      : XFDTDException(message) {}
};

class Network {
 public:
  Network() = default;

  explicit Network(std::string output_dir);

  Network(std::vector<std::shared_ptr<Port>> ports,
          Array1D<Real> frequencies, std::string output_dir);

  Network(const Network& network) = default;

  Network(Network&& network) noexcept = default;

  Network& operator=(const Network& network) = default;

  Network& operator=(Network&& network) noexcept = default;

  ~Network() = default;

  void init(const std::shared_ptr<const GridSpace>& grid_space,
            const std::shared_ptr<const CalculationParam>& calculation_param,
            const std::shared_ptr<const EMF>& emf);

  void output();

  void addPort(std::shared_ptr<Port> port);

  void setFrequencies(Array1D<Real> frequencies);

  void setOutputDir(std::string output_dir);

 private:
  std::vector<std::shared_ptr<Port>> _ports;
  Array1D<Real> _frequencies;
  std::string _output_dir;

  std::set<std::size_t> _port_set;
  std::unordered_map<int, Array1D<std::complex<Real>>> _s_parameters;
};

}  // namespace xfdtd

#endif  // _XFDTD_CORE_NETWORK_H_
