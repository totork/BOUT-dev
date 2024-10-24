#include "bout/g_values.hxx"
#include "bout/coordinates.hxx"
#include "bout/derivs.hxx"
#include "bout/mesh.hxx"

GValues::GValues(const Coordinates& coordinates) {

  const auto& contravariantMetricTensor = coordinates.getContravariantMetricTensor();
  const auto& J = coordinates.J();

  const auto& g11 = contravariantMetricTensor.g11();
  const auto& g22 = contravariantMetricTensor.g22();
  const auto& g33 = contravariantMetricTensor.g33();
  const auto& g12 = contravariantMetricTensor.g12();
  const auto& g13 = contravariantMetricTensor.g13();
  const auto& g23 = contravariantMetricTensor.g23();

  auto tmp = J * g12;
  Coordinates::communicate(tmp);
  G1_m = (DDX(J * g11) + DDY(tmp) + DDZ(J * g13)) / J;
  tmp = J * g22;
  Coordinates::communicate(tmp);
  G2_m = (DDX(J * g12) + DDY(tmp) + DDZ(J * g23)) / J;
  tmp = J * g23;
  Coordinates::communicate(tmp);
  G3_m = (DDX(J * g13) + DDY(tmp) + DDZ(J * g33)) / J;
}

void GValues::communicate(Mesh* mesh) { mesh->communicate(G1_m, G2_m, G3_m); }
