#include <stdio.h>
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <time.h>
#include "ini.h"

#define VERSION "1.0 OpenWRT"

struct cache_data {
	char ip[20];
	time_t 	raw_time;
};

struct configuration {
	const char *user;
	const char *pass;
	const char *domain;
};


size_t get_http_output(void *ptr, size_t size, size_t nmemb, char *output)
{
    int write_size = 20;
    if (size*nmemb < 20)
    	write_size = size*nmemb;
    strncat(output, ptr, write_size);

    return write_size;
}

int get_current_ip(char *ip, const int buff_len)
{
    CURL *curl;
    CURLcode res;
    char s[20] = {0};

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "http://ip.42.pl/raw");
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, get_http_output);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, s);
        res = curl_easy_perform(curl);
        if (res != CURLE_OK) {
            printf("There was an error executing curl_easy_perform()\n");
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();

    strncpy(ip, s, buff_len);

    return 0;
}

void write_cache(struct cache_data *c_data)
{
    FILE *f;
    f = fopen("/tmp/ip_cache", "wb");
    if (!f) {
    	fprintf(stderr, "Unable to open the cache file for writing /tmp/ip_cache");
    }
    else {
    	fwrite(c_data, sizeof(struct cache_data), 1, f);
    	//printf("Cache file written\n");
    	fclose(f);
    }
}

/**
 * Compare cached data with the current within last 5 minutes
 */
int compare_cache(struct cache_data *c_data)
{
	struct cache_data old_cache;
	FILE *f;
	f = fopen("/tmp/ip_cache", "rb");
	if (!f) {
		//fprintf(stderr, "Cache file not found. It will be created.\n");
		return -1;
	}

	fread(&old_cache, sizeof(struct cache_data), 1, f);
	fclose(f);

	//printf("Cache IP: %s\nCurrent IP: %s\nCache Time: %lu\n\n", old_cache.ip, c_data->ip, old_cache.raw_time);

	if (strcmp(c_data->ip, old_cache.ip) == 0) {
		time_t current_time = time(0);
		time_t last_updated = old_cache.raw_time;
		time_t last_updated_p_5min = last_updated + (5*60);

		if (current_time <= last_updated_p_5min) {
			return 0;
		}
	}

    return -1;
}

int get_ip_for_domain(char * hostname , char* ip, int buff_len)
{
    struct hostent *he;
    struct in_addr **addr_list;
    int i;

    if ( (he = gethostbyname( hostname ) ) == NULL)
    {
        // get the host info
        herror("gethostbyname");
        return 1;
    }

    addr_list = (struct in_addr **) he->h_addr_list;

    for(i = 0; addr_list[i] != NULL; i++)
    {
        //Return the first one;
        strncpy(ip , inet_ntoa(*addr_list[i]), buff_len);
        return 0;
    }

    return 1;
}

int update_dyn_dns(const char *ip, const char *user, const char *password, const char *domain, const force)
{
	struct cache_data c_data;
	strcpy(c_data.ip, ip);
	c_data.raw_time = time(0);

	int compared = compare_cache(&c_data);

	// if things are fine
	if (compared == 0 && force != 1) {
		printf("Update already requested, waiting for server to do the update.\n");
		return 0;
	}

	if (force == 1) {
		printf("Executing force update\n");
	}
	// else continue below

	CURL *curl;
	CURLcode res;
	long status_code;

	curl_global_init(CURL_GLOBAL_DEFAULT);
	curl = curl_easy_init();
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, "https://ezegate.com:8443/ezedns/");
		char buff_args[100];
		strcpy(buff_args, "USER=");
		strcat(buff_args, user);
		strcat(buff_args, "&PASSWD=");
		strcat(buff_args, password);
		strcat(buff_args, "&DOMAIN=");
		strcat(buff_args, domain);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, buff_args);

		//printf("Arguments : %s\n", buff_args);

		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);

		res = curl_easy_perform(curl);
		if (res != CURLE_OK)
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

		curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &status_code);

		if (status_code == 200 && status_code != CURLE_ABORTED_BY_CALLBACK) {
			printf("\n\nSuccessfully udpated.\n");
			write_cache(&c_data);
		}
		else if (status_code == 403)
			printf("\n\nAuthorization denied by the server\n");
		else
			printf("\n\nBad response %lu\n", status_code);

		curl_easy_cleanup(curl);
	}

	curl_global_cleanup();

    return 0;
}

static int handler(void* user, const char* section, const char* name,
                   const char* value)
{
    struct configuration* pconfig = (struct configuration*)user;

    #define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
    if (MATCH("client", "user")) {
        pconfig->user = strdup(value);
    } else if (MATCH("client", "password")) {
        pconfig->pass = strdup(value);
    } else if (MATCH("client", "domain")) {
        pconfig->domain = strdup(value);
    } else {
        return 0;  /* unknown section/name, error */
    }
    return 1;
}

void show_help()
{
	printf("eZeDNS Client. Version %s\n", VERSION);
	printf("Copyright (c) eZeLink Telecom. Dec, 2014, Dubai, UAE.\n");
	printf("\n");
	printf("\n");
	printf("USAGE: ezedns-client [option]\n");
	printf("\t-f\tForce Update\n");
	printf("\t-h\tShow this help\n");
}


int main(int argc, char *argv[])
{
	int res;
	// process config file
	struct configuration config;
	if (ini_parse("/etc/ezedns-client.conf", handler, &config) < 0) {
		printf("Can't load configuration file: '/etc/ezedns-client.conf'\n");
		printf("Please make sure the file exists in the mentioned location.");
	    return 1;
	}

	int force = 0;
	if (argc > 1 && strcmp(argv[1], "-f") == 0)
		force = 1;
	else if (argc > 1 && strcmp(argv[1], "-h") == 0) {
		show_help();
		return 0;
	}


    //process update
    char current_ip[20] = {0};
    char ip_for_domain[20] = {0};
    get_current_ip(current_ip, 19);

    char *full_domain;
    full_domain = (char *) malloc(strlen(config.domain) + 15);
    strcpy(full_domain, config.domain);
    strcat(full_domain, ".ezegate.com");

    res = get_ip_for_domain(full_domain, ip_for_domain, 19);
    if (strlen(current_ip) > 0)
    	printf("Current IP : %s\n", current_ip);
    if (strlen(ip_for_domain) > 0)
    	printf("Current IP for domain is: %s\n", ip_for_domain);

    //IPs collected and proceed to verification
    if (strcmp(current_ip, ip_for_domain) == 0 && force == 0) {
        printf("IPs are same, No need updates\n");
    }
    else {
    	if (force == 0) {
    		printf("IPs are different need to be updated\n");
    	}
        update_dyn_dns(current_ip, config.user, config.pass, config.domain, force);
    }

    if (full_domain != NULL)
    	free (full_domain);

    return 0;
}
