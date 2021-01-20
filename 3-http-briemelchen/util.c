/**
 * @author briemelchen
 * @date 04.01.2021
 * @brief module contains some functionality, which does not contain to a 
 * specific module (client/server)
 * @details e.g. functionalities to check ports and urls on validity are 
 * provided by the module.
 * For more information @see util.h documentation of the functions.
 * */

#include "util.h"

bool is_valid_port(const char *port)
{
    while (*port)
    {
        if (!isdigit(*port++))
            return false;
    }
    return true;
}

bool is_valid_url(const char *URL)
{
    const char *http = "http://";
    if (strncmp(URL, http, strlen(http)) != 0)
        return false;
    return true;
}

int extract_host(const char *URL, char *path, char *host)
{
    int http_len = strlen("http://");
    char *temp = calloc(sizeof(char) * strlen(URL), sizeof(char));
    char *temp2 = calloc(sizeof(char) * strlen(URL), sizeof(char));
    if (temp == NULL || temp2 == NULL)
        return -1;

    strncpy(temp, (URL + http_len), strlen(URL) - http_len);
    strncpy(temp2, (URL + http_len), strlen(URL) - http_len);

    char *path_temp = strpbrk(temp2, ";/?:@=&");
    if (path_temp == NULL)
        strcpy(path, "/");
    else
        strcpy(path, path_temp);

    temp = strsep(&temp, ";/?:@=&");

    strcpy(host, temp);
    free(temp);
    free(temp2);
    return 0;
}

bool is_header_valid(char *line, char *res, int *response_code)
{
    const char *http = "HTTP/1.1";
    if (strncmp(line, http, strlen(http)) != 0)
        return false;
    strncpy(res, (line + strlen(http)), strlen(line) - strlen(http));
    char *endptr;
    *response_code = strtol(res, &endptr, 10);
    if (res == endptr)
    {
        return false;
    }
    return true;
}

int get_file_name(char **d_opt_path, const char *path)
{

    char *file = strrchr(path, '/');
    if (strcmp(file, "/") == 0)
    {
        if (((*d_opt_path) = realloc((*d_opt_path), (strlen(*d_opt_path) + 1 + strlen("/index.html")) * sizeof(char))) == NULL)
            return -1;
        strcat(*d_opt_path, "/index.html");
    }
    else
    {
        if (((*d_opt_path) = realloc((*d_opt_path), (strlen(*d_opt_path) + 1 + strlen(file)) * sizeof(char))) == NULL)
            return -1;
        strcat(*d_opt_path, file);
    }
    return 0;
}

char *get_mime_type(char *full_path)
{
    char *temp = strrchr(full_path, '.');
    if (temp == NULL)
        return NULL;
    if (strcmp(temp, ".html") == 0 || strcmp(temp, ".htm") == 0)
        return "text/html";
    if (strcmp(temp, ".css") == 0)
        return "text/css";
    if (strcmp(temp, ".js") == 0)
        return "application/javascript";
    return NULL;
}

int get_file_size(FILE *file)
{
    if (fseek(file, 0L, SEEK_END) == -1)
        return -1;
    int size = ftell(file);
    rewind(file);
    return size;
}

void error(const char *error_message, const char *add_error_msg, char *prog_name)
{
    fprintf(stderr, "[%s] ERROR: %s: %s.\n", prog_name, error_message,
            add_error_msg == NULL ? "" : add_error_msg);
    exit(EXIT_FAILURE);
}

void error_m(const char *error_message, const char *add_error_msg, char *prog_name)
{
    fprintf(stderr, "[%s] ERROR: %s: %s.\n", prog_name, error_message,
            add_error_msg == NULL ? "" : add_error_msg);
}
