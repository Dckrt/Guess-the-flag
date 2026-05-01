/*
 * flag_client.c
 * Guess the Flag — CLIENT (Player 2)
 * Compile: gcc -o flag_client flag_client.c -w
 * Run:     ./flag_client localhost 5001
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
#include <locale.h>

#define BUFFER_SIZE 2048
#define BOARD_SIZE  30
#define COLS        3

typedef struct {
    char name[32];
    char emoji[16];
    int red, blue, green, yellow, white, black, orange, purple;
    int has_star, has_stars;
    int has_stripes, has_cross;
    int has_symbol, has_crescent, has_sun, has_circle;
    int has_eagle, has_dragon, has_map_outline;
    int is_tricolor, is_bicolor, is_horizontal, is_vertical;
    int has_union_jack, has_text;
    int in_asia, in_europe, in_africa, in_americas, in_oceania;
} Country;

Country pool[] = {
{"Philippines",       "PH",  1,1,0,1, 1,0,0,0,  1,0, 0,0, 1,0,1,0, 0,0,0,  0,0,0,0, 0,0,  1,0,0,0,0},
{"United States",     "US",  1,1,0,0, 1,0,0,0,  0,1, 1,0, 0,0,0,0, 0,0,0,  0,0,1,0, 0,0,  0,0,0,1,0},
{"Japan",             "JP",  1,0,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,1, 0,0,0,  0,0,0,0, 0,0,  1,0,0,0,0},
{"China",             "CN",  1,0,0,1, 0,0,0,0,  0,1, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  1,0,0,0,0},
{"France",            "FR",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,0,1, 0,0,  0,1,0,0,0},
{"Germany",           "DE",  1,0,0,1, 0,1,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,1,0, 0,0,  0,1,0,0,0},
{"Brazil",            "BR",  0,1,1,1, 1,0,0,0,  1,0, 0,0, 1,0,0,1, 0,0,0,  0,0,0,0, 0,0,  0,0,0,1,0},
{"Canada",            "CA",  1,0,0,0, 1,0,0,0,  0,0, 1,0, 1,0,0,0, 0,0,0,  0,0,0,1, 0,0,  0,0,0,1,0},
{"Australia",         "AU",  1,1,0,0, 1,0,0,0,  0,1, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 1,0,  0,0,0,0,1},
{"India",             "IN",  1,1,1,0, 1,0,0,0,  0,0, 0,0, 1,0,0,1, 0,0,0,  1,0,1,0, 0,0,  1,0,0,0,0},
{"South Korea",       "KR",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 1,0,0,1, 0,0,0,  0,0,0,0, 0,0,  1,0,0,0,0},
{"Turkey",            "TR",  1,0,0,0, 1,0,0,0,  1,0, 0,1, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  0,1,0,0,0},
{"Mexico",            "MX",  1,0,1,0, 1,0,0,0,  0,0, 0,0, 1,0,0,0, 1,0,0,  1,0,0,1, 0,0,  0,0,0,1,0},
{"Italy",             "IT",  1,0,1,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,0,1, 0,0,  0,1,0,0,0},
{"Spain",             "ES",  1,0,0,1, 0,0,0,0,  0,0, 0,0, 1,0,0,0, 0,0,0,  0,0,1,0, 0,0,  0,1,0,0,0},
{"Argentina",         "AR",  0,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,1,0, 0,0,0,  0,0,1,0, 0,0,  0,0,0,1,0},
{"United Kingdom",    "GB",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 1,0,  0,1,0,0,0},
{"Portugal",          "PT",  1,0,1,1, 0,0,0,0,  0,0, 0,0, 1,0,0,0, 0,0,0,  0,0,0,1, 0,0,  0,1,0,0,0},
{"Netherlands",       "NL",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,1,0, 0,0,  0,1,0,0,0},
{"Poland",            "PL",  1,0,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,1,1,0, 0,0,  0,1,0,0,0},
{"Switzerland",       "CH",  1,0,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  0,1,0,0,0},
{"Saudi Arabia",      "SA",  1,0,1,0, 1,0,0,0,  0,0, 0,0, 1,0,0,0, 0,0,0,  0,0,0,0, 0,1,  1,0,0,0,0},
{"Pakistan",          "PK",  1,0,1,0, 1,0,0,0,  1,0, 0,1, 0,0,0,0, 0,0,0,  0,0,0,1, 0,0,  1,0,0,0,0},
{"Nigeria",           "NG",  0,0,1,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,1, 0,0,  0,0,1,0,0},
{"Egypt",             "EG",  1,0,0,1, 1,1,0,0,  0,0, 0,0, 1,0,0,0, 1,0,0,  1,0,1,0, 0,0,  0,0,1,0,0},
{"South Africa",      "ZA",  1,1,1,1, 1,1,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  0,0,1,0,0},
{"Kenya",             "KE",  1,0,1,0, 1,1,0,0,  0,0, 0,0, 1,0,0,0, 0,0,0,  1,0,1,0, 0,0,  0,0,1,0,0},
{"Morocco",           "MA",  1,0,1,0, 0,0,0,0,  1,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  0,0,1,0,0},
{"Sweden",            "SE",  0,1,0,1, 0,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  0,1,0,0,0},
{"Norway",            "NO",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  0,1,0,0,0},
{"Denmark",           "DK",  1,0,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  0,1,0,0,0},
{"Greece",            "GR",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,1,0, 0,0,  0,1,0,0,0},
{"Ukraine",           "UA",  0,1,0,1, 0,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,1,1,0, 0,0,  0,1,0,0,0},
{"Thailand",          "TH",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,0,1,0, 0,0,  1,0,0,0,0},
{"Vietnam",           "VN",  1,0,0,1, 0,0,0,0,  1,0, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 0,0,  1,0,0,0,0},
{"Indonesia",         "ID",  1,0,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  0,1,1,0, 0,0,  1,0,0,0,0},
{"Malaysia",          "MY",  1,1,0,1, 1,0,0,0,  0,0, 1,1, 0,0,0,0, 0,0,0,  0,0,1,0, 0,0,  1,0,0,0,0},
{"New Zealand",       "NZ",  1,1,0,0, 1,0,0,0,  0,1, 0,0, 0,0,0,0, 0,0,0,  0,0,0,0, 1,0,  0,0,0,0,1},
{"Ireland",           "IE",  1,0,1,1, 1,0,1,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,0,1, 0,0,  0,1,0,0,0},
{"Belgium",           "BE",  1,0,0,1, 0,1,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,0,1, 0,0,  0,1,0,0,0},
{"Russia",            "RU",  1,1,0,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,1,0, 0,0,  0,1,0,0,0},
{"Colombia",          "CO",  1,1,0,1, 0,0,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,1,0, 0,0,  0,0,0,1,0},
{"Chile",             "CL",  1,1,0,0, 1,0,0,0,  1,0, 0,0, 0,0,0,0, 0,0,0,  0,0,1,0, 0,0,  0,0,0,1,0},
{"Cuba",              "CU",  1,1,0,0, 1,0,0,0,  1,0, 1,0, 0,0,0,0, 0,0,0,  0,0,1,0, 0,0,  0,0,0,1,0},
{"Israel",            "IL",  0,1,0,0, 1,0,0,0,  1,0, 0,0, 1,0,0,0, 0,0,0,  0,0,1,0, 0,0,  0,0,0,0,0},
{"Iran",              "IR",  1,0,1,0, 1,0,0,0,  0,0, 0,0, 1,0,0,0, 0,0,0,  1,0,1,0, 0,1,  1,0,0,0,0},
{"Iraq",              "IQ",  1,0,1,0, 1,1,0,0,  0,0, 0,0, 0,0,0,0, 0,0,0,  1,0,1,0, 0,1,  1,0,0,0,0},
{"Ethiopia",          "ET",  1,0,1,1, 0,0,0,0,  1,0, 0,0, 1,0,0,0, 0,0,0,  1,0,1,0, 0,0,  0,0,1,0,0},
{"Ghana",             "GH",  1,0,1,1, 0,1,0,0,  1,0, 0,0, 0,0,0,0, 0,0,0,  1,0,1,0, 0,0,  0,0,1,0,0},
{"Wales",             "WA",  1,0,1,0, 1,0,0,0,  0,0, 0,0, 0,0,0,0, 0,1,0,  0,0,1,0, 0,0,  0,1,0,0,0},
};

char *questions[] = {
    "Does your flag have RED?",
    "Does your flag have BLUE?",
    "Does your flag have GREEN?",
    "Does your flag have YELLOW?",
    "Does your flag have WHITE?",
    "Does your flag have BLACK?",
    "Does your flag have ORANGE?",
    "Does your flag have a STAR? (one star)",
    "Does your flag have MULTIPLE STARS?",
    "Does your flag have STRIPES?",
    "Does your flag have a CROSS?",
    "Does your flag have a SYMBOL or EMBLEM?",
    "Does your flag have a CRESCENT or MOON?",
    "Does your flag have a SUN?",
    "Does your flag have a CIRCLE or DOT?",
    "Does your flag have an EAGLE?",
    "Does your flag have a DRAGON?",
    "Is your flag a TRICOLOR (3 equal bands)?",
    "Is your flag a BICOLOR (2 equal bands)?",
    "Are the bands HORIZONTAL?",
    "Are the bands VERTICAL?",
    "Does it have the UNION JACK in the corner?",
    "Does your flag have TEXT or WRITING?",
    "Is the country in ASIA?",
    "Is the country in EUROPE?",
    "Is the country in AFRICA?",
    "Is the country in the AMERICAS?",
    "Is the country in OCEANIA?",
};
#define NUM_Q 28

void die_with_error(char *m){printf("%s\n",m);exit(-1);}
void net_send(int s,const char *m){if(send(s,m,strlen(m),0)<0)die_with_error("send failed");}
int  net_recv(int s,char *b){bzero(b,BUFFER_SIZE);int n=recv(s,b,BUFFER_SIZE-1,0);if(n<0)die_with_error("recv failed");b[strcspn(b,"\r\n")]='\0';return n;}
void read_input(char *b){bzero(b,BUFFER_SIZE);fgets(b,BUFFER_SIZE-1,stdin);b[strcspn(b,"\r\n")]='\0';}

int answer_q(Country *c,const char *q){
    char lq[BUFFER_SIZE]; strncpy(lq,q,BUFFER_SIZE-1);
    for(int i=0;lq[i];i++) if(lq[i]>='A'&&lq[i]<='Z') lq[i]+=32;
    if(strstr(lq,"red"))      return c->red;
    if(strstr(lq,"blue"))     return c->blue;
    if(strstr(lq,"green"))    return c->green;
    if(strstr(lq,"yellow"))   return c->yellow;
    if(strstr(lq,"white"))    return c->white;
    if(strstr(lq,"black"))    return c->black;
    if(strstr(lq,"orange"))   return c->orange;
    if(strstr(lq,"multiple star")) return c->has_stars;
    if(strstr(lq,"star"))     return c->has_star||c->has_stars;
    if(strstr(lq,"strip"))    return c->has_stripes;
    if(strstr(lq,"cross"))    return c->has_cross;
    if(strstr(lq,"symbol")||strstr(lq,"emblem")||strstr(lq,"coat")) return c->has_symbol;
    if(strstr(lq,"crescent")||strstr(lq,"moon")) return c->has_crescent;
    if(strstr(lq,"sun"))      return c->has_sun;
    if(strstr(lq,"circle")||strstr(lq,"dot")) return c->has_circle;
    if(strstr(lq,"eagle"))    return c->has_eagle;
    if(strstr(lq,"dragon"))   return c->has_dragon;
    if(strstr(lq,"tricolor")||strstr(lq,"3 equal")||strstr(lq,"three equal")) return c->is_tricolor;
    if(strstr(lq,"bicolor")||strstr(lq,"2 equal")||strstr(lq,"two equal"))    return c->is_bicolor;
    if(strstr(lq,"horizontal")) return c->is_horizontal;
    if(strstr(lq,"vertical"))   return c->is_vertical;
    if(strstr(lq,"union jack")||strstr(lq,"uk flag")) return c->has_union_jack;
    if(strstr(lq,"text")||strstr(lq,"writing")||strstr(lq,"word")) return c->has_text;
    if(strstr(lq,"asia"))     return c->in_asia;
    if(strstr(lq,"europe"))   return c->in_europe;
    if(strstr(lq,"africa"))   return c->in_africa;
    if(strstr(lq,"america"))  return c->in_americas;
    if(strstr(lq,"oceania"))  return c->in_oceania;
    return -1;
}

void print_board(int *board,int *elim){
    printf("\n");
    printf("+==============================================================+\n");
    printf("|                      FLAG BOARD                             |\n");
    printf("+==============================================================+\n");
    for(int r=0;r<10;r++){
        printf("| ");
        for(int c=0;c<COLS;c++){
            int i=r*COLS+c;
            if(i>=BOARD_SIZE){printf("%-20s","");continue;}
            if(elim[i]) printf("%2d.[  %-10s  ]  ",i+1,"ELIMINATED");
            else        printf("%2d.[%s %-10s]  ",i+1,pool[board[i]].emoji,pool[board[i]].name);
        }
        printf("\n");
    }
    printf("+==============================================================+\n");
}

void print_menu(){
    printf("\n+----------------------------------------------------------+\n");
    printf("|                   QUESTION MENU                         |\n");
    printf("+----------------------------------------------------------+\n");
    printf("|  --- COLORS ---                                          |\n");
    printf("|   1. Does your flag have RED?                            |\n");
    printf("|   2. Does your flag have BLUE?                           |\n");
    printf("|   3. Does your flag have GREEN?                          |\n");
    printf("|   4. Does your flag have YELLOW?                         |\n");
    printf("|   5. Does your flag have WHITE?                          |\n");
    printf("|   6. Does your flag have BLACK?                          |\n");
    printf("|   7. Does your flag have ORANGE?                         |\n");
    printf("|  --- SYMBOLS ---                                         |\n");
    printf("|   8. Does your flag have a STAR? (one)                   |\n");
    printf("|   9. Does your flag have MULTIPLE STARS?                 |\n");
    printf("|  10. Does your flag have STRIPES?                        |\n");
    printf("|  11. Does your flag have a CROSS?                        |\n");
    printf("|  12. Does your flag have a SYMBOL or EMBLEM?             |\n");
    printf("|  13. Does your flag have a CRESCENT or MOON?             |\n");
    printf("|  14. Does your flag have a SUN?                          |\n");
    printf("|  15. Does your flag have a CIRCLE or DOT?                |\n");
    printf("|  16. Does your flag have an EAGLE?                       |\n");
    printf("|  17. Does your flag have a DRAGON?                       |\n");
    printf("|  --- DESIGN ---                                          |\n");
    printf("|  18. Is your flag a TRICOLOR? (3 equal bands)            |\n");
    printf("|  19. Is your flag a BICOLOR? (2 equal bands)             |\n");
    printf("|  20. Are the bands HORIZONTAL?                           |\n");
    printf("|  21. Are the bands VERTICAL?                             |\n");
    printf("|  22. Does it have the UNION JACK in the corner?          |\n");
    printf("|  23. Does your flag have TEXT or WRITING?                |\n");
    printf("|  --- LOCATION ---                                        |\n");
    printf("|  24. Is the country in ASIA?                             |\n");
    printf("|  25. Is the country in EUROPE?                           |\n");
    printf("|  26. Is the country in AFRICA?                           |\n");
    printf("|  27. Is the country in the AMERICAS?                     |\n");
    printf("|  28. Is the country in OCEANIA?                          |\n");
    printf("|  --                                                      |\n");
    printf("|   0. Type a CUSTOM question                              |\n");
    printf("|  99. GUESS the country name                              |\n");
    printf("+----------------------------------------------------------+\n");
    printf("Enter number: ");
    fflush(stdout);
}

int main(int argc,char *argv[]){
    if(argc<3){printf("Usage: %s host port\n",argv[0]);exit(1);}
    setlocale(LC_ALL,"");

    int sock,port;
    struct sockaddr_in saddr;
    struct hostent *server;
    char buf[BUFFER_SIZE],out[BUFFER_SIZE],question[BUFFER_SIZE];

    printf("+=================================+\n");
    printf("|    GUESS THE FLAG - PLAYER 2    |\n");
    printf("+=================================+\n");

    sock=socket(AF_INET,SOCK_STREAM,0);
    if(sock<0)die_with_error("socket() failed");
    server=gethostbyname(argv[1]);
    if(!server)die_with_error("No such host.");
    port=atoi(argv[2]);
    bzero((char*)&saddr,sizeof(saddr));
    saddr.sin_family=AF_INET;
    bcopy((char*)server->h_addr,(char*)&saddr.sin_addr.s_addr,server->h_length);
    saddr.sin_port=htons(port);
    printf("Connecting...\n");
    if(connect(sock,(struct sockaddr*)&saddr,sizeof(saddr))<0)die_with_error("connect() failed");
    printf("Connected!\n\n");
    net_send(sock,"READY");

    /* Receive board */
    net_recv(sock,buf);
    int board[BOARD_SIZE];
    char *tok=strtok(buf,",");
    for(int i=0;i<BOARD_SIZE&&tok;i++){board[i]=atoi(tok);tok=strtok(NULL,",");}
    net_send(sock,"ACK");

    /* Receive assigned country */
    net_recv(sock,buf);
    char *p=strstr(buf,"|"); if(p)p++;
    int my_c=p?atoi(p):0;
    net_send(sock,"ACK");

    printf("Your secret flag (Player 1 must guess): [%s] %s\n",
        pool[my_c].emoji,pool[my_c].name);
    printf("Player 1's flag: [HIDDEN]\n\n");

    int my_elim[BOARD_SIZE]={0};
    int game_over=0,my_skip=0;

    print_board(board,my_elim);

    while(!game_over){
        net_recv(sock,buf);

        /* SERVER ASKING */
        if(strncmp(buf,"SERVER_QUESTION|",16)==0){
            char *q=buf+16;
            printf("\n[server] > QUESTION: %s\n",q);
            printf("Your secret: [%s] %s | Answer (YES/NO): ",
                pool[my_c].emoji,pool[my_c].name);
            fflush(stdout);
            read_input(buf);
            for(int i=0;buf[i];i++) if(buf[i]>='a'&&buf[i]<='z') buf[i]-=32;
            snprintf(out,BUFFER_SIZE,"ANSWER|%s",buf);
            net_send(sock,out);

        /* SERVER GUESSING */
        } else if(strncmp(buf,"SERVER_GUESS|",13)==0){
            char *g=buf+13;
            printf("\n[server] > GUESS: %s\n",g);
            if(strcasecmp(g,pool[my_c].name)==0){
                printf("[Server CORRECT!] Player 1 wins!\n");
                net_send(sock,"GUESS_RESULT|correct");
                net_recv(sock,buf);
                printf("\n*** GAME OVER — Player 1 wins! ***\n");
                game_over=1;
            } else {
                printf("[Server WRONG] They lose next turn.\n");
                net_send(sock,"GUESS_RESULT|wrong");
                net_recv(sock,buf);
                net_send(sock,"ACK");
            }

        /* SERVER SKIP */
        } else if(strncmp(buf,"SERVER_SKIP",11)==0){
            printf("\n[Server skipping — wrong guess penalty]\n");
            net_send(sock,"ACK");

        /* MY TURN */
        } else if(strncmp(buf,"CLIENT_TURN",11)==0){
            if(my_skip){
                printf("\n[YOUR TURN IS SKIPPED — wrong guess penalty]\n");
                my_skip=0;
                net_send(sock,"CLIENT_SKIP");
                net_recv(sock,buf);
                continue;
            }

            printf("\n+----------------------------------+\n");
            printf("|           YOUR TURN              |\n");
            printf("+----------------------------------+\n");
            print_menu();
            read_input(buf);
            int choice=atoi(buf);

            if(choice==99){
                printf("Enter country name:\n> "); fflush(stdout);
                read_input(buf);
                snprintf(out,BUFFER_SIZE,"CLIENT_GUESS|%s",buf);
                net_send(sock,out);
                net_recv(sock,buf);
                char *res=strstr(buf,"|"); if(res)res++;
                if(res&&strcmp(res,"correct")==0){
                    printf("\n*** CORRECT! YOU WIN! ***\n");
                    net_send(sock,"ACK");
                    net_recv(sock,buf);
                    game_over=1;
                } else {
                    printf("[WRONG] You lose your next turn.\n");
                    my_skip=1;
                    net_send(sock,"ACK");
                }
            } else {
                if(choice==0){
                    printf("Type your question:\n> "); fflush(stdout);
                    read_input(question);
                } else if(choice>=1&&choice<=NUM_Q){
                    strncpy(question,questions[choice-1],BUFFER_SIZE-1);
                    printf("Asking: %s\n",question);
                } else {
                    printf("Invalid. Try again.\n");
                    /* still need to send something to server to avoid deadlock */
                    /* re-request turn by sending a dummy that server ignores */
                    /* actually just continue and wait for next CLIENT_TURN */
                    net_send(sock,"CLIENT_QUESTION|invalid");
                    net_recv(sock,buf);
                    continue;
                }
                snprintf(out,BUFFER_SIZE,"CLIENT_QUESTION|%s",question);
                net_send(sock,out);
                net_recv(sock,buf);
                char *ans=strstr(buf,"|"); if(ans)ans++;
                printf("[server] > %s\n",ans?ans:buf);
                if(ans){
                    int is_yes=(strcasecmp(ans,"YES")==0);
                    int ec=0;
                    for(int i=0;i<BOARD_SIZE;i++){
                        if(!my_elim[i]){
                            int m=answer_q(&pool[board[i]],question);
                            if(m>=0&&((is_yes&&m==0)||(!is_yes&&m==1))){my_elim[i]=1;ec++;}
                        }
                    }
                    printf("(%d eliminated)\n",ec);
                    print_board(board,my_elim);
                }
            }

        /* GAME OVER */
        } else if(strncmp(buf,"GAME_OVER|",10)==0){
            char *w=buf+10;
            if(strcmp(w,"client_wins")==0)
                printf("\n*** GAME OVER — YOU WIN! ***\n");
            else
                printf("\n*** GAME OVER — Player 1 wins! ***\n");
            net_send(sock,"ACK");
            game_over=1;
        }
    }

    printf("\nThanks for playing!\n");
    close(sock);
    return 0;
}
