#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <windows.h>


#define U64 unsigned long long

#define empty_board "8/8/8/8/8/8/8/8 w - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1"
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9"
#define repetitions "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 "

#define setBitToOne(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define bitEqualsOne(bitboard, square) ((bitboard) & (1ULL << (square)))
#define setBitToZero(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

static inline int countBits(unsigned long long bitboard) {
    int count = 0;
    while(bitboard) {
        count++;
        bitboard &= bitboard - 1;
    }
    return count;
}

static inline int getTheLeastSignificantBitIndex(unsigned long long bitboard){
    if ( bitboard ) {
        return countBits ( ( bitboard & -bitboard ) - 1 ); 
    }
    else {
        return -1;
    }
}

unsigned int state = 1804289383;

unsigned long long pieceKeys[12][64];
unsigned long long enpassantKeys[64];
unsigned long long castleKeys[16];
unsigned long long sideKey;

unsigned int getARandomU32Number(){
    unsigned int number = state;
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;
    
    state = number;
    
    return number;
}

unsigned long long getARandomU64Number(){
    U64 n1, n2, n3, n4;
    n1 = (U64)(getARandomU32Number() & 0xffff);
    n2 = (U64)(getARandomU32Number() & 0xffff);
    n3 = (U64)(getARandomU32Number() & 0xffff);
    n4 = (U64)(getARandomU32Number() & 0xffff);
    return n1 ^ (n2 << 16) ^ (n3 << 32) ^ (n4 << 48);
}

unsigned long long generateAMagicNumber(){
    return getARandomU64Number () & getARandomU64Number () & getARandomU64Number ();
}


void initializeRandomKeys () {
    state = 1804289383;
    for ( int piece = 0; piece <= 11; piece++ ) {
        for ( int square = 0; square < 64; square++ ) {
            pieceKeys[piece][square] = getARandomU64Number ();
        }
    }
    for ( int square = 0; square < 64; square++ ) {
        enpassantKeys[square] = getARandomU64Number ();
    }
    for ( int index = 0; index < 16; index++ ) {
        castleKeys[index] = getARandomU64Number ();
    }
    sideKey = getARandomU64Number ();
}


enum {
    a8, b8, c8, d8, e8, f8, g8, h8,
    a7, b7, c7, d7, e7, f7, g7, h7,
    a6, b6, c6, d6, e6, f6, g6, h6,
    a5, b5, c5, d5, e5, f5, g5, h5,
    a4, b4, c4, d4, e4, f4, g4, h4,
    a3, b3, c3, d3, e3, f3, g3, h3,
    a2, b2, c2, d2, e2, f2, g2, h2,
    a1, b1, c1, d1, e1, f1, g1, h1, noSq
};

enum {
    white, black, both
};

enum { wk = 1, wq = 2, bk = 4, bq = 8 };

enum { rook, bishop };

enum { P, N, B, R, Q, K, p, n, b, r, q, k };

const char * squareToCoordinates[] = {
    "a8", "b8", "c8", "d8", "e8", "f8", "g8", "h8",
    "a7", "b7", "c7", "d7", "e7", "f7", "g7", "h7",    
    "a6", "b6", "c6", "d6", "e6", "f6", "g6", "h6",
    "a5", "b5", "c5", "d5", "e5", "f5", "g5", "h5",
    "a4", "b4", "c4", "d4", "e4", "f4", "g4", "h4",
    "a3", "b3", "c3", "d3", "e3", "f3", "g3", "h3",
    "a2", "b2", "c2", "d2", "e2", "f2", "g2", "h2",
    "a1", "b1", "c1", "d1", "e1", "f1", "g1", "h1",
};

char ascii_pieces[] = "PNBRQKpnbrqk";

char *unicode_pieces[12] = {"♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};

int char_pieces[] = {
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['K'] = K,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['r'] = r,
    ['q'] = q,
    ['k'] = k,
};

char promotedPieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n'
};

U64 bitboards [ 12 ];
U64 occupancies[ 3 ];
int side = -1;
int enpassant = noSq;
int castle;

unsigned long long hashKey;

unsigned long long repetitionTable [1000];
int repetitionIndex;

int ply;

unsigned long long generateHashKey() {
    unsigned long long finalKey = 0ULL;
    unsigned long long bitboard = 0ULL;
    for ( int piece = 0; piece <= 11; piece++ ) {
        bitboard = bitboards[piece];
        while ( bitboard ) {
            int square = getTheLeastSignificantBitIndex(bitboard);
            finalKey ^= pieceKeys[piece][square];
            bitboard &= ~(1ULL << square);
            
        }
    }
    if ( enpassant != noSq ) finalKey ^= enpassantKeys[enpassant];
    finalKey ^= castleKeys[castle];
    if ( side == black ) finalKey ^= sideKey;
    return finalKey;
}


// exit from engine flag
int quit = 0;

// UCI "movestogo" command moves counter
int movestogo = 30;

// UCI "movetime" command time counter
int movetime = -1;

// UCI "time" command holder (ms)
int time = -1;

// UCI "inc" command's time increment holder
int inc = 0;

// UCI "starttime" command time holder
int starttime = 0;

// UCI "stoptime" command time holder
int stoptime = 0;

// variable to flag time control availability
int timeset = 0;

// variable to flag when the time is up
int stopped = 0;


/**********************************\
 ==================================
 
       Miscellaneous functions
          forked from VICE
         by Richard Allbert
 
 ==================================
\**********************************/

// get time in milliseconds
int get_time_ms()
{
    #ifdef WIN64
        return GetTickCount();
    #else
        struct timeval time_value;
        gettimeofday(&time_value, NULL);
        return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
    #endif
}

/*

  Function to "listen" to GUI's input during search.
  It's waiting for the user input from STDIN.
  OS dependent.
  
  First Richard Allbert aka BluefeverSoftware grabbed it from somewhere...
  And then Code Monkey King has grabbed it from VICE)
  
*/
  
int input_waiting()
{
    #ifndef WIN32
        fd_set readfds;
        struct timeval tv;
        FD_ZERO (&readfds);
        FD_SET (fileno(stdin), &readfds);
        tv.tv_sec=0; tv.tv_usec=0;
        select(16, &readfds, 0, 0, &tv);

        return (FD_ISSET(fileno(stdin), &readfds));
    #else
        static int init = 0, pipe;
        static HANDLE inh;
        DWORD dw;

        if (!init)
        {
            init = 1;
            inh = GetStdHandle(STD_INPUT_HANDLE);
            pipe = !GetConsoleMode(inh, &dw);
            if (!pipe)
            {
                SetConsoleMode(inh, dw & ~(ENABLE_MOUSE_INPUT|ENABLE_WINDOW_INPUT));
                FlushConsoleInputBuffer(inh);
            }
        }
        
        if (pipe)
        {
           if (!PeekNamedPipe(inh, NULL, 0, NULL, &dw, NULL)) return 1;
           return dw;
        }
        
        else
        {
           GetNumberOfConsoleInputEvents(inh, &dw);
           return dw <= 1 ? 0 : dw;
        }

    #endif
}

// read GUI/user input
void read_input()
{
    // bytes to read holder
    int bytes;
    
    // GUI/user input
    char input[256] = "", *endc;

    // "listen" to STDIN
    if (input_waiting())
    {
        // tell engine to stop calculating
        if ( timeset == 1 ) stopped = 1;
        
        // loop to read bytes from STDIN
        do
        {
            // read bytes from STDIN
            bytes=read(fileno(stdin), input, 256);
        }
        
        // until bytes available
        while (bytes < 0);
        
        // searches for the first occurrence of '\n'
        endc = strchr(input,'\n');
        
        // if found new line set value at pointer to 0
        if (endc) *endc=0;
        
        // if input is available
        if (strlen(input) > 0)
        {
            // match UCI "quit" command
            if (!strncmp(input, "quit", 4))
            {
                // tell engine to terminate exacution    
                quit = 1;
            }

            // // match UCI "stop" command
            else if (!strncmp(input, "stop", 4))    {
                // tell engine to terminate exacution
                quit = 1;
            }
        }   
    }
}

// a bridge function to interact between search and GUI input
static void communicate() {
	// if time is up break here
    if(timeset == 1 && get_time_ms() > stoptime) {
		// tell engine to stop calculating
		stopped = 1;
	}
	
    // read GUI input
	read_input();
}

const unsigned long long notAFile = 18374403900871474942ULL;
const unsigned long long notHFile = 9187201950435737471ULL;
const unsigned long long notHGFile = 4557430888798830399ULL;
const unsigned long long notABFile = 18229723555195321596ULL;

// bishop relevant occupancy bit count for every square on board
const int bishopRelevantBits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6, 
    5, 5, 5, 5, 5, 5, 5, 5, 
    5, 5, 7, 7, 7, 7, 5, 5, 
    5, 5, 7, 9, 9, 7, 5, 5, 
    5, 5, 7, 9, 9, 7, 5, 5, 
    5, 5, 7, 7, 7, 7, 5, 5, 
    5, 5, 5, 5, 5, 5, 5, 5, 
    6, 5, 5, 5, 5, 5, 5, 6
};

// rook relevant occupancy bit count for every square on board
const int rookRelevantBits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    11, 10, 10, 10, 10, 10, 10, 11, 
    12, 11, 11, 11, 11, 11, 11, 12
};

// rook magic numbers
unsigned long long rookMagicNumbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL
};

// bishop magic numbers
unsigned long long bishopMagicNumbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL
};

unsigned long long pawnAttacks[2][64];
unsigned long long kingAttacks[64];
unsigned long long knightAttacks[64];
unsigned long long bishopAttackMasks[64];
unsigned long long rookAttackMasks[64];
unsigned long long bishopAttacksTable[64][512];
unsigned long long rookAttacksTable[64][4096];

void printTheBitboard ( unsigned long long bitboard ) {
    
    printf("\n");
    
    for ( int rank = 0; rank < 8; rank++ ) {
        
        for ( int file = 0; file < 8; file++ ) {
            
            int square = 8 * rank + file;
            if(!file) printf(" %d ", 8 - rank);
            printf(" %d ", bitEqualsOne(bitboard, square)?1:0);
            
        }
        
        printf("\n");
        
    }
    printf("\n    a  b  c  d  e  f  g  h\n\n");
    printf("    Bitboard: %llud\n\n", bitboard);
}

void printBoard () {
    
    printf("\n");
    
    for ( int rank = 0; rank < 8; rank++ ) {
        
        for ( int file = 0; file < 8; file++ ) {
            
            int square = 8 * rank + file;
            
            if ( !file ) printf(" %d ", 8 - rank);
            
            int piece = -1;
            
            for ( int bb_piece = P; bb_piece <=k; bb_piece++ ) {
                
                if ( bitEqualsOne ( bitboards[bb_piece], square ) ) {
                    
                    piece = bb_piece;
                    
                }
                
            }
            #if defined(_WIN32) || defined(_WIN64)
                printf(" %c", (piece == -1)?'.':ascii_pieces[piece]);
            #else
                printf(" %s", (piece == -1)?'.':unicode_pieces[piece]);
            #endif            
            
            
        }
        
        printf("\n");
        
    }
    printf("\n    a b c d e f g h\n\n");
    printf("    Side:     %s\n", !side ? "white":"black");
    printf("    Enpas        %s\n", (enpassant != noSq)?squareToCoordinates[enpassant]:"no");
    printf("    Castling:  %c%c%c%c\n\n", ( castle & wk )?'K':'-',( castle & wq )?'Q':'-', ( castle & bk )?'k':'-', ( castle & bq )?'q':'-');
    printf("    Hash key: %llx\n\n", hashKey);
}

void parseFen(char * fen) {
    
    memset(bitboards, 0ULL, sizeof(bitboards));
    memset(occupancies, 0ULL, sizeof(occupancies));
    side = 0;
    enpassant = noSq;
    castle = 0;
    hashKey = 0ULL;
    repetitionIndex = 0;
    memset(repetitionTable, 0ULL, sizeof(repetitionTable));
    for ( int rank = 0; rank < 8; rank++ ) {
        
        for ( int file = 0; file < 8; file++ ) {
            
            int square = 8 * rank + file;
            
            if (( *fen >= 'a' && *fen <= 'z' )||( *fen >= 'A' && *fen <= 'Z' )) {
                int piece = char_pieces[*fen];
                setBitToOne(bitboards[piece], square);
                fen++;
            }
            if ( *fen >= '1' && *fen <= '8' ) {
                int piece = -1;
                for(int pieceType = P; pieceType<=k; pieceType++){
                    if(bitEqualsOne(bitboards[pieceType], square)) piece = pieceType; 
                }
                if ( piece == -1 ) file--;
                file += *fen - '0';
                fen++;
            }
            if(*fen == '/') fen++;
        }
    }
    fen++;
    (*fen=='w')?(side = white):(side = black);
    fen += 2;
    while(*fen!=' '){
        switch (*fen){
            case 'K': castle |= wk; break;
            case 'Q': castle |= wq; break;
            case 'k': castle |= bk; break;
            case 'q': castle |= bq; break;
            case '-': break;
        }
        fen++;
    }
    fen++;
    if ( *fen!='-'){
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');
        enpassant = 8*rank + file;
    }
    else enpassant = noSq;
    fen++;
    for ( int piece = P; piece<=K;piece++){
        occupancies[white] |= bitboards[piece];
    }
    for ( int piece = p; piece<=k;piece++){
        occupancies[black] |= bitboards[piece];
    }
    occupancies[both] |= occupancies[white];
    occupancies[both] |= occupancies[black];
    hashKey = generateHashKey ();
}    

unsigned long long maskPawnAttacks(int side, int square){
    
    unsigned long long attacks = 0ULL;
    unsigned long long bitboard = 0ULL;
    setBitToOne(bitboard, square);
    if ( !side ) {
        if((bitboard >> 7) & notAFile) attacks |= bitboard >> 7;
        if((bitboard >> 9) & notHFile) attacks |= bitboard >> 9;
    }
    else {
        if((bitboard << 7) & notHFile) attacks |= bitboard << 7;
        if((bitboard << 9) & notAFile) attacks |= bitboard << 9;
        
    }
    return attacks;
}

unsigned long long maskKnightAttacks(int square){
    unsigned long long attacks = 0ULL;
    unsigned long long bitboard = 0ULL;
    setBitToOne(bitboard, square);
    if ((bitboard >> 17) & notHFile) attacks |= bitboard >> 17;
    if ((bitboard >> 15) & notAFile) attacks |= bitboard >> 15;
    if ((bitboard >> 10) & notHGFile) attacks |= bitboard >> 10;
    if ((bitboard >> 6) & notABFile) attacks |= bitboard >> 6;
    if ((bitboard << 17) & notAFile) attacks |= bitboard << 17;
    if ((bitboard << 15) & notHFile) attacks |= bitboard << 15;
    if ((bitboard << 10) & notABFile) attacks |= bitboard << 10;
    if ((bitboard << 6) & notHGFile) attacks |= bitboard << 6;
    
    return attacks;
}
unsigned long long maskKingAttacks(int square){
    unsigned long long attacks = 0ULL;
    unsigned long long bitboard = 0ULL;
    setBitToOne(bitboard, square);
    if (bitboard >> 8) attacks |= bitboard >> 8;
    if ((bitboard >> 9) & notHFile) attacks |= bitboard >> 9;
    if ((bitboard >> 7) & notAFile) attacks |= bitboard >> 7;
    if ((bitboard >> 1) & notHFile) attacks |= bitboard >> 1;
    if (bitboard << 8) attacks |= bitboard << 8;
    if ((bitboard << 9) & notAFile) attacks |= bitboard << 9;
    if ((bitboard << 7) & notHFile) attacks |= bitboard << 7;
    if ((bitboard << 1) & notAFile) attacks |= bitboard << 1;
    
    return attacks;
}
unsigned long long maskBishopAttacks(int square){
    unsigned long long attacks = 0ULL;
    unsigned long long bitboard = 0ULL;
    setBitToOne(bitboard, square);
    int rank, file;
    
    int targetRank = square / 8;
    int targetFile = square % 8;
    
    for ( rank = targetRank + 1, file = targetFile + 1; rank <= 6 && file <= 6; rank++, file++ ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        
    }
    for ( rank = targetRank - 1, file = targetFile + 1; rank >= 1 && file <= 6; rank--, file++ ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        
    }
    for ( rank = targetRank + 1, file = targetFile - 1; rank <= 6 && file >= 1; rank++, file-- ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        
    }
    for ( rank = targetRank - 1, file = targetFile - 1; rank >= 1 && file >= 1; rank--, file-- ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        
    }

    
    return attacks;
}

unsigned long long setBlocks( int index, int bitsInMask, unsigned long long attackMask) {
    
    unsigned long long occupancy = 0ULL;
    
    for ( int count = 0; count < bitsInMask; count++ ) {
        
        int square = getTheLeastSignificantBitIndex(attackMask);
        
        setBitToZero ( attackMask, square );
        
        if ( index & ( 1ULL << count ) ) occupancy |= (1ULL << square );
        
    }
    
    return occupancy;
    
}

unsigned long long maskRookAttacks(int square){
    unsigned long long attacks = 0ULL;
    unsigned long long bitboard = 0ULL;
    setBitToOne(bitboard, square);
    int rank, file;
    
    int targetRank = square / 8;
    int targetFile = square % 8;
    
    for ( rank = targetRank + 1; rank <= 6; rank++ ) {
        
        attacks |= (1ULL << (8 * rank + targetFile ));
        
    }
    for ( rank = targetRank - 1; rank >= 1; rank-- ) {
        
        attacks |= (1ULL << (8 * rank + targetFile ));
        
    }
    for ( file = targetFile + 1; file <= 6; file++ ) {
        
        attacks |= (1ULL << (8 * targetRank + file ));
        
    }
    for ( file = targetFile - 1; file >= 1; file-- ) {
        
        attacks |= (1ULL << (8 * targetRank + file ));
        
    }

    
    return attacks;
}

unsigned long long bishopAttacksOnTheFly(int square, unsigned long long block){
    unsigned long long attacks = 0ULL;
    unsigned long long bitboard = 0ULL;
    setBitToOne(bitboard, square);
    int rank, file;
    
    int targetRank = square / 8;
    int targetFile = square % 8;
    
    for ( rank = targetRank + 1, file = targetFile + 1; rank <= 7 && file <= 7; rank++, file++ ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        if( (1ULL << (8 * rank + file )) & block ) break;
        
    }
    for ( rank = targetRank - 1, file = targetFile + 1; rank >= 0 && file <= 7; rank--, file++ ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        if( (1ULL << (8 * rank + file )) & block ) break;
        
    }
    for ( rank = targetRank + 1, file = targetFile - 1; rank <= 7 && file >= 0; rank++, file-- ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        if( (1ULL << (8 * rank + file )) & block ) break;
        
    }
    for ( rank = targetRank - 1, file = targetFile - 1; rank >= 0 && file >= 0; rank--, file-- ) {
        
        attacks |= (1ULL << (8 * rank + file ));
        if( (1ULL << (8 * rank + file )) & block ) break;
        
    }
    
    return attacks;
}

unsigned long long rookAttacksOnTheFly(int square, unsigned long long block){
    unsigned long long attacks = 0ULL;
    unsigned long long bitboard = 0ULL;
    setBitToOne(bitboard, square);
    int rank, file;
    
    int targetRank = square / 8;
    int targetFile = square % 8;
    
    for ( rank = targetRank + 1; rank <= 7; rank++ ) {
        
        attacks |= (1ULL << (8 * rank + targetFile ));
        if( (1ULL << (8 * rank + targetFile )) & block ) break;
        
    }
    for ( rank = targetRank - 1; rank >= 0; rank-- ) {
        
        attacks |= (1ULL << (8 * rank + targetFile ));
        if( (1ULL << (8 * rank + targetFile )) & block ) break;
        
    }
    for ( file = targetFile + 1; file <= 7; file++ ) {
        
        attacks |= (1ULL << (8 * targetRank + file ));
        if( (1ULL << (8 * targetRank + file )) & block ) break;
        
    }
    for ( file = targetFile - 1; file >= 0  ; file-- ) {
        
        attacks |= (1ULL << (8 * targetRank + file ));
        if( (1ULL << (8 * targetRank + file )) & block ) break; 
        
    }

    
    return attacks;
}

void initializeSlidingPiecesAttacks(int itIsABishop){
    for ( int square = 0; square < 64; square++ ) {
        
        bishopAttackMasks [ square ] = maskBishopAttacks ( square );
        rookAttackMasks [ square ] = maskRookAttacks ( square );
        
        unsigned long long attackMask = itIsABishop ? bishopAttackMasks [ square ]:rookAttackMasks [ square ];
        
        int relevantBitsCount = countBits(attackMask);
        
        int occupancyIndices = 1 << relevantBitsCount;
        
        for ( int index = 0; index < occupancyIndices; index++ ) {
            
            if ( itIsABishop ) {
                
                unsigned long long blocks = setBlocks(index, relevantBitsCount, attackMask);
                
                int magicIndex = (blocks * bishopMagicNumbers [ square ]) >> (64 - bishopRelevantBits [ square ] );
                
                bishopAttacksTable [ square ] [ magicIndex ] = bishopAttacksOnTheFly ( square, blocks );
                
            }
            else {
                
                unsigned long long blocks = setBlocks(index, relevantBitsCount, attackMask);
                
                int magicIndex = (blocks * rookMagicNumbers [ square ]) >> (64 - rookRelevantBits [ square ] );
                
                rookAttacksTable [ square ] [ magicIndex ] = rookAttacksOnTheFly ( square, blocks );                                        
                
            }
        }
        
    }
}

static inline U64 getBishopAttacks(int square, U64 occupancy){
    
    occupancy &= bishopAttackMasks[square];
    occupancy *= bishopMagicNumbers[square];
    occupancy >>= 64 - bishopRelevantBits[square];
    
    return bishopAttacksTable[square][occupancy];
    
}

static inline U64 getRookAttacks(int square, U64 occupancy){
    
    occupancy &= rookAttackMasks[square];
    occupancy *= rookMagicNumbers[square];
    occupancy >>= 64 - rookRelevantBits[square];
    
    return rookAttacksTable[square][occupancy];
}

static inline U64 getQueenAttacks(int square, U64 occupancy){
    
    U64 result = 0ULL;
    U64 bishopOccupancy = occupancy;
    U64 rookOccupancy = occupancy;

    bishopOccupancy &= bishopAttackMasks[square];
    bishopOccupancy *= bishopMagicNumbers[square];
    bishopOccupancy >>= 64 - bishopRelevantBits[square];
    
    result = bishopAttacksTable[square][bishopOccupancy];
    
    rookOccupancy &= rookAttackMasks[square];
    rookOccupancy *= rookMagicNumbers[square];
    rookOccupancy >>= 64 - rookRelevantBits[square];
    
    result |= rookAttacksTable[square][rookOccupancy];
    
    return result;
}


void initLeaperAttacks(){
    for(int square = 0; square < 64; square++ ) {
        kingAttacks[square] = maskKingAttacks(square);
        knightAttacks[square] = maskKnightAttacks(square);
        pawnAttacks[white][square] = maskPawnAttacks(white, square);
        pawnAttacks[black][square] = maskPawnAttacks(black, square);
    }
}

static inline int isSquareAttacked(int square, int side){
    
    if ( ( side==white) && (pawnAttacks[black][square] & bitboards[P])) return 1;
    if ( ( side==black) && (pawnAttacks[white][square] & bitboards[p])) return 1;
    
    if(knightAttacks[square] & ( (side==white) ? bitboards[N] : bitboards[n])) return 1;
    if(knightAttacks[square] & ( (side==black) ? bitboards[n] : bitboards[N])) return 1;
    
    if (getBishopAttacks(square, occupancies[both]) & ( (side==white) ? bitboards[B] : bitboards[b])) return 1;    
    
    if (getRookAttacks(square, occupancies[both]) & ( (side==white) ? bitboards[R] : bitboards[r])) return 1;    
    
    if (getQueenAttacks(square, occupancies[both]) & ( (side==white) ? bitboards[Q] : bitboards[q])) return 1;        
    
    if(kingAttacks[square] & ( (side==white) ? bitboards[K] : bitboards[k])) return 1;
    if(kingAttacks[square] & ( (side==black) ? bitboards[k] : bitboards[K])) return 1;
    return 0;
}

void printAttackedSquares(int side){
    for ( int rank = 0; rank < 8; rank++ ) {
        for ( int file = 0; file < 8; file++ ) {
            int square = 8 * rank + file;
            if ( !file ) printf("  %d ", 8 - rank);
            printf("%d ", isSquareAttacked(square, side)?1:0);
        }
        printf("\n");
    }
    printf("\n    a  b  c  d  e  f  g  h\n\n");
}

#define encode_move(source, target, piece, promoted, capture, double, enpassant, castling) \
((source) | \
((target) << 6) | \
((piece) << 12) | \
((promoted) << 16) | \
((capture) << 20) | \
((double) << 21) | \
((enpassant) << 22) | \
((castling) << 23))

#define getMoveSource(move) (move & 0x3f)
#define getMoveTarget(move) ((move & 0xfc0) >> 6)
#define getMovePiece(move) ((move & 0xf000) >> 12)
#define getMovePromoted(move) ((move & 0xf0000) >> 16)
#define getMoveCapture(move) (move & 0x100000)
#define getMoveDouble(move) (move & 0x200000)
#define getMoveEnpassant(move) (move & 0x400000)
#define getMoveCastling(move) (move & 0x800000)

typedef struct {
    int moves[256];
    int count;
} moves;

static inline void addMove(moves * moveList, int move){
    moveList->moves[moveList->count] = move;
    moveList->count++;
}

void printMove(int move){
    if ( getMovePromoted(move)) printf("%s%s%c", squareToCoordinates[getMoveSource(move)], squareToCoordinates[getMoveTarget(move)], promotedPieces[getMovePromoted(move)]);
    else printf("%s%s", squareToCoordinates[getMoveSource(move)], squareToCoordinates[getMoveTarget(move)]);
}

void printMoveList(moves * moveList){
    
    if ( !moveList->count ) {
        printf("\n   No moves in the move list!\n");
        return;
    }
    
    printf("\n   move     piece   capture    double   enpassant   castling\n\n");
    
    for ( int moveCount = 0; moveCount < moveList->count; moveCount++ ) {
        int move = moveList->moves[moveCount];
        
        printf("   %s%s%c    %c       %d          %d          %d          %d\n", squareToCoordinates[getMoveSource(move)], squareToCoordinates[getMoveTarget(move)], getMovePromoted(move) ? promotedPieces[getMovePromoted(move)] : ' ', ascii_pieces[getMovePiece(move)], getMoveCapture(move) ? 1 : 0, getMoveDouble(move) ? 1 : 0, getMoveEnpassant(move) ? 1 : 0, getMoveCastling(move) ? 1 : 0);
        
    }
    
    printf("\n\n   Total number of moves: %d\n\n", moveList->count);
    
}

#define copyBoard()                                                   \
    U64 bitboardsCopy[12], occupanciesCopy[3];                        \
    int sideCopy, enpassantCopy, castleCopy;                          \
    memcpy(bitboardsCopy, bitboards, 96);                             \
    memcpy(occupanciesCopy, occupancies, 24);                         \
    sideCopy = side, enpassantCopy = enpassant, castleCopy = castle;  \
    U64 hashKeyCopy = hashKey;  \

#define takeBack()                                                    \
    memcpy(bitboards, bitboardsCopy, 96);                             \
    memcpy(occupancies, occupanciesCopy,24);                          \
    side = sideCopy, enpassant = enpassantCopy, castle = castleCopy;  \
    hashKey = hashKeyCopy;  \

enum { allMoves, onlyCaptures };

const int castlingRights[] = {
	7 , 15, 15, 15, 3,  15, 15, 11,
	15, 15, 15, 15, 15, 15, 15, 15, 
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	15, 15, 15, 15, 15, 15, 15, 15,
	13, 15, 15, 15, 12, 15, 15, 14
};

static inline int makeMove(int move, int moveFlag){
    if ( moveFlag == allMoves ) {
        copyBoard();
        int sourceSquare = getMoveSource(move);
        int targetSquare = getMoveTarget(move);
        int piece = getMovePiece(move);
        int promotedPiece = getMovePromoted(move);
        int capture = getMoveCapture(move);
        int doublePush = getMoveDouble(move);
        int enpass = getMoveEnpassant(move);
        int castling = getMoveCastling(move);
        
        setBitToZero(bitboards[piece], sourceSquare);
        setBitToOne(bitboards[piece], targetSquare);
        
        hashKey ^= pieceKeys[piece][sourceSquare];
        hashKey ^= pieceKeys[piece][targetSquare];
        
        if ( capture ) {
            int startPiece, endPiece;
            if ( side == white){
                startPiece = p;
                endPiece = k;
            }
            else{
                startPiece = P;
                endPiece = K;
            }
            for ( int bitboardPiece = startPiece; bitboardPiece <= endPiece; bitboardPiece++ ) {
                if ( bitEqualsOne(bitboards[bitboardPiece], targetSquare )) {
                    setBitToZero(bitboards[bitboardPiece], targetSquare );
                    hashKey ^= pieceKeys[bitboardPiece][targetSquare];
                    break;
                }
            }
        }
        if (promotedPiece){
            //setBitToZero(bitboards[(side==white)?P:p], targetSquare);
			if ( side==0){
				bitboards[P] &= ~(1ULL << targetSquare);
				hashKey ^= pieceKeys[P][targetSquare];
			}
			else{
				bitboards[p] &= ~(1ULL << targetSquare);
				hashKey ^= pieceKeys[p][targetSquare];
			}
            setBitToOne(bitboards[promotedPiece], targetSquare);			
			hashKey ^= pieceKeys[promotedPiece][targetSquare];
        }
		if ( enpass) {
			(side == 0) ? (bitboards[6] &= ~(1ULL << (targetSquare + 8))) : (bitboards[0] &= ~(1ULL << (targetSquare - 8)));
            if ( side == 0 ) {
                bitboards[p] &= ~(1ULL << (targetSquare + 8 ) );
                hashKey ^= pieceKeys[p][targetSquare + 8];
            }
            else {
                bitboards[P] &= ~(1ULL << (targetSquare - 8 ) );                
                hashKey ^= pieceKeys[P][targetSquare - 8];
            }
		}
        if ( enpassant != noSq ) hashKey ^= enpassantKeys[enpassant];        
		enpassant = noSq;
		if ( doublePush){
			(side==0)?(enpassant = targetSquare + 8):(enpassant = targetSquare - 8);
            if ( side == 0 ) {
                enpassant = targetSquare + 8;
                hashKey ^= enpassantKeys[targetSquare + 8];
            }
            else {
                enpassant = targetSquare - 8;
                hashKey ^= enpassantKeys[targetSquare - 8];
            }
		}
		if ( castling){
			switch(targetSquare){
				case (g1):
					bitboards[3] &= ~(1ULL << h1);
					bitboards[3] |= (1ULL << f1);
                    hashKey ^= pieceKeys[R][h1];
                    hashKey ^= pieceKeys[R][f1];
					break;
				case (c1):
					bitboards[3] &= ~(1ULL << a1);
					bitboards[3] |= (1ULL << d1);
                    hashKey ^= pieceKeys[R][a1];
                    hashKey ^= pieceKeys[R][d1];
					break;
				case (g8):
					bitboards[9] &= ~(1ULL << h8);
					bitboards[9] |= (1ULL << f8);
                    hashKey ^= pieceKeys[r][h8];
                    hashKey ^= pieceKeys[r][f8];
					break;
				case (c8):
					bitboards[9] &= ~(1ULL << a8);
					bitboards[9] |= (1ULL << d8);
                    hashKey ^= pieceKeys[r][a8];
                    hashKey ^= pieceKeys[r][d8];
					break;
			}
		}
        
        hashKey ^= castleKeys[castle];
        
		castle &= castlingRights[sourceSquare];
		castle &= castlingRights[targetSquare];
        
        hashKey ^= castleKeys[castle];
        
		memset(occupancies, 0ULL, 24);
		for ( int piece2 = 0; piece2 < 6; piece2++){
			occupancies[white] |= bitboards[piece2];
		}
		for ( int piece2 = 6; piece2 < 12; piece2++){
			occupancies[black] |= bitboards[piece2];
		}
		occupancies[both] |= occupancies[white];
		occupancies[both] |= occupancies[black];
        
        side ^= 1;
        
        hashKey ^= sideKey;
        
        /*unsigned long long hashFromScratch = generateHashKey ();
        if ( hashKey != hashFromScratch){
            printf("\n\nMake move\n");
            printf("move: ");
            printMove(move);
            printBoard ();
            printf("hash should be: %llx\n", hashFromScratch);
            getchar();
        }*/
        
        if (isSquareAttacked((side==0) ? getTheLeastSignificantBitIndex(bitboards[11]):getTheLeastSignificantBitIndex(bitboards[5]), side)) {
            takeBack();
            return 0;
        }
        return 1;
    }
    else {
        if ( getMoveCapture(move) ) {
            return makeMove(move, allMoves);
        }
        else return 0;
    }
}

static inline void generateMoves (moves * moveList) {
    
    moveList->count = 0;
    
    int sourceSquare, targetSquare;
    
    U64 bitboard, attacks;
    
    for ( int piece=P;piece<=k;piece++){
        bitboard = bitboards[piece];
        if(side==white){
			if ( piece == P ) {
				while(bitboard){
					sourceSquare = getTheLeastSignificantBitIndex(bitboard);
					targetSquare = sourceSquare - 8;
					if ( !(targetSquare < a8 ) && !bitEqualsOne(occupancies[both], targetSquare)){
						if ( sourceSquare >= a7 && sourceSquare <= h7 ) {
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, Q, 0, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, R, 0, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, B, 0, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, N, 0, 0, 0, 0));
						}
						else {
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
							if ( sourceSquare >= a2 && sourceSquare <= h2 && !bitEqualsOne(occupancies[both], targetSquare - 8)){
                                addMove(moveList, encode_move(sourceSquare, targetSquare - 8, piece, 0, 0, 1, 0, 0));
							}
						}
					}
					attacks = pawnAttacks[side][sourceSquare] & occupancies[black];
					while(attacks){
						targetSquare = getTheLeastSignificantBitIndex(attacks);
						if ( sourceSquare >= a7 && sourceSquare <= h7 ) {
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, Q, 1, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, R, 1, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, B, 1, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, N, 1, 0, 0, 0));
                                
						}
						else {                            
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
						}
						setBitToZero(attacks, targetSquare);
					}
					if (enpassant != noSq){
						U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);
						if (enpassantAttacks){
							int targetEnpassant = getTheLeastSignificantBitIndex(enpassantAttacks);
                            addMove(moveList, encode_move(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
						}
					}
					attacks = pawnAttacks[side][sourceSquare];
					while(attacks){
						targetSquare = getTheLeastSignificantBitIndex(attacks);
						setBitToZero(attacks, targetSquare);
					}
					setBitToZero(bitboard, sourceSquare);
				}
			}
            if ( piece == K ) {
				if (castle & wk){
					if(!bitEqualsOne(occupancies[both], f1) && !bitEqualsOne(occupancies[both], g1)){
						if(!isSquareAttacked(e1, black) && !isSquareAttacked(f1, black)){
                            addMove(moveList, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
						}
					}
				}
				if (castle & wq){
					if(!bitEqualsOne(occupancies[both], d1) && !bitEqualsOne(occupancies[both], c1) && !bitEqualsOne(occupancies[both], b1)){
						if(!isSquareAttacked(e1, black) && !isSquareAttacked(d1, black)){
                            addMove(moveList, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
						}
					}
				}				
			}
		}
        else{
			if ( piece == p ) {
				while(bitboard){
					sourceSquare = getTheLeastSignificantBitIndex(bitboard);
					targetSquare = sourceSquare + 8;
					if ( !(targetSquare > h1 ) && !bitEqualsOne(occupancies[both], targetSquare)){
						if ( sourceSquare >= a2 && sourceSquare <= h2 ) {
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, q, 0, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, r, 0, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, b, 0, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, n, 0, 0, 0, 0));
						}
						else {
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
							if ( sourceSquare >= a7 && sourceSquare <= h7 && !bitEqualsOne(occupancies[both], targetSquare + 8)){
                                addMove(moveList, encode_move(sourceSquare, targetSquare + 8, piece, 0, 0, 1, 0, 0));
							}
						} 						
					}
					attacks = pawnAttacks[side][sourceSquare] & occupancies[white];
					while(attacks){
						targetSquare = getTheLeastSignificantBitIndex(attacks);
						if ( sourceSquare >= a2 && sourceSquare <= h2 ) {                            
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, q, 1, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, r, 1, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, b, 1, 0, 0, 0));
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, n, 1, 0, 0, 0));
						}
						else {
                            addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
						}
						setBitToZero(attacks, targetSquare);
					}
					if (enpassant != noSq){
						U64 enpassantAttacks = pawnAttacks[side][sourceSquare] & (1ULL << enpassant);
						if (enpassantAttacks){
							int targetEnpassant = getTheLeastSignificantBitIndex(enpassantAttacks);
                            addMove(moveList, encode_move(sourceSquare, targetEnpassant, piece, 0, 1, 0, 1, 0));
						}
					}					
					setBitToZero(bitboard, sourceSquare);
				}
			}
            if ( piece == k ) {
				if (castle & bk){
					if(!bitEqualsOne(occupancies[both], f8) && !bitEqualsOne(occupancies[both], g8)){
						if(!isSquareAttacked(e8, white) && !isSquareAttacked(f8, white)){
                            addMove(moveList, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
						}

					}
				}
				if (castle & bq){
					if(!bitEqualsOne(occupancies[both], d8) && !bitEqualsOne(occupancies[both], c8) && !bitEqualsOne(occupancies[both], b8)){
						if(!isSquareAttacked(e8, white) && !isSquareAttacked(d8, white)){
                            addMove(moveList, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
						}
					}					
				}				
			}						
        }
		if (( side == white ) ? piece == N : piece == n) {
			while(bitboard){
				sourceSquare = getTheLeastSignificantBitIndex(bitboard);
                attacks = knightAttacks[sourceSquare] & (( side == white ) ? ~occupancies[white] : ~occupancies[black] );
                while(attacks){
                    targetSquare = getTheLeastSignificantBitIndex(attacks);
                    if(!bitEqualsOne((( side == white ) ? occupancies[black] : occupancies[white]), targetSquare)) {
                        addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    else addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    setBitToZero(attacks, targetSquare);
                }
				setBitToZero(bitboard, sourceSquare);
			}
		}
        if (( side == white ) ? piece == B : piece == b) {
			while(bitboard){
				sourceSquare = getTheLeastSignificantBitIndex(bitboard);
                attacks = getBishopAttacks(sourceSquare, occupancies[both]) & (( side == white ) ? ~occupancies[white] : ~occupancies[black] );
                while(attacks){
                    targetSquare = getTheLeastSignificantBitIndex(attacks);
                    if(!bitEqualsOne((( side == white ) ? occupancies[black] : occupancies[white]), targetSquare)) {
                        addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    else addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    setBitToZero(attacks, targetSquare);
                }
				setBitToZero(bitboard, sourceSquare);
			}
		}
        if (( side == white ) ? piece == R : piece == r) {
			while(bitboard){
				sourceSquare = getTheLeastSignificantBitIndex(bitboard);
                attacks = getRookAttacks(sourceSquare, occupancies[both]) & (( side == white ) ? ~occupancies[white] : ~occupancies[black] );
                while(attacks){
                    targetSquare = getTheLeastSignificantBitIndex(attacks);
                    if(!bitEqualsOne((( side == white ) ? occupancies[black] : occupancies[white]), targetSquare)) {
                        addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    else addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    setBitToZero(attacks, targetSquare);
                }
				setBitToZero(bitboard, sourceSquare);
			}
		}
        if (( side == white ) ? piece == Q : piece == q) {
			while(bitboard){
				sourceSquare = getTheLeastSignificantBitIndex(bitboard);
                attacks = getQueenAttacks(sourceSquare, occupancies[both]) & (( side == white ) ? ~occupancies[white] : ~occupancies[black] );
                while(attacks){
                    targetSquare = getTheLeastSignificantBitIndex(attacks);
                    if(!bitEqualsOne((( side == white ) ? occupancies[black] : occupancies[white]), targetSquare)) {
                        addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    else addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    setBitToZero(attacks, targetSquare);
                }
				setBitToZero(bitboard, sourceSquare);
			}
		}
        if (( side == white ) ? piece == K : piece == k) {
			while(bitboard){
				sourceSquare = getTheLeastSignificantBitIndex(bitboard);
                attacks = kingAttacks[sourceSquare] & (( side == white ) ? ~occupancies[white] : ~occupancies[black] );
                while(attacks){
                    targetSquare = getTheLeastSignificantBitIndex(attacks);
                    if(!bitEqualsOne((( side == white ) ? occupancies[black] : occupancies[white]), targetSquare)) {
                        addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 0, 0, 0, 0));
                    }
                    else addMove(moveList, encode_move(sourceSquare, targetSquare, piece, 0, 1, 0, 0, 0));
                    setBitToZero(attacks, targetSquare);
                }
				setBitToZero(bitboard, sourceSquare);
			}
		}
    }    
}

unsigned long long findAMagicNumber(int square, int relevantBits, int bishop){
    unsigned long long attacks[4096];
    unsigned long long occupancies[4096];
    unsigned long long usedAttacks[4096];
    unsigned long long mask = bishop ? maskBishopAttacks(square):maskRookAttacks(square);
    int count = 1 << relevantBits;
    for ( int i = 0; i < count; i++ ) {
        occupancies[i] = setBlocks(i, relevantBits, mask);
        attacks[i] = bishop ? bishopAttacksOnTheFly(square, occupancies[i]) : rookAttacksOnTheFly(square, occupancies[i]);
    }
    for ( int i = 0; i < 100000000; i++ ){
        unsigned long long magicNumber = generateAMagicNumber();
        if ( countBits((mask * magicNumber) & 0xFF00000000000000) < 6 ) continue;
        int index, fail;
        memset(usedAttacks, 0, sizeof(usedAttacks));
        for ( index = 0, fail = 0; !fail && index < count; index++ ) {
            int magicIndex = (int) ((occupancies[index] * magicNumber) >> (64 - relevantBits));
            if ( usedAttacks[magicIndex] == 0ULL ) usedAttacks[magicIndex] = attacks[index];
            else if ( usedAttacks[magicIndex] != attacks[index] ) fail = 1;
        }
        if ( !fail ) return magicNumber;
    }
    printf("Magic number generation failed!\n");
    return 0ULL;
}

void initializeMagicNumbers(){
    for ( int square = 0; square < 64; square++ ){
        printf(" 0x%llxULL\n", findAMagicNumber(square, rookRelevantBits[square], 0));
    }
}

unsigned long long numberOfLeafNodes;

static inline void perftDriver(int depth){
	if ( depth==0){
		numberOfLeafNodes++;
		return;
	}
	
    moves moveList[1];
    generateMoves(moveList);
    
    for ( int moveCount = 0; moveCount < moveList->count; moveCount++ ) {
        copyBoard();
        if ( !makeMove(moveList->moves[moveCount], allMoves) ) continue;
		perftDriver(depth-1);
        takeBack();
        /*unsigned long long hashFromScratch = generateHashKey ();
        if ( hashKey != hashFromScratch){
            printf("\n\nTake back\n");
            printf("move: ");
            printMove(moveList->moves[moveCount]);
            printBoard ();
            printf("hash should be: %llx\n", hashFromScratch);
            getchar();
        }*/
        
    }
	
}

void perftTest(int depth){
	printf("Performance test\n");
    moves moveList[1];
    generateMoves(moveList);
	int start = GetTickCount();
    
    for ( int moveCount = 0; moveCount < moveList->count; moveCount++ ) {
        copyBoard();
        if ( !makeMove(moveList->moves[moveCount], allMoves) ) continue;
		long cumulativeNodes = numberOfLeafNodes;
		perftDriver(depth-1);
        takeBack();
		long oldNodes = numberOfLeafNodes - cumulativeNodes;
		printf("     move: %s%s%c nodes: %ld\n", squareToCoordinates[getMoveSource(moveList->moves[moveCount])], squareToCoordinates[getMoveTarget(moveList->moves[moveCount])], getMovePromoted(moveList->moves[moveCount]) ? promotedPieces[getMovePromoted(moveList->moves[moveCount])] : ' ', oldNodes);
    }
	printf("     Depth: %d\n", depth);
	printf("     Nodes: %lld\n", numberOfLeafNodes);
	printf("     Time: %ld\n", GetTickCount () - start);
}

int materialScore[12] = {
    100,
    300,
    350,
    500,
   1000,
  10000,
   -100,
   -300,
   -350,
   -500,
  -1000,
 -10000,
};

const int pawnScore[64] = 
{ 85, 95, 85, 90, 95, 85, 95, 90, 30, 25, 25, 35, 40, 30, 35, 35, 20, 25, 25, 35, 35, 25, 15, 20, 15, 5, 10, 25, 15, 5, 10, 15, 5, 5, 10, 20, 20, 5, 0, 0, -5, 5, 5, 5, 5, -5, -5, 5, -5, 0, 5, -5, -5, -5, 0, -5, 0, 5, 5, 0, 0, 0, -5, -5, };

const int knightScore[64] = 
{
    -5,   0,   0,   0,  0,   0,   0,  -5,
    -5,   0,   0,  10, 10,   0,   0,  -5,
    -5,   5,  20,  20, 20,  20,   5,  -5,
    -5,  10,  20,  30, 30,  20,  10,  -5,
    -5,  10,  20,  30, 30,  20,  10,  -5,
    -5,   5,  20,  10, 10,  20,   5,  -5,
    -5,   0,   0,   0,  0,   0,   0,  -5,
    -5, -10,   0,   0,  0,   0,  -10, -5
};

const int bishopScore[64] = 
{
     0,   0,   0,   0,  0,   0,   0,   0,
     0,   0,   0,   0,  0,   0,   0,   0,
     0,  20,   0,  10, 10,   0,  20,   0,
     0,   0,  10,  20, 20,  10,   0,   0,
     0,   0,  10,  20, 20,  10,   0,   0,
     0,  10,   0,   0,  0,   0,  10,   0,
     0,  30,   0,   0,  0,   0,  30,   0,
     0,   0, -10,   0,  0, -10,   0,   0
};

const int rookScore[64] = 
{
    50,  50,  50,  50, 50,  50,  50,  50,
    50,  50,  50,  50, 50,  50,  50,  50,
    0,   0,   10,  20, 20,  10,   0,   0,
    0,   0,   10,  20, 20,  10,   0,   0,
    0,   0,   10,  20, 20,  10,   0,   0,
    0,   0,   10,  20, 20,  10,   0,   0,
    0,   0,   10,  20, 20,  10,   0,   0,
    0,   0,    0,  20, 20,   0,   0,   0
};

const int kingScore[64] = 
{
     0,   0,   0,   0,  0,   0,   0,   0,
     0,   0,   5,   5,  5,   5,   0,   0,
     0,   5,   5,  10, 10,   5,   5,   0,
     0,   5,  10,  20, 20,  10,   5,   0,
     0,   5,  10,  20, 20,  10,   5,   0,
     0,   0,   5,  10, 10,   5,   0,   0,
     0,   5,   5,  -5, -5,   0,   5,   0,
     0,   0,   5,   0,-15,   0,  10,   0
};

const int mirrorScore [128] = {
    a1, b1, c1, d1, e1, f1, g1, h1, 
    a2, b2, c2, d2, e2, f2, g2, h2, 
    a3, b3, c3, d3, e3, f3, g3, h3, 
    a4, b4, c4, d4, e4, f4, g4, h4, 
    a5, b5, c5, d5, e5, f5, g5, h5, 
    a6, b6, c6, d6, e6, f6, g6, h6, 
    a7, b7, c7, d7, e7, f7, g7, h7, 
    a8, b8, c8, d8, e8, f8, g8, h8
};

unsigned long long fileMasks[64];
unsigned long long rankMasks[64];
unsigned long long isolatedMasks[64];
unsigned long long whitePassedMasks[64];
unsigned long long blackPassedMasks[64];

const int getRank[64] = {
    7, 7, 7, 7, 7, 7, 7, 7,
    6, 6, 6, 6, 6, 6, 6, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    4, 4, 4, 4, 4, 4, 4, 4,
    3, 3, 3, 3, 3, 3, 3, 3,
    2, 2, 2, 2, 2, 2, 2, 2,
    1, 1, 1, 1, 1, 1, 1, 1,
    0, 0, 0, 0, 0, 0, 0, 0
};

const int doublePawnPenalty = -10;
const int isolatedPawnPenalty = -10;
const int passedPawnBonus[8] = { 0, 10, 30, 50, 75, 100, 150, 200 };
const int semiOpenFileScore = 10;
const int openFileScore = 15;
const int kingShieldBonus = 5;

unsigned long long setFileAndRankMasks (int fileNumber,int rankNumber) {
    unsigned long long mask = 0ULL;
    for ( int rank = 0; rank < 8; rank++ ) {
        for ( int file = 0; file < 8; file++ ) {
            int square = 8 * rank + file;
            if ( fileNumber != -1 ) {
                if ( file == fileNumber ) {
                    mask |= (1ULL << square);
                }
            }
            else if ( rankNumber != -1 ){
                if ( rank == rankNumber ) {
                    mask |= (1ULL << square);
                }                
            }
        }
    }
    return mask;
}

void initiateEvaluationMasks () {
    for ( int rank = 0; rank < 8; rank++ ) {
        for ( int file = 0; file < 8; file++ ) {
            int square = 8 * rank + file;
            fileMasks[square] |= setFileAndRankMasks ( file, -1);
        }
    }
    for ( int rank = 0; rank < 8; rank++ ) {
        for ( int file = 0; file < 8; file++ ) {
            int square = 8 * rank + file;
            rankMasks[square] |= setFileAndRankMasks ( -1, rank);
        }
    }
    for ( int rank = 0; rank < 8; rank++ ) {
        for ( int file = 0; file < 8; file++ ) {
            int square = 8 * rank + file;
            isolatedMasks[square] |= setFileAndRankMasks ( file - 1, -1);
            isolatedMasks[square] |= setFileAndRankMasks ( file + 1, -1);
        }
    }
    for ( int rank = 0; rank < 8; rank++ ) {
        for ( int file = 0; file < 8; file++ ) {
            int square = 8 * rank + file;
            whitePassedMasks[square] |= setFileAndRankMasks ( file - 1, -1);
            whitePassedMasks[square] |= setFileAndRankMasks ( file, -1);
            whitePassedMasks[square] |= setFileAndRankMasks ( file + 1, -1);
            
            for ( int i = 0; i < (8 - rank); i++ ) {
                whitePassedMasks[square] &= ~rankMasks[8 * (7 - i) + file];
            }
        }
    }
    for ( int rank = 0; rank < 8; rank++ ) {
        for ( int file = 0; file < 8; file++ ) {
            int square = 8 * rank + file;
            blackPassedMasks[square] |= setFileAndRankMasks ( file - 1, -1);
            blackPassedMasks[square] |= setFileAndRankMasks ( file, -1);
            blackPassedMasks[square] |= setFileAndRankMasks ( file + 1, -1);
            
            for ( int i = 0; i < rank + 1; i++ ) {
                blackPassedMasks[square] &= ~rankMasks[8 * i + file];
            }
        }
    }
}

static inline int evaluate(){
    int score = 0;
    unsigned long long bitboard;
    int piece, square;
    int doublePawns = 0;
    for ( int piece2 = 0; piece2 <= 11; piece2++ ) {
        bitboard = bitboards[piece2];
        while ( bitboard ) {
            piece = piece2;
            square = getTheLeastSignificantBitIndex(bitboard);
            score += materialScore[piece];
            switch(piece){
                case P: 
                    score += pawnScore[square]; 
                    doublePawns = countBits(bitboards[P] & fileMasks[square]);
                    if ( doublePawns > 1 ) {
                        score += doublePawns * doublePawnPenalty;
                    }
                    if ( ( bitboards[P] & isolatedMasks[square] ) == 0 ) {
                        score += isolatedPawnPenalty;
                    }
                    if ( ( whitePassedMasks [ square ] & bitboards[p] ) == 0 ) {
                        score += passedPawnBonus [ getRank [ square ]];
                    }
                    break;
                case N: score += knightScore[square]; break;
                case B: 
                    score += bishopScore[square];
                    score += countBits ( getBishopAttacks ( square, occupancies [ both ] ));
                    break;
                case R: 
                    score += rookScore[square]; 
                    if ( ( bitboards[P] & fileMasks[square] ) == 0 ) {
                        score += semiOpenFileScore;
                    }
                    if ( ( ( bitboards [ P ] | bitboards [ p ] ) & fileMasks [ square ] ) == 0 ) {
                        score += openFileScore;
                    }
                    
                    break;
                
                case Q:
                    score += countBits ( getQueenAttacks ( square, occupancies [ both ] ));
                    break;
                case K: 
                    score += kingScore[square]; 
                    if ( ( bitboards[P] & fileMasks[square] ) == 0 ) {
                        score -= semiOpenFileScore;
                    }
                    if ( ( ( bitboards [ P ] | bitboards [ p ] ) & fileMasks [ square ] ) == 0 ) {
                        score -= openFileScore;
                    }
                    score += countBits ( kingAttacks[square] & occupancies[white] ) * kingShieldBonus;
                    break;
                
                case p: 
                    score -= pawnScore[mirrorScore[square]]; 
                    doublePawns = countBits(bitboards[p] & fileMasks[square]);
                    if ( doublePawns > 1 ) {
                        score -= doublePawns * doublePawnPenalty;
                    }                    
                    if ( ( bitboards[p] & isolatedMasks[square] ) == 0 ) {
                        score -= isolatedPawnPenalty;
                    }
                    if ( ( blackPassedMasks [ square ] & bitboards[P] ) == 0 ) {
                        score -= passedPawnBonus [ getRank [ mirrorScore [ square ] ]];
                    }
                    break;
                case n: score -= knightScore[mirrorScore[square]]; break;
                case b: 
                    score -= bishopScore[mirrorScore[square]]; 
                    score -= countBits ( getBishopAttacks ( square, occupancies [ both ] ));                    
                    break;
                case r: 
                    score -= rookScore[mirrorScore[square]]; 
                    if ( ( bitboards[p] & fileMasks[square] ) == 0 ) {
                        score -= semiOpenFileScore;
                    }
                    if ( ( ( bitboards [ P ] | bitboards [ p ] ) & fileMasks [ square ] ) == 0 ) {
                        score -= openFileScore;
                    }
                    break;
                case q:
                    score -= countBits ( getQueenAttacks ( square, occupancies [ both ] ));
                    break;
                case k: 
                    score -= kingScore[mirrorScore[square]];
                    if ( ( bitboards[p] & fileMasks[square] ) == 0 ) {
                        score += semiOpenFileScore;
                    }
                    if ( ( ( bitboards [ P ] | bitboards [ p ] ) & fileMasks [ square ] ) == 0 ) {
                        score += openFileScore;
                    }
                    score -= countBits ( kingAttacks[square] & occupancies[black] ) * kingShieldBonus;
                    break;
            }
            bitboard &= ~(1ULL << square);
        }
    }
    return (side==0)?score:-score;
}

#define infinity 50000
#define mateValue 49000
#define mateScore 48000

static int mvvLva[12][12] = {
    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605,  105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604,  104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603,  103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602,  102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601,  101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600,  100, 200, 300, 400, 500, 600,
};

#define maxPly 64

int killerMoves[2][maxPly];
int historyMoves[12][maxPly];

int pvLength[maxPly];
int pvTable[maxPly][maxPly];

int followPv, scorePv;

#define hashSize 800000
#define noHashEntry 100000
#define hashFlagExact 0
#define hashFlagAlpha 1
#define hashFlagBeta 2

typedef struct{
    unsigned long long hashKey;
    int depth;
    int flag;
    int score;
} tt;

tt hashTable [hashSize];

void clearHashTable () {
    for ( int index=0;index<hashSize; index++ ){
        hashTable[index].hashKey = 0;
        hashTable[index].depth = 0;
        hashTable[index].flag = 0;
        hashTable[index].score = 0;
    }
}

static inline int readHashEntry(int alpha, int beta, int depth){
    tt *hashEntry = &hashTable[hashKey % hashSize];
    if ( hashEntry->hashKey == hashKey) {
        if (hashEntry->depth >= depth ) {
            int score = hashEntry->score;
            if ( score < -mateScore ) score += ply;
            if ( score > mateScore ) score -= ply;
            if ( hashEntry->flag == hashFlagExact ) {
                return score;
            }
            if (( hashEntry->flag == hashFlagAlpha ) && (score <= alpha)) {
                return alpha;
            }
            if (( hashEntry->flag == hashFlagBeta ) && (score >= beta)) {
                return beta;
            }
        }
    }
    return noHashEntry;
}

static inline void writeHashEntry(int score, int depth, int hashFlag){
    tt *hashEntry = &hashTable[hashKey % hashSize];
    if ( score < -mateScore ) score -= ply;
    if ( score > mateScore ) score += ply;
    hashEntry->hashKey = hashKey;
    hashEntry->score = score;
    hashEntry->flag = hashFlag;
    hashEntry->depth = depth;
}

static inline void enablePvScoring(moves * moveList){
	followPv = 0;

    for ( int count = 0; count < moveList->count; count++ ) {
        if ( pvTable[0][ply] == moveList->moves[count] ) {
			scorePv = 1;
			followPv=1;
		}
    }
	
}

static inline int scoreMove(int move ) {
	if ( scorePv){
		if ( pvTable[0][ply] == move ) {
			scorePv = 0;
			return 20000;
		}
	}
    if ( getMoveCapture(move)){
        int targetPiece = P;
        int startPiece, endPiece;
        if ( side==white ) { startPiece = p; endPiece = k; }
        else { startPiece = P; endPiece = K; }
        for ( int piece = startPiece; piece <= endPiece; piece++ ) {
            if ( bitboards[piece] & ( 1ULL << getMoveTarget(move) ) ) {
                targetPiece = piece;
                break;
            }
        }
        return mvvLva[getMovePiece(move)][targetPiece] + 10000;
    }
    else {
        if ( killerMoves[0][ply] == move ) return 9000;
        else if ( killerMoves[1][ply] == move ) return 8000;
        else return historyMoves[getMovePiece(move)][getMoveTarget(move)];
    }
    return 0;
}

static inline int sortMoves(moves * moveList) {
    int moveScores [moveList->count];
    for ( int count = 0; count < moveList->count; count++ ) {
        moveScores[count] = scoreMove(moveList->moves[count]);
    }
    for ( int currentMove = 0; currentMove < moveList->count; currentMove++ ) {
        for ( int nextMove = currentMove; nextMove < moveList->count; nextMove++ ) {
            if ( moveScores[currentMove] < moveScores[nextMove]){
                int tempScore = moveScores[currentMove];
                moveScores[currentMove] = moveScores[nextMove];
                moveScores[nextMove] = tempScore;
                
                int tempMove = moveList->moves[currentMove];
                moveList->moves[currentMove] = moveList->moves[nextMove];
                moveList->moves[nextMove] = tempMove;

            }
        }
    } 
}

void printMoveScores(moves * moveList){
    printf("    Move scores\n\n");
    for ( int count = 0; count < moveList->count; count++ ) {
        printf("    move: ");
        printMove(moveList->moves[count]);
        printf(" score: %d\n", scoreMove(moveList->moves[count]));
    }    
}
    
static inline int isRepetition () {
    for ( int index = 0; index < repetitionIndex; index++){
        if ( repetitionTable[index] == hashKey ) return 1;
    }
    return 0;
}
    
static inline int quiescence(int alpha, int beta){
    if ( ( numberOfLeafNodes & 2047 ) == 0 ) communicate();
    numberOfLeafNodes++;
	if ( ply > maxPly - 1) return evaluate ();
    int evaluation = evaluate ();
    if ( evaluation >= beta ) {
        return beta;
    }
    if ( evaluation > alpha){
        alpha = evaluation;
    }
    moves moveList[1];
    generateMoves(moveList);
    sortMoves(moveList);
    for ( int count= 0; count < moveList->count; count++){
        copyBoard();
        ply++;
        repetitionIndex++;
        repetitionTable [ repetitionIndex ] = hashKey;
        if ( makeMove(moveList->moves[count], onlyCaptures) == 0 ){
            ply--;
            repetitionIndex--;
            continue;
        }
        int score = -quiescence(-beta, -alpha);
        ply--;
        repetitionIndex--;
        takeBack();
        if ( stopped == 1 ) return 0;
        if ( score > alpha){
            alpha = score;
			if ( score >= beta ) {
				return beta;
			}
        }
    }
    return alpha;
}

const int fullDepthMoves = 4;
const int reductionLimit = 3;

static inline int negamax(int alpha, int beta, int depth){
	int score;
	int hashFlag = hashFlagAlpha;
    if ( ply && isRepetition () ) {
        return 0;
    }    
    int pvNode = beta - alpha > 1;
	if (ply && (score = readHashEntry(alpha, beta, depth)) != noHashEntry && pvNode == 0 ) {
		return score; 
	}
    if ( ( numberOfLeafNodes & 2047 ) == 0 ) communicate();
    int foundPv = 0;
	pvLength[ply] = ply;
    if ( depth == 0 ) return quiescence(alpha, beta);
	if ( ply > maxPly - 1) return evaluate ();
    numberOfLeafNodes++;
    int inCheck = isSquareAttacked((side==0)?getTheLeastSignificantBitIndex(bitboards[K]):getTheLeastSignificantBitIndex(bitboards[k]), side^1);
    if ( inCheck ) depth++;
    int legalMoves = 0;
    if ( depth >= 3 && inCheck == 0 && ply ) {
        copyBoard ();
		ply++;
        repetitionIndex++;
        repetitionTable [ repetitionIndex ] = hashKey;
		if ( enpassant != noSq ) hashKey ^= enpassantKeys[enpassant];
        enpassant = noSq;
        side ^= 1;
		hashKey ^= sideKey;
        int score = -negamax(-beta, -beta + 1, depth - 1 - 2);
		ply--;
        repetitionIndex--;
        takeBack ();
        if ( stopped == 1 ) return 0;
        if ( score >= beta ) return beta;
    }
    moves moveList[1];
    generateMoves(moveList);
	if ( followPv ) enablePvScoring(moveList);
    sortMoves(moveList);
    int movesSearched = 0;
    for ( int count= 0; count < moveList->count; count++){
        copyBoard();
        ply++;
        repetitionIndex++;
        repetitionTable [ repetitionIndex ] = hashKey;        
        if ( makeMove(moveList->moves[count], allMoves) == 0 ){
            ply--;
            repetitionIndex--;
            continue;
        }
        legalMoves++;
        
        if ( movesSearched == 0 ) {
            score = -negamax(-beta, -alpha, depth-1 );
        }
        else {
            if ( movesSearched >= fullDepthMoves && depth >= reductionLimit && inCheck == 0 && getMoveCapture(moveList->moves[count]) == 0 && getMovePromoted(moveList->moves[count]) == 0) {
                score = -negamax(-alpha - 1, -alpha, depth - 2);
            }
            else score = alpha + 1;
            if ( score > alpha ) {
                score = -negamax(-alpha - 1, -alpha, depth - 1);
                if ( (score > alpha) && (score < beta) ) {
                    score = -negamax(-beta, -alpha, depth - 1);
                }   
            }
        }

        ply--;
        repetitionIndex--;
        takeBack();
        if ( stopped == 1 ) return 0;
        movesSearched++;
        if ( score > alpha){
			hashFlag = hashFlagExact;
			if ( getMoveCapture(moveList->moves[count]) == 0 ) {
				historyMoves[getMovePiece(moveList->moves[count])][getMoveTarget(moveList->moves[count])] += depth;
			}
            alpha = score;
			pvTable[ply][ply] = moveList->moves[count];
			for ( int nextPly = ply + 1; nextPly < pvLength[ply + 1]; nextPly++ ) {
				pvTable[ply][nextPly] = pvTable[ply + 1][nextPly];
			}
			pvLength[ply] = pvLength[ply + 1];
			
			if ( score >= beta ) {
				writeHashEntry ( beta, depth, hashFlagBeta );
				if ( getMoveCapture(moveList->moves[count]) == 0 ) {
					killerMoves[1][ply] = killerMoves[0][ply];
					killerMoves[0][ply] = moveList->moves[count];
				}
				return beta;
			}
			
        }
    }
    if ( legalMoves == 0 ) {
        if ( inCheck ) return -mateValue + ply; 
        else return 0;
    }
	
	writeHashEntry(alpha, depth, hashFlag);
	
    return alpha;
}

void searchPosition(int depth){

	int score = 0;

	numberOfLeafNodes = 0;
    
    stopped = 0;
	
	followPv = 0;
	scorePv = 0;
	
	memset(killerMoves, 0, sizeof(killerMoves));
	memset(historyMoves, 0, sizeof(historyMoves));
	memset(pvTable, 0, sizeof(pvTable));
	memset(pvLength, 0, sizeof(pvLength));
	
    int alpha = -infinity;
    int beta = infinity;
	
	for ( int currentDepth = 1; currentDepth <= depth; currentDepth++ ) {
        
        if ( stopped == 1 ) break;
		
		followPv = 1;
		
		score = negamax(alpha, beta, currentDepth);
        if ( ( score <= alpha ) || ( score >= beta ) ){
            alpha = -infinity;
            beta = infinity;
            continue;
        }
        alpha = score - 50;
        beta = score + 50;
        if ( score > -mateValue && score < -mateScore ) {
            printf("info score mate %d depth %d nodes %lld time %d pv ", -(score + mateValue) / 2 + 1, currentDepth, numberOfLeafNodes, get_time_ms () - starttime );            
        }
        else if ( score > mateScore && score < mateValue ) {
            printf("info score mate %d depth %d nodes %lld time %d pv ", (mateValue - score) / 2 - 1, currentDepth, numberOfLeafNodes, get_time_ms () - starttime );            
        }
        else printf("info score cp %d depth %d nodes %lld time %d pv ", score, currentDepth, numberOfLeafNodes, get_time_ms () - starttime );            
		for ( int count = 0; count < pvLength[0]; count++ ) {
			printMove(pvTable[0][count]);
			printf(" ");
		}
		printf("\n");
	}
	printf("bestmove ");
	printMove(pvTable[0][0]);
	printf("\n");
}

int parseMove(char * moveString){
	
	moves moveList[1];
	generateMoves(moveList);
	
	int sourceSquare = ( moveString[0] - 'a') + 8 * (8 - (moveString[1] - '0'));
	int targetSquare =  ( moveString[2] - 'a') + 8 * (8 - (moveString[3] - '0'));
	int promotedPiece = 0;
	for ( int moveCount = 0; moveCount < moveList->count; moveCount++ ) {
		int move = moveList->moves[moveCount];
		if ( sourceSquare == getMoveSource(move) && targetSquare == getMoveTarget(move) ) {
			int promotedPiece = getMovePromoted(move);
			if (promotedPiece ){
				if ( ( promotedPiece == Q || promotedPiece == q) && moveString[4] == 'q') return move;
				if ( ( promotedPiece == R || promotedPiece == r) && moveString[4] == 'r') return move;
				if ( ( promotedPiece == B || promotedPiece == b) && moveString[4] == 'b') return move;
				if ( ( promotedPiece == N || promotedPiece == n) && moveString[4] == 'n') return move;
				continue;
			}
			return move;
		}
	}
	
	return 0;
}

void parsePosition(char * command){
    command += 9;
    char * currentChar = command;
    if ( strncmp(command, "startpos", 8) == 0 ) parseFen(start_position);
    else {        
        currentChar = strstr(command, "fen");
        if ( currentChar == NULL  ) parseFen(start_position);
        else {
            currentChar += 4;
            parseFen(currentChar);
        }
    }
    currentChar = strstr(command, "moves");
    if ( currentChar != NULL ) {
        currentChar += 6;
        while(*currentChar){
            int move = parseMove(currentChar);
            if (move == 0) break;
            repetitionIndex++;
            repetitionTable[repetitionIndex] = hashKey;
            makeMove(move, allMoves);
            while(*currentChar && *currentChar != ' ') currentChar++;
            currentChar++;
        }
    }
}

void parseGo(char * command){
    // init parameters
    int depth = -1;

    // init argument
    char *argument = NULL;

    // infinite search
    if ((argument = strstr(command,"infinite"))) {}

    // match UCI "binc" command
    if ((argument = strstr(command,"binc")) && side == black)
        // parse black time increment
        inc = atoi(argument + 5);

    // match UCI "winc" command
    if ((argument = strstr(command,"winc")) && side == white)
        // parse white time increment
        inc = atoi(argument + 5);

    // match UCI "wtime" command
    if ((argument = strstr(command,"wtime")) && side == white)
        // parse white time limit
        time = atoi(argument + 6);

    // match UCI "btime" command
    if ((argument = strstr(command,"btime")) && side == black)
        // parse black time limit
        time = atoi(argument + 6);

    // match UCI "movestogo" command
    if ((argument = strstr(command,"movestogo")))
        // parse number of moves to go
        movestogo = atoi(argument + 10);

    // match UCI "movetime" command
    if ((argument = strstr(command,"movetime")))
        // parse amount of time allowed to spend to make a move
        movetime = atoi(argument + 9);

    // match UCI "depth" command
    if ((argument = strstr(command,"depth")))
        // parse search depth
        depth = atoi(argument + 6);

    // if move time is not available
    if(movetime != -1)
    {
        // set time equal to move time
        time = movetime;

        // set moves to go to 1
        movestogo = 1;
    }

    // init start time
    starttime = get_time_ms();

    // init search depth
    depth = depth;

    // if time control is available
    if(time != -1)
    {
        // flag we're playing with time control
        timeset = 1;

        // set up timing
        time /= movestogo;
        if ( time > 1500 ) time -= 50;
        stoptime = starttime + time + inc;
    }

    // if depth is not available
    if(depth == -1)
        // set depth to 64 plies (takes ages to complete...)
        depth = 64;

    // print debug info
    printf("time:%d start:%d stop:%d depth:%d timeset:%d\n",
    time, starttime, stoptime, depth, timeset);

    // search position
    searchPosition(depth);
 }

void uciLoop (){
    setbuf(stdin, NULL);
    setbuf(stdout, NULL);
    char input[2000];
    printf("id name ID_NAME\n");
    printf("uciok\n");
    while(1){
        memset(input, 0, sizeof(input));
        fflush(stdout);
        if ( !fgets(input, 2000, stdin)) continue;
        if ( input[0] == '\n') continue;
        if ( strncmp(input, "isready", 7) == 0 ) {
            printf("readyok\n");
            continue;
        }
        else if ( strncmp(input, "position", 8) == 0 ) {
            parsePosition(input);
            clearHashTable ();
        }
        else if ( strncmp(input, "ucinewgame", 10) == 0 ) {
            parsePosition("position startpos");
            clearHashTable ();
        }
        else if ( strncmp(input, "go", 2) == 0 ) {
            parseGo(input);
        }
        else if ( strncmp(input, "quit", 4) == 0 ) {
            break;
        }
        else if ( strncmp(input, "uci", 3) == 0 ) {
            printf("id name ID_NAME\n");
            printf("uciok\n");
        }        
    }
}



void initializeEverything () {
    initLeaperAttacks ();
    initializeSlidingPiecesAttacks(1);
    initializeSlidingPiecesAttacks(0);
    initializeRandomKeys ();
    clearHashTable ();
    initiateEvaluationMasks ();
}

int main () {
    
    initializeEverything ();    
    
    int debug = 0;
    
    if ( debug ) {
		parseFen("6k1/ppppprbp/8/8/8/8/PPPPPRBP/6K1 w - - ");
        printBoard ();
        printf("score: %d\n", evaluate ());
        //searchPosition(10);
    }
    
    else uciLoop ();
    return 0;
}


