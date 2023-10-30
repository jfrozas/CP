#include <sys/types.h>
#include <openssl/md5.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

#define PASS_LEN 6
#define N_THREADS 8
#define BARRA 50
#define PPS "pps"


struct data{

    unsigned char *md5[MD5_DIGEST_LENGTH];
    int found;
    int npass;
    int progress;
    pthread_mutex_t *mutexprogress;
};


struct args{
    int thread_num;
    struct data *data;
};

struct thread_info{
    pthread_t id;       //id returned by thread_create
    struct args *args;  //pointer to the arguments
};

long ipow(long base, int exp)
{
    long res = 1;
    for (;;)
    {
        if (exp & 1)
            res *= base;
        exp >>= 1;
        if (!exp)
            break;
        base *= base;
    }

    return res;
}

long pass_to_long(char *str) {
    long res = 0;

    for(int i=0; i < PASS_LEN; i++)
        res = res * 26 + str[i]-'a';

    return res;
};

void long_to_pass(long n, unsigned char *str) {  // str should have size PASS_SIZE+1
    for(int i=PASS_LEN-1; i >= 0; i--) {
        str[i] = n % 26 + 'a';
        n /= 26;
    }
    str[PASS_LEN] = '\0';
}

int hex_value(char c) {
    if (c>='0' && c <='9')
        return c - '0';
    else if (c>= 'A' && c <='F')
        return c-'A'+10;
    else if (c>= 'a' && c <='f')
        return c-'a'+10;
    else return 0;
}


void hex_to_num(char *str, unsigned char *hex) {
    for(int i=0; i < MD5_DIGEST_LENGTH; i++)
        hex[i] = (hex_value(str[i*2]) << 4) + hex_value(str[i*2 + 1]);
}

void *break_pass(void *ptr) {
    struct args *args = ptr;
    int base,i ,j;
    unsigned char res[MD5_DIGEST_LENGTH];
    unsigned char *pass = malloc((PASS_LEN + 1) * sizeof(char));
    long bound = ipow(26, PASS_LEN);
    while(args->data->found != args->data->npass || args->data->progress < bound){

        pthread_mutex_lock(args->data->mutexprogress);
        base = args->data->progress;
        args->data->progress += 1000;
        pthread_mutex_unlock(args->data->mutexprogress);

        for(i = base; i<base+1000; i++) {

                long_to_pass(i, pass);

                MD5(pass, PASS_LEN, res);

                for(j=0; j<args->data->npass; j++){
                    if (0 == memcmp(res, args->data->md5[j], MD5_DIGEST_LENGTH)){
                        args->data->found++;
                        printf("\r Password %d: %s found at %1.0f%%%60s\n", j+1, pass,((double)(args->data->progress)/(double) (bound))*100, " ");


                    }
                }
        }
    }
    return NULL;;
}


void *progressbar(void *ptr){
    long ncomb = ipow(26, PASS_LEN);
    struct args *args = ptr;
    double tiempo1, tfinal;
    long porcentaje = 0;


    while(args->data->found != args ->data->npass && porcentaje <= 100){

        porcentaje =  ((double)(args->data->progress)/(double) (ncomb))*100;

        long numchars =  ((double)args->data->progress/(double)ncomb)*BARRA;

        printf("\r\033[0;32m");
        printf("[");

        for(int i= 0; i< numchars;i++ ){
            printf("█");
        }
        for(int i=0; i< BARRA-numchars; i++){
            printf(" ");
        }
        printf("]");
        printf(("\033[0m"));
        printf(" %.2f%%", (double)porcentaje);
        printf(" %.2f %s ",tfinal,PPS);

        tiempo1 = (double) args->data->progress;
        usleep(125000);
        tfinal = (((double)args->data->progress)-tiempo1)*8;

        if(args->data->found == args->data->npass){
            printf("\r\033[0;32m");
            printf("[");

            for(int i= 0; i< BARRA;i++ ){
                printf("█");
            }
            printf("] 100%%");
            printf(("\033[0m"));
        }

        fflush(stdout);

    }
    printf("\n");

    return NULL;
}


struct thread_info *start_threads_barra(struct data *data){
    struct thread_info *threads;

    threads = malloc(sizeof(struct thread_info));

    if (threads == NULL) {
        printf("Not enough memory\n");
        exit(1);
    }

    // Create num_thread threads running swap()
    threads->args = malloc(sizeof(struct args));

    threads->args -> thread_num = 100;
    threads->args ->data = data;

    if (0 != pthread_create(&threads->id, NULL, progressbar, threads->args)) {
        printf("Could not create thread");
        exit(1);
    }
    return threads;
}

struct thread_info *start_threads(struct data *data){
    struct thread_info *threads;
    int i;

    threads = malloc(sizeof(struct thread_info)*(N_THREADS));

    if (threads == NULL){
        exit(1);
    }
    for (i = 0; i < N_THREADS; i++) {
        threads[i].args = malloc(sizeof(struct args));

        threads[i].args -> thread_num = i;
        threads[i].args -> data = data;

        if (0 != pthread_create(&threads[i].id, NULL, break_pass, threads[i].args)) {
            printf("Could not create thread #%d", i);
            exit(1);
        }
    }
    return threads;


}

struct data *initdata( char *argv[],int argc){
    printf("a");
    struct data *data;
    data = malloc(sizeof (struct data));

    data->mutexprogress = malloc(sizeof (pthread_mutex_t));

    pthread_mutex_init(data->mutexprogress,NULL);


    for(int i = 0; i<argc-1; i++){

        data->md5[i] = malloc(sizeof (unsigned char)*(MD5_DIGEST_LENGTH));
        hex_to_num(argv[i+1], data->md5[i]);
    }

    data ->found = 0;
    data ->progress = 0;
    data ->npass = argc-1;

    return data;
}

void waitbarra(struct thread_info *threads){
    pthread_join(threads->id,NULL);


    free(threads->args);
    free(threads);
}

void waitthreads(struct thread_info *threads, struct data *data){
    for (int i = 0; i < N_THREADS; i++){
        pthread_join(threads[i].id, NULL);
    }

    for (int i = 0; i < N_THREADS; i++)
        free(threads[i].args);


    pthread_mutex_destroy(data->mutexprogress);


    free(data->mutexprogress);

    for(int i=0; i<data->npass;i++){
        free(data->md5[i]);
    }

    free(threads);

}

int main(int argc, char *argv[]) {
    struct thread_info *thrs;
    struct thread_info *thrs2;
    struct data *data;

    if(argc < 2) {
        printf("Use: %s string\n", argv[0]);
        exit(0);
    }


    data = initdata(argv,argc);

    thrs = start_threads_barra(data);
    thrs2 = start_threads(data);

    waitthreads(thrs2, data);
    waitbarra(thrs);

    return 0;
}