#include "profile_notif.c"
#include "commons.h"
#include "rm.c"

void save_profiles(profile list_of_profiles[CLIENTLIMIT])
{
	FILE *profiles, *followers;
	int i = 0, j;

	profiles = fopen("profiles.txt", "w");
	followers = fopen("followers.txt", "w");

	while (list_of_profiles[i].user_name != "")
	{
		if (strlen(list_of_profiles[i].user_name) != 0)
		{
			// printf("%i %s %i ", (int)strlen(profile_list[i].name), profile_list[i].name, profile_list[i].num_followers);
			fprintf(profiles, "%i %s %i ", (int)strlen(list_of_profiles[i].user_name), list_of_profiles[i].user_name, list_of_profiles[i].follower_count);
			for (j = 0; j < list_of_profiles[i].follower_count; j++)
			{
				fprintf(followers, "%i %s ", (int)strlen(list_of_profiles[i].followed_users[j]->user_name), list_of_profiles[i].followed_users[j]->user_name);
				// printf("%i %s ", (int)strlen(profile_list[i].followers[j]->name), profile_list[i].followers[j]->name);
			}
		}
		i++;
	}

	fclose(profiles);
	fclose(followers);
}

void read_profiles(profile *list_of_profiles, int rm_identifier)
{
	FILE *profiles, *followers;
	int num_profiles, i, j, length, follow_id;
	num_profiles = 0;
	char *aux;
	char profiles_file[20];
	char followers_file[20];
	//Agora fazemos um arquivo de followers e profiles para cada RM
	sprintf(profiles_file, "%d", rm_identifier);
	sprintf(followers_file, "%d", rm_identifier);
	strcat(profiles_file, "profiles.txt");
	strcat(followers_file, "followers.txt");

	if ((profiles = fopen("profiles.txt", "r")) && (followers = fopen("followers.txt", "r")))
	{

		while (!feof(profiles))
		{
			fscanf(profiles, "%i ", &length);
			list_of_profiles[num_profiles].user_name = malloc((length + 1) * sizeof(char));
			fscanf(profiles, "%s %i ", list_of_profiles[num_profiles].user_name, &(list_of_profiles[num_profiles].follower_count));
			list_of_profiles[num_profiles].user_name[length] = '\0';
			num_profiles++;
		}

		for (i = 0; i < num_profiles; i++)
		{
			for (j = 0; j < list_of_profiles[i].follower_count; j++)
			{
				fscanf(followers, "%i ", &length);
				aux = malloc((length + 1) * sizeof(char));
				fscanf(followers, "%s ", aux);
				aux[length] = '\0';
				follow_id = get_profile_id(list_of_profiles, aux);
				list_of_profiles[i].followed_users[j] = &(list_of_profiles[follow_id]);
				free(aux);
			}
		}

		fclose(profiles);
		fclose(followers);
	}
}

/// Leitura do arquivo de configurações de RM
// ao inicializar o server pega nos args o nome do arquivo onde está a definição
// das RMs e a identificação de que RM esse processo representa.
// Associa os dados lidos das settings aos objetos RM para que o processo
// saiba internamente:
// Que RM ele é
// Quem é o primário
// Qual a lista de RMs ligadas
void read_server_settings(char *file_name, int rm_identifier, rm *caller, rm *list_of_rms, int *size_of_list_of_rms, rm *primary_rm)
{
	FILE *settings = fopen(file_name, "r");
	if (settings == NULL)
	{
		printf("Server settings file not found.");
		exit(1);
	}
	int counter, foundPrimary = 0, foundRM = 0;
	char * buffer = NULL;
	char thisline_id[5];
	int thisline_port;
	int thisline_isprimary;
	int rm_count;
	ssize_t read;
	size_t length;

	///// Conta o numero de linhas do arquivo
	int n = 15, n_lines = 0;
	char *line;
	while(fgets(line, n, settings) != NULL)
	{
			n_lines++;
	}
	rewind(settings); //vollta para o comeco do arquivo
	

	while ((read = getline(&buffer, &length, settings)) != -1)
	{
		counter = 0;
		char *separator = strtok(buffer, " \n");

		// Pra cada linha:
		while (separator != NULL)
		{
			atoi(separator);

			switch(counter)
			{
			case 0:
				strcpy(thisline_id, separator);
				break;
			case 1:
				thisline_port = atoi(separator);
				break;
			case 2:
				thisline_isprimary = atoi(separator);
				break;
			}
			separator = strtok (NULL, " \n");
			if ( counter > 2 ){
				printf("Settings file read error\n");
				exit(1);
			}
			counter++;
		}

		// Está lendo a linha que contém o RM primário, joga isso na struct dele
		if (thisline_isprimary)
		{
			foundPrimary = 1;

			primary_rm->id = atoi(thisline_id);
			strcpy(primary_rm->id_string, thisline_id);
			primary_rm->port = thisline_port;
			primary_rm->is_primary = thisline_isprimary;
		}

		// Achou o RM que chamou a função
		// Atribui os valores lidos à struct dele
		if (atoi(thisline_id) == rm_identifier)
		{
			foundRM = 1;

			caller->id = atoi(thisline_id);
			strcpy(caller->id_string, thisline_id);
			caller->port = thisline_port;
			caller->is_primary = thisline_isprimary;
		}

		// O RM lido da linha é um outro, não é o que chamou e nem primário
		// bota na lista de RMs existentes
		if (atoi(thisline_id) != rm_identifier && !thisline_isprimary)
		{
			rm_count++;

			list_of_rms[rm_count].id = atoi(thisline_id);
			strcpy(list_of_rms[rm_count].id_string, thisline_id);
			list_of_rms[rm_count].port = thisline_port;
			list_of_rms[rm_count].is_primary = thisline_isprimary;
			list_of_rms[rm_count].socket = -1;
		}

		// Feita a leitura do arquivo:
		// fecha, limpa o buffer
		// se não tiver achado o RM que chamou a função cai fora
		// se não tiver achado um primário tb cai fora
		// atribui à lista de rms tamanho igual ao número de rms lidos no arquivo

		// Da exit se nao achou a RM depois de percorrer todo o settings file
		if (!foundRM && (atoi(thisline_id) == n_lines-1))
		{
			printf("RM not present in settings file.\n");
			exit(1);
		}

		if (!foundPrimary)
		{
			printf("Could not find primary RM in settings file.\n");
			exit(1);
		}

		*size_of_list_of_rms = rm_count;
	}

	fclose(settings);
	if (buffer)
	{
		free(buffer);
	}
}