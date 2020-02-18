#include <unistd.h>
#include <stdlib.h>
#include <wait.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h>
#include <ctype.h>
#include <stdbool.h>

struct values {
	char *word_to_search;
	int length_word_to_seach;
	int recursive;
	int complete_word;
	int case_insensitive;
	int exists_in_file;
	bool seeLines;
	bool seeNlines;
	bool insert_path;
	int counter_words;
	int counter_lines;
	int count;
};

void readDir(char* path, struct values *valores,
		struct sigaction *sigint_new_action,
		struct sigaction *sigsegv_new_action);

int* readFile(int file_descriptor, struct values *valores);

void showValues(struct values *valores, char* path, char* file, int *lines);

void noArguments(struct values *valores);

void sigint_handler(int signo) {
	char option;

	if (getpid() == getpgrp()) {
		write(STDOUT_FILENO,
				"\nAre you sure you want to terminate the program? (Y/N) : ",
				57);
		fflush(STDIN_FILENO);
		read(STDIN_FILENO, &option, 1);

		while (option != 'Y' && option != 'N') {
			fflush(STDIN_FILENO);
			read(STDIN_FILENO, &option, 1);
		}
		fflush(STDIN_FILENO);
		if (option == 'Y')
			exit(0);
		//kill(0, SIGINT);
		else
			return;
	}
}

void sigsegv_handler(int signo) {
	printf("Acesso inválido a memória.\n");
	exit(5);
}

int main(int argc, char* argv[], char* envp[]) {
	if (argc < 2) {
		printf("Numero de argumentos inválido\n");
		exit(1);
	}

	struct sigaction sigint_new_action, sigsegv_new_action;

	sigint_new_action.sa_handler = sigint_handler;
	sigsegv_new_action.sa_handler = sigsegv_handler;

	sigemptyset(&sigint_new_action.sa_mask);
	sigemptyset(&sigsegv_new_action.sa_mask);

	sigint_new_action.sa_flags = 0;
	sigsegv_new_action.sa_flags = 0;

	struct values valores;
	valores.word_to_search = argv[argc - 2];
	valores.length_word_to_seach = strlen(valores.word_to_search);
	valores.recursive = 0;
	valores.case_insensitive = 0;
	valores.complete_word = 0;
	valores.seeLines = false;
	valores.seeNlines = false;
	valores.counter_words = 0;
	valores.counter_lines = 0;
	valores.count = 0;
	valores.insert_path = true;
	valores.exists_in_file = 0;

	int numer_arguments = 0;

	for (int i = 0; i < argc - 1; i++) {
		if (strcmp(argv[i], "-i") == 0) {
			valores.case_insensitive = 1;
			numer_arguments++;
		} else if (strcmp(argv[i], "-l") == 0) {
			valores.exists_in_file = 1;
			numer_arguments++;
		} else if (strcmp(argv[i], "-n") == 0) {
			valores.seeLines = true;
			numer_arguments++;
		} else if (strcmp(argv[i], "-c") == 0) {
			valores.seeNlines = true;
			numer_arguments++;
		} else if (strcmp(argv[i], "-w") == 0) {
			valores.complete_word = true;
			numer_arguments++;
		} else if (strcmp(argv[i], "-r") == 0) {
			valores.recursive = 1;
			numer_arguments++;
		}
	}

	/**
	 * SE O numer_arguments FOR IGUAL AO ARGC - 2 É SINAL QUE O UTILIZADOR NAO INSERIU O CAMINHO, ASSUMIR "STD_IN",
	 */
	if (numer_arguments == argc - 2) {

		if (valores.recursive == 1) {
			char cwd[1024];
			getcwd(cwd, sizeof(cwd));
			//valores.insert_path = false;
			valores.word_to_search = argv[argc - numer_arguments];
			printf("palavra : %s\n", argv[argc - numer_arguments]);
			readDir(cwd, &valores, &sigint_new_action, &sigsegv_new_action);
			exit(0);
		}

		else {

			valores.insert_path = false;
			valores.word_to_search = argv[argc - 1];
			noArguments(&valores);
			exit(0);
		}

	}
	/**
	 * Quando o utilizador inseriu a opcao para ignorar letras maiusculas ou minusculas
	 */
	if (valores.case_insensitive) {

		int i = 0;
		while (valores.word_to_search[i] != '\0') {
			valores.word_to_search[i] = tolower(valores.word_to_search[i]);
			i++;

		}
	}

	printf("palavra : %s\n", valores.word_to_search);
	readDir(argv[argc - 1], &valores, &sigint_new_action, &sigsegv_new_action);
	exit(0);
}

void readDir(char *path, struct values *valores,
		struct sigaction *sigint_new_action,
		struct sigaction *sigsegv_new_action) {
	int* lines;
	DIR *directory;
	struct dirent *direntp;
	struct stat stat_buf;
	int file_descriptor = 0;
	pid_t pid = 0;

	if (valores->insert_path) {
		/**
		 * Abrir o diretorio, pode ser dado o caminho para uma pasta ou um ficheiro só /////////////////////////////////VER COMO SE FAZ
		 */
		if ((directory = opendir(path)) == NULL) {
			printf("Erro ao abrir a pasta\n");
			exit(1);
		}

		/**
		 * mudar para o dirétório pretendido
		 */
		if (chdir(path) != 0) {
			printf("Erro ao mudar de diretório\n");
			exit(6);
		}

		while ((direntp = readdir(directory)) != NULL) {
			if (lstat(direntp->d_name, &stat_buf) == -1) {
				//printf("%s\n", strerror(errno));
				exit(2);
			} else {
				/**
				 * Se for ficheiro regular
				 */
				if (S_ISREG(stat_buf.st_mode)) {
					if ((file_descriptor = open(direntp->d_name, O_RDONLY, 0644))
							== -1) {
						perror(direntp->d_name);
						printf("%s\n", strerror(errno));
						exit(3);
					} else {
						lines = readFile(file_descriptor, valores);

						if (sigaction(SIGSEGV, &(*sigsegv_new_action), NULL)
								< 0) {
							sigsegv_handler(SIGSEGV);
						}
						if (sigaction(SIGINT, &(*sigint_new_action), NULL)
								< 0) {
							sigint_handler(SIGINT);
						}
					}
					close(file_descriptor);
					showValues(valores, path, direntp->d_name, lines);
				}

				/**
				 * Se for dirétório
				 */
				else if (S_ISDIR(stat_buf.st_mode)
						&& (strcmp(direntp->d_name, ".") != 0)
						&& (strcmp(direntp->d_name, "..") != 0)) {
					if (valores->recursive) {
						//setpgrp();
						pid = fork();

						/**
						 * Erro
						 */
						if (pid == -1) {
							printf("%s\n", strerror(errno));
							exit(7);
						}
						/**
						 * Processo filho
						 */
						else if (pid == 0) {
							if (path[strlen(path)] == '/') {
								path = strcat(path, direntp->d_name);
							} else {
								path = strcat(path, "/");
								path = strcat(path, direntp->d_name);
							}
							readDir(path, valores, sigint_new_action,
									sigsegv_new_action);
						}
						/**
						 * Processo pai
						 */
						else {
							//waitpid(pid, NULL, 0);
						}
					}
				}
				/**
				 * Outro tipo de ficheiros
				 */
				else {
				}
				valores->count = 0;
				valores->counter_words = 0;
			}
		}
	} else {
		printf("nao inseriu caminho\n");
	}
}

int* readFile(int file_descriptor, struct values *valores) {
	char buffer[30];
	char character = ' ';
	bool exist = false;
	int static result[255];
	int contAux = 0;
	int readNumber;
	int i = 0;
	int j = 0;

	while ((readNumber = read(file_descriptor, &character, 1)) > 0) {
		/**
		 * Caso de palavra completa com um espaço de cada lado
		 */
		if (valores->complete_word == 1) {
			if (character != ' ' && character != '\n') {
				if (valores->case_insensitive) {

					buffer[i] = tolower(character);
				} else {
					buffer[i] = character;
				}
				i++;
			} else if (character == ' ' || character == '\n' || character == EOF) {
				if (character == '\n') {

					contAux++;
					if (exist == true) {
						valores->count++;
						result[j] = contAux;
						j++;
						exist = false;
					}
				}

				buffer[i] = '\0';
				i = 0;

				if (strcmp(buffer, valores->word_to_search) == 0) {
					exist = true;
					printf("k");
					valores->counter_words++;
					valores->count++;
				}
			}
		}
		/**
		 * So uma palavra, pode ser uma sub-string
		 */
		else if (!valores->complete_word) {
			if (valores->case_insensitive) {
				if (character == '\n') {
					contAux++;
					if (exist == true) {
						result[j] = contAux;
						valores->count++;
						j++;
						exist = false;
					}
				}

				if (tolower(character) == valores->word_to_search[i]) {
					buffer[i] = tolower(character);
					i++;
				} else {
					i = 0;
				}
			} else {
				if (character == '\n') {
					contAux++;
					if (exist == true) {
						valores->count++;
						result[j] = contAux;
						j++;
						exist = false;
					}
				}

				if (character == valores->word_to_search[i]) {
					i++;
				} else {
					i = 0;
				}
			}

			if (i == valores->length_word_to_seach) {
				exist = true;
				valores->counter_words++;
				i = 0;
			}
		}
	}
	return result;
}

void showValues(struct values *valores, char* path, char* file, int *lines) {
	printf("\n");

	if (valores->exists_in_file == 1 && valores->count != 0) {
		printf("%s/%s\n", path, file);

	} else if (valores->exists_in_file == 0) {
		printf("Existe %d vez(es), no ficheiro %s\n", valores->counter_words,
				file);

		if (valores->seeNlines == true) {
			printf("Existente em %d linha(s) no ficheiro %s\n", valores->count,
					file);
		}

		if (valores->seeLines == true) {
			int result = 1;
			int i = 0;

			printf("Presente na(s) seguintes linha(s)\n");

			while ((*(lines + i)) != 0) {
				printf("Linha-> %d\n", *(lines + i));
				i++;
			}
		}
	}
}

void noArguments(struct values *valores) {
	printf("Introduza 1 para terminar.\n");
	char str[255];
	char str2[4] = { 1 };
	int nLines = 0;
	int lines = 1;
	if (valores->case_insensitive == 1)
		valores->complete_word = tolower(valores->complete_word);

	if (valores->recursive)
		return;
	while (scanf("%s", str)) {
		if (str[0] == '1')
			break;

		if (valores->case_insensitive == 1)
			*str = tolower(*str);

		if (strcmp(str, (valores->word_to_search)) == 0) {
			valores->count++;
			if (valores->exists_in_file) {
				printf("(standard input)\n");
				return;
			}
			if (valores->seeLines) {
				printf("%d:%s\n", lines, str);
			} else
				printf("%s\n", str);
		}
		lines++;
	}
	if (valores->seeNlines) {
		printf("Existe em %d linhas.\n", valores->count);
	}
}

