__kernel
void mat_mul(__const int taille, __global int *A, __global int *B, __global int *C, __local int * ligne){
    int i = get_global_id(0);
    int j = get_global_id(1);
    int id = get_local_id(1);
    if(id == 0){
        for(int k = 0; k<taille; k++){
            ligne[k] = A[i*taille+k];
        }
    }
    barrier(CLK_LOCAL_MEM_FENCE);
    int acc =0;
    for(int k=0;k<taille;k++){
        acc+= ligne[k]*B[k*taille+j];
    }
    C[i*taille+j]=acc;
    //printf("%d",R[i]);
}