/*
 * le9_server.c
 * Guess the Country - SERVER (Player 1)
 * Compile: gcc -o le9_server le9_server.c -w
 * Run:     ./le9_server 5001
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
#define HINT_COST   5

typedef struct { char name[64]; char continent[32]; } Country;

Country easy_pool[] = {
    {"United States","Americas"},{"Philippines","Asia"},{"Japan","Asia"},
    {"France","Europe"},{"Brazil","Americas"},{"Australia","Oceania"},
    {"Germany","Europe"},{"China","Asia"},{"Canada","Americas"},{"India","Asia"},
};
Country medium_pool[] = {
    {"Portugal","Europe"},{"Argentina","Americas"},{"Nigeria","Africa"},
    {"Vietnam","Asia"},{"Turkey","Asia"},{"Poland","Europe"},
    {"Morocco","Africa"},{"Colombia","Americas"},{"New Zealand","Oceania"},{"Ethiopia","Africa"},
};
Country hard_pool[] = {
    {"Djibouti","Africa"},{"Eritrea","Africa"},{"Haiti","Americas"},
    {"Bhutan","Asia"},{"Suriname","Americas"},{"Kyrgyzstan","Asia"},
    {"Vanuatu","Oceania"},{"Liechtenstein","Europe"},{"Nauru","Oceania"},{"Moldova","Europe"},
};

void die_with_error(char *m){printf("%s\n",m);exit(-1);}
void net_send(int s,const char *m){if(send(s,m,strlen(m),0)<0)die_with_error("send failed");}
int  net_recv(int s,char *b){bzero(b,BUFFER_SIZE);int n=recv(s,b,BUFFER_SIZE-1,0);if(n<0)die_with_error("recv failed");b[strcspn(b,"\r\n")]='\0';return n;}
void read_input(char *b){bzero(b,BUFFER_SIZE);fgets(b,BUFFER_SIZE-1,stdin);b[strcspn(b,"\r\n")]='\0';}
Country pick(Country *p,int sz,int ex){int i;do{i=rand()%sz;}while(i==ex);return p[i];}

/*
 * play_stage()
 *   mine      = server's country (client must guess)
 *   opp       = client's country (server must guess)
 *   max_q     = max questions per player
 *   stage_pts = base points for correct guess
 *   hints     = 1 if hints allowed
 *   s_total   = server running total (for hint check)
 *   c_total   = client running total (for hint check)
 *
 * Returns points earned this stage for each player.
 * Stage ends as soon as EITHER player guesses correctly.
 * Wrong guess = opponent +5, game continues.
 */
void play_stage(int sock,
                Country *mine, Country *opp,
                int max_q, int stage_pts, int hints,
                int s_total, int c_total,
                int *s_pts_out, int *c_pts_out)
{
    char buf[BUFFER_SIZE], out[BUFFER_SIZE];
    int sq = max_q, cq = max_q;
    int s_pts = 0, c_pts = 0;
    int s_hint = 0, c_hint = 0;
    int stage_over = 0;

    /* Send stage info to client */
    snprintf(out, BUFFER_SIZE, "STAGE_INFO|%d|%s|%s|%d|%d|%d",
        stage_pts, mine->name, mine->continent,
        hints, s_total, c_total);
    net_send(sock, out);
    net_recv(sock, buf); /* ACK */

    while (!stage_over) {

        /* ══ SERVER TURN ══ */
        printf("\n+--------------------------------------+\n");
        printf("|  YOUR TURN  |  Questions left: %2d   |\n", sq);
        printf("|  Total pts: %3d                      |\n", s_total + s_pts);
        printf("+--------------------------------------+\n");
        if (hints && !s_hint)
            printf("  Type HINT (-5 pts) to reveal opponent's continent\n");
        printf("  Ask a question or type GUESS");
        if (hints && !s_hint) printf(" / HINT");
        printf(":\n< "); fflush(stdout);
        read_input(buf);

        /* HINT */
        if (hints && !s_hint && strcasecmp(buf,"HINT")==0) {
            if ((s_total+s_pts) < HINT_COST) {
                printf("Not enough points! Need %d pts.\n", HINT_COST);
            } else {
                s_pts -= HINT_COST; s_hint = 1;
                printf("[HINT] Opponent's country is in: %s\n", opp->continent);
                net_send(sock, "SERVER_HINT|ok");
                net_recv(sock, buf);
            }
            continue;
        }

        /* GUESS */
        if (strcasecmp(buf,"GUESS")==0 || sq==0) {
            if (sq==0) printf("No questions left! You must guess:\n");
            else       printf("Enter your guess:\n");
            printf("< "); fflush(stdout);
            read_input(buf);
            snprintf(out, BUFFER_SIZE, "SERVER_GUESS|%s", buf);
            net_send(sock, out);
            net_recv(sock, buf);
            char *res = strstr(buf,"|"); if(res) res++;
            if (res && strcmp(res,"correct")==0) {
                int bonus = sq*2;
                printf("[CORRECT!] +%d pts + %d bonus = +%d\n", stage_pts, bonus, stage_pts+bonus);
                s_pts += stage_pts + bonus;
                net_send(sock, "ACK");
                stage_over = 1; /* stage ends here */
            } else {
                printf("[WRONG!] Their country: %s. Opponent +5.\n", opp->name);
                c_pts += 5;
                net_send(sock, "ACK");
                /* stage continues */
            }

        /* QUESTION */
        } else if (sq > 0) {
            snprintf(out, BUFFER_SIZE, "SERVER_QUESTION|%s", buf);
            net_send(sock, out); sq--;
            net_recv(sock, buf);
            char *ans = strstr(buf,"|"); if(ans) ans++;
            printf("[client] > %s\n", ans?ans:buf);
        }

        if (stage_over) break;

        /* ══ CLIENT TURN ══ */
        snprintf(out, BUFFER_SIZE, "CLIENT_TURN|%d", cq);
        net_send(sock, out);
        net_recv(sock, buf);

        if (strncmp(buf,"CLIENT_HINT|",12)==0) {
            if (!c_hint && (c_total+c_pts)>=HINT_COST) {
                c_pts -= HINT_COST; c_hint = 1;
                snprintf(out, BUFFER_SIZE, "HINT_RESULT|%s", mine->continent);
                net_send(sock, out);
                printf("[Client bought a hint]\n");
            } else {
                net_send(sock, "HINT_RESULT|denied");
            }

        } else if (strncmp(buf,"CLIENT_QUESTION|",16)==0) {
            char *q = buf+16; cq--;
            printf("\n[client] > %s\n", q);
            printf("Your country: %s | Answer (YES/NO): ", mine->name);
            fflush(stdout);
            read_input(buf);
            for(int i=0;buf[i];i++) if(buf[i]>='a'&&buf[i]<='z') buf[i]-=32;
            snprintf(out, BUFFER_SIZE, "ANSWER|%s", buf);
            net_send(sock, out);

        } else if (strncmp(buf,"CLIENT_GUESS|",13)==0) {
            char *g = buf+13;
            printf("\n[client] > GUESS: %s\n", g);
            if (strcasecmp(g, mine->name)==0) {
                int bonus = cq*2;
                printf("[Client CORRECT!] Opponent +%d+%d bonus\n", stage_pts, bonus);
                c_pts += stage_pts + bonus;
                net_send(sock, "GUESS_RESULT|correct");
                stage_over = 1; /* stage ends here */
            } else {
                printf("[Client WRONG!] You get +5\n");
                s_pts += 5;
                net_send(sock, "GUESS_RESULT|wrong");
                /* stage continues */
            }
        }
    }

    /* Send stage result to client */
    snprintf(out, BUFFER_SIZE, "STAGE_RESULT|%d|%d|%s|%s",
        s_pts, c_pts, mine->name, opp->name);
    net_send(sock, out);
    net_recv(sock, buf); /* ACK */

    *s_pts_out = s_pts;
    *c_pts_out = c_pts;
}

int main(int argc, char *argv[]) {
    if (argc<2){printf("Usage: %s port\n",argv[0]);exit(1);}
    srand(time(NULL));

    int ssock, csock, port, csz;
    struct sockaddr_in saddr, caddr;
    char buf[BUFFER_SIZE];

    printf("+=================================+\n");
    printf("|  GUESS THE COUNTRY - PLAYER 1   |\n");
    printf("+=================================+\n");

    ssock = socket(AF_INET,SOCK_STREAM,0);
    if (ssock<0) die_with_error("socket() failed");
    int opt=1; setsockopt(ssock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    bzero((char*)&saddr,sizeof(saddr));
    port = atoi(argv[1]);
    saddr.sin_family      = AF_INET;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_port        = htons(port);
    if (bind(ssock,(struct sockaddr*)&saddr,sizeof(saddr))<0) die_with_error("bind() failed");
    listen(ssock,1);
    printf("Waiting for Player 2 on port %d...\n", port);
    csz = sizeof(caddr);
    csock = accept(ssock,(struct sockaddr*)&caddr,&csz);
    if (csock<0) die_with_error("accept() failed");
    printf("Player 2 connected! Game starting...\n\n");
    net_recv(csock, buf);

    int ec=sizeof(easy_pool)/sizeof(Country);
    int mc=sizeof(medium_pool)/sizeof(Country);
    int hc=sizeof(hard_pool)/sizeof(Country);
    int s=0,c=0,st=0,ct=0,idx;

    /* ── STAGE 1: EASY ── */
    printf("+==============================+\n");
    printf("|  STAGE 1 - EASY   (10 pts)   |\n");
    printf("|  First correct guess wins!   |\n");
    printf("+==============================+\n");
    idx = rand()%ec;
    Country e1=easy_pool[idx], e2=pick(easy_pool,ec,idx);
    printf("Your country (P2 must guess): %s\n", e1.name);
    printf("Player 2's country: [HIDDEN]\n\n");
    play_stage(csock,&e1,&e2,5,10,0,st,ct,&s,&c);
    st+=s; ct+=c;
    printf("\n[Stage 1] You: %d pts | Player 2: %d pts\n",s,c);
    printf("[Total]   You: %d pts | Player 2: %d pts\n",st,ct);

    /* ── STAGE 2: MEDIUM ── */
    printf("\n+==============================+\n");
    printf("|  STAGE 2 - MEDIUM (20 pts)   |\n");
    printf("|  First correct guess wins!   |\n");
    printf("|  Hints available! (-5 pts)   |\n");
    printf("+==============================+\n");
    idx = rand()%mc;
    Country m1=medium_pool[idx], m2=pick(medium_pool,mc,idx);
    printf("Your country (P2 must guess): %s\n", m1.name);
    printf("Player 2's country: [HIDDEN]\n\n");
    play_stage(csock,&m1,&m2,8,20,1,st,ct,&s,&c);
    st+=s; ct+=c;
    printf("\n[Stage 2] You: %d pts | Player 2: %d pts\n",s,c);
    printf("[Total]   You: %d pts | Player 2: %d pts\n",st,ct);

    /* ── STAGE 3: HARD ── */
    printf("\n+==============================+\n");
    printf("|  STAGE 3 - HARD   (30 pts)   |\n");
    printf("|  First correct guess wins!   |\n");
    printf("|  Hints available! (-5 pts)   |\n");
    printf("+==============================+\n");
    idx = rand()%hc;
    Country h1=hard_pool[idx], h2=pick(hard_pool,hc,idx);
    printf("Your country (P2 must guess): %s\n", h1.name);
    printf("Player 2's country: [HIDDEN]\n\n");
    play_stage(csock,&h1,&h2,10,30,1,st,ct,&s,&c);
    st+=s; ct+=c;
    printf("\n[Stage 3] You: %d pts | Player 2: %d pts\n",s,c);

    /* ── FINAL RESULT ── */
    printf("\n+==============================+\n");
    printf("|       FINAL SCORES           |\n");
    printf("+==============================+\n");
    printf("  Player 1 (You)   : %d pts\n", st);
    printf("  Player 2 (Client): %d pts\n", ct);
    if      (st > ct) printf("  *** PLAYER 1 WINS! ***\n");
    else if (ct > st) printf("  *** PLAYER 2 WINS! ***\n");
    else              printf("  *** DRAW! ***\n");

    close(csock); close(ssock);
    return 0;
}
