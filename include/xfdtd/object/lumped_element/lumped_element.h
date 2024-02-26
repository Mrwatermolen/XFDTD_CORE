#ifndef _XFDTD_LIB_LUMPED_ELEMENT_H_
#define _XFDTD_LIB_LUMPED_ELEMENT_H_

#include <xfdtd/object/object.h>

#include "xfdtd/coordinate_system/coordinate_system.h"
#include "xfdtd/electromagnetic_field/electromagnetic_field.h"

namespace xfdtd {

class XFDTDLumpedElementException : public XFDTDObjectException {
 public:
  explicit XFDTDLumpedElementException(
      std::string message = "XFDTD Lumped Element Exception")
      : XFDTDObjectException(std::move(message)) {}
};

class LumpedElement : public Object {
 public:
  LumpedElement(std::string name, std::unique_ptr<Cube> cube, Axis::XYZ xyz,
                std::unique_ptr<Material> material = Material::createAir());

  LumpedElement(const LumpedElement&) = delete;

  LumpedElement(LumpedElement&&) noexcept = default;

  LumpedElement& operator=(const LumpedElement&) = delete;

  LumpedElement& operator=(LumpedElement&&) noexcept = default;

  ~LumpedElement() override = default;

  Axis::XYZ xyz() const;

  // void correctMaterialSpace(std::size_t index) override;

  void init(std::shared_ptr<const GridSpace> grid_space,
            std::shared_ptr<CalculationParam> calculation_param,
            std::shared_ptr<EMF> emf) override;

  auto rangeX() const;

  auto rangeY() const;

  auto rangeZ() const;

  std::size_t nodeCountX() const;

  std::size_t nodeCountY() const;

  std::size_t nodeCountZ() const;

  std::size_t nodeCountMainAxis() const;

  std::size_t nodeCountSubAxisA() const;

  std::size_t nodeCountSubAxisB() const;

 protected:
  std::size_t _is, _ie, _js, _je, _ks, _ke;

  xt::xarray<double>& fieldMainAxis(EMF::Attribute attribute);

 private:
  Axis::XYZ _xyz;
};

inline auto LumpedElement::rangeX() const {
  switch (xyz()) {
    case Axis::XYZ::X:
      return xt::range(_is, _ie);
    default:
      return xt::range(_is, _ie + 1);
  }
}

inline auto LumpedElement::rangeY() const {
  switch (xyz()) {
    case Axis::XYZ::Y:
      return xt::range(_js, _je);
    default:
      return xt::range(_js, _je + 1);
  }
}

inline auto LumpedElement::rangeZ() const {
  switch (xyz()) {
    case Axis::XYZ::Z:
      return xt::range(_ks, _ke);
    default:
      return xt::range(_ks, _ke + 1);
  }
}

}  // namespace xfdtd

#endif  // _XFDTD_LIB_LUMPED_ELEMENT_H_
