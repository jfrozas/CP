#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <sys/time.h>

#define DEBUG 1

#define N 1000

int main(int argc, char *argv[] ) {

  int i, j, sum=0;
  float matrix[N][N];
  float vector[N];
  float result[N], resultAux[N];

  int numprocs, rank;
  int *displs;
  float rec_buf[N*N];
  int *rows;

  struct timeval  tvcomp1, tvcomp2, tvscatter1, tvscatter2, tvgather1, tvgather2;
  int comm_time, comp_time;

  MPI_Status status;

  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  rows = malloc(sizeof(int)*N);
  displs = malloc(sizeof(int)*N);

  if(numprocs>N){
    if (rank == 0){
      printf("ERROR: Execute the program with less than %d numprocs\n", N+1);
    }
    MPI_Finalize();
    free(rows);
    free(displs);
    return 0;
  }


  for(i=0; i<N; i++){
    result[i]=0;
  }

  float rem = N%numprocs;

  for (int i = 0; i < numprocs; i++) {
      rows[i] = N/numprocs;
      if (rem > 0) {
          rows[i]++;
          rem--;
      }

      displs[i] = sum;
      sum += rows[i]*N;
      rows[i] = rows[i]*N;
  } 

  if (rank == 0){

    for(i=0;i<N;i++) {
      vector[i] = i;
      for(j=0;j<N;j++) {
        matrix[i][j] = i+j;
      } 
    }
  }
  gettimeofday(&tvscatter1, NULL);

  MPI_Scatterv(&matrix, rows, displs, MPI_FLOAT, &rec_buf, N*rows[rank], MPI_FLOAT, 0, MPI_COMM_WORLD);
  MPI_Bcast(vector, N, MPI_FLOAT, 0, MPI_COMM_WORLD);

  gettimeofday(&tvscatter2, NULL);
  gettimeofday(&tvcomp1, NULL);

  for (int i = 0; i < (rows[rank]/N); i++){
    for(j=i*N; j<(i+1)*N; j++){
      result[i] += rec_buf[j] * vector[j-(i*N)];
    }
  }

  gettimeofday(&tvcomp2, NULL);
  
  int microseconds_scatter = (tvscatter2.tv_usec - tvscatter1.tv_usec)+ 1000000 * (tvscatter2.tv_sec - tvscatter1.tv_sec);
  int microseconds_comp = (tvcomp2.tv_usec - tvcomp1.tv_usec)+ 1000000 * (tvcomp2.tv_sec - tvcomp2.tv_sec);  

  for(i = 0; i < numprocs; i++){
    rows[i] = rows[i] / N;
    displs[i] = displs [i] / N;
  }

  gettimeofday(&tvgather1, NULL);

  MPI_Gatherv(&result, rows[rank], MPI_FLOAT, resultAux, rows, displs, MPI_FLOAT, 0, MPI_COMM_WORLD);
  MPI_Barrier(MPI_COMM_WORLD);

  gettimeofday(&tvgather2, NULL);

  int microseconds_gather = (tvgather2.tv_usec - tvgather1.tv_usec)+ 1000000 * (tvgather2.tv_sec - tvgather1.tv_sec);
  int microseconds_comm = microseconds_scatter + microseconds_gather; 
  

  if (rank != 0) {
        MPI_Send(&microseconds_comm, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
        MPI_Send(&microseconds_comp, 1, MPI_INT, 0, 1, MPI_COMM_WORLD);
  } else {
        for (int i = 1; i < numprocs; ++i) {
            MPI_Recv(&comm_time, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            MPI_Recv(&comp_time, 1, MPI_INT, i, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            printf("Comm Time (seconds) [%d] = %lf\n", i, (double) comm_time/1E6);
            printf("Comp Time (seconds) [%d] = %lf\n", i, (double) comp_time/1E6);
        }
        printf("Comm Time (seconds) [0] = %lf\n", (double) microseconds_comm/1E6);
        printf("Comp Time (seconds) [0] = %lf\n", (double) microseconds_comp/1E6);
  }     

  if(rank == 0){
    if (DEBUG){
      for(i=0;i<N;i++) {
        printf("ANSWER [%d]: %.f \n",i,resultAux[i]);
      }
    }
  }

  MPI_Finalize();
  free(rows);
  free(displs);

  return 0;
}