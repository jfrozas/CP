#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

int main(int argc, char *argv[])
{
    int i;
    int n, count;
    int numprocs, rank;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z, piAux;

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == 0) {
        printf("Enter the number of points: (0 breaks) ");
        scanf("%d",&n);
    }

    MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    
    if (n == 0) 
        return 0;

    count = 0;  

    for (i = rank; i < n; i+=numprocs) {
        srand(i); 
        x = ((double) rand()) / ((double) RAND_MAX);
        y = ((double) rand()) / ((double) RAND_MAX);
        
        z = sqrt((x*x)+(y*y));
        if(z <= 1.0)
            count++;
    }
    
    pi = ((double) count/(double) n)*4.0;
    printf("proccess %d estimates that pi is: %f \n", rank, pi);

    MPI_Reduce(&pi, &piAux, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0)
        printf("pi is approx. %.16f, Error is %.16f\n", piAux, fabs(piAux - PI25DT));
        
    MPI_Finalize();
}
