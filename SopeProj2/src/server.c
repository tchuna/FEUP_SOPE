#include "server.h"

Seat all_seats[MAX_ROOM_SEATS];
int NUMBER_ROOM_SEATS;
int AVAILABLE_SEATS;
int NUMBER_TICKET_OFFICES;
int OPEN_TIME;

int* threads_status_opened;
int* threads_status_running;
int alarmflag = 0;

sem_t semaphore;
pthread_mutex_t mutex_server = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_files = PTHREAD_MUTEX_INITIALIZER;

void alarm_handler(int signo) {
	unlink(SERVER_FIFO_NAME);
	alarmflag = 1;
}

int main(int argc, char* argv[]) {
	if(argc < 3) {
		fprintf(stderr,"invalid number of arguments\n");
		exit(PARAMETERES_ERRORS);
	}

	NUMBER_ROOM_SEATS = min(atoi(argv[1]), MAX_ROOM_SEATS);
	AVAILABLE_SEATS = NUMBER_ROOM_SEATS;
	NUMBER_TICKET_OFFICES = atoi(argv[2]);
	OPEN_TIME = atoi(argv[3]);
	int indice_threads = 0;
	int i;

	init_seats();

	pthread_t all_threads[NUMBER_TICKET_OFFICES];
	Client attending_simultaneous_clients[NUMBER_TICKET_OFFICES];

	//inicializar o semaforo. com o numero de bilheteiras???
	sem_init(&semaphore, NOT_SHARED_SEMAPHORE, NUMBER_TICKET_OFFICES);

	threads_status_opened = malloc(NUMBER_TICKET_OFFICES * sizeof(int));
	threads_status_running = malloc(NUMBER_TICKET_OFFICES * sizeof(int));

	for(i = 0 ; i < NUMBER_TICKET_OFFICES ; i++) {
		threads_status_opened[i] = 0;
		threads_status_running[i] = 0;
	}

	mkfifo(SERVER_FIFO_NAME,FIFO_MODE);
	int fifo_server;
	int running_threads = 0;

	struct sigaction action;
	sigset_t sigmask;
	action.sa_handler = alarm_handler;
	sigemptyset(&action.sa_mask);
	//all signals are delivered
	action.sa_flags = 0;
	sigaction(SIGALRM, &action, NULL);
	// prepare mask for 'sigsuspend'
	sigfillset(&sigmask);
	//all signals blocked ...
	sigdelset(&sigmask, SIGALRM);

	alarm(OPEN_TIME);

	//start program
	while(!alarmflag) {
		do{
			fifo_server = open(SERVER_FIFO_NAME, O_RDONLY);
			if(fifo_server == -1) {
				break;
			}
		} while (fifo_server == -1);

		int readed_bytes;
		readed_bytes = read(fifo_server , &attending_simultaneous_clients[indice_threads], sizeof(struct Client));
		if(readed_bytes > 0) {
			if(threads_status_opened[indice_threads] == 0) {
				threads_status_opened[indice_threads] = 1;
				openThreadFilePrint(indice_threads + 1);
			}
			int sem_res = sem_wait(&semaphore);

			if(sem_res == 0) {
				running_threads++;
				threads_status_running[indice_threads] = 1;
				//escrever na struct client que thread o vai ateder
				attending_simultaneous_clients[indice_threads].thread = indice_threads + 1;
				//criar a thread
				pthread_create(&all_threads[indice_threads], NULL, attend_client, &attending_simultaneous_clients[indice_threads]);
				indice_threads++;
			}
			if(running_threads == NUMBER_TICKET_OFFICES) {
				while(running_threads >= NUMBER_TICKET_OFFICES) {
					for(i = 0 ; i < NUMBER_TICKET_OFFICES ; i++) {
						if(threads_status_running[i] == 0) {
							indice_threads = i;
							running_threads--;
							sem_post(&semaphore);
							break;
						}
					}
				}
			}
		}
	}
	//destruir o fifo
	close(fifo_server);
	unlink(SERVER_FIFO_NAME);
	pthread_mutex_destroy(&mutex_server);

	//informar que os threads devem terminar e esperar que eles terminem
	for(i = 0 ; i < NUMBER_TICKET_OFFICES ; i++) {
		if(threads_status_opened[i] == 1) {
			if(threads_status_running[i] == 1) {
				pthread_join(all_threads[i], NULL);
			}
			closeThreadFilePrint(i + 1);
		}
	}
	slogFilePrintClosed();
	return 0;
}

void init_seats() {
	for(int i = 0 ; i < NUMBER_ROOM_SEATS ; i++) {
		all_seats[i].free = SEAT_FREE;
		all_seats[i].client_id = 0;
	}
}

int isSeatFree(Seat *seats, int seat_num) {
	DELAY();
	return seats[seat_num].free;
}

void bookSeat(Seat *seats, int seat_num, int client_id) {
	DELAY();
	seats[seat_num].free = SEAT_OCCUPED;
	seats[seat_num].client_id = client_id;
}

void freeSeat(Seat *seats, int seat_num) {
	DELAY();
	seats[seat_num].free = SEAT_FREE;
	seats[seat_num].client_id = 0;
}

void* attend_client(void *c1) {
	Client client = *(Client*) c1;
	Answer_Server answer;
	int i;

	int x = verify_seats_is_valid(&client);
	//se algum dos parametros nao e valido nao continua e acaba a thread
	if(x != VALID_SEATS) {
		answer.numbers[0] = x;
		answer.array_lenght = 1;
		create_answer_fifo(&answer, &client);
		threads_status_running[client.thread - 1] = 0;
		return NULL;
	}

	int reserved_seats = 0;
	int id_reserved_seats[client.num_wanted_seats];

	for(i = 0 ; i < client.seats_length ; i++) {

		//iniciar secção critica
		pthread_mutex_lock(&mutex_server);

		int result = isSeatFree(&all_seats[0], client.list_seats[i]);

		if(result == SEAT_FREE) {
			bookSeat(&all_seats[0], client.list_seats[i], client.pid);
			id_reserved_seats[reserved_seats] = client.list_seats[i];
			reserved_seats++;
			//printf("%d\n", reserved_seats);
			AVAILABLE_SEATS--;
		}
		if(reserved_seats == client.num_wanted_seats) {
			pthread_mutex_unlock(&mutex_server);
			break;
		}
		//fechar secção critica
		pthread_mutex_unlock(&mutex_server);
	}

	//se existe pelo menos um lugar que o cliente queria mas esta ocupado
	if(reserved_seats >= 0 && reserved_seats < client.num_wanted_seats) {
		for(i = 0 ; i < reserved_seats ; i++) {
			pthread_mutex_lock(&mutex_server);
			freeSeat(&all_seats[0], id_reserved_seats[i]);
			AVAILABLE_SEATS++;

			pthread_mutex_unlock(&mutex_server);

		}
		reserved_seats = 1;
		answer.numbers[0] = AT_LEAST_ONE_SEATS_UNAVAILABLE;
		answer.array_lenght = 1;
		create_answer_fifo(&answer, &client);
		threads_status_running[client.thread - 1] = 0;
		return NULL;
	}
	//se foi bem sucedida a reserva
	answer.numbers[0] = client.num_wanted_seats;
	for(i = 0 ; i < client.num_wanted_seats ; i++) {
		answer.numbers[i + 1] = id_reserved_seats[i];
	}
	answer.array_lenght = client.num_wanted_seats + 1;
	create_answer_fifo(&answer, &client);
	threads_status_running[client.thread - 1] = 0;
	return NULL;
}

int verify_seats_is_valid(Client *client) {
	int i;

	if(client->num_wanted_seats <= 0 || client->num_wanted_seats > MAX_CLI_SEATS) {
		return HIGER_MAX_SEATS;
	}
	if(client->seats_length < client->num_wanted_seats || client->seats_length > NUMBER_ROOM_SEATS) {
		return INVALID_SEATS_NUMBER_ID;
	}
	if(client->seats_length <= 0 || client->seats_length > NUMBER_ROOM_SEATS) {
		return INVALID_SEATS_ID;
	}
	for(i = 0 ; i < client->seats_length ; i++) {
		if(client->list_seats[i] <= 0 || client->list_seats[i] > NUMBER_ROOM_SEATS) {
			return INVALID_SEATS_ID;
		}
	}
	if(AVAILABLE_SEATS <= 0) {
		return FUll_ROOM;
	}
	return VALID_SEATS;
}

void create_answer_fifo(Answer_Server *answer, Client *client) {
	//escrever no ficheiro o resultado das reservas de todos os clientes
	slogFilePrint(client->thread, *client, *answer);
	sbookFilePrint(*answer);
	//o fifo de resposta tem de ser criado pelo cliente, basta abrir em modo esrita e escrever o resultado
	char str[15];
	sprintf(str, "/tmp/ans%d", client->pid);
	int fifo_answer_to_client;
	do{
		fifo_answer_to_client = open(str,O_WRONLY);
		if(fifo_answer_to_client == -1) {
			//esgotou o tempo que o cliente queria esperar
			return;
		}
	} while (fifo_answer_to_client == -1);
	write(fifo_answer_to_client, answer, sizeof(Answer_Server));
	close(fifo_answer_to_client);
}

// Files Print func
int digitCount(int num){
	int count=0;

	while(num!=0){
		num /=10;
		count++;
	}

	return count;
}

void openThreadFilePrint(int numThread){
	pthread_mutex_lock(&mutex_files);
	FILE *file;

	file=fopen (SLOG_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open slog.txt file" );
		exit (1);
	}

	fprintf(file,"%02d-OPEN \n",numThread);

	fclose(file);
	pthread_mutex_unlock(&mutex_files);
}

void closeThreadFilePrint(int numThread){
	pthread_mutex_lock(&mutex_files);
	FILE *file;
	file=fopen (SLOG_FILE,"a");
	if(file==NULL){
		fprintf(stderr, "Error to open slog.txt file" );
		exit (1);
	}
	fprintf(file,"%02d-CLOSE \n",numThread);
	fclose(file);
	pthread_mutex_unlock(&mutex_files);
}

void slogFilePrint(int numThread,Client client,Answer_Server answer){
	pthread_mutex_lock(&mutex_files);
	FILE *file;
	int i;

	file=fopen (SLOG_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open slog.txt file" );
		exit (1);
	}

	fprintf(file,"%02d-%05d-%02d:",numThread,client.pid,client.num_wanted_seats);

	for(i=0;i<client.seats_length;i++){
		fprintf(file," %04d ",client.list_seats[i]);
	}

	switch (answer.numbers[0]) {
	case FUll_ROOM:  fprintf(file,"- FULL\n");break;
	case AT_LEAST_ONE_SEATS_UNAVAILABLE:  fprintf(file,"- NAV\n");break;
	case PARAMETERES_ERRORS:  fprintf(file,"- ERR\n");break;
	case INVALID_SEATS_ID:  fprintf(file,"- IID\n");break;
	case INVALID_SEATS_NUMBER_ID:  fprintf(file,"- NST\n");break;
	case HIGER_MAX_SEATS:  fprintf(file,"- MAX\n");break;
	default:
		fprintf(file,"- ");
		for(i=1;i<answer.array_lenght;i++){
			fprintf(file,"%04d ",answer.numbers[i]);
		}
		fprintf(file,"\n");
	}
	fclose(file);
	pthread_mutex_unlock(&mutex_files);
}

void slogFilePrintClosed() {
	pthread_mutex_lock(&mutex_files);
	FILE *file;
	file=fopen (SLOG_FILE,"a");
	if(file==NULL){
		fprintf(stderr, "Error to open slog.txt file" );
		exit (1);
	}
	fprintf(file,"SERVER CLOSED");
	fclose(file);

	pthread_mutex_unlock(&mutex_files);
}

void sbookFilePrint(Answer_Server answer){
	pthread_mutex_lock(&mutex_files);
	FILE *file;
	int i;

	file=fopen (SBOOK_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open sbook.txt file" );
		exit (1);
	}
	if(answer.array_lenght > 1) {
		for(i=1;i<answer.array_lenght;i++) {
			fprintf(file,"%04d\n",answer.numbers[i]);
		}
	}
	fclose(file);
	pthread_mutex_unlock(&mutex_files);
}
