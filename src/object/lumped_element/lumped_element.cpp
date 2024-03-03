#include <xfdtd/object/lumped_element/lumped_element.h>

#include "divider/divider.h"
#include "xfdtd/electromagnetic_field/electromagnetic_field.h"
#include "xfdtd/grid_space/grid_space.h"

namespace xfdtd {

LumpedElement::LumpedElement(std::string name, std::unique_ptr<Cube> cube,
                             Axis::XYZ xyz, std::unique_ptr<Material> material)
    : Object{std::move(name), std::move(cube), std::move(material)},
      _xyz{xyz} {}

void LumpedElement::init(std::shared_ptr<const GridSpace> grid_space,
                         std::shared_ptr<CalculationParam> calculation_param,
                         std::shared_ptr<EMF> emf) {
  Object::init(std::move(grid_space), std::move(calculation_param),
               std::move(emf));

  auto grid_box{gridBoxPtr()};
  _is = grid_box->origin().i();
  _js = grid_box->origin().j();
  _ks = grid_box->origin().k();
  _ie = grid_box->end().i();
  _je = grid_box->end().j();
  _ke = grid_box->end().k();
}

// void LumpedElement::correctMaterialSpace(std::size_t index) {}

Axis::XYZ LumpedElement::xyz() const { return _xyz; }

std::size_t LumpedElement::nodeCountX() const {
  switch (xyz()) {
    case Axis::XYZ::X:
      return _ie - _is;
    default:
      return _ie - _is + 1;
  }
}

std::size_t LumpedElement::nodeCountY() const {
  switch (xyz()) {
    case Axis::XYZ::Y:
      return _je - _js;
    default:
      return _je - _js + 1;
  }
}

std::size_t LumpedElement::nodeCountZ() const {
  switch (xyz()) {
    case Axis::XYZ::Z:
      return _ke - _ks;
    default:
      return _ke - _ks + 1;
  }
}

std::size_t LumpedElement::nodeCountMainAxis() const {
  switch (xyz()) {
    case Axis::XYZ::X:
      return _ie - _is;
    case Axis::XYZ::Y:
      return _je - _js;
    case Axis::XYZ::Z:
      return _ke - _ks;
    default:
      throw std::runtime_error("Invalid xyz");
  }
}

std::size_t LumpedElement::nodeCountSubAxisA() const {
  switch (xyz()) {
    case Axis::XYZ::X:
      return _je - _js + 1;
    case Axis::XYZ::Y:
      return _ke - _ks + 1;
    case Axis::XYZ::Z:
      return _ie - _is + 1;
    default:
      throw std::runtime_error("Invalid xyz");
  }
}

std::size_t LumpedElement::nodeCountSubAxisB() const {
  switch (xyz()) {
    case Axis::XYZ::X:
      return _ke - _ks + 1;
    case Axis::XYZ::Y:
      return _ie - _is + 1;
    case Axis::XYZ::Z:
      return _je - _js + 1;
    default:
      throw std::runtime_error("Invalid xyz");
  }
}

xt::xarray<double>& LumpedElement::fieldMainAxis(EMF::Attribute attribute) {
  switch (xyz()) {
    case Axis::XYZ::X:
      return emfPtr()->field(attribute, EMF::Component::X);
    case Axis::XYZ::Y:
      return emfPtr()->field(attribute, EMF::Component::Y);
    case Axis::XYZ::Z:
      return emfPtr()->field(attribute, EMF::Component::Z);
    default:
      throw std::runtime_error("Invalid xyz");
  }
}

bool LumpedElement::taskContainLumpedElement(
    const Divider::Task<std::size_t>& task) const {
  return Divider::intersected(task, makeIndexTask());
}

Divider::IndexTask LumpedElement::makeIndexTask() const {
  return Divider::makeTask(Divider::makeRange(_is, _is + nodeCountX()),
                           Divider::makeRange(_js, _js + nodeCountY()),
                           Divider::makeRange(_ks, _ks + nodeCountZ()));
}

}  // namespace xfdtd
