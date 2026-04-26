/*
 * le9_client.c
 * Guess the Country - CLIENT (Player 2)
 * Compile: gcc -o le9_client le9_client.c -w
 * Run:     ./le9_client localhost 5001
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
#define HINT_COST   5

void die_with_error(char *m){printf("%s\n",m);exit(-1);}
void net_send(int s,const char *m){if(send(s,m,strlen(m),0)<0)die_with_error("send failed");}
int  net_recv(int s,char *b){bzero(b,BUFFER_SIZE);int n=recv(s,b,BUFFER_SIZE-1,0);if(n<0)die_with_error("recv failed");b[strcspn(b,"\r\n")]='\0';return n;}
void read_input(char *b){bzero(b,BUFFER_SIZE);fgets(b,BUFFER_SIZE-1,stdin);b[strcspn(b,"\r\n")]='\0';}

void play_stage(int sock, char *my_country, int stage_pts,
                int hints, int my_total, int opp_total,
                int *my_pts_out, int *opp_pts_out)
{
    char buf[BUFFER_SIZE], out[BUFFER_SIZE], tmp[BUFFER_SIZE];
    int my_q = 0;
    int my_pts=0, opp_pts=0;
    int hint_used=0;
    char opp_continent[32]="";

    /* receive stage info and ACK */
    net_recv(sock, buf);
    net_send(sock, "ACK");

    printf("\nYour country (Player 1 must guess): %s\n", my_country);
    printf("Player 1's country: [HIDDEN]\n\n");

    /* Main loop — keep receiving messages from server */
    while (1) {
        net_recv(sock, buf);

        /* ── SERVER IS ASKING A QUESTION ── */
        if (strncmp(buf,"SERVER_QUESTION|",16)==0) {
            char *q = buf+16;
            printf("\n[server] > %s\n", q);
            printf("Your country: %s | Answer (YES/NO): ", my_country);
            fflush(stdout);
            read_input(buf);
            for(int i=0;buf[i];i++) if(buf[i]>='a'&&buf[i]<='z') buf[i]-=32;
            snprintf(out, BUFFER_SIZE, "ANSWER|%s", buf);
            net_send(sock, out);

        /* ── SERVER IS GUESSING ── */
        } else if (strncmp(buf,"SERVER_GUESS|",13)==0) {
            char *g = buf+13;
            printf("\n[server] > GUESS: %s\n", g);
            if (strcasecmp(g, my_country)==0) {
                printf("[Server CORRECT!] They win this stage.\n");
                net_send(sock, "GUESS_RESULT|correct");
            } else {
                printf("[Server WRONG!] You get +5 pts.\n");
                my_pts += 5;
                net_send(sock, "GUESS_RESULT|wrong");
            }
            net_recv(sock, buf); /* ACK */
            /* stage may be over — wait for STAGE_RESULT or CLIENT_TURN */

        /* ── SERVER HINT SYNC ── */
        } else if (strncmp(buf,"SERVER_HINT|",12)==0) {
            net_send(sock, "ACK");

        /* ── MY TURN ── */
        } else if (strncmp(buf,"CLIENT_TURN|",12)==0) {
            my_q = atoi(buf+12);
            printf("\n+--------------------------------------+\n");
            printf("|  YOUR TURN  |  Questions left: %2d   |\n", my_q);
            printf("|  Total pts: %3d                      |\n", my_total+my_pts);
            printf("+--------------------------------------+\n");
            if (opp_continent[0]!='\0')
                printf("  [HINT] Opponent is in: %s\n", opp_continent);
            if (hints && !hint_used)
                printf("  Type HINT (-5 pts) to reveal opponent's continent\n");
            printf("  Ask a question or type GUESS");
            if (hints && !hint_used) printf(" / HINT");
            printf(":\n< "); fflush(stdout);
            read_input(buf);

            /* HINT */
            if (hints && !hint_used && strcasecmp(buf,"HINT")==0) {
                if ((my_total+my_pts)<HINT_COST) {
                    printf("Not enough points! Need %d pts.\n", HINT_COST);
                    net_send(sock, "CLIENT_HINT|request");
                    net_recv(sock, buf); /* denied */
                } else {
                    net_send(sock, "CLIENT_HINT|request");
                    net_recv(sock, buf);
                    char *r = strstr(buf,"|"); if(r) r++;
                    if (r && strcmp(r,"denied")!=0) {
                        strncpy(opp_continent,r,31);
                        my_pts -= HINT_COST; hint_used=1;
                        printf("[HINT] Opponent is in: %s\n", opp_continent);
                    } else {
                        printf("Hint not available.\n");
                    }
                }
                /* server will resend CLIENT_TURN next loop */
                continue;

            /* GUESS */
            } else if (strcasecmp(buf,"GUESS")==0 || my_q==0) {
                if (my_q==0) printf("No questions left! You must guess:\n");
                else         printf("Enter your guess:\n");
                printf("< "); fflush(stdout);
                read_input(buf);
                snprintf(out, BUFFER_SIZE, "CLIENT_GUESS|%s", buf);
                net_send(sock, out);
                net_recv(sock, buf); /* GUESS_RESULT */
                char *r = strstr(buf,"|"); if(r) r++;
                if (r && strcmp(r,"correct")==0) {
                    int bonus = my_q*2;
                    printf("[CORRECT!] +%d pts + %d bonus = +%d\n",
                        stage_pts, bonus, stage_pts+bonus);
                    my_pts += stage_pts+bonus;
                } else {
                    printf("[WRONG!] Opponent gets +5.\n");
                    opp_pts += 5;
                }
                /* wait for STAGE_RESULT or CLIENT_TURN */

            /* QUESTION */
            } else if (my_q>0) {
                snprintf(out, BUFFER_SIZE, "CLIENT_QUESTION|%s", buf);
                net_send(sock, out);
                net_recv(sock, buf);
                char *ans = strstr(buf,"|"); if(ans) ans++;
                printf("[server] > %s\n", ans?ans:buf);
            }

        /* ── STAGE OVER ── */
        } else if (strncmp(buf,"STAGE_RESULT|",13)==0) {
            strncpy(tmp, buf, BUFFER_SIZE-1);
            strtok(tmp,"|");
            char *sp = strtok(NULL,"|"); /* server pts */
            char *cp = strtok(NULL,"|"); /* client pts */
            char *sc = strtok(NULL,"|"); /* server country */
            char *cc = strtok(NULL,"|"); /* client country */
            opp_pts = atoi(sp);
            my_pts  = atoi(cp);
            printf("\n[Reveal] Player 1's country: %s\n", sc?sc:"?");
            printf("[Reveal] Your country:       %s\n",   cc?cc:"?");
            net_send(sock, "ACK");
            break;
        }
    }

    *my_pts_out  = my_pts;
    *opp_pts_out = opp_pts;
}

int main(int argc, char *argv[]) {
    if (argc<3){printf("Usage: %s host port\n",argv[0]);exit(1);}
    srand(time(NULL)+999);

    int sock, port;
    struct sockaddr_in saddr;
    struct hostent *server;
    char buf[BUFFER_SIZE];

    printf("+=================================+\n");
    printf("|  GUESS THE COUNTRY - PLAYER 2   |\n");
    printf("+=================================+\n");

    sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock<0) die_with_error("socket() failed");
    server = gethostbyname(argv[1]);
    if (!server) die_with_error("No such host.");
    port = atoi(argv[2]);
    bzero((char*)&saddr,sizeof(saddr));
    saddr.sin_family = AF_INET;
    bcopy((char*)server->h_addr,(char*)&saddr.sin_addr.s_addr,server->h_length);
    saddr.sin_port = htons(port);
    printf("Connecting...\n");
    if (connect(sock,(struct sockaddr*)&saddr,sizeof(saddr))<0) die_with_error("connect() failed");
    printf("Connected! Game starting...\n");
    net_send(sock,"READY");

    char *easy[]={"United States","Philippines","Japan","France","Brazil",
                  "Australia","Germany","China","Canada","India"};
    char *med[]= {"Portugal","Argentina","Nigeria","Vietnam","Turkey",
                  "Poland","Morocco","Colombia","New Zealand","Ethiopia"};
    char *hard[]={"Djibouti","Eritrea","Haiti","Bhutan","Suriname",
                  "Kyrgyzstan","Vanuatu","Liechtenstein","Nauru","Moldova"};

    int mp,op,mt=0,ot=0;

    /* STAGE 1 */
    printf("\n+==============================+\n");
    printf("|  STAGE 1 - EASY   (10 pts)   |\n");
    printf("|  First correct guess wins!   |\n");
    printf("+==============================+\n");
    play_stage(sock,easy[rand()%10],10,0,mt,ot,&mp,&op);
    mt+=mp; ot+=op;
    printf("\n[Stage 1] You: %d pts | Player 1: %d pts\n",mp,op);
    printf("[Total]   You: %d pts | Player 1: %d pts\n",mt,ot);

    /* STAGE 2 */
    printf("\n+==============================+\n");
    printf("|  STAGE 2 - MEDIUM (20 pts)   |\n");
    printf("|  First correct guess wins!   |\n");
    printf("|  Hints available! (-5 pts)   |\n");
    printf("+==============================+\n");
    play_stage(sock,med[rand()%10],20,1,mt,ot,&mp,&op);
    mt+=mp; ot+=op;
    printf("\n[Stage 2] You: %d pts | Player 1: %d pts\n",mp,op);
    printf("[Total]   You: %d pts | Player 1: %d pts\n",mt,ot);

    /* STAGE 3 */
    printf("\n+==============================+\n");
    printf("|  STAGE 3 - HARD   (30 pts)   |\n");
    printf("|  First correct guess wins!   |\n");
    printf("|  Hints available! (-5 pts)   |\n");
    printf("+==============================+\n");
    play_stage(sock,hard[rand()%10],30,1,mt,ot,&mp,&op);
    mt+=mp; ot+=op;
    printf("\n[Stage 3] You: %d pts | Player 1: %d pts\n",mp,op);

    /* FINAL */
    printf("\n+==============================+\n");
    printf("|       FINAL SCORES           |\n");
    printf("+==============================+\n");
    printf("  Player 1 (Server): %d pts\n", ot);
    printf("  Player 2 (You)   : %d pts\n", mt);
    if      (mt > ot) printf("  *** PLAYER 2 WINS! ***\n");
    else if (ot > mt) printf("  *** PLAYER 1 WINS! ***\n");
    else              printf("  *** DRAW! ***\n");

    close(sock);
    return 0;
}
