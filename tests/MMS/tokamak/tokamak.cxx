/*
 * MMS test in tokamak geometry
 *
 * Independently tests the bracket operator, perpendicular diffusion,
 * and parallel diffusion operators.
 * 
 */

#include <bout/derivs.hxx>
#include <bout/field_factory.hxx>
#include <bout/physicsmodel.hxx>

class TokamakMMS : public PhysicsModel {
public:
  int init(bool UNUSED(restarting)) {
    solver->add(laplacepar, "laplacepar");
    solver->add(delp2, "delp2");
    solver->add(advect, "advect");

    // Load the metric tensor
    LoadMetric(1.0, 1.0);

    return 0;
  }
  int rhs(BoutReal time) {
    mesh->communicate(advect, delp2, laplacepar);

    drive = FieldFactory::get()->create3D("drive:solution", Options::getRoot(), mesh,
                                          CELL_CENTRE, time);

    // Test bracket advection operator
    ddt(advect) = -1e-3 * bracket(drive, advect, BRACKET_ARAKAWA)
                  - 10.
                        * (SQ(SQ(mesh->getCoordinates()->dx())) * D4DX4(advect)
                           + SQ(SQ(mesh->getCoordinates()->dz())) * D4DZ4(advect));

    // Test perpendicular diffusion operator
    ddt(delp2) = 1e-5 * Delp2(delp2);

    // Test parallel diffusion operator
    ddt(laplacepar) = Laplace_par(laplacepar);

    return 0;
  }
  void LoadMetric(BoutReal Lnorm, BoutReal Bnorm) {
    // Load metric coefficients from the mesh
    Field2D Rxy, Bpxy, Btxy, hthe, sinty;
    GRID_LOAD5(Rxy, Bpxy, Btxy, hthe, sinty); // Load metrics

    Coordinates* coords = mesh->getCoordinates();

    // Checking for dpsi used in BOUT grids
    Field2D dx;
    if (!mesh->get(dx, "dpsi")) {
      output << "\tUsing dpsi as the x grid spacing\n";
      coords->dx() = dx; // Only use dpsi if found
    } else {
      // dx will have been read already from the grid
      output << "\tUsing dx as the x grid spacing\n";
    }

    Rxy /= Lnorm;
    hthe /= Lnorm;
    sinty *= SQ(Lnorm) * Bnorm;
    coords->setDx(coords->dx() / (SQ(Lnorm) * Bnorm));

    Bpxy /= Bnorm;
    Btxy /= Bnorm;
    coords->setBxy(coords->Bxy() / Bnorm);

    // Calculate metric components
    bool ShiftXderivs;
    Options::getRoot()->get("shiftXderivs", ShiftXderivs, false); // Read global flag
    if (ShiftXderivs) {
      sinty = 0.0; // I disappears from metric
    }

    BoutReal sbp = 1.0; // Sign of Bp
    if (min(Bpxy, true) < 0.0) {
      sbp = -1.0;
    }

    const auto g11 = SQ(Rxy * Bpxy);
    const auto g22 = 1.0 / SQ(hthe);
    const auto g33 = SQ(sinty) * coords->g11() + SQ(coords->Bxy()) / coords->g11();
    const auto g12 = 0.0;
    const auto g13 = -sinty * coords->g11();
    const auto g23 = -sbp * Btxy / (hthe * Bpxy * Rxy);
    coords->setContravariantMetricTensor(
        ContravariantMetricTensor(g11, g22, g33, g12, g13, g23));

    coords->setJ(hthe / Bpxy);

    const auto g_11 = 1.0 / coords->g11() + SQ(sinty * Rxy);
    const auto g_22 = SQ(coords->Bxy() * hthe / Bpxy);
    const auto g_33 = Rxy * Rxy;
    const auto g_12 = sbp * Btxy * hthe * sinty * Rxy / Bpxy;
    const auto g_13 = sinty * Rxy * Rxy;
    const auto g_23 = sbp * Btxy * hthe * Rxy / Bpxy;
    coords->setCovariantMetricTensor(
        CovariantMetricTensor(g_11, g_22, g_33, g_12, g_13, g_23));
  }

private:
  Field3D drive;
  Field3D laplacepar, delp2, advect;
};

BOUTMAIN(TokamakMMS);
