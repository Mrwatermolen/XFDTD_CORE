#include <xfdtd/coordinate_system/coordinate_system.h>

#include <string>

namespace xfdtd {

Axis::Axis(const Vector& direction) : Vector{Vector::normalized(direction)} {}

bool Axis::operator==(const Axis& rhs) const { return Vector::operator==(rhs); }

bool Axis::operator!=(const Axis& rhs) const { return !(*this == rhs); }

bool Axis::operator==(const Vector& rhs) const {
  return Vector::operator==(rhs);
}

bool Axis::operator!=(const Vector& rhs) const { return !(*this == rhs); }

bool Axis::operator==(Axis::Direction rhs) const {
  return *this == Axis::fromDirectionToAxis(rhs);
}

bool Axis::operator!=(Axis::Direction rhs) const { return !(*this == rhs); }

bool Axis::operator==(Axis::XYZ rhs) const {
  switch (rhs) {
    case Axis::XYZ::X:
      return *this == Axis::XP || *this == Axis::XN;
    case Axis::XYZ::Y:
      return *this == Axis::YP || *this == Axis::YN;
    case Axis::XYZ::Z:
      return *this == Axis::ZP || *this == Axis::ZN;
    default:
      throw XFDTDCoordinateSystemAxisException{"Invalid XYZ value"};
  }
}

bool Axis::operator!=(Axis::XYZ rhs) const { return !(*this == rhs); }

std::string Axis::toString() const {
  return std::string{"Axis("} + std::to_string(x()) + ", " +
         std::to_string(y()) + ", " + std::to_string(z()) + ")";
}

const Axis Axis::XN{Vector{-1, 0, 0}};
const Axis Axis::XP{Vector{1, 0, 0}};
const Axis Axis::YN{Vector{0, -1, 0}};
const Axis Axis::YP{Vector{0, 1, 0}};
const Axis Axis::ZN{Vector{0, 0, -1}};
const Axis Axis::ZP{Vector{0, 0, 1}};

Axis Axis::fromDirectionToAxis(Axis::Direction direction) {
  switch (direction) {
    case Axis::Direction::XN:
      return Axis::XN;
    case Axis::Direction::XP:
      return Axis::XP;
    case Axis::Direction::YN:
      return Axis::YN;
    case Axis::Direction::YP:
      return Axis::YP;
    case Axis::Direction::ZN:
      return Axis::ZN;
    case Axis::Direction::ZP:
      return Axis::ZP;
    default:
      throw XFDTDCoordinateSystemAxisDirectionException{};
  }
}

Axis::XYZ Axis::formDirectionToXYZ(Axis::Direction direction) {
  switch (direction) {
    case Axis::Direction::XN:
    case Axis::Direction::XP:
      return Axis::XYZ::X;
    case Axis::Direction::YN:
    case Axis::Direction::YP:
      return Axis::XYZ::Y;
    case Axis::Direction::ZN:
    case Axis::Direction::ZP:
      return Axis::XYZ::Z;
    default:
      throw XFDTDCoordinateSystemAxisDirectionException{};
  }
}

bool Axis::directionNegative(Axis::Direction direction) {
  switch (direction) {
    case Axis::Direction::XN:
    case Axis::Direction::YN:
    case Axis::Direction::ZN:
      return true;
    case Axis::Direction::XP:
    case Axis::Direction::YP:
    case Axis::Direction::ZP:
      return false;
    default:
      throw XFDTDCoordinateSystemAxisDirectionException{};
  }
}

bool Axis::directionPositive(Axis::Direction direction) {
  switch (direction) {
    case Axis::Direction::XN:
    case Axis::Direction::YN:
    case Axis::Direction::ZN:
      return false;
    case Axis::Direction::XP:
    case Axis::Direction::YP:
    case Axis::Direction::ZP:
      return true;
    default:
      throw XFDTDCoordinateSystemAxisDirectionException{};
  }
}

}  // namespace xfdtd
