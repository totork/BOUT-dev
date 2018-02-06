/*
 * Test LaplaceXZ solver
 *
 * Matrix assembly information:
 *    -mat_view ::ascii_info
 *
 * Useful diagnostics for SuperLU_dist solver
 * (pctype=lu, factor_package=superlu_dist)
 * -mat_superlu_dist_statprint
 */
#include <bout.hxx>

#include <bout/invert/laplacexy.hxx>
#include <field_factory.hxx>
#include "loadmetric.hxx"

int main(int argc, char** argv) {
  BoutInitialise(argc, argv);

  LoadMetric(1.,1.);

  LaplaceXY *inv = new LaplaceXY(mesh);

  output.write("Setting coefficients\n");

  Field2D A = FieldFactory::get()->create2D("a1", Options::getRoot(), mesh);
  Field2D B = FieldFactory::get()->create2D("b1", Options::getRoot(), mesh);
  //Field2D A = 1.;
  //Field2D B = 0.;
  inv->setCoefs(A,B);

  output.write("First solve\n");

  Field2D rhs = FieldFactory::get()->create2D("rhs", Options::getRoot(), mesh);
  Field2D x = inv->solve(rhs, 0.0);
  mesh->communicate(x);
  Field2D check = Laplace_perpXY(A,x) + B*x - rhs;
  Field2D check_laplaceperp = A*Laplace_perp(x) + B*x - rhs;

  SAVE_ONCE4(rhs, x, check, check_laplaceperp);

  output.write("Second solve\n");

  A = FieldFactory::get()->create2D("a2", Options::getRoot(), mesh);
  B = FieldFactory::get()->create2D("b2", Options::getRoot(), mesh);
  //A = 2.;
  //B = 0.1;
  inv->setCoefs(A,B);

  Field2D rhs2 = FieldFactory::get()->create2D("rhs2", Options::getRoot(), mesh);
  Field2D x2 = inv->solve(rhs2, 0.0);
  mesh->communicate(x2);
  Field2D check2 = Laplace_perpXY(A,x2) + B*x2 - rhs2;
  Field2D check2_laplaceperp = A*Laplace_perp(x2) + B*x2 - rhs2;
  SAVE_ONCE4(rhs2, x2, check2, check2_laplaceperp);

  dump.write();

  delete inv;

  BoutFinalise();
  return 0;
}
