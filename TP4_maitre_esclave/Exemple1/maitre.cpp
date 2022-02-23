#include <mpi.h>
#include <iostream>
using namespace std;
int main(int argc, char**argv){
  int nmasters, pid;
  MPI_Comm intercom;
  MPI_Init (&argc,&argv);
  MPI_Comm_size(MPI_COMM_WORLD,&nmasters);
  MPI_Comm_rank(MPI_COMM_WORLD,&pid);

  // le code des maîtres

  int n = atoi(argv[1]);
  int nslaves = atoi(argv[2]);
  int root = atoi(argv[3]);

  cout << "je suis le maître " << pid << " parmi " << nmasters << " maîtres" << endl;
  int* tab = new int[n];

  srand(time(NULL));
  for (int i=0; i<n; i++)
      tab[i] = pid+i;

  MPI_Comm_spawn("esclave", // exécutable
                     argv, // arguments à passer sur la ligne de commande
                     nslaves, // nombre de processus esclave
                     MPI_INFO_NULL, //permet de préciser où et comment lancer les processus
                     root, // rang du processus maître qui effectue réellement le spawn
                     MPI_COMM_WORLD, // Monde dans lequel est effectué le spwan
                     &intercom, // l'intercommunicateur permettant de communiquer avec les esclaves
                     MPI_ERRCODES_IGNORE // tableau contenant les erreurs
      );


  if (pid==root) {
      for (int i = 0; i < nslaves; i++)
          MPI_Ssend(tab, n, MPI_INT, i, 10, intercom);
  }
  MPI_Comm_free(&intercom);
  MPI_Finalize();
  return 0;
}
