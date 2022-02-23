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


    int div = n/nslaves;
    int reste = n%nslaves;

    int *sendcounts = new int[nslaves];
    int *displ = new int[nslaves];

    for (int i=0; i <nslaves; i++)
        sendcounts[i]=div;

    int j = 0;
    while (reste!=0) {
        sendcounts[j] += 1;
        reste--;
        j++;
    }

    displ[0]=0;
    for (int i = 1; i < nslaves; i++)
        displ[i]=displ[i-1]+sendcounts[i-1];


    for (int i = 0; i < nslaves; ++i)
        MPI_Ssend(&tab[displ[i]], sendcounts[i], MPI_INT, i, 10, intercom);


    MPI_Status status;
    int* tab_result = new int[nslaves];
    for (int i = 0; i < nslaves; ++i) {
        MPI_Recv(&tab_result[i],1,MPI_INT,i,15,intercom,&status);
    }

    for (int i=0; i< nslaves; i++)
        cout << tab_result[i] << " ";
    cout << endl;

    int mini = tab_result[0];
    for (int i = 1; i < nslaves; ++i) {
        if (tab_result[i] < mini){
            mini = tab_result[i];
        }
    }

    cout << "Mini de tous les tableaux sur le maître : " << mini << endl;

  MPI_Comm_free(&intercom);
  MPI_Finalize();
  return 0;
}
