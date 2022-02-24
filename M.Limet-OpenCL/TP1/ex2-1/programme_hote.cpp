#define __CL_ENABLE_EXCEPTIONS

#include "CL/cl.hpp"
#include <omp.h>

#include <vector>
#include <iostream>
#include <fstream>
#include <vector>
#include <chrono>
// pour la génération aléatoire
#include <cstdlib>
#include <ctime>

// compilation
// g++ programme_hote.cpp -O3 -lOpenCL -o programme_hote -fopenmp && ./programme_hote

// variables globales
// taille des données (soit le vecteur ou le coté d'une matrice carrée)
const int taille=4096;
// taille des données en octets Attention au type des données)
size_t nboctets=sizeof(float)*taille*taille;
// pointeurs vers le stockage des données en mémoire centrale
float *A;
float *B;
float *C;


// fonction permettant de récupérer le temps écoulé entre debut et fin
double temps(std::chrono::time_point<std::chrono::system_clock> debut, std::chrono::time_point<std::chrono::system_clock> fin){
    std::chrono::duration<double> tps=fin-debut;
    return tps.count();
}

// initialisation d'un vecteur à une valeur aléatoire entre min et max 
//void init_vec(int *vec,int taille, int min, int max){
//  if (min==max)
//    for (int i=0;i<taille;vec[i++]=min);
//  else{
//    int interval=max-min+1;
//    for (int i=0;i<taille;vec[i++]=min+rand()%interval);
//  }
//}

// initialisation d'une matrice carré avec des valeur aléatoire entre min et max
void init_mat(int *mat, int taille, int min, int max){
    if (min==max) {
        for (int i = 0; i < taille; ++i) {
            for (int j = 0; j < taille; ++j) {
                mat[i * taille + j] = min;
            }
        }
    }else{
        int interval=max-min+1;
        for (int i = 0; i < taille; ++i) {
            for (int j = 0; j < taille; ++j) {
                mat[i * taille + j] = min+rand()%interval;
            }
        }
    }
}

// initialisation d'un vecteur à une valeur aléatoire entre min et max
//void init_vec(float *vec,int taille, float min, float max){
//  if (min==max)
//    for (int i=0;i<taille;vec[i++]=min);
//  else{
//    int interval=max-min+1;
//    for (int i=0;i<taille;i++){
//      float val = min + static_cast <float> (rand()) /( static_cast <float> (RAND_MAX/(max-min)));
//      vec[i]=val;
//    }
//  }
//}

void init_mat( float *mat, int taille, float min, float max){
    if (min==max) {
        for (int i = 0; i < taille; ++i) {
            for (int j = 0; j < taille; ++j) {
                mat[i * taille + j] = min;
            }
        }
    }else{
        for (int i = 0; i < taille; ++i) {
            for (int j = 0; j < taille; ++j) {
                int random = rand();
                //float val = 2;
                float val = min + static_cast<float>(random) /(static_cast<float>(RAND_MAX/(max-min)));
                mat[i * taille + j] = val;
            }
        }
    }
}

//void affiche_vec(int * vec, int taille, int nb_col=-1){
//  if (nb_col==-1) nb_col=taille;
//  for (int i =0;i<taille; i++){
//    if (i%nb_col==0) std::cout<<std::endl;
//    std::cout<<vec[i]<<" ";
//  }
//  std::cout<<std::endl;
//}


void affiche_mat(int * mat, int taille, int nb_col=-1){
    if (nb_col==-1) nb_col=taille;
    for (int i = 0; i < taille ; i++){
        for (int j = 0; j < taille; ++j) {
            if (j%nb_col==0) std::cout<<std::endl;
                std::cout<<mat[j]<<" ";
        }
        std::cout<<std::endl;
    }
}


//void affiche_vec(float * vec, int taille, int nb_col=-1){
//  if (nb_col==-1) nb_col=taille;
//  for (int i =0;i<taille; i++){
//    if (i%nb_col==0) std::cout<<std::endl;
//    std::cout<<std::fixed<<vec[i]<<" ";
//
//  }
//  std::cout<<std::endl;
//}

void affiche_mat(float * mat, int taille, int nb_col=-1) {
    if (nb_col==-1) nb_col=taille;
    for (int i = 0; i < taille ; i++){
        for (int j = 0; j < taille; ++j) {
            if (j%nb_col==0) std::cout<<std::endl;
            std::cout<<std::fixed<<mat[i*taille+j]<<" ";
        }
    }
    std::cout<<std::endl;
}

void affiche_device(cl::Device device){
    std::cout << "\t\tDevice Name: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
    std::cout << "\t\tDevice Type: " << device.getInfo<CL_DEVICE_TYPE>();
    std::cout << " (GPU: " << CL_DEVICE_TYPE_GPU << ", CPU: " << CL_DEVICE_TYPE_CPU << ")" << std::endl;
    std::cout << "\t\tDevice Vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
    std::cout << "\t\tDevice Max Compute Units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
    std::cout << "\t\tDevice Global Memory: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl;
    std::cout << "\t\tDevice Max Clock Frequency: " << device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << std::endl;
    std::cout << "\t\tDevice Max Allocateable Memory: " << device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << std::endl;
    std::cout << "\t\tDevice Local Memory: " << device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl;
    std::cout << "\t\tDevice Available: " << device.getInfo<CL_DEVICE_AVAILABLE>() << std::endl;
    std::cout << "\t\tMax Work-group Total Size: " << device.getInfo<CL_DEVICE_MAX_WORK_GROUP_SIZE>() << std::endl;
    std::vector<size_t> d= device.getInfo<CL_DEVICE_MAX_WORK_ITEM_SIZES>();
    std::cout << "\t\tMax Work-group Dims: (";
    for (std::vector<size_t>::iterator st = d.begin(); st != d.end(); st++)
        std::cout << *st << " ";
    std::cout << "\x08)" << std::endl;
}


cl::Program creationProgramme(std::string nomFicSource, cl::Context contexte){
    // lecture du programme source
    std::ifstream sourceFile(nomFicSource);
    std::string sourceCode(std::istreambuf_iterator <char>(sourceFile),(std::istreambuf_iterator < char >()));
    // la premier argument indique le nombre de programmes sources utilisés, le deuxième est une paire (texte, taille du programme)
    cl::Program::Sources source(1,std::make_pair(sourceCode.c_str(),sourceCode.length()+1));
    // creation du programme dans le contexte
    return cl::Program(contexte,source);
}


void test_CPU(){
    std::chrono::time_point<std::chrono::system_clock> debut=std::chrono::system_clock::now();
#pragma omp parallel for num_threads(4) collapse(2)
        for (int i = 0; i < taille; i++) {
            for (int j = 0; j < taille; ++j) {
                for (int k = 0; k < taille; ++k) {
                    C[i * taille + j] = A[i * taille + k] + B[k * taille + j];
                }
            }
        }
    std::chrono::time_point<std::chrono::system_clock> fin=std::chrono::system_clock::now();
    std::cout<<"temps execution "<<temps(debut,fin)<<std::endl;
    //std::cout<<"Résultat CPU"<<std::endl;
    //affiche_vec(C,taille);
}
void test_GPU(cl::Program programme, cl::CommandQueue queue, cl::Context contexte){
    std::chrono::time_point<std::chrono::system_clock> debut=std::chrono::system_clock::now();
    // Création des buffers de données dans le contexte
    cl::Buffer bufferA = cl::Buffer(contexte, CL_MEM_READ_ONLY, nboctets);
    cl::Buffer bufferB = cl::Buffer(contexte, CL_MEM_READ_ONLY, nboctets);
    cl::Buffer bufferC = cl::Buffer(contexte, CL_MEM_WRITE_ONLY, nboctets);

    // Chargement des données en mémoire video
    queue.enqueueWriteBuffer(bufferA , CL_TRUE, 0, nboctets , A);
    queue.enqueueWriteBuffer(bufferB , CL_TRUE, 0, nboctets , B);
    // creation du kernel (fonction à exécuter)
    cl::Kernel kernel(programme,"nom_du_kernel");
    // Attribution des paramètres de ce kernel
    kernel.setArg(0,taille);
    kernel.setArg(1,bufferA);
    kernel.setArg(2,bufferB);
    kernel.setArg(3,bufferC);

    // création de la topologie des processeurs
    // le local ne peut être plus grand que le global
    cl::NDRange global(taille,taille); // nombre total d'éléments de calcul -processing elements
    cl::NDRange local(16,16); // dimension des unités de calcul -compute units- c'à-dire le nombre d'éléments de calcul par unités de calcul

    // lancement du programme en GPU
    queue.enqueueNDRangeKernel(kernel,cl::NullRange,global,local);

    // recupération du résultat
    queue.enqueueReadBuffer(bufferC,CL_TRUE,0,nboctets,C);
    std::chrono::time_point<std::chrono::system_clock> fin=std::chrono::system_clock::now();

    std::cout<<"temps execution "<<temps(debut,fin)<<std::endl;
    //  std::cout<<"Résultat GPU"<<std::endl;
    //affiche_vec(C,taille);
}


int main(){
    // pour mesurer le temps
    std::chrono::time_point<std::chrono::system_clock> debut,debut2,fin;
    // initialisation de générateur aléatoire
    srand (time(NULL));


    // création des zone de stockage de données en mémoire centrale
    A= new float[taille*taille];
    B= new float[taille*taille];
    C= new float[taille*taille];



    try{ // debut de la zone d'utilisation de l'API pour OpenCL
        // les plateformes
        std::vector <cl::Platform> plateformes;
        cl::Platform::get(&plateformes); // recherche des plateformes normalement 1 sur un PC

        //les devices
        std::vector <cl::Device> devices;
        plateformes[0].getDevices(CL_DEVICE_TYPE_ALL,&devices); // recherche des devices (normalement 1)

        // affichage des informations des devices
        for (int i=0;i<devices.size();i++){
            std::cout << "\tDevice " << i << ": " << std::endl;
            affiche_device(devices[i]);
        }

        // création d'un contexte pour les devices
        cl::Context contexte(devices);

        // création du programme dans le contexte (voir code fonction)
        cl::Program programme=creationProgramme("exemple.cl",contexte);
        // compilation du programme
        try{
            programme.build(devices);
        } catch (...) {
            // Récupération des messages d'erreur au cas où...
            cl_int buildErr = CL_SUCCESS;
            auto buildInfo = programme.getBuildInfo<CL_PROGRAM_BUILD_LOG>(devices[0],&buildErr);
            std::cerr << buildInfo << std::endl << std::endl;
            exit(0);
        }

        // création de la file de commandes (ordres de l'hote pour le GPU)
        cl::CommandQueue queue= cl::CommandQueue(contexte,devices[0]);

        // initialisation des données sur l'hote
        init_mat(A,taille,-10,10);
        init_mat(B,taille,-10,10);
        // affichage des données initialisées
        std::cout<<" Données initialisées"<<std::endl;
        //affiche_mat(A,taille);
        //affiche_mat(B,taille);

        test_CPU();
        //test_GPU(programme,queue,contexte);
        //affiche_mat(C,taille);

    } catch (cl::Error err) { // Affichage des erreurs en cas de pb OpenCL
        std::cout << "Exception\n";
        std::cerr << "ERROR: " << err.what() << "(" << err.err() << ")" << std::endl;
        return EXIT_FAILURE;
    }
}
