#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <sys/time.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#define NUM_MAX 20

int NUM_FILOSOFI = 0;
sem_t *forchette[NUM_MAX];
sem_t *starv;
sem_t *dead;
pid_t pid_f[NUM_MAX];
bool parent = true;
volatile sig_atomic_t interrupt = 0; // Variabile condivisa per segnalare l'interrupt
bool flag_rileva_starvation;
bool soluzione;
bool flag_rileva_deadlock;
int *shared_variable;
int shm_id;
void chiudi();


void *stallo(void * arg) {
    sleep(5);
    while (1) {
	//printf("\nShared Variable= %d\n",*shared_variable);
        if (*shared_variable == NUM_FILOSOFI && !soluzione) {
            printf("\n\n Deadlock rilevato! Uscita in corso!\n\n");
            chiudi();
            pthread_exit((void * ) 0);
        }
	
    }

    pthread_exit((void * ) 0);
}

void Handler(int iSignalCode) {

    if (interrupt) {
        return;
    }
    interrupt = 1;
    printf("\n\nHandler: ricevuto signal %d\n. %s\n", iSignalCode, strsignal(iSignalCode));

    chiudi();
    sleep(10);
    return;

}

void mangia(int id) {
    printf("\nFILOSOFO %d STA MANGIANDO.\n", id);
    struct timespec tss;
    tss.tv_sec = 0;
    tss.tv_nsec = 100000000;
    nanosleep(&tss, NULL);

    if (soluzione && id == (NUM_FILOSOFI - 1)) {
        printf("\n L'ultimo Filosofo ( %d ) POSA PRIMA la forchetta alla sua DX\n", id);
        (*shared_variable)--;
        sem_post(forchette[(id + 1) % NUM_FILOSOFI]); //il filosofo ha finito di mangiare e posa la forchetta alla sua destra
        printf("\nFilosofo %d POSA la forchetta alla sua SX\n", id);
        sem_post(forchette[id]); //il filosofo ha finito di mangiare e posa la forchetta alla sua sinistra
        

    } else {
        printf("\nFILOSOFO %d HA FINITO DI MANGIARE.\n", id);
        //if(id<4){
        printf("\nFilosofo %d POSA la forchetta alla sua SX\n", id);
        (*shared_variable)--;
        sem_post(forchette[id]); //il filosofo ha finito di mangiare e posa la forchetta alla sua sinistra
        printf("\nFilosofo %d POSA la forchetta alla sua DX\n", id);
        sem_post(forchette[(id + 1) % NUM_FILOSOFI]); //il filosofo ha finito di mangiare e posa la forchetta alla sua destra
        /*}else{
        sem_post(forchette[(id + 1) % NUM_FILOSOFI]); //il filosofo ha finito di mangiare e posa la forchetta alla sua destra
        sem_post(forchette[id]); //il filosofo ha finito di mangiare e posa la forchetta alla sua sinistra
        }*/
        
    }
}

void filosofo(int id) {
    int min = 1;
    int max = 4;
    int c = 0;
    int no = 0; //se vale 0 vuol dire che non sono attivi flag e la gestione dei semafori deve essere eseguita normalmente per tutti
    struct timespec timeout;
    while (1) {
        printf("\nFilosofo %d ha fame.\n", id);
        clock_gettime(CLOCK_REALTIME, & timeout);
        timeout.tv_sec += 1;

        if (flag_rileva_starvation) {
            no = 1;
            printf("\nFilosofo %d VUOLE PRENDERE la forchetta alla sua SX\n", id);
            if (soluzione) {
                if (id == NUM_FILOSOFI - 1) {
                    (*shared_variable)++;
                    if (sem_timedwait(forchette[(id + 1) % NUM_FILOSOFI], & timeout) == -1 || sem_timedwait(forchette[id], & timeout) == -1) {
                        printf("\nFILOSOFO %d in STARVATION\n", id);
                        sem_post(starv);
                    }

                } else {
                    (*shared_variable)++;
                    if (sem_timedwait(forchette[id], & timeout) == -1 || sem_timedwait(forchette[(id + 1) % NUM_FILOSOFI], & timeout) == -1) {
                        printf("\nFILOSOFO %d in STARVATION\n", id);
                        sem_post(starv);
                    }
                }

            } else {
            	(*shared_variable)++;
                if (sem_timedwait(forchette[id], & timeout) == -1 || sem_timedwait(forchette[(id + 1) % NUM_FILOSOFI], & timeout) == -1) {
                    printf("\nFILOSOFO %d in STARVATION\n", id);
                    sem_post(starv);
                    //sleep(30);
                }
            }
            printf("\nFilosofo %d VUOLE PRENDERE  la forchetta alla sua DX\n", id);
        }

        //printf("\n\n\nsoluzione = %d, id = %d \n\n", soluzione, id);
        if (soluzione && !flag_rileva_starvation) {
            no = 1;
            if (id == NUM_FILOSOFI - 1) {
                //printf("ultimo =%d", ultimo);
                printf("\nL'ultimo filosofo %d PRENDE  la forchetta alla sua DX per evitare lo stallo\n", id);
                (*shared_variable)++;
                sem_wait(forchette[(id + 1) % NUM_FILOSOFI]); //il filosofo acquisisce il semaforo e lo blocca (0) per prendere la forchetta prima a dx
                printf("\nFilosofo %d VUOLE PRENDERE  la forchetta alla sua SX\n", id);
                sem_wait(forchette[id]); //il filosofo acquisisce il semaforo e lo blocca (0) per prendere la forchetta poi quella a sx
            } else {
                printf("\nFilosofo %d VUOLE PRENDERE  la forchetta alla sua SX\n", id);
                (*shared_variable)++;
                sem_wait(forchette[id]); //il filosofo acquisisce il semaforo e lo blocca (0) per prendere la forchetta poi quella a sx
                printf("\nFilosofo %d VUOLE PRENDERE  la forchetta alla sua DX\n", id);
                sem_wait(forchette[(id + 1) % NUM_FILOSOFI]); //il filosofo acquisisce il semaforo e lo blocca (0) per prendere la forchetta prima a dx
            }

        }
        if (no == 0) {
            printf("\nFilosofo %d VUOLE PRENDERE  la forchetta alla sua SX\n", id);
            (*shared_variable)++;
            sem_wait(forchette[id]); //il filosofo acquisisce il semaforo e lo blocca (0) per prendere la forchetta poi quella a sx
            printf("\nFilosofo %d VUOLE PRENDERE  la forchetta alla sua DX\n", id);
            sem_wait(forchette[(id + 1) % NUM_FILOSOFI]); //il filosofo acquisisce il semaforo e lo blocca (0) per prendere la forchetta prima a dx

        }

        mangia(id);
        /*int tempo = num_ran(min,max);
	time_t inizio= time(NULL);
    sleep(tempo);
    time_t fine = time(NULL);
    double tempo_di_esecuzione = difftime(fine, inizio);
    printf("Tempo di esecuzione del filosofo %d : %.2f secondi\n", id, tempo_di_esecuzione);*/
        /*int attesa = num_ran(max,min);
        sleep(attesa);*/
    }
}

void chiudi() {

    if (parent) {

        for (int i = 0; i <= NUM_FILOSOFI - 1; i++) {
            char nome_sem[30];
            sprintf(nome_sem, "/forchetta%d", i);
            printf("\nSemaforo eliminato numero: %d\n", i);

            if (pid_f[i] > 0) {
                kill(pid_f[i], SIGTERM); // invia SIGTERM a ogni processo figlio ancora attivo
            }
            if (sem_close(forchette[i]) == -1) {
                perror("Errore in sem_close()...");
                exit(EXIT_FAILURE);
            }

            if (sem_unlink(nome_sem) == -1) {
                perror("Errore in sem_unlink()...");
                exit(EXIT_FAILURE);
            }

        }
        if (starv != NULL) {
            sem_close(starv);
            sem_unlink("/starv");
        }

        if (shmdt(shared_variable) == -1) {
            perror("shmdt");
            exit(1);
        }
        /*if (dead != NULL) {
                 sem_close(dead);
                 sem_unlink("/dead");
             }*/

        exit(EXIT_SUCCESS);
    }
}

int main(int argc, char * argv[]) {

    
    key_t key = ftok("shared_memory_key", 1234);

    struct sigaction sa;
    memset( & sa, '\0', sizeof(struct sigaction));
    sigset_t sigset;
    sa.sa_handler = Handler;
    sigaddset( & sigset, SIGINT);
    sigaction(SIGINT, & sa, NULL);
    NUM_FILOSOFI = atoi(argv[1]);
    pthread_t verificaStallo;
    int err;

    shm_id = shmget(key, sizeof(int), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }
    shared_variable = (int * ) shmat(shm_id, NULL, 0);
    if (shared_variable == (int * ) - 1) {
        perror("shmat");
        exit(1);
    }
    *shared_variable = 0;

    for (int i = 0; i < NUM_FILOSOFI; i++) {
        char nome_sem[30];
        sprintf(nome_sem, "/forchetta%d", i);
        forchette[i] = sem_open(nome_sem, O_CREAT, 0644, 1);

        if (forchette[i] == SEM_FAILED) {
            perror("Errore in sem_open()...");
            exit(EXIT_FAILURE);
        }
    }

    flag_rileva_starvation = false;
    flag_rileva_deadlock = false;
    soluzione = false;

    if (argc > 1 && strcmp(argv[2], "S") == 0) {
        flag_rileva_starvation = true;
        starv = sem_open("/starv", O_CREAT, 0644, 0);
    }
    if (argc > 1 && strcmp(argv[3], "S") == 0) {
        soluzione = true;
    }
    if (argc > 1 && strcmp(argv[4], "S") == 0) {
        flag_rileva_deadlock = true;
        //dead  = sem_open("/dead", O_CREAT, 0644, 1);
    }
    if (flag_rileva_deadlock) {
        if (err = pthread_create( & verificaStallo, NULL, & stallo, NULL) != 0) {
            perror("pthread_create: ERRORE NELLA CREAZIONE DEL THREAD");
            exit(1);
        }
    }

    for (int i = 0; i < NUM_FILOSOFI; i++) {
        pid_f[i] = fork();
        if (pid_f[i] == -1) {
            perror("Errore in fork()...");
            exit(EXIT_FAILURE);
        } else if (pid_f[i] == 0) {
            parent = false;
            //processi child
            printf("\nFilosofo %d creato\n", i);
            //sleep(1);
            filosofo(i);
            exit(EXIT_SUCCESS);
        }

        //printf("Tutti i filosofi hanno terminato\n");

    }

    //for(int i=0; i < NUM_FILOSOFI; i++){
    // waitpid(pid_f[i], NULL, 0); 
    //}

    if (flag_rileva_starvation) {
        sem_wait(starv);
        /*sem_close(starv);
        sem_unlink("/starv");*/
        chiudi(forchette);
    }

    /*if(flag_rileva_deadlock){
    sem_wait(dead);
    chiudi(forchette);
    }*/
    else {
        waitpid(pid_f[1], NULL, 0);
        chiudi(forchette);
    }

    exit(EXIT_SUCCESS);
}

