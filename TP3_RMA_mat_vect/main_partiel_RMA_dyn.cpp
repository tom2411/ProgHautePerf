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

    int x = atoi(argv[4]); // nombre de vecteur par processeur

    string name = argv[5]; // le nom du fichier pour que le processus root copie les données initiales et les résultats


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

    // Transfert de la matrice à tous les autres processus
    if (pid != root) {
        MPI_Get(matrice, n*n, MPI_INT, root, 0, n*n , MPI_INT, TheMat);
    }

    MPI_Win_fence(0,TheMat);
    MPI_Win_fence(0,TheVects);

    int nb_processeur_rempli= m/x;
    int reste_vecteur = m%x;

    int reste_boucle = m%(nprocs*x);

    int nb_boucle;

    if (reste_boucle > 0){
        nb_boucle = m/(nprocs*x)+1;
    }else{
        nb_boucle = m/(nprocs*x);
    }

    // Répartition des vecteurs pour chaque processeurs
    if ( pid!= root ) {
        int cpt = 0;
        for (int i = 0; i < nb_boucle; ++i) {
            if ((nb_processeur_rempli - 1) - cpt > 0) {
                vecteurs = new int[x * n];
                MPI_Get(vecteurs, x * n, MPI_INT, root, pid * (x * n), x * n, MPI_INT, TheVects);
            } else if (reste_vecteur % x < x && reste_vecteur != 0) {
                vecteurs = new int[reste_vecteur * n];
                MPI_Get(vecteurs, reste_vecteur * n, MPI_INT, root, pid * (x * n), reste_vecteur * n, MPI_INT,
                        TheVects);
                cpt++;
            }


        }
    }

    MPI_Win_fence(0,TheMat);
    MPI_Win_fence(0,TheVects);

    // Print received vectors
    if ( pid != 0 && reste_vecteur !=0 ){
        if (pid < nprocs-reste_vecteur) {
            for (size_t i = 0; i < x; ++i) {
                cout << "PID: " << pid << "; Vector " << i << " [ ";
                for (size_t j = 0; j < n; ++j) cout << vecteurs[i * n + j] << " ";
                cout << "]" << endl;
            }
        }else if (pid == nprocs-reste_vecteur){
            for (size_t i = 0; i < reste_vecteur; ++i) {
                cout << "PID: " << pid << "; Vector " << i <<" [ ";
                for (size_t j = 0; j < n; ++j) cout << vecteurs[i * n + j] << " ";
                cout << "]" << endl;
            }
        }
    }

    // calcul matrice-vecteur
    for (size_t i = 0; i < sizeof(vecteurs)/sizeof(int); ++i) {
        int result[n];
        matrice_vecteur(n, matrice, vecteurs + (i * n), result);
        // On copie le resultat à la place des données du vecteur pour faire le calcul
        memcpy(vecteurs + (i * n), result, n * sizeof(int));
    }

    MPI_Win_fence(0,TheMat);
    MPI_Win_fence(0,TheVects);

//    if (pid!=root){
//        if (pid <= nb_processeur_rempli-reste_vecteur){
//            MPI_Put(vecteurs,n,MPI_INT,0, pid*(x*n), x*n, MPI_INT, TheVects);
//        }else{
//            MPI_Put(vecteurs,n,MPI_INT,0, pid*(x*n), reste_vecteur*n, MPI_INT, TheVects);
//        }
//
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
