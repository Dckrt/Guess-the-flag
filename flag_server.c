/*
 * flag_server.c
 * Guess the Flag — SERVER (Player 1)
 * Compile: gcc -o flag_server flag_server.c -w
 * Run:     ./flag_server 5001
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <time.h>
#include <locale.h>

#define BUFFER_SIZE 2048
#define BOARD_SIZE  30
#define COLS        3

typedef struct {
    char name[32];
    char emoji[16];
    /* Colors */
    int red, blue, green, yellow, white, black, orange, purple;
    /* Design elements */
    int has_star, has_stars;      /* one star / multiple stars */
    int has_stripes, has_cross;
    int has_symbol;               /* coat of arms / emblem */
    int has_crescent;
    int has_sun;
    int has_circle;               /* circle/dot design */
    int has_eagle;
    int has_dragon;
    int has_map_outline;
    int is_tricolor;              /* 3 equal bands */
    int is_bicolor;               /* 2 equal bands */
    int is_horizontal;            /* horizontal stripes/bands */
    int is_vertical;              /* vertical stripes/bands */
    int has_union_jack;           /* UK flag in corner */
    int has_text;                 /* text/writing on flag */
    int in_asia, in_europe, in_africa, in_americas, in_oceania;
} Country;

Country pool[] = {
/*         name              emoji   r  b  g  y  w  bk or pu  1* **  st cr sy  mo  su  ci  ea  dr  mp  tc  bc  hz  vt  uj  tx  As  Eu  Af  Am  Oc */
{"Philippines",       "🇵🇭",  1, 1, 0, 1, 1, 0, 0, 0,  1, 0, 0, 0, 1, 0, 1, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  1, 0, 0, 0, 0},
{"United States",     "🇺🇸",  1, 1, 0, 0, 1, 0, 0, 0,  0, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 0, 0, 1, 0},
{"Japan",             "🇯🇵",  1, 0, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0,  1, 0, 0, 0, 0},
{"China",             "🇨🇳",  1, 0, 0, 1, 0, 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  1, 0, 0, 0, 0},
{"France",            "🇫🇷",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 0, 1, 0, 0,  0, 1, 0, 0, 0},
{"Germany",           "🇩🇪",  1, 0, 0, 1, 0, 1, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0},
{"Brazil",            "🇧🇷",  0, 1, 1, 1, 1, 0, 0, 0,  1, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 0},
{"Canada",            "🇨🇦",  1, 0, 0, 0, 1, 0, 0, 0,  0, 0, 1, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 0, 0,  0, 0, 0, 1, 0},
{"Australia",         "🇦🇺",  1, 1, 0, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 1, 0,  0, 0, 0, 0, 1},
{"India",             "🇮🇳",  1, 1, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0,  1, 0, 1, 0, 0, 0,  1, 0, 0, 0, 0},
{"South Korea",       "🇰🇷",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0,  1, 0, 0, 0, 0},
{"Turkey",            "🇹🇷",  1, 0, 0, 0, 1, 0, 0, 0,  1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 1, 0, 0, 0},
{"Mexico",            "🇲🇽",  1, 0, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,  1, 0, 0, 1, 0, 0,  0, 0, 0, 1, 0},
{"Italy",             "🇮🇹",  1, 0, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 0, 1, 0, 0,  0, 1, 0, 0, 0},
{"Spain",             "🇪🇸",  1, 0, 0, 1, 0, 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0},
{"Argentina",         "🇦🇷",  0, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 0, 0, 1, 0},
{"United Kingdom",    "🇬🇧",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 1, 0,  0, 1, 0, 0, 0},
{"Portugal",          "🇵🇹",  1, 0, 1, 1, 0, 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 0, 0,  0, 1, 0, 0, 0},
{"Netherlands",       "🇳🇱",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0},
{"Poland",            "🇵🇱",  1, 0, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 1, 1, 0, 0, 0,  0, 1, 0, 0, 0},
{"Switzerland",       "🇨🇭",  1, 0, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 1, 0, 0, 0},
{"Saudi Arabia",      "🇸🇦",  1, 0, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 1,  1, 0, 0, 0, 0},
{"Pakistan",          "🇵🇰",  1, 0, 1, 0, 1, 0, 0, 0,  1, 0, 0, 1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 0, 0,  1, 0, 0, 0, 0},
{"Nigeria",           "🇳🇬",  0, 0, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 1, 0, 0,  0, 0, 1, 0, 0},
{"Egypt",             "🇪🇬",  1, 0, 0, 1, 1, 1, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 1, 0, 0,  1, 0, 1, 0, 0, 0,  0, 0, 1, 0, 0},
{"South Africa",      "🇿🇦",  1, 1, 1, 1, 1, 1, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0},
{"Kenya",             "🇰🇪",  1, 0, 1, 0, 1, 1, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 0,  0, 0, 1, 0, 0},
{"Morocco",           "🇲🇦",  1, 0, 1, 0, 0, 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0},
{"Sweden",            "🇸🇪",  0, 1, 0, 1, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 1, 0, 0, 0},
{"Norway",            "🇳🇴",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 1, 0, 0, 0},
{"Denmark",           "🇩🇰",  1, 0, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 1, 0, 0, 0},
{"Greece",            "🇬🇷",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0},
{"Ukraine",           "🇺🇦",  0, 1, 0, 1, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 1, 1, 0, 0, 0,  0, 1, 0, 0, 0},
{"Thailand",          "🇹🇭",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  1, 0, 0, 0, 0},
{"Vietnam",           "🇻🇳",  1, 0, 0, 1, 0, 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  1, 0, 0, 0, 0},
{"Indonesia",         "🇮🇩",  1, 0, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 1, 1, 0, 0, 0,  1, 0, 0, 0, 0},
{"Malaysia",          "🇲🇾",  1, 1, 0, 1, 1, 0, 0, 0,  0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  1, 0, 0, 0, 0},
{"New Zealand",       "🇳🇿",  1, 1, 0, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 1, 0,  0, 0, 0, 0, 1},
{"Ireland",           "🇮🇪",  1, 0, 1, 1, 1, 0, 1, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 0, 1, 0, 0,  0, 1, 0, 0, 0},
{"Belgium",           "🇧🇪",  1, 0, 0, 1, 0, 1, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 0, 1, 0, 0,  0, 1, 0, 0, 0},
{"Russia",            "🇷🇺",  1, 1, 0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0},
{"Colombia",          "🇨🇴",  1, 1, 0, 1, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 0,  0, 0, 0, 1, 0},
{"Chile",             "🇨🇱",  1, 1, 0, 0, 1, 0, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 0, 0, 1, 0},
{"Cuba",              "🇨🇺",  1, 1, 0, 0, 1, 0, 0, 0,  1, 0, 1, 0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 0, 0, 1, 0},
{"Israel",            "🇮🇱",  0, 1, 0, 0, 1, 0, 0, 0,  1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  0, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0},
{"Iran",              "🇮🇷",  1, 0, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 1,  1, 0, 0, 0, 0},
{"Iraq",              "🇮🇶",  1, 0, 1, 0, 1, 1, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 1,  1, 0, 0, 0, 0},
{"Ethiopia",          "🇪🇹",  1, 0, 1, 1, 0, 0, 0, 0,  1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 0,  0, 0, 1, 0, 0},
{"Ghana",             "🇬🇭",  1, 0, 1, 1, 0, 1, 0, 0,  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,  1, 0, 1, 0, 0, 0,  0, 0, 1, 0, 0},
{"Wales",             "🏴󠁧󠁢󠁷󠁬󠁳󠁿",  1, 0, 1, 0, 1, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0,  0, 0, 1, 0, 0, 0,  0, 1, 0, 0, 0},
};
#define POOL_SIZE (sizeof(pool)/sizeof(Country))

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
    "Does your flag have a CIRCLE or DOT design?",
    "Does your flag have an EAGLE?",
    "Does your flag have a DRAGON?",
    "Is your flag a TRICOLOR (3 equal bands)?",
    "Is your flag a BICOLOR (2 equal bands)?",
    "Are the bands/stripes HORIZONTAL?",
    "Are the bands/stripes VERTICAL?",
    "Does your flag have the UNION JACK (UK flag) in the corner?",
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
void shuffle(int *a,int n){for(int i=n-1;i>0;i--){int j=rand()%(i+1);int t=a[i];a[i]=a[j];a[j]=t;}}

int answer_q(Country *c, const char *q){
    char lq[BUFFER_SIZE]; strncpy(lq,q,BUFFER_SIZE-1);
    for(int i=0;lq[i];i++) if(lq[i]>='A'&&lq[i]<='Z') lq[i]+=32;
    if(strstr(lq,"red"))      return c->red;
    if(strstr(lq,"blue"))     return c->blue;
    if(strstr(lq,"green"))    return c->green;
    if(strstr(lq,"yellow"))   return c->yellow;
    if(strstr(lq,"white"))    return c->white;
    if(strstr(lq,"black"))    return c->black;
    if(strstr(lq,"orange"))   return c->orange;
    if(strstr(lq,"multiple star")||strstr(lq,"multiple star")) return c->has_stars;
    if(strstr(lq,"star"))     return c->has_star || c->has_stars;
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

void print_board(int *board, int *elim){
    int remaining=0;
    for(int i=0;i<BOARD_SIZE;i++) if(!elim[i]) remaining++;
    printf("\n");
    printf("+----+------------------+----+------------------+----+------------------+----+------------------+----+------------------+\n");
    printf("| ## |     COUNTRY      | ## |     COUNTRY      | ## |     COUNTRY      | ## |     COUNTRY      | ## |     COUNTRY      |\n");
    printf("+----+------------------+----+------------------+----+------------------+----+------------------+----+------------------+\n");
    for(int r=0;r<6;r++){
        printf("|");
        for(int c=0;c<5;c++){
            int i=r*5+c;
            if(i>=BOARD_SIZE){ printf("    |                  |"); continue; }
            if(elim[i]) printf(" %2d | ~~~ REMOVED ~~~  |",i+1);
            else        printf(" %2d | %-16s |",i+1,pool[board[i]].name);
        }
        printf("\n");
    }
    printf("+----+------------------+----+------------------+----+------------------+----+------------------+----+------------------+\n");
    printf("  Remaining: %d / %d flags\n",remaining,BOARD_SIZE);
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
    if(argc<2){printf("Usage: %s port\n",argv[0]);exit(1);}
    setlocale(LC_ALL,"");
    srand(time(NULL));

    int ssock,csock,port,csz;
    struct sockaddr_in saddr,caddr;
    char buf[BUFFER_SIZE],out[BUFFER_SIZE],question[BUFFER_SIZE];

    printf("+=================================+\n");
    printf("|    GUESS THE FLAG - PLAYER 1    |\n");
    printf("+=================================+\n");

    ssock=socket(AF_INET,SOCK_STREAM,0);
    if(ssock<0)die_with_error("socket() failed");
    int opt=1; setsockopt(ssock,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    bzero((char*)&saddr,sizeof(saddr));
    port=atoi(argv[1]);
    saddr.sin_family=AF_INET;
    saddr.sin_addr.s_addr=INADDR_ANY;
    saddr.sin_port=htons(port);
    if(bind(ssock,(struct sockaddr*)&saddr,sizeof(saddr))<0)die_with_error("bind() failed");
    listen(ssock,1);
    printf("Waiting for Player 2 on port %d...\n",port);
    csz=sizeof(caddr);
    csock=accept(ssock,(struct sockaddr*)&caddr,&csz);
    if(csock<0)die_with_error("accept() failed");
    printf("Player 2 connected!\n\n");
    net_recv(csock,buf);

    /* Pick 30 random countries */
    int all[POOL_SIZE];
    for(int i=0;i<(int)POOL_SIZE;i++) all[i]=i;
    shuffle(all,(int)POOL_SIZE);
    int board[BOARD_SIZE];
    for(int i=0;i<BOARD_SIZE;i++) board[i]=all[i];

    /* Send board to client */
    out[0]='\0';
    for(int i=0;i<BOARD_SIZE;i++){
        char tmp[8]; sprintf(tmp,"%d",board[i]);
        strcat(out,tmp);
        if(i<BOARD_SIZE-1) strcat(out,",");
    }
    net_send(csock,out);
    net_recv(csock,buf);

    /* Assign secret countries */
    int si=rand()%BOARD_SIZE, ci=rand()%BOARD_SIZE;
    while(ci==si) ci=rand()%BOARD_SIZE;
    int my_c=board[si], opp_c=board[ci];

    snprintf(out,BUFFER_SIZE,"ASSIGN|%d",opp_c);
    net_send(csock,out);
    net_recv(csock,buf);

    printf("Your secret flag (P2 must guess): %s %s\n",pool[my_c].emoji,pool[my_c].name);
    printf("P2's secret flag (you must guess): [HIDDEN]\n\n");

    int my_elim[BOARD_SIZE]={0};
    int game_over=0,my_skip=0;

    print_board(board,my_elim);

    while(!game_over){

        /* ══ SERVER TURN ══ */
        if(my_skip){
            printf("\n[YOUR TURN IS SKIPPED — wrong guess penalty]\n");
            my_skip=0;
            net_send(csock,"SERVER_SKIP");
            net_recv(csock,buf);
            goto client_turn;
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
            snprintf(out,BUFFER_SIZE,"SERVER_GUESS|%s",buf);
            net_send(csock,out);
            net_recv(csock,buf);
            char *res=strstr(buf,"|"); if(res)res++;
            if(res&&strcmp(res,"correct")==0){
                printf("\n*** CORRECT! YOU WIN! ***\n");
                net_send(csock,"GAME_OVER|server_wins");
                net_recv(csock,buf);
                game_over=1;
            } else {
                printf("[WRONG] You lose your next turn.\n");
                my_skip=1;
                net_send(csock,"PENALTY_ACK");
                net_recv(csock,buf);
            }
        } else {
            if(choice==0){
                printf("Type your question:\n> "); fflush(stdout);
                read_input(question);
            } else if(choice>=1&&choice<=NUM_Q){
                strncpy(question,questions[choice-1],BUFFER_SIZE-1);
                printf("Asking: %s\n",question);
            } else {
                printf("Invalid. Try again.\n"); continue;
            }
            snprintf(out,BUFFER_SIZE,"SERVER_QUESTION|%s",question);
            net_send(csock,out);
            net_recv(csock,buf);
            char *ans=strstr(buf,"|"); if(ans)ans++;
            printf("[client] > %s\n",ans?ans:buf);
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

        if(game_over) break;

        client_turn:
        net_send(csock,"CLIENT_TURN");
        net_recv(csock,buf);

        if(strncmp(buf,"CLIENT_QUESTION|",16)==0){
            char *q=buf+16;
            printf("\n[client] > QUESTION: %s\n",q);
            printf("Your secret: %s %s | Answer (YES/NO): ",pool[my_c].emoji,pool[my_c].name);
            fflush(stdout);
            read_input(buf);
            for(int i=0;buf[i];i++) if(buf[i]>='a'&&buf[i]<='z') buf[i]-=32;
            snprintf(out,BUFFER_SIZE,"ANSWER|%s",buf);
            net_send(csock,out);
        } else if(strncmp(buf,"CLIENT_GUESS|",13)==0){
            char *g=buf+13;
            printf("\n[client] > GUESS: %s\n",g);
            if(strcasecmp(g,pool[my_c].name)==0){
                printf("*** CLIENT CORRECT! Player 2 WINS! ***\n");
                net_send(csock,"GUESS_RESULT|correct");
                net_recv(csock,buf);
                net_send(csock,"GAME_OVER|client_wins");
                net_recv(csock,buf);
                game_over=1;
            } else {
                printf("[Client WRONG] They lose next turn.\n");
                net_send(csock,"GUESS_RESULT|wrong");
                net_recv(csock,buf);
            }
        } else if(strncmp(buf,"CLIENT_SKIP",11)==0){
            printf("[Client skipping — penalty]\n");
            net_send(csock,"ACK");
        }
    }

    printf("\nGame over! Thanks for playing.\n");
    close(csock); close(ssock);
    return 0;
}
