/*
 * algorithm: Enumeration sort na lineárním poli o n procesorech 
 * author: Jan Vybíral, xvybir05
 *
 */

#include <mpi.h>
#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
 
using namespace std;

constexpr int TAG = 0;

int main(int argc, char* argv[])
{
    constexpr int EMPTY = -1;   //hodnota znacici prazdny registr

    int numprocs;               //pocet procesoru
    int myid;                   //muj rank
    int neighnumber;            //hodnota souseda (kvuli posouvani obsahu registru Y doprava)
    int input;                  //vstupni hodnota
    int inputcounter = 0;       //pocitadlo vstupnich hodnot
    int x = EMPTY;              //registr X
    int y = EMPTY;              //registr Y
    int z = EMPTY;              //registr Z
    int c;                      //registr C
    vector<int> result;         //pole kam si bude ridici procesor ukladat serazeny vysledek
    MPI_Status stat;            //struct- obsahuje kod- source, tag, error
    fstream fin;                //cteni ze souboru

    //MPI INIT
    MPI_Init(&argc, &argv);                          // inicializace MPI 
    MPI_Comm_size(MPI_COMM_WORLD, &numprocs);       // zjistíme, kolik procesů běží 
    MPI_Comm_rank(MPI_COMM_WORLD, &myid);           // zjistíme id svého procesu 

    int n = numprocs - 1;       //pocet procesoru krome ridiciho a zaroven index posledniho procesoru

    if (myid == 0) //ridici procesor
    {
        char input[] = "numbers";     //jmeno vstupniho souboru    
        fin.open(input, ios::in);     //otevre vstupni soubor pro cteni

        result = vector<int>(n); //alokuj pole pro vysledek
    }

    c = 1;  //registr C, na zacatku maji vsechny procesory nastaveno na 1, odpovida kroku 1 v algoritmu ze slidu

    //hlavni cyklus algoritmu, odpovida kroku 2 algoritmu ze slidu
    for (int k = 1; k <= 2 * n; ++k)
    {
        int h; //zacatek pro vnitrni paralelni cykly algoritmu
        if (k <= n) //jsme v prvni pulce hlavniho cyklu
        {
            h = 1;
        }
        else        //jsme v druhe pulce hlavniho cyklu
        {
            h = k - n;
        }

        //prvni vnitrni paralelni cyklus algoritmu 
        //procesory h az n inkrementuji svoji hodnotu C pokud je jejich hodnota X > Y
        if (myid >= h && x != EMPTY && y != EMPTY && x > y)
        {
            c++;
        }

        //druhy vnitrni paralelni cyklus algoritmu
        //procesory h az n-1 poslou svoji hodnotu Y procesoru s o 1 vyssim rankem   
        if (myid >= h && myid <= n - 1)
        {
            MPI_Send(&y, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);
        }
        //procesory h+1 az n obdrzi hodnotu od procesoru s o 1 nizsim rankem, a pokud to neni prazdna hodnota, ulozi ji do sveho Y
        if (myid >= h + 1)
        {
            MPI_Recv(&neighnumber, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);
            if (neighnumber != EMPTY)
            {
                y = neighnumber;
            }
        }

        if (k <= n) //jsme v prvni pulce hlavniho cyklu
        {
            if (myid == 0) //ridici procesor
            {
                input = fin.get();      //precte vstupni hodnotu
                cout << input << " ";   //vytiskne vstupni hodnotu

                //pridame ke vstupni hodnote 4 cislice na konec kvuli odliseni stejnych hodnot
                input *= 10000;
                input += inputcounter;
                if (inputcounter < 9999) inputcounter++; //inkrementujeme pocitadlo vstupnich hodnot

                //posle vstupni hodnotu procesorum s ranky 1 a k 
                MPI_Send(&input, 1, MPI_INT, 1, TAG, MPI_COMM_WORLD);
                MPI_Send(&input, 1, MPI_INT, k, TAG, MPI_COMM_WORLD);
            }
            else
            {
                if (myid == 1) //procesor s rankem 1
                {
                    //prijme vstupni hodnotu od ridiciho procesoru a ulozi ji do sveho Y
                    MPI_Recv(&y, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
                }

                if (myid == k) //procesor s rankem k
                {
                    //prijme vstupni hodnotu od ridiciho procesoru a ulozi ji do sveho X
                    MPI_Recv(&x, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD, &stat);
                }
            }
        }
        else //jsme v druhe pulce hlavniho cyklu
        {
            //procesor s rankem k-n posle vsem ostatnim svoje hodnoty C a X
            int cx[2] = { c, x };
            MPI_Bcast(&cx, 2, MPI_INT, k - n, MPI_COMM_WORLD);

            if (myid == cx[0]) //procesor s rankem shodnym s hodnotou C procesoru k-n
            {
                z = cx[1]; //ulozi obdrzenou hodnotu X procesoru k-n do sveho Z
            }
        }
    }

    if (myid == 0)    //ridici procesor
    {
        fin.close();  //zavre vstupni soubor
        cout << endl; //odradkovani na vystup
    }

    //posledni vnejsi cyklus algoritmu produkujici vystup
    //odpovida kroku 3 algoritmu ze slidu
    for (int k = 1; k <= n; ++k)
    {
        if (myid == n) //procesor s rankem n
        {
            //posle hodnotu sveho Z ridicimu procesoru
            MPI_Send(&z, 1, MPI_INT, 0, TAG, MPI_COMM_WORLD);
        }
        else if (myid >= k && myid <= n - 1) //procesory s rankem k az n-1
        {
            //posle hodnotu sveho Z procesoru s rankem o 1 vyssim
            MPI_Send(&z, 1, MPI_INT, myid + 1, TAG, MPI_COMM_WORLD);
        }
        else if (myid == 0) //ridici procesor
        {
            //prijme hodnotu od procesoru s rankem n a prida ji na zacatek vysledku
            MPI_Recv(&z, 1, MPI_INT, n, TAG, MPI_COMM_WORLD, &stat);
            result[n - k] = z / 10000; //umazani 4 extra pridanych cislic z konce
        }

        if (myid >= k + 1) //procesory s rankem k+1 az n
        {
            //obdrzi hodnotu od procesoru s rankem o 1 nizsim a ulozi ji do sveho Z
            MPI_Recv(&z, 1, MPI_INT, myid - 1, TAG, MPI_COMM_WORLD, &stat);
        }
    }

    if (myid == 0) //ridici procesor
    {
        //tisk vysledku
        for (int i = 0; i < n; ++i)
        {
            if (result[i] != EMPTY) //pripadna prazdna hodnota ve vysledku (kvuli duplicitnim hodnotam na vstupu) se netiskne
            {
                cout << result[i] << endl;
            }
        }
    }

    MPI_Finalize();

    return 0;

}

