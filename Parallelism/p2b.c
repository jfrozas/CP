#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <mpi.h>
#include <time.h>

int MPI_FlattreeCollective(void *sendbuff, void *recbuff, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int err, i;
    int numprocs, rank, value;

    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank == root) {
        // Sets "value" to the initial one of "sendbuff".
        value = ((int*)sendbuff)[0];  
        
        // Loop for all the number of processes.
        for (i = 0; i < numprocs; i++) {   
            // Make sure the root doesn't receive from self.
            if (i != root) {  
                err = MPI_Recv(sendbuff, 1, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
                if (err != MPI_SUCCESS) 
                    return err;
                value += ((int*)sendbuff)[0];  // Keep adding the received values.
            }
        }
    } else { 
        // Every additional process send its count value to the root.
        err = MPI_Send(sendbuff, 1, MPI_INT, root, rank, MPI_COMM_WORLD);
        if (err != MPI_SUCCESS) 
            return err;
    }
       
    // Pass the total value to "recbuff".
    ((int*)recbuff)[0] = value;   
    
    return MPI_SUCCESS;
}


int ipow(int base, int exp)
{
    int res = 1;
    for (;;) {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return res;
}

int MPI_BinomialBcast(void *buf, int count, MPI_Datatype datatype, int root, MPI_Comm comm)
{
    int err, i;
    int numprocs, rank;
    int rec, send;

    MPI_Status status;

    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    // Repeats for as many floors as the tree has.
    for (i = 1; i <= ceil(log2(numprocs)); i++)
        // Process < 2^(k-1).
        if (rank < ipow(2, i-1)) {
            // Chooses the receiver (the next following 2^(k-1)) and send to root if it is less than the number of processes.
            rec = rank + ipow(2, i-1);
            if (rec < numprocs) {
                printf("process %d sends to process %d\n", rank,rec );  //print the process that send/receive
                err = MPI_Send(buf, count, datatype, rec, 0, comm);
                if (err != MPI_SUCCESS)
                    return err;
            }
        } else {
            // The rank does not pass the max values of that "floor".
            if (rank < ipow(2, i)) {
                send = rank - ipow(2, i-1);
                err = MPI_Recv(buf, count, datatype, send, 0, comm, &status);
                if (err != MPI_SUCCESS) 
                    return err;
            }
        }
    
    return MPI_SUCCESS;
}

int main(int argc, char *argv[])
{
    int i;
    int n, count, countAux;
    int numprocs, rank;
    double PI25DT = 3.141592653589793238462643;
    double pi, x, y, z;

    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    if (rank == 0) {
        printf("Enter the number of points: (0 breaks) ");
        scanf("%d",&n);
    }

    //MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_BinomialBcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

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

    MPI_FlattreeCollective(&count, &countAux, 1, MPI_INT, 0, MPI_COMM_WORLD);
    //MPI_Reduce(&pi, &piAux, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    if (rank == 0){
        pi = ((double) countAux/(double) n)*4.0;
        printf("pi is approximately %.16f, Error is %.16f\n", pi, fabs(pi - PI25DT));
    }
        
    MPI_Finalize();
}
