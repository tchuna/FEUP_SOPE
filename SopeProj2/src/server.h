#include <unistd.h>
#include <wait.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>

#include "client.h"

#define min(a, b) (a)<(b)?(a):(b)

#define MAX_ROOM_SEATS 9999
#define MAX_CLI_SEATS 99

#define WIDTH_SEAT 4
#define WIDTH_PID  5
#define WIDTH_XXNN 5

//semaforos
#define NOT_SHARED_SEMAPHORE 0

//delay
#define DELAY() sleep(1)

//seat
#define SEAT_FREE 1
#define SEAT_OCCUPED 0

//Files

#define SLOG_FILE   "slog.txt"
#define SBOOK_FILE  "sbook.txt"

//struct lugar
typedef struct Seat{
	int free;
	int client_id;
} Seat;

void init_seats();
int isSeatFree(Seat *seats, int seat_num);
void bookSeat(Seat *seats, int seat_num, int client_id);
void freeSeat(Seat *seats, int seat_num);
void* attend_client(void *c);
int verify_seats_is_valid(Client *client);
void create_answer_fifo(Answer_Server *answer, Client *client);

int digitCount(int num);

void openThreadFilePrint(int numThread);
void closeThreadFilePrint(int numThread);
void slogFilePrint(int numThread,Client client,Answer_Server answer);
void slogFilePrintClosed();
void sbookFilePrint(Answer_Server answer);
