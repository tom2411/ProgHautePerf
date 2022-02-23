#include <iostream>
#include <mpi.h>
using namespace std;
int main(int argc, char**argv) {

    int nprocs;
    int pid;

    MPI_Init (&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&pid);

    MPI_Win TheWin; // Déclaration de la fenêtre

    int root = 0;
    // tableau intermédiaire
    int taille_tab = nprocs*3;
    int *tab;

    // Création du tableau rempli aléatoirement sur root
    if (pid == root){
        srand(time(NULL)+pid);
        tab = new int[taille_tab];
        for (int i=0; i<taille_tab; i++){
            tab[i] = rand()%100;
        }
        cout << "je suis " << pid << " tab avant:";
        for (int i=0; i < taille_tab; i++)
            cout << tab[i] << " ";
        cout << endl;
        MPI_Win_create(tab, taille_tab*sizeof(int), sizeof(int),MPI_INFO_NULL, MPI_COMM_WORLD,&TheWin);
    }else{
        MPI_Win_create(NULL, 0, 1,MPI_INFO_NULL, MPI_COMM_WORLD,&TheWin);
    }

    MPI_Win_fence(0,TheWin);

    if (pid != root){
        int* tab_inter = new int[3];
        MPI_Get(tab_inter, 3, MPI_INT, root, pid*3, 3, MPI_INT, TheWin); // On va chercher la donnée dans la fenêtre distante pour la copier dans tab_inter
        cout << "je suis " << pid << " tab_inter " << endl;
        for (int i=0; i < 3; i++)
            cout << tab_inter[i] << " ";
        cout << endl;
        MPI_Accumulate(tab_inter, 3, MPI_INT, root, 0, 3, MPI_INT, MPI_SUM, TheWin);
    }
    // Barrière de fin sans assertion (on peut en donner pour indiquer des règles sur les instructions entre les 2 barrières)
    MPI_Win_fence(0,TheWin);
    if (pid == root){
        cout << "je suis " << pid << " tab après: ";
        for (int i=0; i<taille_tab; i++)
            cout << tab[i] << " ";
        cout << endl;
    }

    // Libération de la fenêtre.
    MPI_Win_free(&TheWin);
    MPI_Finalize();

    return 0;
}
