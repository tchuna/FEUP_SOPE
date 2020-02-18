#include "client.h"


int p;

int main(int argc, char *argv[]) {
	//printf("** Running process %d (PGID %d) **\n", getpid(), getpgrp());

	if (argc != 4) {
		printf("client <time_out> <num_wanted_seats> <pref_seat_list>");
		exit(0);
	}

	struct Client cliente;
	cliente.pid = getpid();
	p = cliente.pid;

	cliente.time_out = atoi(argv[1]);

	cliente.num_wanted_seats = atoi(argv[2]);
	char *pref_seat = argv[3];
	char * pch;
	//int list_seats[MAXSEATS];

	pch = strtok(pref_seat, " ");
	int i = 0;

	while (pch != NULL) {


		cliente.list_seats[i] = atoi(pch);
		i++;

		pch = strtok(NULL, " ");
	}
	cliente.seats_length = i;
	//printf ("%d\n",cliente.num_wanted_seats);

	char str[20];
	sprintf(str, "/tmp/ans%d", cliente.pid);

	struct Answer_Server answer;
	int fifo_write;
	fifo_write = open(SERVER_FIFO_NAME, O_WRONLY);
	write(fifo_write, &cliente, sizeof(Client));

	//client fifo
	mkfifo(str, FIFO_MODE);
	create_client_fifo(&answer, &cliente);
	unlink(str);

	return 0;
}

void create_client_fifo(Answer_Server *answer, Client *client) {

	signal(SIGALRM, timeout_read_client);

	char str[20];
	sprintf(str, "/tmp/ans%d", client->pid);

	alarm(client->time_out);

	int fifo_read;
	do {
		fifo_read = open(str, O_RDONLY);
	} while (fifo_read == -1);


	read(fifo_read, answer, sizeof(Answer_Server));

	clogFilePrint(*client, *answer);

	close(fifo_read);
}

void timeout_read_client(int signal) {
	FILE *file;
	file = fopen(CLOG_FILE, "a");

	if (file == NULL) {
		fprintf(stderr, "Error to open clog.txt file");
		exit(1);
	}
	fprintf(file, "%05d OUT\n", p);

	exit(0);

}

void cbookFilePrint(int seat) {

	FILE *file;

	file = fopen(CBOOK_FILE, "a");

	if (file == NULL) {
		fprintf(stderr, "Error to open cbook.txt file\n");
		exit(1);
	}

	fprintf(file, "%04d\n", seat);

	fclose(file);

}

void clogFilePrint(struct Client cliente, struct Answer_Server answer) {

	FILE *file;

	file = fopen(CLOG_FILE, "a");

	if (file == NULL) {
		fprintf(stderr, "Error to open clog.txt file");
		exit(1);
	}
	int i;
	for (i = 1; i < answer.array_lenght; i++) {
		fprintf(file, "%05d ", cliente.pid);

		switch (answer.numbers[0]) {
		case FUll_ROOM:
			fprintf(file, "FULL");
			break;
		case AT_LEAST_ONE_SEATS_UNAVAILABLE:
			fprintf(file, "NAV");
			break;
		case PARAMETERES_ERRORS:
			fprintf(file, "ERR");
			break;
		case INVALID_SEATS_ID:
			fprintf(file, "IID");
			break;
		case INVALID_SEATS_NUMBER_ID:
			fprintf(file, "NST");
			break;
		case HIGER_MAX_SEATS:
			fprintf(file, "MAX");
			break;
		default:

			fprintf(file, "%02d.%02d %04d", i + 1, answer.array_lenght,
					answer.numbers[i]);

			cbookFilePrint(answer.numbers[i]);
		}
		fprintf(file, "\n");
	}
	fclose(file);
}
