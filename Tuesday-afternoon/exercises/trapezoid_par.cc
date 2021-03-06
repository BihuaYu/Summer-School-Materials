#include <mpi.h>
#include <cstdio>
#include <iostream>
#include <cmath>

double g(double x) {
  return exp(-x*x)*cos(3*x);
}


double integrate(const int N, const double a, const double b, double (*f)(double)) {
  int nproc, rank;
  MPI_Comm_size(MPI_COMM_WORLD, &nproc);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  const double h = (b-a)/N;
  double sum=0.0;

  for (int i=1+rank; i<(N-1); i+=nproc) {
    sum += f(a + i*h);
  }
  sum += 0.5*(f(b) + f(a));

  double tmp;
  MPI_Allreduce(&sum, &tmp, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
  sum = tmp;
  return sum*h;
}

int main(int argc, char** argv) {
    MPI_Init(&argc, &argv);
    int nproc, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    const double gexact = std::sqrt(4.0*std::atan(1.0))*std::exp(-9.0/4.0);
    const double a=-6.0, b=6.0;

    double result_prev = integrate(1, a, b, g);
    if (rank == 0) {
        for (int N=2; N<=1024; N*=2) {
            MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD); 
            double result = integrate(N, a, b, g);
            
            double err_prev = std::abs(result-result_prev);
            double err_exact = std::abs(result-gexact);
            printf("N=%2d   result=%.10e   err-prev=%.2e   err-exact=%.2e\n",
                   N, result, err_prev, err_exact);
            
            bool converged = (err_prev < 1e-10 && N>4);
            MPI_Bcast(&converged, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
            if (converged) break;
            result_prev = result;
        }
    }
    else {
        bool converged = false;
        while (!converged) {
            int N;
            MPI_Bcast(&N, 1, MPI_INT, 0, MPI_COMM_WORLD);
            integrate(N, a, b, g);
            MPI_Bcast(&converged, 1, MPI_C_BOOL, 0, MPI_COMM_WORLD);
            if (converged) break;
        }
    }

    MPI_Finalize();
  return 0;
}
