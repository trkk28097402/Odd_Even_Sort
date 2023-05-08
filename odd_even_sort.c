#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int cmp(const void* a, const void *b){
    return *((float *)a) > *((float *)b);
}

void merge_up(float *local, float *recv, float *tmp, int local_n, int recv_n){
    int a_id = 0, b_id = 0, j = 0, k = 0;
    while(a_id < local_n && b_id < recv_n && j < local_n){
        if(local[a_id] < recv[b_id]) tmp[j++] = local[a_id++];  
        else tmp[j++] = recv[b_id++]; 
    }
    while(j < local_n){
        printf("a %d b %d j %d\n",a_id,b_id,j);
        if(b_id < recv_n) tmp[j++] = recv[b_id++];
        else tmp[j++] = local[a_id++];  
    }
    for(; k < local_n; k++) local[k] = tmp[k];    
}

void merge_down(float *local, float *recv, float *tmp, int local_n, int recv_n){
    int a_id = local_n - 1, b_id = recv_n - 1, j = 0, k = 0;
    while(a_id >= 0 && b_id >= 0 && j < local_n){
        if(local[a_id] > recv[b_id]) tmp[j++] = local[a_id--];  
        else tmp[j++] = recv[b_id--]; 
    }
    while(j < local_n){
        if(b_id >= 0) tmp[j++] = recv[b_id--];
        else tmp[j++] = local[a_id--];  
    }
    for(; k < local_n; k++) local[k] = tmp[local_n - k - 1];    
}

int main(int argc, char** argv){
    
    int my_rank, comm_size;
    int i;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);

    int n = atoi(argv[1]);
    char *in_name = argv[2];
    char *out_name = argv[3];

    MPI_File in_file, out_file;
    int *count = (int *)malloc(sizeof(int) * comm_size);
    int *display = (int *)malloc(sizeof(int) * comm_size);
    int base_data_n = n / comm_size;
    int left = n % comm_size;

    for(i = 0; i < comm_size; i++){
        count[i] = base_data_n;
        if(left > i) count[i]+=1;
    }

    display[0] = 0;
    for(i = 1; i < comm_size; i++) display[i] = display[i - 1] + count[i - 1];

    float *local_data = (float *)malloc(sizeof(float) * count[my_rank]);

    MPI_File_open(MPI_COMM_WORLD, in_name, MPI_MODE_RDONLY, MPI_INFO_NULL, &in_file);
    MPI_File_read_at(in_file, sizeof(float) * display[my_rank], local_data, count[my_rank], MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&in_file);

    qsort(local_data, count[my_rank], sizeof(float), cmp);
    MPI_Barrier(MPI_COMM_WORLD);
    
    float *recv_data = (float *)malloc(sizeof(float) * (base_data_n * 2 + 2));
    float *tmp = (float *)malloc(sizeof(float) * (base_data_n * 2 + 2));

    for(i = 0; i < comm_size; i++){
        if(i % 2 == 0){ //even phase
            if(my_rank % 2 == 0){
                if(my_rank < comm_size - 1){
                    MPI_Recv(recv_data, count[my_rank + 1], MPI_FLOAT, my_rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(local_data, count[my_rank], MPI_FLOAT, my_rank + 1, 0, MPI_COMM_WORLD);
                    merge_up(local_data, recv_data, tmp, count[my_rank], count[my_rank + 1]);
                }
            }
            else{
                if(my_rank > 0){
                    MPI_Send(local_data, count[my_rank], MPI_FLOAT, my_rank - 1, 0, MPI_COMM_WORLD);
                    MPI_Recv(recv_data, count[my_rank - 1], MPI_FLOAT, my_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    merge_down(local_data, recv_data, tmp, count[my_rank], count[my_rank - 1]);
                }
            }
        }
        else{ //odd phase
            if(my_rank % 2 == 0){
                if(my_rank > 0){
                    MPI_Send(local_data, count[my_rank], MPI_FLOAT, my_rank - 1, 0, MPI_COMM_WORLD);
                    MPI_Recv(recv_data, count[my_rank - 1], MPI_FLOAT, my_rank - 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    merge_down(local_data, recv_data, tmp, count[my_rank], count[my_rank - 1]);
                }
            }
            else{
                if(my_rank < comm_size - 1){
                    MPI_Recv(recv_data, count[my_rank + 1], MPI_FLOAT, my_rank + 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                    MPI_Send(local_data, count[my_rank], MPI_FLOAT, my_rank + 1, 0, MPI_COMM_WORLD);
                    merge_up(local_data, recv_data, tmp, count[my_rank], count[my_rank + 1]);                    
                }
            }

        }
    }    

    float *out = (float *)malloc(sizeof(float) * n);
    MPI_Gatherv(local_data, count[my_rank], MPI_FLOAT, out, count, display, MPI_FLOAT, 0, MPI_COMM_WORLD);

    MPI_File_delete(out_name, MPI_INFO_NULL);
    MPI_File_open(MPI_COMM_WORLD, out_name, MPI_MODE_CREATE | MPI_MODE_WRONLY, MPI_INFO_NULL, &out_file);
    if(my_rank == 0) MPI_File_write_at(out_file, 0, out, n, MPI_FLOAT, MPI_STATUS_IGNORE);
    MPI_File_close(&out_file); 
    MPI_Barrier(MPI_COMM_WORLD);

    free(out);
    free(local_data);
    free(recv_data);
    free(tmp);
    free(count);
    free(display);
    
    MPI_Finalize();
    return 0;
}