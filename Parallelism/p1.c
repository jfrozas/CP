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
        
        for(i = 1; i < numprocs; i++)
            MPI_Send(&n, 1, MPI_INT, i, 0, MPI_COMM_WORLD);
    } else {
        MPI_Recv(&n, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
    } 

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
    
    pi = ((double) count/(double) n)*4;
    printf("proccess %d estimates that pi is: %f \n", rank, pi);

    if (rank != 0) {
        MPI_Send(&pi, 1, MPI_DOUBLE, 0, 1, MPI_COMM_WORLD);
    } else {
        for (int i = 1; i < numprocs; ++i) {
            MPI_Recv(&piAux, 1, MPI_DOUBLE, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            pi += piAux;
        }
        printf("pi is approx. %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
    }
    
    MPI_Finalize();
}
