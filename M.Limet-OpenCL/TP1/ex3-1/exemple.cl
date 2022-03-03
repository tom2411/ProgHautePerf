__kernel
void nom_du_kernel(__const int taille, __global float * M1, __global float *M2){
    int i=get_global_id(0);
    int j=get_global_id(1);
    float lambda = 0.1f;

    float parti1 = (1 - 4 * lambda) * M1[i * taille + j];
    float gauche = (j - 1 < 0) ? M1[i * taille + (taille - 1)] : M1[i * taille + (j-1)];
    float droite = (j + 1 > taille) ? M1[i * taille] : M1[i * taille + (j+1)];
    float haut = (i - 1 < 0) ? M1[(taille - 1) * taille + j] : M1[(i-1) * taille + j];
    float bas = (i + 1 > taille) ? M1[j] : M1[(i+1) * taille + j];
    float parti2 = lambda * (haut + bas + gauche + droite);
    float acc = parti1 + parti2;

    M2[i * taille + j] = parti1 + parti2;
}