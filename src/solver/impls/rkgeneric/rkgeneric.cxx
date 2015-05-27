
#include "rkgeneric.hxx"

#include <boutcomm.hxx>
#include <utils.hxx>
#include <boutexception.hxx>
#include <msg_stack.hxx>

#include <cmath>

#include <output.hxx>

RKGenericSolver::RKGenericSolver(Options *options) : Solver(options) {
  f0 = 0; // Mark as uninitialised
}

RKGenericSolver::~RKGenericSolver() {
  if(f0 != 0) {
    delete[] f0;
    delete[] f1;
    delete[] f2;
  }
}

void RKGenericSolver::setMaxTimestep(BoutReal dt) {
  if(dt > timestep)
    return; // Already less than this
  
  if(adaptive)
    timestep = dt; // Won't be used this time, but next
}

int RKGenericSolver::init(bool restarting, int nout, BoutReal tstep) {

  int msg_point = msg_stack.push("Initialising RK4 solver");
  
  /// Call the generic initialisation first
  if(Solver::init(restarting, nout, tstep))
    return 1;
  
  output << "\n\tRunge-Kutta 4th-order solver\n";

  nsteps = nout; // Save number of output steps
  out_timestep = tstep;
  max_dt = tstep;
  
  // Calculate number of variables
  nlocal = getLocalN();
  
  // Get total problem size
  int ntmp;
  if(MPI_Allreduce(&nlocal, &ntmp, 1, MPI_INT, MPI_SUM, BoutComm::get())) {
    throw BoutException("MPI_Allreduce failed!");
  }
  neq = ntmp;
  
  output.write("\t3d fields = %d, 2d fields = %d neq=%d, local_N=%d\n",
	       n3Dvars(), n2Dvars(), neq, nlocal);
  
  // Allocate memory
  f0 = new BoutReal[nlocal];
  f1 = new BoutReal[nlocal];
  f2 = new BoutReal[nlocal];
  
  // Put starting values into f0
  save_vars(f0);
  
  // Get options
  OPTION(options, atol, 1.e-5); // Absolute tolerance
  OPTION(options, rtol, 1.e-3); // Relative tolerance
  OPTION(options, max_timestep, tstep); // Maximum timestep
  OPTION(options, timestep, max_timestep); // Starting timestep
  OPTION(options, mxstep, 500); // Maximum number of steps between outputs
  OPTION(options, adaptive, false);

  msg_stack.pop(msg_point);

  return 0;
}

int RKGenericSolver::run() {
  int msg_point = msg_stack.push("RKGenericSolver::run()");
  
  for(int s=0;s<nsteps;s++) {
    BoutReal target = simtime + out_timestep;
    
    BoutReal dt;
    bool running = true;
    int internal_steps = 0;
    do {
      // Take a single time step
      
      do {
        dt = timestep;
        running = true;
        if((simtime + dt) >= target) {
          dt = target - simtime; // Make sure the last timestep is on the output 
          running = false;
        }
        if(adaptive) {
          // Take two half-steps
          take_step(simtime,          0.5*dt, f0, f1);
          take_step(simtime + 0.5*dt, 0.5*dt, f1, f2);
          
          // Take a full step
          take_step(simtime, dt, f0, f1);
          
          // Check accuracy
          BoutReal local_err = 0.;
          #pragma omp parallel for reduction(+: local_err)   
          for(int i=0;i<nlocal;i++) {
            local_err += fabs(f2[i] - f1[i]) / ( fabs(f1[i]) + fabs(f2[i]) + atol );
          }
        
          // Average over all processors
          BoutReal err;
          if(MPI_Allreduce(&local_err, &err, 1, MPI_DOUBLE, MPI_SUM, BoutComm::get())) {
            throw BoutException("MPI_Allreduce failed");
          }

          err /= (BoutReal) neq;
        
          internal_steps++;
          if(internal_steps > mxstep)
            throw BoutException("ERROR: MXSTEP exceeded. timestep = %e, err=%e\n", timestep, err);

          if((err > rtol) || (err < 0.1*rtol)) {
            // Need to change timestep. Error ~ dt^5
            timestep /= pow(err / (0.5*rtol), 0.2);
            
            if((max_timestep > 0) && (timestep > max_timestep))
              timestep = max_timestep;
          }
          if(err < rtol) {
            break; // Acceptable accuracy
          }
        }else {
          // No adaptive timestepping
          take_step(simtime, dt, f0, f2);
          break;
        }
      }while(true);
      
      // Taken a step, swap buffers
      swap(f2, f0);
      simtime += dt;
      
      call_timestep_monitors(simtime, dt);
    }while(running);
    
    load_vars(f0); // Put result into variables

    iteration++; // Advance iteration number
    
    /// Call the monitor function
    
    if(call_monitors(simtime, s, nsteps)) {
      break; // Stop simulation
    }
    
    // Reset iteration and wall-time count
    rhs_ncalls = 0;
  }
  
  msg_stack.pop(msg_point);
  
  return 0;
}

void RKGenericSolver::take_step(BoutReal curtime, BoutReal dt, BoutReal *start, BoutReal *result) {
}
