#include <iostream>
#include <fstream>
#include <chrono>
#include <mpi.h>
#include <omp.h>

#include "fonctions.h"

using namespace std;

int main(int argc, char **argv) {

        // Pour initialiser l'environnement MPI avec la possibilité d'utiliser des threads (OpenMP)
        int provided; // renvoi le mode d'initialisation effectué
        MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &provided); // MPI_THREAD_MULITPLE chaque processus MPI peut faire appel à plusieurs threads.

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
#pragma omp critical
                cout << "je suis le thread " << id << " pour pid=" << pid << endl;
        }


        // Pour mesurer le temps (géré par le processus root)
        chrono::time_point<chrono::system_clock> debut, fin;


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

        if (pid == root)
                debut = chrono::system_clock::now();

        // Transfert de la matrice à tous les processus
        MPI_Bcast(matrice, n*n, MPI_INT, root, MPI_COMM_WORLD);

        // Déclaration des paramètres pour le Scatterv pour tous les processus
        int *sendcounts = new int[nprocs];
        int *displ = new int[nprocs];
        int recvcount;

        //Calcul de combien d'éléments par processus
        int div = m/nprocs;
        int reste = m%nprocs;

        // Construction de sendcounts avec la div entière
        for (int i=0; i <nprocs; i++)
                sendcounts[i]=div*n;

        // Ajout du reste à sendcounts
        int j = 0;
        while (reste!=0) {
                sendcounts[j] += n;
                reste--;
                j++;
        }

        //Construction de displ
        displ[0]=0;
        for (int i = 1; i < nprocs; i++)
                displ[i]=displ[i-1]+sendcounts[i-1];

        // Initialisation des tableaux de réception pour tous les processus
        recvcount = sendcounts[pid];
        int *recvbuf = new int[sendcounts[pid]];

        // Dispatch de tous les vecteurs sur les processus
        MPI_Scatterv(vecteurs, sendcounts, displ, MPI_INT, recvbuf, recvcount, MPI_INT, 0, MPI_COMM_WORLD );

        // affichage de la répartition des vecteurs
        for (size_t i = 0; i < (sendcounts[pid] / n); ++i) {
                cout << "PID: " << pid << "; Vector " << i <<" [ ";
                for (size_t j = 0; j < n; ++j) cout << recvbuf[i * n + j] << " ";
                cout << "]" << endl;
        }

        // Calcul du matrice vecteur
        #pragma omp parallel for
        for (size_t i = 0; i < (sendcounts[pid]/n); ++i) {
                int result[n];
                matrice_vecteur(n, matrice, recvbuf + (i * n), result);
                // On copie le resultat à la place des données du vecteur pour faire le calcul
                memcpy(recvbuf + (i * n), result, n * sizeof(int));
        }

        // Récupération des résultats dans vecteur sur root
        MPI_Gatherv(recvbuf, sendcounts[pid], MPI_INT, vecteurs, sendcounts, displ, MPI_INT, root, MPI_COMM_WORLD);

        // Dans le temps écoulé on ne s'occupe que de la partie communications et calculs
        // (on oublie la génération des données et l'écriture des résultats sur le fichier de sortie)
        if (pid == root) {
                fin = chrono::system_clock::now();
                chrono::duration<double> elapsed_seconds = fin - debut;
                cout << "temps en secondes : " << elapsed_seconds.count() << endl;
        }

        // affichage des résultats du calcul matrice-vecteur
        if (pid == root) {
                f << "Les vecteurs résultats" << endl;
                for (int i = 0; i < m; i++) {
                        for (int j = 0; j < n; j++)
                                f << vecteurs[i * n + j] << " ";
                        f << endl;
                }
                f.close();
        }

        MPI_Finalize();
        delete[] matrice;
        delete[] recvbuf;
        if (pid == 0) delete[] vecteurs;
        return 0;
}
