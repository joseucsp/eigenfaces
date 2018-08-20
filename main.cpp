/**
*   @autor      : José Chávez    
*   @file       : main.cpp
*   @viersion   : v1.0
*   @purpose    : Parallel implementation of eigenfaces
*/

#include <stdio.h>
#include <string>
#include <omp.h>

#include "tools.hpp"
#include "eig.hpp"
#include "images.hpp"
#include "nn.hpp"

using namespace cv;

#define thread_count 8

void readImages(double **X, int folders, int nimgs, int rows, int cols);
void getS(double **S, double **X, double **Xm, double **Xm_t, double *V, int X_row, int X_cols);
void getVD(double **S, double *lambda, double **eigV, double tol1, double tol2);
void showImage(double **I, int j, int w, int h);

int main(int argc, char *argv[]){
    
    int             i,j,k;
    int             nfiles = 40;            //  # folders
    int             file_imgs  = 10;        //  # imgs x folder

//  Matrix X, Xm, Xm_t
    double          **X;
    double          **Xm;
    double          **Xm_t;
    double          **S;
    double          **eigenVec;
    double          *eigenVal;
    double          *muV;
    int             X_rows=nfiles*file_imgs;
    
    int             imgw    = 92;
    int             imgh    = 112;
    int             X_cols  = imgh*imgw;    //  h = 112, w = 92
    
    X           = new double*[X_rows];
    Xm          = new double*[X_rows];
    Xm_t        = new double*[X_cols];
    S           = new double*[X_rows];
    muV         = new double[X_cols];    
    

    for(int ni=0; ni<X_rows; ni++){
        X[ni]   = new double[X_cols];
        Xm[ni]  = new double[X_cols];
        S[ni]   = new double[X_rows];
    }

    for(int pi=0; pi<X_cols; pi++){
        Xm_t[pi] = new double[X_rows];
    }

    double **W = new double*[X_cols];
    for(int iw=0; iw<X_cols; iw++){
        W[iw] = new double[X_rows];
    }   

    double *D = new double[X_rows];
    double **V = new double*[X_rows];
    for(int i=0; i<X_rows; i++){
        V[i] = new double[X_rows];
    }

    double tol = 1.0e-20;
    
    double t1 = omp_get_wtime();    
    readImages(X,nfiles,file_imgs,imgh,imgw);
    double t2 = omp_get_wtime();    
    getS(S,X,Xm,Xm_t, muV, X_rows, X_cols);
    double t3 = omp_get_wtime();    
    eigenfn(S,V,D,X_rows,tol);
    double t4 = omp_get_wtime();    
    getW(Xm_t, V, W, X_cols, X_rows);
    double t5 = omp_get_wtime();    
    double *temp = new double[X_cols];

#   pragma omp parallel num_threads(thread_count)

#   pragma omp for
    for(int j=0; j < 16; j++){
        for(int i=0; i< X_cols; i++){
            temp[i] = W[i][j];
        }
    }
    //    showImage(temp, imgw, imgh);
    //}

    //for(int ir=10; ir < X_rows; ir+=10){
    //    temp = Reconstructor(Xm[0], W, X_cols, ir);
    //    cout<<"\tk -> "<<ir<<endl;
    //    showImage(temp, imgw, imgh);   
    //}

    int key = 300;
    cout<<"->\tk\t= "<<key<<endl;
    double **Y = new double*[X_rows];
#   pragma omp for
    for (int iy = 0; iy < X_rows; iy++)
    {
        Y[iy] = new double[key];
    }
#   pragma omp for
    for (int i = 0; i < X_rows; i++)
    {
        Y[i] = getProyection(Xm[i], W, X_cols, key);
    }
    double **nn_W      = getW(nfiles, X_cols);
    double  *nn_b      = getBias(nfiles, 0.0);
    double *pred       = new double[nfiles];
    
    double loss;
    double **labels    = new double*[X_rows];
    double *output     = new double[nfiles];

#   pragma omp for
    for (int in=0; in < X_rows; in++){
        labels[in] = new double[nfiles];
    }

#   pragma omp for    
    for (int in=0; in < X_rows; in++){
        for (int il = 0; il < nfiles; il++)
        {
            labels[in][il] = (double)((in / 10) == il); 
        }
    } 
    double lr = 5e-1;

    double t6 = omp_get_wtime();    
    cout<<"\ntraining:"<<endl;
    cout<<"=========\n"<<endl;
    for (int epoch=0; epoch < 100; epoch++){
        loss = 0.0;
        for (int i = 0; i < X_rows; i++){
            pred = getPred(Y[i],key, nfiles, nn_W, nn_b);
            output = softMax(pred, nfiles);
            loss += crossEntropy(output,labels[i], nfiles);
            backProp(nn_W,nn_b,Y[i],key,nfiles,labels[i], output, lr);
        }
        
        loss /= (double)X_rows;
        //printf("\tloss (epoch: %3d)\t: %0.6f\n", epoch, loss);       
    }
    printf("\tloss (epoch: %3d)\t: %0.6f\n", 100, loss);
    double t7 = omp_get_wtime();    
    cout<<"\ntest:"<<endl;
    cout<<"====="<<endl;

    int rand_indx;
    int validation;
    for (int i = 0; i < X_rows; i++){
        rand_indx = rand()%X_rows;
        pred = getPred(Y[rand_indx], key, nfiles, nn_W, nn_b);
        //printf("label[%3d]\t= %2d\n", rand_indx/10, maxIndx(pred, nfiles));
        //showImage(X[rand_indx], imgw, imgh);
    }

    double t8 = omp_get_wtime();
    
    printf("dt2: %.5lf\n", t2 - t1);
    printf("dt3: %.5lf\n", t3 - t2);
    printf("dt4: %.5lf\n", t4 - t3);
    printf("dt5: %.5lf\n", t5 - t4);
    printf("dt6: %.5lf\n", t6 - t5);
    printf("dt7: %.5lf\n", t7 - t6);
    printf("dt8: %.5lf\n", t8 - t7);    
//  free
    for(int ni=0; ni<X_rows; ni++){
        delete [] X[ni];
        delete [] Xm[ni];
        delete [] labels[ni];
        delete [] V[ni];
    }
    
    for(int pi=0; pi<X_cols; pi++){
        delete [] Xm_t[pi];
        delete [] W[pi]; 
    }

    for (int iw = 0; iw < nfiles; iw++)
    {
        delete [] nn_W[iw];  
    }

    delete [] X;
    delete [] muV;
    delete [] Xm;
    delete [] Xm_t;
    delete [] V;
    delete [] D;
    delete [] W;
    delete [] labels;
    delete [] nn_b;
    delete [] output;
    delete [] pred;
    delete [] temp;
    delete [] nn_W;

    return 0;
}
