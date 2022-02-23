#include <iostream>
#include <mpi.h>
using namespace std;

int main(int argc, char**argv)
{
    int nprocs;
    int pid;

    MPI_Init (&argc,&argv);
    MPI_Comm_size(MPI_COMM_WORLD,&nprocs);
    MPI_Comm_rank(MPI_COMM_WORLD,&pid);

    int taille_tab = atoi(argv[1]);
    int root = atoi(argv[2]); // processeur root : référence pour les données
    int nb_elem_par_p = taille_tab / nprocs; // taille des tableaux de chacun des procs

    MPI_Win TheWin; // Déclaration de la fenêtre pour tous 
    int* tab = new int[taille_tab]; // Création d'un tableau de taille taille_tab

    if(pid == root){
        srand(time(NULL)+pid);
        int elem;
        for (size_t i = 0; i < taille_tab; i++)
        {
            elem = rand()%100;
            tab[i] = elem; // remplis avec des int de 0 à 100        
        }
        cout << "Root créer le tableau : " << endl;
        for (size_t i = 0; i < taille_tab; i++)
        {
            cout << tab[i] << endl;
        }
        MPI_Win_create(tab, taille_tab*sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD,&TheWin); // Le root pointe vers le tab
    }else{
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD,&TheWin); // Les autres vers rien
        
    }
    MPI_Win_fence(MPI_MODE_NOPRECEDE,TheWin); // On est sur que toutes les op sont faites (init etc)
    int* tab_sous_proc;
    if(pid != root){
        //Tous les autres que root
        tab_sous_proc = new int[nb_elem_par_p]; // Structure local qui va stocker sa partie du tableau
        MPI_Get(tab_sous_proc, nb_elem_par_p, MPI_INT, root, pid*nb_elem_par_p, nb_elem_par_p, MPI_INT, TheWin);
    }

    MPI_Win_fence(0, TheWin);

    if (pid!= root){
        MPI_Accumulate(tab_sous_proc , nb_elem_par_p, MPI_INT, root, 0, nb_elem_par_p, MPI_INT, MPI_SUM, TheWin);
    }
    MPI_Win_fence(MPI_MODE_NOSUCCEED, TheWin);

    if(pid == root){
        int* res = new int[nb_elem_par_p];
        for (size_t i = 0; i < nb_elem_par_p; i++)
        {
            res[i] = tab[i];
        }
        cout << "Fin du prog : tableau res sur root : " << endl;
        for (size_t i = 0; i < nb_elem_par_p; i++)
        {
            cout << res[i] << endl;
        }        
    }

    MPI_Win_free(&TheWin);
    MPI_Finalize();
    return 0;
}
