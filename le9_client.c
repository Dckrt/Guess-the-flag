/* Filename:
 *      le9_client.c
 *
 * Description:
 *      "Guess the Country" - Two-player game over TCP sockets.
 *      CLIENT = Player 2.
 *
 * Compile:  gcc -o le9_client le9_client.c
 * Run:      ./le9_client localhost 5001
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 512

void die_with_error(char *msg) { printf("%s\n", msg); exit(-1); }

void net_send(int sock, const char *msg) {
    if (send(sock, msg, strlen(msg), 0) < 0)
        die_with_error("Error: send() Failed.");
}

int net_recv(int sock, char *buffer) {
    bzero(buffer, BUFFER_SIZE);
    int n = recv(sock, buffer, BUFFER_SIZE - 1, 0);
    if (n < 0) die_with_error("Error: recv() Failed.");
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return n;
}

void read_input(char *buffer) {
    bzero(buffer, BUFFER_SIZE);
    fgets(buffer, BUFFER_SIZE - 1, stdin);
    buffer[strcspn(buffer, "\r\n")] = '\0';
}

void play_stage(int sock, char *my_country_name, int stage_pts,
                int *my_pts_out, int *opp_pts_out)
{
    char buffer[BUFFER_SIZE];
    char out[BUFFER_SIZE];
    char tmp[BUFFER_SIZE];
    int my_q_left   = 0;
    int i_guessed   = 0;
    int opp_guessed = 0;
    int my_pts  = 0;
    int opp_pts = 0;
    char opp_country_name[64] = "?";

    /* Receive stage info */
    net_recv(sock, buffer); /* STAGE_INFO|pts|server_country */
    strncpy(tmp, buffer, BUFFER_SIZE - 1);
    strtok(tmp, "|");             /* "STAGE_INFO" */
    strtok(NULL, "|");            /* pts (already known) */
    char *sc = strtok(NULL, "|"); /* server's country - we store but don't show */
    if (sc) strncpy(opp_country_name, sc, 63);
    net_send(sock, "ACK");

    printf("\nYour country (P1 must guess): %s\n", my_country_name);
    printf("P1's country: [HIDDEN]\n");

    while (!i_guessed || !opp_guessed) {

        net_recv(sock, buffer);

        /* ══ SERVER ASKING A QUESTION ══ */
        if (strncmp(buffer, "SERVER_QUESTION|", 16) == 0) {
            char *q = buffer + 16;
            printf("\n[P1 asks] %s\n(Your country: %s) YES or NO:\n< ", q, my_country_name);
            fflush(stdout);
            read_input(buffer);
            for (int i = 0; buffer[i]; i++)
                if (buffer[i] >= 'a' && buffer[i] <= 'z') buffer[i] -= 32;
            snprintf(out, BUFFER_SIZE, "ANSWER|%s", buffer);
            net_send(sock, out);
            /* wait for TURN_DONE from server */
            net_recv(sock, buffer);

        /* ══ SERVER GUESSING ══ */
        } else if (strncmp(buffer, "SERVER_GUESS|", 13) == 0) {
            char *guess = buffer + 13;
            printf("\n[P1 guesses]: %s\n", guess);
            if (strcasecmp(guess, my_country_name) == 0) {
                printf("[CORRECT] P1 gets points.\n");
                opp_pts += stage_pts; /* server earns, tracked server-side */
                net_send(sock, "GUESS_RESULT|correct");
            } else {
                printf("[WRONG] You get +5.\n");
                my_pts += 5;
                net_send(sock, "GUESS_RESULT|wrong");
            }
            opp_guessed = 1;
            /* wait for TURN_DONE */
            net_recv(sock, buffer);

        /* ══ MY TURN ══ */
        } else if (strncmp(buffer, "CLIENT_TURN|", 12) == 0) {
            my_q_left = atoi(buffer + 12);

            if (my_q_left == 0) {
                printf("\n--- YOUR TURN (no questions left - must guess!) ---\n< ");
                fflush(stdout);
                read_input(buffer);
                snprintf(out, BUFFER_SIZE, "CLIENT_GUESS|%s", buffer);
                net_send(sock, out);
                net_recv(sock, buffer); /* GUESS_RESULT|correct/wrong */
                char *res = strstr(buffer, "|"); if (res) res++;
                if (res && strcmp(res, "correct") == 0) {
                    printf("[CORRECT!] +%d pts\n", stage_pts);
                    my_pts += stage_pts;
                } else {
                    printf("[WRONG] Opponent's country: %s. They get +5.\n", opp_country_name);
                    opp_pts += 5;
                }
                i_guessed = 1;
            } else {
                printf("\n--- YOUR TURN (Q left: %d) | type question or GUESS ---\n< ", my_q_left);
                fflush(stdout);
                read_input(buffer);

                if (strcasecmp(buffer, "GUESS") == 0) {
                    printf("< ");
                    fflush(stdout);
                    read_input(buffer);
                    snprintf(out, BUFFER_SIZE, "CLIENT_GUESS|%s", buffer);
                    net_send(sock, out);
                    net_recv(sock, buffer); /* GUESS_RESULT|correct/wrong */
                    char *res = strstr(buffer, "|"); if (res) res++;
                    if (res && strcmp(res, "correct") == 0) {
                        int bonus = my_q_left * 2;
                        printf("[CORRECT!] +%d pts + %d bonus = +%d\n",
                            stage_pts, bonus, stage_pts + bonus);
                        my_pts += stage_pts + bonus;
                    } else {
                        printf("[WRONG] Opponent's country: %s. They get +5.\n", opp_country_name);
                        opp_pts += 5;
                    }
                    i_guessed = 1;
                } else {
                    snprintf(out, BUFFER_SIZE, "CLIENT_QUESTION|%s", buffer);
                    net_send(sock, out);
                    net_recv(sock, buffer); /* ANSWER|YES/NO */
                    char *ans = strstr(buffer, "|"); if (ans) ans++;
                    printf("< %s\n", ans ? ans : buffer);
                }
            }
            net_send(sock, "CLIENT_TURN_DONE");

        /* ══ STAGE RESULT ══ */
        } else if (strncmp(buffer, "STAGE_RESULT|", 13) == 0) {
            strncpy(tmp, buffer, BUFFER_SIZE - 1);
            strtok(tmp, "|");
            char *sp  = strtok(NULL, "|"); /* server pts */
            char *cp  = strtok(NULL, "|"); /* client pts */
            char *sc2 = strtok(NULL, "|"); /* server country */
            char *cc  = strtok(NULL, "|"); /* client country */
            opp_pts = sp ? atoi(sp) : 0;
            my_pts  = cp ? atoi(cp) : 0;
            printf("\n[Reveal] P1's country was: %s\n", sc2 ? sc2 : "?");
            printf("[Reveal] Your country was: %s\n", cc ? cc : "?");
            net_send(sock, "ACK");
            break;
        }
    }

    *my_pts_out  = my_pts;
    *opp_pts_out = opp_pts;
}

int main(int argc, char *argv[]) {
    if (argc < 3) { printf("Usage: %s hostname port_no\n", argv[0]); exit(1); }
    srand(time(NULL) + 999);

    int client_sock, port_no;
    struct sockaddr_in server_addr;
    struct hostent *server;
    char buffer[BUFFER_SIZE];

    printf("+=================================+\n");
    printf("|   GUESS THE COUNTRY - CLIENT    |\n");
    printf("|          (Player 2)             |\n");
    printf("+=================================+\n");

    client_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (client_sock < 0) die_with_error("Error: socket() Failed.");

    server = gethostbyname(argv[1]);
    if (!server) die_with_error("Error: No such host.");

    port_no = atoi(argv[2]);
    bzero((char *)&server_addr, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
          (char *)&server_addr.sin_addr.s_addr,
          server->h_length);
    server_addr.sin_port = htons(port_no);

    printf("Connecting to server...\n");
    if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die_with_error("Error: connect() Failed.");
    printf("Connected! Game starting...\n");
    net_send(client_sock, "READY");

    char *easy[]   = {"United States","Philippines","Japan","France","Brazil",
                      "Australia","Germany","China","Canada","India"};
    char *medium[] = {"Portugal","Argentina","Nigeria","Vietnam","Turkey",
                      "Poland","Morocco","Colombia","New Zealand","Ethiopia"};
    char *hard[]   = {"Djibouti","Eritrea","Haiti","Bhutan","Suriname",
                      "Kyrgyzstan","Vanuatu","Liechtenstein","Nauru","Moldova"};

    int my_pts, opp_pts, my_total = 0, opp_total = 0;

    /* STAGE 1 */
    printf("\n+==============================+\n");
    printf("|   STAGE 1 - EASY  (10 pts)   |\n");
    printf("+==============================+\n");
    char *e = easy[rand() % 10];
    play_stage(client_sock, e, 10, &my_pts, &opp_pts);
    my_total += my_pts; opp_total += opp_pts;
    printf("\n[Stage 1] You: %d pts | Player 1: %d pts\n", my_pts, opp_pts);

    /* STAGE 2 */
    printf("\n+==============================+\n");
    printf("|  STAGE 2 - MEDIUM (20 pts)   |\n");
    printf("+==============================+\n");
    char *m = medium[rand() % 10];
    play_stage(client_sock, m, 20, &my_pts, &opp_pts);
    my_total += my_pts; opp_total += opp_pts;
    printf("\n[Stage 2] You: %d pts | Player 1: %d pts\n", my_pts, opp_pts);

    /* STAGE 3 */
    printf("\n+==============================+\n");
    printf("|   STAGE 3 - HARD  (30 pts)   |\n");
    printf("+==============================+\n");
    char *h = hard[rand() % 10];
    play_stage(client_sock, h, 30, &my_pts, &opp_pts);
    my_total += my_pts; opp_total += opp_pts;
    printf("\n[Stage 3] You: %d pts | Player 1: %d pts\n", my_pts, opp_pts);

    /* FINAL */
    printf("\n+==============================+\n");
    printf("|        FINAL SCORES          |\n");
    printf("+==============================+\n");
    printf("  Player 1 (Server): %d pts\n", opp_total);
    printf("  Player 2 (You)   : %d pts\n", my_total);
    if      (my_total > opp_total) printf("  *** PLAYER 2 WINS! ***\n");
    else if (opp_total > my_total) printf("  *** PLAYER 1 WINS! ***\n");
    else                           printf("  *** IT'S A DRAW! ***\n");

    close(client_sock);
    return 0;
}
