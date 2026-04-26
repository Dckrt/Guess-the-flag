/* Filename:
 *      le9_server.c
 *
 * Description:
 *      "Guess the Country" - Two-player game over TCP sockets.
 *      SERVER = Player 1.
 *
 * Compile:  gcc -o le9_server le9_server.c
 * Run:      ./le9_server 5001
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>

#define BUFFER_SIZE 512

typedef struct { char name[64]; } Country;

Country easy_countries[] = {
    {"United States"}, {"Philippines"}, {"Japan"}, {"France"},
    {"Brazil"}, {"Australia"}, {"Germany"}, {"China"},
    {"Canada"}, {"India"},
};

Country medium_countries[] = {
    {"Portugal"}, {"Argentina"}, {"Nigeria"}, {"Vietnam"},
    {"Turkey"}, {"Poland"}, {"Morocco"}, {"Colombia"},
    {"New Zealand"}, {"Ethiopia"},
};

Country hard_countries[] = {
    {"Djibouti"}, {"Eritrea"}, {"Haiti"}, {"Bhutan"},
    {"Suriname"}, {"Kyrgyzstan"}, {"Vanuatu"}, {"Liechtenstein"},
    {"Nauru"}, {"Moldova"},
};

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

Country pick_country(Country *pool, int size, int exclude_idx) {
    int idx;
    do { idx = rand() % size; } while (idx == exclude_idx);
    return pool[idx];
}

void read_input(char *buffer) {
    bzero(buffer, BUFFER_SIZE);
    fgets(buffer, BUFFER_SIZE - 1, stdin);
    buffer[strcspn(buffer, "\r\n")] = '\0';
}

/*
 * Turn protocol (strictly alternating):
 *
 *   STAGE_INFO|pts|my_country  -->
 *                               <-- ACK
 *
 *   Repeat until both guessed:
 *     [Server turn]
 *       reads question/GUESS from stdin
 *       sends SERVER_QUESTION|q  OR  SERVER_GUESS|g
 *       if question: waits ANSWER|x, prints it
 *       if guess:    waits GUESS_RESULT|correct/wrong
 *       sends TURN_DONE
 *
 *     [Client turn]
 *       sends CLIENT_TURN|q_left
 *       waits CLIENT_QUESTION|q  OR  CLIENT_GUESS|g
 *       if question: read answer, sends ANSWER|x, waits CLIENT_TURN_DONE
 *       if guess:    sends GUESS_RESULT|x,         waits CLIENT_TURN_DONE
 *
 *   STAGE_RESULT|s_pts|c_pts|s_country|c_country  -->
 *                               <-- ACK
 */

void play_stage(int client_sock,
                char *my_country_name,
                char *opp_country_name,
                int max_q, int stage_pts,
                int *my_pts_out, int *opp_pts_out)
{
    char buffer[BUFFER_SIZE];
    char out[BUFFER_SIZE];
    int my_q_left   = max_q;
    int opp_q_left  = max_q;
    int i_guessed   = 0;
    int opp_guessed = 0;
    int my_pts  = 0;
    int opp_pts = 0;

    snprintf(out, BUFFER_SIZE, "STAGE_INFO|%d|%s", stage_pts, my_country_name);
    net_send(client_sock, out);
    net_recv(client_sock, buffer); /* ACK */

    while (!i_guessed || !opp_guessed) {

        /* ══ SERVER TURN ══ */
        if (!i_guessed) {
            if (my_q_left == 0) {
                printf("\n--- YOUR TURN (no questions left - must guess!) ---\n< ");
                fflush(stdout);
                read_input(buffer);
                snprintf(out, BUFFER_SIZE, "SERVER_GUESS|%s", buffer);
                net_send(client_sock, out);
                net_recv(client_sock, buffer); /* GUESS_RESULT|correct/wrong */
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
                    snprintf(out, BUFFER_SIZE, "SERVER_GUESS|%s", buffer);
                    net_send(client_sock, out);
                    net_recv(client_sock, buffer); /* GUESS_RESULT|correct/wrong */
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
                    snprintf(out, BUFFER_SIZE, "SERVER_QUESTION|%s", buffer);
                    net_send(client_sock, out);
                    my_q_left--;
                    net_recv(client_sock, buffer); /* ANSWER|YES/NO */
                    char *ans = strstr(buffer, "|"); if (ans) ans++;
                    printf("< %s\n", ans ? ans : buffer);
                }
            }
            net_send(client_sock, "TURN_DONE");
        }

        if (i_guessed && opp_guessed) break;

        /* ══ CLIENT TURN ══ */
        if (!opp_guessed) {
            snprintf(out, BUFFER_SIZE, "CLIENT_TURN|%d", opp_q_left);
            net_send(client_sock, out);
            net_recv(client_sock, buffer); /* CLIENT_QUESTION|q or CLIENT_GUESS|g */

            if (strncmp(buffer, "CLIENT_QUESTION|", 16) == 0) {
                char *q = buffer + 16;
                opp_q_left--;
                printf("\n[P2 asks] %s\n(Your country: %s) YES or NO:\n< ", q, my_country_name);
                fflush(stdout);
                read_input(buffer);
                for (int i = 0; buffer[i]; i++)
                    if (buffer[i] >= 'a' && buffer[i] <= 'z') buffer[i] -= 32;
                snprintf(out, BUFFER_SIZE, "ANSWER|%s", buffer);
                net_send(client_sock, out);

            } else if (strncmp(buffer, "CLIENT_GUESS|", 13) == 0) {
                char *guess = buffer + 13;
                printf("\n[P2 guesses]: %s\n", guess);
                if (strcasecmp(guess, my_country_name) == 0) {
                    int bonus = opp_q_left * 2;
                    printf("[CORRECT] P2 +%d pts + %d bonus = +%d\n",
                        stage_pts, bonus, stage_pts + bonus);
                    opp_pts += stage_pts + bonus;
                    net_send(client_sock, "GUESS_RESULT|correct");
                } else {
                    printf("[WRONG] You get +5\n");
                    my_pts += 5;
                    net_send(client_sock, "GUESS_RESULT|wrong");
                }
                opp_guessed = 1;
            }
            net_recv(client_sock, buffer); /* CLIENT_TURN_DONE */
        }
    }

    snprintf(out, BUFFER_SIZE, "STAGE_RESULT|%d|%d|%s|%s",
        my_pts, opp_pts, my_country_name, opp_country_name);
    net_send(client_sock, out);
    net_recv(client_sock, buffer); /* ACK */

    *my_pts_out  = my_pts;
    *opp_pts_out = opp_pts;
}

int main(int argc, char *argv[]) {
    if (argc < 2) { printf("Usage: %s port_no\n", argv[0]); exit(1); }
    srand(time(NULL));

    int server_sock, client_sock, port_no, client_size;
    struct sockaddr_in server_addr, client_addr;
    char buffer[BUFFER_SIZE];

    printf("+=================================+\n");
    printf("|   GUESS THE COUNTRY - SERVER    |\n");
    printf("|          (Player 1)             |\n");
    printf("+=================================+\n");

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) die_with_error("Error: socket() Failed.");

    int opt = 1;
    setsockopt(server_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    bzero((char *)&server_addr, sizeof(server_addr));
    port_no = atoi(argv[1]);
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(port_no);

    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
        die_with_error("Error: bind() Failed.");

    listen(server_sock, 1);
    printf("Waiting for Player 2 on port %d...\n", port_no);

    client_size = sizeof(client_addr);
    client_sock = accept(server_sock, (struct sockaddr *)&client_addr, &client_size);
    if (client_sock < 0) die_with_error("Error: accept() Failed.");
    printf("Player 2 connected! Game starting...\n\n");
    net_recv(client_sock, buffer); /* READY */

    int easy_count = sizeof(easy_countries)  / sizeof(Country);
    int med_count  = sizeof(medium_countries) / sizeof(Country);
    int hard_count = sizeof(hard_countries)   / sizeof(Country);
    int s = 0, c = 0, st = 0, ct = 0, idx;

    /* STAGE 1 */
    printf("+==============================+\n");
    printf("|   STAGE 1 - EASY  (10 pts)   |\n");
    printf("+==============================+\n");
    idx = rand() % easy_count;
    Country e1 = easy_countries[idx];
    Country e2 = pick_country(easy_countries, easy_count, idx);
    printf("Your country (P2 must guess): %s\n", e1.name);
    printf("P2's country: [HIDDEN]\n");
    play_stage(client_sock, e1.name, e2.name, 5, 10, &s, &c);
    st += s; ct += c;
    printf("\n[Stage 1] You: %d pts | Player 2: %d pts\n", s, c);

    /* STAGE 2 */
    printf("\n+==============================+\n");
    printf("|  STAGE 2 - MEDIUM (20 pts)   |\n");
    printf("+==============================+\n");
    idx = rand() % med_count;
    Country m1 = medium_countries[idx];
    Country m2 = pick_country(medium_countries, med_count, idx);
    printf("Your country (P2 must guess): %s\n", m1.name);
    printf("P2's country: [HIDDEN]\n");
    play_stage(client_sock, m1.name, m2.name, 8, 20, &s, &c);
    st += s; ct += c;
    printf("\n[Stage 2] You: %d pts | Player 2: %d pts\n", s, c);

    /* STAGE 3 */
    printf("\n+==============================+\n");
    printf("|   STAGE 3 - HARD  (30 pts)   |\n");
    printf("+==============================+\n");
    idx = rand() % hard_count;
    Country h1 = hard_countries[idx];
    Country h2 = pick_country(hard_countries, hard_count, idx);
    printf("Your country (P2 must guess): %s\n", h1.name);
    printf("P2's country: [HIDDEN]\n");
    play_stage(client_sock, h1.name, h2.name, 10, 30, &s, &c);
    st += s; ct += c;
    printf("\n[Stage 3] You: %d pts | Player 2: %d pts\n", s, c);

    /* FINAL */
    printf("\n+==============================+\n");
    printf("|        FINAL SCORES          |\n");
    printf("+==============================+\n");
    printf("  Player 1 (You)   : %d pts\n", st);
    printf("  Player 2 (Client): %d pts\n", ct);
    if      (st > ct) printf("  *** PLAYER 1 WINS! ***\n");
    else if (ct > st) printf("  *** PLAYER 2 WINS! ***\n");
    else              printf("  *** IT'S A DRAW! ***\n");

    close(client_sock);
    close(server_sock);
    return 0;
}
