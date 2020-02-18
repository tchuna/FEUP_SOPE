#include <unistd.h>
#include <wait.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/stat.h>


#define READ 0
#define WRITE 1
#define MAXCHAR 1024
#define MAXSEATS 30
#define MAX_CLI_SEATS 99


#define CLOG_FILE  "clog.txt"
#define CBOOK_FILE  "cbook.txt"
#define SERVER_FIFO_NAME	"/tmp/requests"

//respostas possiveis de retorno do servidor para o cliente
#define TIME_OUT -7
#define FUll_ROOM -6
#define AT_LEAST_ONE_SEATS_UNAVAILABLE -5
#define PARAMETERES_ERRORS -4
#define INVALID_SEATS_ID -3
#define INVALID_SEATS_NUMBER_ID -2
#define HIGER_MAX_SEATS -1
#define VALID_SEATS 0


//previlegios FIFO

#define FIFO_MODE 0660


typedef struct Client{
	pid_t pid;
	int time_out;
	int num_wanted_seats;
	int list_seats[MAXSEATS];
	int seats_length;
	int thread;
} Client;

typedef struct Answer_Server{
	int numbers[MAX_CLI_SEATS + 1];
	int array_lenght;
} Answer_Server;

void timeout_read_client(int signal);
void create_client_fifo(struct Answer_Server *answer,struct Client *client);
void cbookFilePrint(int seat);
void clogFilePrint(struct Client cliente,struct Answer_Server answer);
