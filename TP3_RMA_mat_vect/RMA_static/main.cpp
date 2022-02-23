#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>
#include <omp.h>
#include <math.h>

#include "fonctions.h"

using namespace std;

int main(int argc, char **argv) {

    // Pour initialiser l'environnement MPI avec la possibilité d'utiliser des threads (OpenMP)
    int provided; // renvoi le mode d'initialisation effectué
    MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE,
                    &provided); // MPI_THREAD_MULITPLE chaque processus MPI peut faire appel à plusieurs threads.

    // Pour connaître son pid et le nombre de processus de l'exécution paralléle (sans les threads)
    int pid, nprocs;
    MPI_Comm_rank(MPI_COMM_WORLD, &pid);
    MPI_Comm_size(MPI_COMM_WORLD, &nprocs);

    int n = atoi(argv[1]); // taille de la matrice carrée
    int m = atoi(argv[2]); // nombre de vecteurs en entrée

    int root = atoi(argv[3]); // processeur root : référence pour les données

    string name = argv[4]; // le nom du fichier pour que le processus root copie les données initiales et les résultats


    // Petit test pour vérifier qu'on peut avoir plusieurs threads par processus.
#pragma omp parallel num_threads(4)
    {
        int id = omp_get_thread_num();
//#pragma omp critical
        //cout << "je suis le thread " << id << " pour pid=" << pid << endl;
    }

    // Pour mesurer le temps (géré par le processus root)
    chrono::time_point <chrono::system_clock> debut, fin;

    int *matrice = new int[n * n]; // la matrice
    int *vecteurs; // l'ensemble des vecteurs connu uniquement par root et distribué à tous.

    fstream f;
    if (pid == root) {
        f.open(name, std::fstream::out);
        srand(time(NULL));
        for (int i = 0; i < n * n; i++)
            matrice[i] = rand() % 20;

        f << "Matrice" << endl;
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++)
                f << matrice[i * n + j] << " ";
            f << endl;
        }
        f << endl;

        vecteurs = new int[m * n];
        for (int i = 0; i < m; i++) {
            int nb_zero = rand() % (n / 2);
            generation_vecteur(n, vecteurs + i * n, nb_zero);
        }
        f << "Les vecteurs" << endl;
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++)
                f << vecteurs[i * n + j] << " ";
            f << endl;
        }
    }

    if (pid == root){
        debut = chrono::system_clock::now();
    }

    // Déclaration de la fenêtre pour tous
    MPI_Win TheMat;
    MPI_Win TheVects;

    // Création de la fenêtre de la matrice et pour les vecteurs
    if (pid == root) {
        MPI_Win_create(matrice, n*n * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &TheMat);
        MPI_Win_create(vecteurs, m*n * sizeof(int), sizeof(int), MPI_INFO_NULL, MPI_COMM_WORLD, &TheVects);
    } else {
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &TheMat);
        MPI_Win_create(NULL, 0, 1, MPI_INFO_NULL, MPI_COMM_WORLD, &TheVects);
    }

    MPI_Win_fence(0,TheMat);
    MPI_Win_fence(0,TheVects);

    // transfert de la matrice à tous les autres processus
    if (pid != root) {
        MPI_Get(matrice, n*n, MPI_INT, root, 0, n*n , MPI_INT, TheMat);
    }

    MPI_Win_fence(0,TheMat);

    //Déclaration des variables nécessaires au Scatterv
    int *sendcounts = new int[nprocs];
    int *displ = new int[nprocs];

    //Calcul de combien d'éléments par processus
    int div = m/nprocs;
    int reste = m%nprocs;


    // Construction de sendcounts avec la div entière
    for (int i=0; i < nprocs; i++) {
        sendcounts[i]=div*n;
    }
    // Ajout du reste à sendcounts
    int j = 0;
    while (reste!=0) {
        sendcounts[j] += n;
        reste--;
        j++;
    }

    //Construction de displ
    displ[0]=0;
    for (int i = 1; i < nprocs; i++) {
        displ[i]=displ[i-1]+sendcounts[i-1];
    }

    //Dispatch de tous les vecteurs
    if (pid != root){
        vecteurs = new int[sendcounts[pid]];
        MPI_Get(vecteurs, sendcounts[pid], MPI_INT, root, displ[pid], sendcounts[pid], MPI_INT, TheVects);
    }

    MPI_Win_fence(0,TheVects);

    // calcul matrice-vecteur
    for (size_t i = 0; i < (sendcounts[pid]/n); ++i) {
        int result[n];
        matrice_vecteur(n, matrice, vecteurs + (i * n), result);
        // On copie le resultat à la place des données du vecteur pour faire le calcul
        memcpy(vecteurs + (i * n), result, n * sizeof(int));
    }

    MPI_Win_fence(0,TheVects);

    if (pid!=root){
        MPI_Put(vecteurs,sendcounts[pid],MPI_INT,0, displ[pid], sendcounts[pid], MPI_INT, TheVects);
    }

    MPI_Win_fence(0,TheVects);

//    if (pid==root){
//        // affichage de la répartition des vecteurs
//        for (size_t i = 0; i < m; ++i) {
//            cout << "PID: " << pid << "; Vector " << i <<" [ ";
//            for (size_t j = 0; j < n; ++j) cout << vecteurs[i * n + j] << " ";
//            cout << "]" << endl;
//        }
//    }

    MPI_Win_fence(0,TheMat);
    MPI_Win_fence(0,TheVects);

    // Dans le temps écoulé on ne s'occupe que de la partie communications et calculs
    // (on oublie la génération des données et l'écriture des résultats sur le fichier de sortie)
    if (pid == root) {
        fin = chrono::system_clock::now();
        chrono::duration<double> elapsed_seconds = fin - debut;
        cout << "temps en secondes : " << elapsed_seconds.count() << endl;
    }

    if (pid == root) {
        f << "Les vecteurs" << endl;
        for (int i = 0; i < m; i++) {
            for (int j = 0; j < n; j++)
                f << vecteurs[i * n + j] << " ";
            f << endl;
        }
        f.close();
    }

    MPI_Finalize();
    return 0;
}
