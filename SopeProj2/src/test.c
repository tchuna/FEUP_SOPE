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

#define SLOG_FILE  "slog.txt"
#define SBOOK_FILE  "sbook.txt"
#define CLOG_FILE  "clog.txt"
#define CBOOK_FILE  "cbook.txt"

#define TIME_OUT -7
#define FUll_ROOM -6
#define AT_LEAST_ONE_SEATS_UNAVAILABLE -5
#define PARAMETERES_ERRORS -4
#define INVALID_SEATS_ID -3
#define INVALID_SEATS_NUMBER_ID -2
#define HIGER_MAX_SEATS -1
#define VALID_SEATS 0


#define READ 0
#define WRITE 1
#define MAXCHAR 1024
#define MAXSEATS 6
#define MAX_CLI_SEATS 99

typedef struct Client{
	pid_t pid;
	int time_out;
	int num_wanted_seats;
	int list_seats[MAXSEATS];
	int seats_length;

} Client;

typedef struct Answer_Server{
	int numbers[MAX_CLI_SEATS + 1];
	int array_lenght;
} Answer_Server;


void openThreadFilePrint(int numThread){
	FILE *file;

	file=fopen (SLOG_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open slog.txt file" );
		exit (1);
	}


	if(numThread<10){
		fprintf(file,"0%d-OPEN \n",numThread);
	}else{
		fprintf(file,"%d-OPEN \n",numThread);
	}

	fclose(file);
}



void slogFilePrint(int numThread,Client client,Answer_Server answer){


  FILE *file;

	file=fopen (SLOG_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open slog.txt file" );
		exit (1);
	}

  fprintf(file,"%02d-%05d-%02d:",numThread,client.pid,client.num_wanted_seats);

  for(int i=0;i<client.num_wanted_seats;i++){
      fprintf(file," %04d ",client.list_seats[i]);
  }

 switch (answer.numbers[0]) {
   case FUll_ROOM:  fprintf(file,"- FUL\n");break;
   case AT_LEAST_ONE_SEATS_UNAVAILABLE:  fprintf(file,"- NAV\n");break;
   case PARAMETERES_ERRORS:  fprintf(file,"- ERR\n");break;
   case INVALID_SEATS_ID:  fprintf(file,"- IID\n");break;
   case INVALID_SEATS_NUMBER_ID:  fprintf(file,"- NST\n");break;
   case HIGER_MAX_SEATS:  fprintf(file,"- MAX\n");break;
   default:
   fprintf(file,"- ");
   for(int i=0;i<answer.array_lenght;i++){
     fprintf(file,"%04d ",answer.numbers[i]);
   }
    fprintf(file,"\n");
 }


  fclose(file);


}

void sbookFilePrint(Answer_Server answer){

  FILE *file;

  file=fopen (SBOOK_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open sbook.txt file" );
		exit (1);
	}

  for(int i=0;i<answer.array_lenght;i++){
    fprintf(file,"%04d\n",answer.numbers[i]);
  }

  fclose(file);

}



void clogFileFailtPrint(Client client,int error){

	FILE *file;

  file=fopen (CLOG_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open clog.txt file" );
		exit (1);
	}


	  fprintf(file,"%05d ",client.pid);

		switch (error) {
			case FUll_ROOM:  fprintf(file,"FUL\n");break;
			case AT_LEAST_ONE_SEATS_UNAVAILABLE:  fprintf(file,"NAV\n");break;
			case PARAMETERES_ERRORS:  fprintf(file,"ERR\n");break;
			case INVALID_SEATS_ID:  fprintf(file,"IID\n");break;
			case INVALID_SEATS_NUMBER_ID:  fprintf(file,"NST\n");break;
			case HIGER_MAX_SEATS:  fprintf(file,"MAX\n");break;
			case TIME_OUT:  fprintf(file,"OUT\n");break;
		}


		 fclose(file);



}


void clogFileResultPrint(Client client,int rankPosition){

	FILE *file;

	file=fopen (CLOG_FILE,"a");

	if(file==NULL){
		fprintf(stderr, "Error to open clog.txt file" );
		exit (1);
	}

	fprintf(file,"%05d ",client.pid);
	fprintf(file, "%02d.%02d %04d\n",rankPosition,client.num_wanted_seats,client.list_seats[rankPosition-1] );

	fclose(file);
}






int main(int argc, char* argv[]) {

  fprintf(stderr, "tchunaaaaaaaa \n");
  Client client;
  Answer_Server ans;

  ans.array_lenght=2;
  ans.numbers[0]=2233;
  ans.numbers[1]=1833;


printf("tchuna" );
  client.pid=10;
  client.num_wanted_seats=6;

  client.list_seats[0]=18;
  client.list_seats[1]=232;
  client.list_seats[2]=1;
  client.list_seats[3]=452;
  client.list_seats[4]=22;
  client.list_seats[5]=1446;


 //sbookFilePrint (ans);

  slogFilePrint(2,client,ans);

	//clogFileFailtPrint(client,-4);
	//cbookFilePrint(client,2);

	 //clogFileResultPrint(client,5);





  //openThreadFilePrint(22);


 return 0;

}
