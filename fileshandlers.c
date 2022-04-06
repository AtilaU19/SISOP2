#include "profile_notif.c"
#include "commons.h"

void save_profiles(profile list_of_profiles[CLIENTLIMIT])
{
	FILE *profiles, *followers;
	int i, j;
	i = 0;

	profiles = fopen("profiles.txt", "w");
	followers = fopen("followers.txt", "w");

	while(list_of_profiles[i].user_name!="")
	{
		if(strlen(list_of_profiles[i].user_name)!=0){
			//printf("%i %s %i ", (int)strlen(profile_list[i].name), profile_list[i].name, profile_list[i].num_followers);
			fprintf(profiles, "%i %s %i ", (int)strlen(list_of_profiles[i].user_name), list_of_profiles[i].user_name, list_of_profiles[i].follower_count);
			for (j = 0; j<list_of_profiles[i].follower_count; j++)
			{
				fprintf(followers, "%i %s ", (int)strlen(list_of_profiles[i].followers[j]->user_name), list_of_profiles[i].followers[j]->user_name);
				//printf("%i %s ", (int)strlen(profile_list[i].followers[j]->name), profile_list[i].followers[j]->name);
			}
			//printf("\n");
		}
		i++;
	}

	fclose(profiles);
	fclose(followers);
	
}

void read_profiles(profile* list_of_profiles)
{
	FILE *profiles, *followers;
	int num_profiles, i, j, length, follow_id;
	num_profiles = 0;
	char* aux;

	if((profiles = fopen("profiles.txt", "r")) &&  (followers = fopen("followers.txt", "r"))){
	
		
		while(!feof(profiles))
		{
			fscanf(profiles, "%i ", &length);
			list_of_profiles[num_profiles].user_name = malloc((length+1)*sizeof(char));
			fscanf(profiles, "%s %i ", list_of_profiles[num_profiles].user_name, &(list_of_profiles[num_profiles].follower_count));
			list_of_profiles[num_profiles].user_name[length]='\0';
			num_profiles++;
		}


		for (i = 0; i<num_profiles; i++)
		{
			for (j = 0; j<list_of_profiles[i].follower_count; j++)
			{
				fscanf(followers, "%i ", &length);
				aux = malloc((length+1)*sizeof(char));
				fscanf(followers, "%s ", aux);
				aux[length]='\0';
				follow_id = get_profile_id(list_of_profiles,aux);
				list_of_profiles[i].followers[j] = &(list_of_profiles[follow_id]);
				free(aux);
			}
		}

		fclose(profiles);
		fclose(followers);
	}
}