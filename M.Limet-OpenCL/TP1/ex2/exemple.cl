__kernel
void nom_du_kernel(const int taille, __global float * M1, __global float *M2, __global float *R){
   int i=get_global_id(0);
   int j=get_global_id(1);
   float acc=0;
   for (int k =0; k< taille; ++k){
    acc+= M1[i*taille+k]*M2[k*taille+j];
    R[i*taille+j]=acc;
   }


}
