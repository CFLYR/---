#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

typedef long long LLS; // ���峤���ͱ���LLS
using namespace std;

#define GRID_DIM 12 // ����ά��
#define BLANK 0 // �հ�λ�ñ�ʶ
#define PIECE_B 1 // �����ʶ
#define PIECE_W 2 // �����ʶ
const LLS MAX_VAL = 2000000000000000000LL; // ���ֵ
const LLS MIN_VAL = -2000000000000000000LL; // ��Сֵ

// �����
#define CMD_INIT "START" // ��ʼ������
#define CMD_SET "PLACE" // ��������
#define CMD_PLAY "TURN" // ��Ϸ�ִ�����
#define CMD_FINISH "END" // ��������
#define CMD_CRACKER "CRACK" // �ƽ�����
#define CMD_ERASE "ERASE" // ��������
#define CMD_LIMIT_CHECK "LIMIT" // ���Ƽ������
#define CMD_SHOW_BOARD "SHOW" // ��ʾ��������
#define CMD_BUGMODE "LIBERTY_STEP" // ����ģʽ����

bool bugmode = false; // ����ģʽ����

// ������������
#define FIVE5 7 // ������������
#define LIVE4 6 // ��������
#define RUSH4 5 // ��������
#define LIVE3 4 // ��������
#define SLEEPING3 3 // ˯������
#define LIVE2 2 // �������
#define SLEEPING2 1 // ˯������
#define JUMPLIVE3 8 // ����������
#define JUMPSLEEPING4 9 // ��˯������
#define JUMPLIVE4 10 // ����������

#define STYLENUM 11 // �������

#define STYLE_WEIGHT 1 // ���Ȩ��

#define MAX_DEPTH 7 // ����������
#define TIME_LIMIT 1900 // ʱ�����ƣ����룩

clock_t start_turn; // ��¼�غϿ�ʼʱ��

int turnround = 0; // ��ǰ�غ���

int myColor; // �ҵ���ɫ
int rivalColor; // ���ֵ���ɫ

const int CENTER_POS_1 = (GRID_DIM + 1) / 2 - 1; // ��������λ��1
const int CENTER_POS_2 = GRID_DIM / 2; // ��������λ��2
const int dx[] = { 1, 0, 1, 1 }; // ��������x
const int dy[] = { 0, 1, 1, -1 }; // ��������y
// Zobrist��ϣ���С
#define HASH_SIZE (1 << 20) // 2��20�η�
#define HASH_EXACT 0 // ��ȷ��ϣ����
#define HASH_ALPHA 1 // Alpha��ϣ����
#define HASH_BETA 2 // Beta��ϣ����

typedef unsigned long long ULL; // �����޷��ų����ͱ���ULL
// Zobrist��ϣ��
ULL zobristTable[3][GRID_DIM][GRID_DIM]; // Zobrist��ϣ��
ULL currentHash; // ��ǰ��ϣֵ

// ��ϣ��Ŀ�ṹ
struct HashEntry {
    ULL key; // ��ϣ��
    int depth; // ���
    LLS score; // ����
    int type; // ����
};

// ��ϣ��
HashEntry* hashTable;

#define MAX_MOVES 121 // ����ƶ���

// ���νṹ
struct chessshape {
    int score; // ��������
    char shape[9]; // ���α�ʾ
};

// ���ͽṹ
struct ChessStyle {
    LLS score; // ���͵÷�
    ChessStyle(int s = 0) // ���캯��
        : score(s) // ��ʼ���÷�
    {
    }
};

// �������ͽṹ
struct AllChessStyle {
    ChessStyle MyStyle[STYLENUM]; // �ҵ����ͷ��
    ChessStyle EnemyStyle[STYLENUM]; // �з����ͷ��
    AllChessStyle() // ���캯��
    {
        MyStyle[0].score = 0; // ��ʼ������
        MyStyle[SLEEPING2].score = 10;
        MyStyle[LIVE2].score = 100;
        MyStyle[SLEEPING3].score = 100;
        MyStyle[LIVE3].score = 1050;
        MyStyle[RUSH4].score = 1000;
        MyStyle[LIVE4].score = 1100000;
        MyStyle[FIVE5].score = 1000000000;
        MyStyle[JUMPLIVE3].score = 900;
        MyStyle[JUMPSLEEPING4].score = 700;
        MyStyle[JUMPLIVE4].score = 1000;

        // ��ʼ���з����ͷ��
        for (int i = 0; i < STYLENUM; i++) {
            EnemyStyle[i].score = LLS(MyStyle[i].score * STYLE_WEIGHT);
        }
    }
};

// λ�ýṹ
struct Position {
    int row = -1; // ��
    int col = -1; // ��
    LLS score = 0; // ����
    Position(int r = -1, int c = -1) // ���캯��
    {
        row = r; // ��ʼ����
        col = c; // ��ʼ����
    }
};

// �ƶ��б�ṹ
struct MoveList {
    Position moves[MAX_MOVES]; // �ƶ�����
    int count = 0; // �ƶ�����
};

// ��������λ��
void swapPositions(Position& pos1, Position& pos2)
{
    Position temp = pos1; // ��ʱ�洢pos1
    pos1 = pos2; // pos1��ֵΪpos2
    pos2 = temp; // pos2��ֵΪtemp
}

// ����64λ�����
ULL random64()
{
    return ((ULL)rand() << 32) | ((ULL)rand() << 16) | (ULL)rand(); // ���������
}

// ���ҹ�ϣ
LLS lookupHash(ULL hash, int depth, LLS alpha, LLS beta)
{
    HashEntry& entry = hashTable[hash % HASH_SIZE]; // ��ȡ��ϣ����Ŀ

    // ����ϣ����Ŀ�Ƿ���Ч
    if (entry.key == hash && entry.depth >= depth) {
        if (entry.type == HASH_EXACT)
            return entry.score; // ���ؾ�ȷ����
        if (entry.type == HASH_ALPHA && entry.score <= alpha)
            return alpha; // ����alphaֵ
        if (entry.type == HASH_BETA && entry.score >= beta)
            return beta; // ����betaֵ
    }
    return MIN_VAL - 1; // ������Сֵ
}

// �洢��ϣ
void storeHash(ULL hash, int depth, LLS score, int type)
{
    HashEntry& entry = hashTable[hash % HASH_SIZE]; // ��ȡ��ϣ����Ŀ

    // ���¹�ϣ����Ŀ
    if (entry.depth <= depth) {
        entry.key = hash; // ���ù�ϣ��
        entry.depth = depth; // �������
        entry.score = score; // ��������
        entry.type = type; // ��������
    }
}

// ��Ϸ״̬�ṹ
struct GameState {
    int grid[GRID_DIM][GRID_DIM] = { { 0 } }; // ��������
    int gamePhase = 0; // ��Ϸ�׶�
    ULL hash; // ��ǰ��ϣֵ

    GameState() // ���캯��
    {
        initializeBoard(); // ��ʼ������
    }

    // ��ʼ������
    void initializeBoard()
    {
        memset(grid, 0, sizeof(grid)); // �������
        grid[CENTER_POS_1][CENTER_POS_1] = PIECE_W; // �������ĺ���
        grid[CENTER_POS_2][CENTER_POS_2] = PIECE_W; // �������İ���
        grid[CENTER_POS_2][CENTER_POS_1] = PIECE_B; // ���ò�ߺ���
        grid[CENTER_POS_1][CENTER_POS_2] = PIECE_B; // ���ò�߰���

        // �����ʼ��ϣֵ
        hash = 0; // ��ʼ����ϣ
        for (int i = 0; i < GRID_DIM; i++) {
            for (int j = 0; j < GRID_DIM; j++) {
                if (grid[i][j] != BLANK) {
                    hash ^= zobristTable[grid[i][j]][i][j]; // �����ϣֵ
                }
            }
        }
    }

    // ��������
    void updateGrid(int row, int col, int piece)
    {
        // �Ƴ������ӵĹ�ϣ
        hash ^= zobristTable[grid[row][col]][row][col];
        // ��������ӵĹ�ϣ
        hash ^= zobristTable[piece][row][col];
        grid[row][col] = piece; // ��������
    }
};

GameState gameBoard; // ������Ϸ״̬����

// ��������Ƿ�Ϊż��
bool isEven2(int number)
{
    return number % 2 == 0; // �ж��Ƿ�Ϊż��
}

// ��ʼ��Zobrist��ϣ��
void initZobrist()
{
    srand(size_t(time(NULL))); // �����������
    for (int piece = 0; piece < 3; piece++) {
        for (int i = 0; i < GRID_DIM; i++) {
            for (int j = 0; j < GRID_DIM; j++) {
                zobristTable[piece][i][j] = random64(); // ��ʼ��Zobrist��ϣ��
            }
        }
    }

    // ��ʼ����ϣ��
    hashTable = new HashEntry[HASH_SIZE]; // �����ϣ���ڴ�
    memset(hashTable, 0, sizeof(HashEntry) * HASH_SIZE); // ��չ�ϣ��

    // ���㵱ǰ��ϣֵ
    currentHash = 0; // ��ʼ����ǰ��ϣ
    for (int i = 0; i < GRID_DIM; i++) {
        for (int j = 0; j < GRID_DIM; j++) {
            if (gameBoard.grid[i][j] != BLANK) {
                currentHash ^= zobristTable[gameBoard.grid[i][j]][i][j]; // �����ϣֵ
            }
        }
    }
}

// ���ֺ���
int scoreDirects(GameState* state, Position pos, int dx, int dy, int player)
{
    int count = 1; // �������Ӽ���
    int empty_pos = 0; // ��λ����: 1=����, 2=˫��
    int empty_count = 0; // ��λ����
    int block = 0; // �赲����
    int jump_p = 0; // ��Ծ���������
    int jump_n = 0; // ��Ծ���������
    int count_j_p = 0; // ��������Ծ����
    bool block_j_p = 0; // �������赲��־
    bool empty_j_p = 0; // �������λ��־
    int count_j_n = 0; // ��������Ծ����
    bool block_j_n = 0; // �������赲��־
    bool empty_j_n = 0; // �������λ��־

    // ���������
    int tx = pos.row + dx; // ��������
    int ty = pos.col + dy; // ��������
    bool found_empty = false; // �Ƿ��ҵ���λ
    while (tx >= 0 && tx < GRID_DIM && ty >= 0 && ty < GRID_DIM) {
        if (found_empty && !jump_p) { // ������ҵ���λ��δ��Ծ
            if (state->grid[tx][ty] == player) {
                jump_p = 1; // ���Ϊ��Ծ
                count_j_p++; // ��Ծ��������
            } else {
                break; // ����ѭ��
            }
        } else if (found_empty && jump_p) { // ������ҵ���λ������Ծ
            if (state->grid[tx][ty] == player) {
                count_j_p++; // ��Ծ��������
            } else if (state->grid[tx][ty] == 3 - player) {
                block_j_p = true; // ����赲
                break; // ����ѭ��
            } else {
                empty_j_p = true; // ��ǿ�λ
                break; // ����ѭ��
            }
        } else { // ���δ�ҵ���λ
            if (state->grid[tx][ty] == BLANK) {
                empty_count++; // ��λ��������
                empty_pos += 1; // ��λ״̬����
                found_empty = true; // ������ҵ���λ
            } else if (state->grid[tx][ty] != player) {
                block++; // �赲��������
                break; // ����ѭ��
            } else {
                count++; // �������Ӽ�������
            }
        }
        tx += dx; // �ƶ�����һ��λ��
        ty += dy; // �ƶ�����һ��λ��
    }

    // ��鷴����
    tx = pos.row - dx; // ��������
    ty = pos.col - dy; // ��������
    found_empty = false; // ���ÿ�λ��־
    while (tx >= 0 && tx < GRID_DIM && ty >= 0 && ty < GRID_DIM) {
        if (found_empty && !jump_n) { // ������ҵ���λ��δ��Ծ
            if (state->grid[tx][ty] == player) {
                jump_n = 1; // ���Ϊ��Ծ
                count_j_n++; // ��Ծ��������
            } else {
                break; // ����ѭ��
            }
        } else if (found_empty && jump_n) { // ������ҵ���λ������Ծ
            if (state->grid[tx][ty] == player) {
                count_j_n++; // ��Ծ��������
            } else if (state->grid[tx][ty] == 3 - player) {
                block_j_n = true; // ����赲
                break; // ����ѭ��
            } else {
                empty_j_n = true; // ��ǿ�λ
                break; // ����ѭ��
            }
        } else { // ���δ�ҵ���λ
            if (state->grid[tx][ty] == BLANK) {
                empty_count++; // ��λ��������
                empty_pos += 2; // ����˫��λ״̬
                found_empty = true; // ������ҵ���λ
            } else if (state->grid[tx][ty] != player) {
                block++; // �赲��������
                break; // ����ѭ��
            } else {
                count++; // �������Ӽ�������
            }
        }

        tx -= dx; // �ƶ�����һ��λ��
        ty -= dy; // �ƶ�����һ��λ��
    }

    // ���ּ���
    if (jump_p && jump_n) {
        // ��Ծ���
    } else if (jump_p) {
        if (count + count_j_p == 3 && empty_pos == 3 && empty_j_p) {
            return JUMPLIVE3; // ������
        } else if (count + count_j_p == 4 && empty_pos == 3 && block_j_p) {
            return JUMPSLEEPING4; // ��˯��
        } else if (count + count_j_p == 4 && empty_pos == 1 && empty_j_p) {
            return JUMPSLEEPING4; // ��˯��
        } else if (count + count_j_p == 4 && empty_pos == 3 && empty_j_p) {
            return JUMPLIVE4; // ������
        }
    } else if (jump_n) {
        if (count + count_j_n == 3 && empty_pos == 3 && empty_j_n) {
            return JUMPLIVE3; // ������
        } else if (count + count_j_n == 4 && empty_pos == 3 && block_j_n) {
            return JUMPSLEEPING4; // ��˯��
        } else if (count + count_j_n == 4 && empty_pos == 2 && empty_j_n) {
            return JUMPSLEEPING4; // ��˯��
        } else if (count + count_j_n == 4 && empty_pos == 3 && empty_j_n) {
            return JUMPLIVE4; // ������
        }
    }
    if (count >= 5)
        return FIVE5; // ��������

    if (count == 4) {
        if (empty_pos == 3)
            return LIVE4; // ����
        if (empty_pos > 0)
            return RUSH4; // ����
        return 0; // ������
    }

    if (count == 3) {
        if (empty_pos == 3)
            return LIVE3; // ����
        if (empty_pos > 0)
            return SLEEPING3; // ˯��
        return 0; // ������
    }

    if (count == 2) {
        if (empty_pos == 3)
            return LIVE2; // ���
        if (empty_pos > 0)
            return SLEEPING2; // ˯��
        return 0; // ������
    }

    return 0; // ������
}

// �������
LLS scorePlayer(GameState* state, Position pos, int player)
{
    AllChessStyle styles; // �������ͷ�����
    LLS totalScore = 0; // �����ֳ�ʼ��Ϊ0
    int i;
    for (i = 0; i < 4; i++) { // �����ĸ�����
        int Style = scoreDirects(state, pos, dx[i], dy[i], player); // ��ȡ�÷������������
        totalScore += styles.MyStyle[Style].score; // �ۼ�����
    }
    // ������������ض���Χ�ڣ�ǿ�����÷���
    if (totalScore >= 1400 && totalScore < 1000000) {
        totalScore = 1000000; // ����Ϊ1000000
    }

    return totalScore; // ����������
}

// ����λ��
LLS scorePosts(GameState* state, Position pos)
{
    AllChessStyle styles; // �������ͷ�����
    LLS totalScore = 0; // �����ֳ�ʼ��Ϊ0
    int i;
    LLS myScore = 0; // �ҷ�����
    LLS rivalScore = 0; // �з�����
    for (i = 0; i < 4; i++) { // �����ĸ�����
        int myStyle = scoreDirects(state, pos, dx[i], dy[i], myColor); // �����ҷ���������
        int rivalStyle = scoreDirects(state, pos, dx[i], dy[i], rivalColor); // ����з���������

        myScore += styles.MyStyle[myStyle].score; // �ۼ��ҷ�����
        rivalScore += styles.EnemyStyle[rivalStyle].score; // �ۼӵз�����
    }
    // ����з��������ض���Χ�ڣ�ǿ�����÷���
    if (rivalScore >= 1400 && rivalScore < 1000000) {
        rivalScore = 1000000; // ����Ϊ1000000
    }
    // ����ҷ��������ض���Χ�ڣ�ǿ�����÷���
    if (myScore >= 1400 && myScore < 1000000) {
        myScore = 1000000; // ����Ϊ1000000
    }
    totalScore += myScore; // ���ҷ����ּ���������
    totalScore += rivalScore; // ���з����ּ���������

    return totalScore; // ����������
}

// ����Ƿ��к�������
int hasNexter(GameState* state, int row, int col)
{
    const int directions[8][2] = { // ����˸�����
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 },
        { -1, -1 }, { -1, 1 }, { 1, -1 }, { 1, 1 }
    };

    for (int d = 0; d < 8; d++) { // �����˸�����
        for (int distance = 1; distance <= 2; distance++) { // ���1��2����λ�ľ���
            int ni = row + directions[d][0] * distance; // ��������
            int nj = col + directions[d][1] * distance; // ��������

            // �����λ�������̷�Χ���Ҳ��ǿհ�
            if (ni >= 0 && ni < GRID_DIM && nj >= 0 && nj < GRID_DIM && state->grid[ni][nj] != BLANK) {
                return 1; // �ҵ��������ӣ�����1
            }
        }
    }
    return 0; // δ�ҵ��������ӣ�����0
}

// �����ƶ�
int partitionMoves(Position moveArr[], int startIdx, int endIdx, GameState* state, int player)
{
    Position pivotPos = moveArr[endIdx]; // ѡ�����һ��Ԫ����Ϊ��׼
    LLS pivotScore = pivotPos.score; // ��׼����
    int smallerIdx = (startIdx - 1); // С�ڻ�׼��Ԫ������

    for (int currentIdx = startIdx; currentIdx <= endIdx - 1; currentIdx++) { // ��������
        LLS currentScore = moveArr[currentIdx].score; // ��ǰ����

        if (currentScore > pivotScore) { // �����ǰ���ִ��ڻ�׼����
            smallerIdx++; // ����С�ڻ�׼������
            swapPositions(moveArr[smallerIdx], moveArr[currentIdx]); // ����λ��
        }
    }
    swapPositions(moveArr[smallerIdx + 1], moveArr[endIdx]); // ����׼�ŵ���ȷ��λ��
    return (smallerIdx + 1); // ���ػ�׼��������
}

// ���������ƶ�
void quickSortMoves(Position moveArr[], int startIdx, int endIdx, GameState* state, int player)
{
    if (startIdx < endIdx) { // �����ʼ����С�ڽ�������
        int partitionIdx = partitionMoves(moveArr, startIdx, endIdx, state, player); // �����ƶ�
        quickSortMoves(moveArr, startIdx, partitionIdx - 1, state, player); // �ݹ�������벿��
        quickSortMoves(moveArr, partitionIdx + 1, endIdx, state, player); // �ݹ������Ұ벿��
    }
}

// �Ƚ�����λ��
bool comparePositions(const Position& a, const Position& b)
{
    return a.score > b.score; // �����ֽ���Ƚ�
}

// ��ȡ���ƶ�λ��
void getMoves(GameState* state, struct MoveList* moves, int player)
{
    int i, j; // ��������
    moves->count = 0; // ��ʼ���ƶ�����
    LLS value = 0; // �ƶ�����
    int first_zero = 1; // �Ƿ�Ϊ��һ��������

    for (i = 0; i < GRID_DIM; i++) { // ����������
        for (j = 0; j < GRID_DIM; j++) { // ����������
            if (state->grid[i][j] == BLANK && hasNexter(state, i, j)) { // ����ǿհ����к�������
                Position tempPos = { i, j }; // ����λ�ö���
                value = scorePosts(state, tempPos); // �����λ�õ�����
                if (value == 0 && first_zero) { // �������Ϊ�����ǵ�һ��������
                    moves->moves[moves->count].row = i; // ��¼��
                    moves->moves[moves->count].col = j; // ��¼��
                    moves->moves[moves->count].score = value; // ��¼����
                    moves->count++; // ���Ӽ���
                    first_zero = 0; // ����Ϊ�ǵ�һ��������
                } else if (value > 0) { // ������ִ�����
                    moves->moves[moves->count].row = i; // ��¼��
                    moves->moves[moves->count].col = j; // ��¼��
                    moves->moves[moves->count].score = value; // ��¼����
                    moves->count++; // ���Ӽ���
                }
            }
        }
    }

    // ����ƶ�������1����������
    if (moves->count > 1) {
        // quickSortMoves(moves->moves, 0, moves->count - 1, state, player); // ��������,�ѷ���
        stable_sort(moves->moves, moves->moves + moves->count, comparePositions); // �ȶ�����
    }
    if (moves->count > 6) {
        moves->count = 6; // ��������ƶ���Ϊ6
    }
}

// ��������
LLS scoreBoard(GameState* state)
{
    LLS comScore = 0, humScore = 0; // ��ʼ�����������������
    for (int i = 0; i < GRID_DIM; i++) { // ����������
        for (int j = 0; j < GRID_DIM; j++) { // ����������
            Position p = { i, j }; // ����λ�ö���
            if (state->grid[i][j] == myColor) {
                comScore += scorePlayer(state, p, myColor); // �����ҷ�����
            } else if (state->grid[i][j] == rivalColor) {
                humScore += scorePlayer(state, p, rivalColor); // ����з�����
            }
        }
    }
    return comScore - humScore; // �������ֲ�ֵ
}

// Alpha-Beta��֦�㷨
LLS AlphaBeta(GameState* state, int depth, LLS alpha, LLS beta, int player, Position lastMove)
{
    // ���ҵ�ǰ״̬�Ĺ�ϣֵ
    LLS hashValue = lookupHash(state->hash, depth, alpha, beta);
    if (hashValue > MIN_VAL - 1) {
        return hashValue; // ���ع�ϣֵ
    }

    // ����ʤ���
    if (player == myColor && scorePlayer(state, lastMove, rivalColor) >= 1000000000) {
        return MIN_VAL + 1; // ���ؼ�Сֵ
    } else if (player == rivalColor && scorePlayer(state, lastMove, myColor) >= 1000000000) {
        return MAX_VAL - 1; // ���ؼ���ֵ
    }
    if (depth == 0) {
        LLS currentScore = scoreBoard(state); // ���㵱ǰ����
        storeHash(state->hash, depth, currentScore, HASH_EXACT); // �洢��ϣ
        return currentScore; // ��������
    }

    struct MoveList moves; // �����ƶ��б�
    getMoves(state, &moves, player); // ��ȡ���ƶ�λ��
    int hashType = HASH_ALPHA; // ��ʼ����ϣ����

    if (moves.count == 0) {
        LLS currentScore = scoreBoard(state); // ���㵱ǰ����
        storeHash(state->hash, depth, currentScore, HASH_EXACT); // �洢��ϣ
        return currentScore; // ��������
    }

    // �������
    if (player == myColor) {
        LLS maxEval = MIN_VAL; // ��ʼ���������
        for (int i = 0; i < moves.count; i++) { // ���������ƶ�
            ULL oldHash = state->hash; // ���浱ǰ��ϣ
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, myColor); // ��������
            LLS eval = AlphaBeta(state, depth - 1, alpha, beta, 3 - player, moves.moves[i]); // �ݹ����
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, BLANK); // �ָ�����
            state->hash = oldHash; // �ָ���ϣ

            if (eval > maxEval) {
                maxEval = eval; // �����������
                if (eval > alpha) {
                    alpha = eval; // ����alphaֵ
                    hashType = HASH_EXACT; // ���¹�ϣ����
                }
            }
            if (beta <= alpha) {
                hashType = HASH_BETA; // ���¹�ϣ����
                break; // ��֦
            }
        }
        storeHash(state->hash, depth, maxEval, hashType); // �洢��ϣ
        return maxEval; // �����������
    } else { // ��С������
        LLS minEval = MAX_VAL; // ��ʼ����С����
        for (int i = 0; i < moves.count; i++) { // ���������ƶ�
            ULL oldHash = state->hash; // ���浱ǰ��ϣ
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, rivalColor); // ��������
            LLS eval = AlphaBeta(state, depth - 1, alpha, beta, 3 - player, moves.moves[i]); // �ݹ����
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, BLANK); // �ָ�����
            state->hash = oldHash; // �ָ���ϣ

            if (eval < minEval) {
                minEval = eval; // ������С����
                if (eval < beta) {
                    beta = eval; // ����betaֵ
                    hashType = HASH_EXACT; // ���¹�ϣ����
                }
            }
            if (beta <= alpha) {
                hashType = HASH_ALPHA; // ���¹�ϣ����
                break; // ��֦
            }
        }
        storeHash(state->hash, depth, minEval, hashType); // �洢��ϣ
        return minEval; // ������С����
    }
}

// �ҵ�����ƶ�
Position findBestShift(GameState* state)
{
    // ��չ�ϣ��
    memset(hashTable, 0, sizeof(HashEntry) * HASH_SIZE);

    struct MoveList moves; // �����ƶ��б�
    getMoves(state, &moves, myColor); // ��ȡ���ƶ�λ��

    LLS bestScore = MIN_VAL; // ��ʼ���������
    Position bestMove(-1, -1); // ��ʼ������ƶ�

    for (int i = 0; i < moves.count; i++) {
        ULL oldHash = state->hash; // ���浱ǰ��ϣ

        // ��������״̬
        state->updateGrid(moves.moves[i].row, moves.moves[i].col, myColor);

        LLS score = AlphaBeta(state, MAX_DEPTH, MIN_VAL, MAX_VAL, rivalColor, moves.moves[i]); // ʹ��Alpha-Beta�㷨��������

        // �ָ�����״̬
        state->updateGrid(moves.moves[i].row, moves.moves[i].col, BLANK);
        state->hash = oldHash; // �ָ���ϣ

        if (score > bestScore) {
            bestScore = score; // �����������
            bestMove = moves.moves[i]; // ��������ƶ�
        }
    }

    return bestMove; // ��������ƶ�
}

// ����ƶ��Ƿ�Ϸ�
bool isValidMove(Position pos)
{
    return pos.row >= 0 && pos.col >= 0 && pos.row < GRID_DIM && pos.col < GRID_DIM; // ��������Ƿ������̷�Χ��
}

// ��ӡ����
void printBoard()
{
    printf("    "); // ��ӡ�ո�
    fflush(stdout);
    for (int col = 0; col < GRID_DIM; col++) {
        printf("%8d ", col); // ��ӡ�к�
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
    for (int i = 0; i < GRID_DIM; i++) {
        printf("   +"); // ��ӡ�зָ�
        fflush(stdout);
        for (int j = 0; j < GRID_DIM; j++) {
            printf("--------+"); // ��ӡ�зָ�
            fflush(stdout);
        }
        printf("\n%2d ", i); // ��ӡ�к�
        fflush(stdout);
        for (int j = 0; j < GRID_DIM; j++) {
            printf("|"); // ��ӡ�зָ���
            fflush(stdout);
            if (gameBoard.grid[i][j] == myColor) {
                printf(myColor == PIECE_B ? "   ��   " : "   ��   ");
                fflush(stdout);
            } else if (gameBoard.grid[i][j] == rivalColor) {
                printf(rivalColor == PIECE_B ? "   ��   " : "   ��   ");
                fflush(stdout);
            } else {
                printf("        "); // ��ӡ�հ�λ��
                fflush(stdout);
            }
        }
        printf("|\n"); // ������
        fflush(stdout);
    }
    printf("   +"); // ��ӡ�зָ�
    fflush(stdout);
    for (int j = 0; j < GRID_DIM; j++) {
        printf("--------+"); // ��ӡ�зָ�
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
}

// ��Ϸѭ��
void gameLoop()
{
    char command[10] = { 0 }; // �������
    Position movePos, currentPos; // �ƶ�λ�ú͵�ǰλ��
    Position myturn; // �ҵĻغ�
    while (1) { // ����ѭ��
        memset(command, 0, sizeof(command)); // ����������
        scanf("%s", command); // ��ȡ����

        // �����ʼ������
        if (strcmp(command, CMD_INIT) == 0) {
            scanf("%d", &myColor); // ��ȡ�ҵ���ɫ
            rivalColor = 3 - myColor; // ����з���ɫ
            currentPos.row = CENTER_POS_1; // ��ʼ����ǰ��
            currentPos.col = (myColor == PIECE_B) ? CENTER_POS_2 : CENTER_POS_1; // ��ʼ����ǰ��
            gameBoard.initializeBoard(); // ��ʼ������
            printf("OK\n"); // ����OK
            fflush(stdout);
            if (bugmode) {
                printBoard(); // �������ģʽ��������ӡ����
            }
        } else if (strcmp(command, CMD_BUGMODE) == 0) { // ����ģʽ����
            bugmode = true; // ��������ģʽ
            printf("\n\n\n\n\n\n");
            printf("     WELCOME TO LIBERTY     \n"); // ��ӭ��Ϣ
            printf("\n\n\n\n\n\n");
        } else if (strcmp(command, CMD_SET) == 0) { // ������������
            scanf("%d %d", &movePos.row, &movePos.col); // ��ȡ�ƶ�λ��
            gameBoard.updateGrid(movePos.row, movePos.col, rivalColor); // �������̣����õз�����
            if (bugmode) {
                printBoard(); // �������ģʽ��������ӡ����
            }
        } else if (strcmp(command, CMD_CRACKER) == 0 && bugmode) { // �ƽ����ֻ���ڵ���ģʽ����Ч
            scanf("%d %d", &movePos.row, &movePos.col); // ��ȡ�ƶ�λ��
            gameBoard.updateGrid(movePos.row, movePos.col, myColor); // �������̣������ҵ�����
            if (bugmode) {
                printBoard(); // �������ģʽ��������ӡ����
            }
        } else if (strcmp(command, CMD_ERASE) == 0 && bugmode) { // �������ֻ���ڵ���ģʽ����Ч
            scanf("%d %d", &movePos.row, &movePos.col); // ��ȡ�ƶ�λ��
            gameBoard.updateGrid(movePos.row, movePos.col, BLANK); // �������̣���������
            if (bugmode) {
                printBoard(); // �������ģʽ��������ӡ����
            }
        } else if (strcmp(command, CMD_PLAY) == 0) { // ����Ϸ����
            start_turn = clock(); // ��¼��ǰ�غϿ�ʼʱ��
            Position nextMove; // ��ʼ����һ���ƶ�λ��
            nextMove = findBestShift(&gameBoard); // ��������ƶ�λ��
            if (isValidMove(nextMove)) { // ����ƶ�λ����Ч
                printf("%d %d\n", nextMove.row, nextMove.col); // ����ƶ�λ��
                fflush(stdout); // ˢ�����
                gameBoard.updateGrid(nextMove.row, nextMove.col, myColor); // �������̣������ҵ�����
                if (bugmode) {
                    printBoard(); // �������ģʽ��������ӡ����
                }
            }
        } else if (strcmp(command, CMD_SHOW_BOARD) == 0 && bugmode) { // ��ʾ�������ֻ���ڵ���ģʽ����Ч
            if (bugmode) {
                printBoard(); // ��ӡ����
            }
        } else if (strcmp(command, CMD_LIMIT_CHECK) == 0 && bugmode) { // ���Ƽ�����ֻ���ڵ���ģʽ����Ч
            bool failed = false; // ��ʼ��ʧ�ܱ�־
            for (int i = 0; i < MAX_MOVES; i++) { // ������غ�
                auto start1 = clock(); // ��¼��ʼʱ��
                Position nextMove; // ��ʼ����һ���ƶ�λ��
                nextMove = findBestShift(&gameBoard); // ��������ƶ�λ��
                if (isValidMove(nextMove)) { // ����ƶ�λ����Ч
                    fflush(stdout); // ˢ�����
                    gameBoard.updateGrid(nextMove.row, nextMove.col, myColor); // �������̣������ҵ�����
                }
                auto finish = clock(); // ��¼����ʱ��
                auto duration = (double)(finish - start1); // �������ʱ��
                if (duration > TIME_LIMIT) { // �������ʱ������
                    failed = true; // ����ʧ�ܱ�־
                    break; // �˳�ѭ��
                }
                swap(myColor, rivalColor); // ������ɫ
            }
            if (failed) {
                printf("\n\n\n\n\n\n");
                printf("     TIME CHECK FAILED     \n"); // ʱ����ʧ����Ϣ
                printf("\n\n\n\n\n\n");
            } else {
                printf("\n\n\n\n\n\n");
                printf("     TIME CHECK SUCCESSED     \n"); // ʱ����ɹ���Ϣ
                printf("\n\n\n\n\n\n");
            }
        }
    }
}

// ������
int main()
{
    initZobrist(); // ��ʼ��Zobrist��ϣ��
    gameLoop(); // ������Ϸѭ��
    return 0; // ����0����ʾ������������
}