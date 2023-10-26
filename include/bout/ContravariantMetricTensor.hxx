
#ifndef BOUT_CONTRAVARIANTMETRICTENSOR_HXX
#define BOUT_CONTRAVARIANTMETRICTENSOR_HXX

#include "CovariantMetricTensor.hxx"
#include "bout/field2d.hxx"
#include "bout/field3d.hxx"
#include "bout/paralleltransform.hxx"
#include "bout/utils.hxx"
#include <bout/bout_types.hxx>

class ContravariantMetricTensor {

public:
#if BOUT_USE_METRIC_3D
  using FieldMetric = Field
#else
  using FieldMetric = Field2D;
#endif

  ContravariantMetricTensor(const FieldMetric& g11, const FieldMetric& g22,
                            const FieldMetric& g33, const FieldMetric& g12,
                            const FieldMetric& g13, const FieldMetric& g23);

  ContravariantMetricTensor(const BoutReal g11, const BoutReal g22, const BoutReal g33,
                            const BoutReal g12, const BoutReal g13, const BoutReal g23,
                            Mesh* mesh);

  // check that contravariant tensors are positive (if expected) and finite (always)
  void checkContravariant(int ystart);

  const FieldMetric& Getg11() const;
  const FieldMetric& Getg22() const;
  const FieldMetric& Getg33() const;
  const FieldMetric& Getg12() const;
  const FieldMetric& Getg13() const;
  const FieldMetric& Getg23() const;

  void setContravariantMetricTensor(const ContravariantMetricTensor& metric_tensor);

  void Allocate();

  void setLocation(const CELL_LOC location);

  /// Invert covariant metric to get contravariant components
  void calcContravariant(CovariantMetricTensor covariantMetricTensor, CELL_LOC location,
                         const std::string& region = "RGN_ALL");

private:
  FieldMetric g11, g22, g33, g12, g13, g23;
};

#endif //BOUT_CONTRAVARIANTMETRICTENSOR_HXX
