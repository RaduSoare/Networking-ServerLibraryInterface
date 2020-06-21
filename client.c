#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     /* read, write, close */
#include <string.h>     /* memcpy, memset */
#include <sys/socket.h> /* socket, connect */
#include <netinet/in.h> /* struct sockaddr_in, struct sockaddr */
#include <netdb.h>      /* struct hostent, gethostbyname */
#include <arpa/inet.h>
#include "helpers.h"
#include "requests.h"
#include "parson.h"
#include <ctype.h>

#define LONG_STRING 1000
#define SHORT_STRING 50

/*
* Conversie a datelor de login in format json cu ajutorul bibliotecii Parson
*/
char** json_credentials(char* username, char* password) {
    
    char** credentials = (char**)malloc(1 * sizeof(char*));
    credentials[0] = malloc(LONG_STRING * sizeof(char));
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = NULL;
    json_object_set_string(root_object, "username", username);
    json_object_set_string(root_object, "password", password);
    serialized_string = json_serialize_to_string_pretty(root_value);
    strcpy(credentials[0],serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);

    return credentials;

}

/*
* Conversie a datelor despre o carte in format json cu ajutorul bibliotecii Parson
*/
char** json_book_description(char* title, char* author, char* genre, 
    int page_count, char* publisher) {

    char** book_description = (char**)malloc(1 * sizeof(char*));
    book_description[0] = malloc(LONG_STRING * sizeof(char));
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);
    char *serialized_string = malloc(LONG_STRING * sizeof(char));
    json_object_set_string(root_object, "title", title);
    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "genre", genre);
    json_object_set_number(root_object, "page_count", page_count);
    json_object_set_string(root_object, "publisher", publisher);
    serialized_string = json_serialize_to_string_pretty(root_value);
    strcpy(book_description[0],serialized_string);
    json_free_serialized_string(serialized_string);
    json_value_free(root_value);
    
    return book_description;
}

/*
* Obtinere cookie din raspunsul primit de la server
*/
char** get_cookie(char* response) {
    char* response_copy = malloc(LONG_STRING * sizeof(char));
    strcpy(response_copy, response);
    char** cookie = (char**)malloc(1 * sizeof(char*));
    cookie[0] = malloc(SHORT_STRING * sizeof(char));
    char* line = strstr(response_copy, "Set-Cookie");
    char* token;
    token = strtok(line, " ");
    token = strtok(NULL, " ");
    token[strlen(token) - 1] = 0;
    strcpy(cookie[0], token);
    free(response_copy);
    return cookie;
}

/*
* Obtinere token din raspunsul serverului
*/
char* get_token(char* response) {
    char* response_copy = malloc(LONG_STRING * sizeof(char));
    strcpy(response_copy, response);
    char* line = strstr(response_copy, "{\"token\":\"");
    char* temp;
    temp = strtok(line, "\"");
    for (int i = 1; i <= 3; i++)
    {
       temp = strtok(NULL, "\"");
    }
    free(response_copy);
    return temp;
}

/*
* Se verifica daca mesajul serverului contine vreun mesaj de eroare
*/
char* check_error(char* response) {
    char* response_copy = malloc(LONG_STRING * sizeof(char));
    strcpy(response_copy, response);
    // cautare eroare in mesaj
    char* line = strstr(response_copy, "error");
    if(line == NULL) {
        free(response_copy);
        return line;
    }
    char* temp;
    temp = strtok(line, "\"");
    for (int i = 1; i <= 2; i++)
    {
       temp = strtok(NULL, "\"");
    }
    free(response_copy);
    return temp;
    
} 

/*
* Afisarea tuturor cartilor pe linii diferite
*/
void print_books(char* response) {
    char* body = strstr(response, "{");
    // tratare caz de biblioteca goala
    if(body == NULL) {
        printf("Book list is empty\n");
        return;
    }
    char* temp = strtok(body, "{");
    while(temp != NULL) {
        temp[strlen(temp) - 2] = '\0';
        printf("%s\n", temp);
        temp = strtok(NULL, "{");
        
    }
}

/*
* Afisarea fiecarei date despre o carte pe linii diferite
*/
void print_book_info(char* response) {
    char* body = strstr(response, "{");
    body = body + 1;
    body[strlen(body) - 1] = '\0';
    char* temp = strtok(body, ",");
    while(temp != NULL) {
        // caz special page_count
        if(temp[strlen(temp) - 1] == '}') {
            temp[strlen(temp)-1] = '\0';
        } else
            temp[strlen(temp)] = '\0';
        printf("%s\n", temp);
        temp = strtok(NULL, ",");
        
    }
}

/*
* Functie ajutatoare pentru a verifica validitatea comenzilor 
** primite ca input
*** Cod preluat: https://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way
*/
char *trimwhitespace(char *str)
{
  char *end;

  // Trim leading space
  while(isspace((unsigned char)*str)) str++;

  if(*str == 0)  // All spaces?
    return str;

  // Trim trailing space
  end = str + strlen(str) - 1;
  while(end > str && isspace((unsigned char)*end)) end--;

  // Write new null terminator character
  end[1] = '\0';

  return str;
}

int main(int argc, char const *argv[]) {
    // url descoperit cu DIG
    char host_ip[20] = "3.8.116.10"; 
    char host[200] = "ec2-3-8-116-10.eu-west-2.compute.amazonaws.com";
    char json_type[SHORT_STRING] = "application/json";
    int port_number = 8080;
    int sockfd;
    char *message;
    char *response;
    char* command = malloc(LONG_STRING * sizeof(char));
    char** cookie = NULL;
    char* token = NULL;
    // flag care sa spuna daca un client e deja logat la server
    int online_flag = 0;

    while(1) {
        // citire comanda de la STDIN
        scanf(" %[^\n]",command);
        command = trimwhitespace(command);
        if(strcmp(command, "exit") == 0) {
            printf("Client closed\n");
            break;
        } else if(strcmp(command, "register") == 0) {
            char* username = malloc(SHORT_STRING * sizeof(char));
            char* password = malloc(SHORT_STRING * sizeof(char));
            // citire credentiale
            printf("Enter your username: ");
            scanf("%s", username);
            printf("Enter your password: ");
            scanf("%s", password);
            // transformare date de login in format json
            char** credentials = json_credentials(username, password);
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(host, "/api/v1/tema/auth/register", json_type,
                credentials, 1, token, NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            // cautare mesaj de eroare in raspunsul primit de la server
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            printf("The account has been successfully created.\n");
            free(username); free(password);
            close_connection(sockfd); 
        } else if(strcmp(command, "login") == 0) {
            // verificare daca cineva este deja conectat la server
            if(online_flag == 1) {
                printf("You are already logged in!\n");
                continue;
            }
            char* username = malloc(SHORT_STRING * sizeof(char));
            char* password = malloc(SHORT_STRING * sizeof(char));
            printf("Enter your username: ");
            scanf("%s", username);
            printf("Enter your password: ");
            scanf("%s", password);
            char** credentials = json_credentials(username, password);
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(host, "/api/v1/tema/auth/login", json_type,
                credentials, 1, token ,cookie, 1);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            printf("Hello %s\n", username);
            // obtinere cookie din raspuns
            cookie = get_cookie(response);
            free(username); free(password);
            online_flag = 1;
            close_connection(sockfd); 
        } else if(strcmp(command, "enter_library") == 0) {
            token = malloc(LONG_STRING * sizeof(char));
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(host, "/api/v1/tema/library/access", 
                NULL, NULL, cookie, 1);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            // obtinere token JWT din raspuns
            token = get_token(response);
            printf("You are authorized to access the library\n");
            close_connection(sockfd); 
        } else if(strcmp(command, "get_books") == 0) {
            char* token_copy;
            if(token  != NULL) {
                token_copy = malloc(LONG_STRING * sizeof(char));
                strcpy(token_copy, token);
            } else {
                printf("You are not authorized to access the library.\n");
                continue;
            }
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(host, "/api/v1/tema/library/books", 
                NULL, token_copy, NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            // afisare carti existente in biblioteca
            print_books(response);
            free(token_copy);
            close_connection(sockfd);
        } else if(strcmp(command, "get_book") == 0) {
            char* token_copy;
            if(token  != NULL) {
                token_copy = malloc(LONG_STRING * sizeof(char));
                strcpy(token_copy, token);
            } else {
                printf("You are not authorized to access the library.\n");
                continue;
            }
            char* book_id = malloc(LONG_STRING);
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            char url[] = "/api/v1/tema/library/books/";
            printf("Enter book ID: ");
            scanf("%s", book_id);
            strcat(url, book_id);
            message = compute_get_request(host, url, NULL, token_copy, NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            print_book_info(response);
            free(book_id);
            free(token_copy);
            close_connection(sockfd);
        } else if(strcmp(command, "add_book") == 0) {
            char* token_copy;
            if(token  != NULL) {
                token_copy = malloc(LONG_STRING * sizeof(char));
                strcpy(token_copy, token);
            } else {
                printf("You are not authorized to access the library.\n");
                continue;
            }
            char* title = malloc(SHORT_STRING * sizeof(char));
            char* author = malloc(SHORT_STRING * sizeof(char));
            char* genre = malloc(SHORT_STRING * sizeof(char));
            int page_count = 0;
            char* publisher = malloc(SHORT_STRING * sizeof(char));
            printf("Enter title: ");
            scanf(" %[^\n]",title);
            printf("Enter author: ");
            scanf(" %[^\n]",author);           
            printf("Enter genre: ");
            scanf(" %[^\n]",genre);          
            printf("Enter page count: ");
            scanf("%d", &page_count);
            // gardare page_count nenumeric/ negativ
            if(page_count <= 0) {
                // hack sa golesc stdin inainte sa ia urmatoarea comanda
                char stdin_cleaner[20];
                scanf(" %s", stdin_cleaner);
                printf("Invalid page_count.\n");
                free(title); free(author); free(genre); free(publisher);
                free(token_copy);
                continue;
            }
            printf("Enter publisher: ");
            scanf(" %[^\n]",publisher);
            // conversie date despre carte in format json
            char** book_description = json_book_description(title, author, genre, page_count, publisher);
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            message = compute_post_request(host, "/api/v1/tema/library/books", json_type,
                book_description, 1, token_copy, NULL, 0);
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            printf("You added the book successfully.\n");
            free(title); free(author); free(genre); free(publisher);
            free(token_copy);
            close_connection(sockfd);
        } else if(strcmp(command, "delete_book") == 0) {
            char* token_copy;
            if(token  != NULL) {
                token_copy = malloc(LONG_STRING * sizeof(char));
                strcpy(token_copy, token);
            } else {
                printf("You are not authorized to access the library.\n");
                continue;
            }
            char* book_id = malloc(20 * sizeof(char));
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            char url[] = "/api/v1/tema/library/books/";
            printf("Enter book ID: ");
            scanf("%s", book_id);
            strcat(url, book_id);
            message = compute_delete_request(host, url, NULL, token_copy, NULL, 0);

            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            printf("You deleted the book successfully.\n");
            free(book_id);
            free(token_copy);
            close_connection(sockfd); 
        }  else if(strcmp(command, "logout") == 0) {
            sockfd = open_connection(host_ip,port_number, AF_INET, SOCK_STREAM, 0);
            message = compute_get_request(host, "/api/v1/tema/auth/logout", 
                NULL, NULL, cookie, 1);
            // se sterg datele despre cookie si token, pentru a nu persista dupa logout
            if(cookie != NULL) {
                cookie = NULL;
            } 
             if(token != NULL) {
                token = NULL;
            } 
            send_to_server(sockfd, message);
            response = receive_from_server(sockfd);
            char* error = check_error(response);
            if(error != NULL) {
                printf("%s\n", error);
                continue;
            }
            printf("You logged out.\n");
            // se marcheaza faptul ca este posibila o noua conectare
            online_flag = 0;
            close_connection(sockfd);
        } else {
            // caz in care comanda nu este printre cele permise
            printf("Invalid command.\n");
            continue;
        }
    }

    return 0;
}
