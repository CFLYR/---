#define _CRT_SECURE_NO_WARNINGS
#include <algorithm>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <time.h>
#include <vector>

typedef long long LLS; // 定义长整型别名LLS
using namespace std;

#define GRID_DIM 12 // 网格维度
#define BLANK 0 // 空白位置标识
#define PIECE_B 1 // 黑棋标识
#define PIECE_W 2 // 白棋标识
const LLS MAX_VAL = 2000000000000000000LL; // 最大值
const LLS MIN_VAL = -2000000000000000000LL; // 最小值

// 命令定义
#define CMD_INIT "START" // 初始化命令
#define CMD_SET "PLACE" // 设置命令
#define CMD_PLAY "TURN" // 游戏轮次命令
#define CMD_FINISH "END" // 结束命令
#define CMD_CRACKER "CRACK" // 破解命令
#define CMD_ERASE "ERASE" // 擦除命令
#define CMD_LIMIT_CHECK "LIMIT" // 限制检查命令
#define CMD_SHOW_BOARD "SHOW" // 显示棋盘命令
#define CMD_BUGMODE "LIBERTY_STEP" // 调试模式命令

bool bugmode = false; // 调试模式开关

// 棋形评估常量
#define FIVE5 7 // 五子连珠评分
#define LIVE4 6 // 活四评分
#define RUSH4 5 // 冲四评分
#define LIVE3 4 // 活三评分
#define SLEEPING3 3 // 睡三评分
#define LIVE2 2 // 活二评分
#define SLEEPING2 1 // 睡二评分
#define JUMPLIVE3 8 // 跳活三评分
#define JUMPSLEEPING4 9 // 跳睡四评分
#define JUMPLIVE4 10 // 跳活四评分

#define STYLENUM 11 // 风格数量

#define STYLE_WEIGHT 1 // 风格权重

#define MAX_DEPTH 7 // 最大搜索深度
#define TIME_LIMIT 1900 // 时间限制（毫秒）

clock_t start_turn; // 记录回合开始时间

int turnround = 0; // 当前回合数

int myColor; // 我的颜色
int rivalColor; // 对手的颜色

const int CENTER_POS_1 = (GRID_DIM + 1) / 2 - 1; // 棋盘中心位置1
const int CENTER_POS_2 = GRID_DIM / 2; // 棋盘中心位置2
const int dx[] = { 1, 0, 1, 1 }; // 方向向量x
const int dy[] = { 0, 1, 1, -1 }; // 方向向量y
// Zobrist哈希表大小
#define HASH_SIZE (1 << 20) // 2的20次方
#define HASH_EXACT 0 // 精确哈希类型
#define HASH_ALPHA 1 // Alpha哈希类型
#define HASH_BETA 2 // Beta哈希类型

typedef unsigned long long ULL; // 定义无符号长整型别名ULL
// Zobrist哈希表
ULL zobristTable[3][GRID_DIM][GRID_DIM]; // Zobrist哈希表
ULL currentHash; // 当前哈希值

// 哈希条目结构
struct HashEntry {
    ULL key; // 哈希键
    int depth; // 深度
    LLS score; // 评分
    int type; // 类型
};

// 哈希表
HashEntry* hashTable;

#define MAX_MOVES 121 // 最大移动数

// 棋形结构
struct chessshape {
    int score; // 棋形评分
    char shape[9]; // 棋形表示
};

// 棋型结构
struct ChessStyle {
    LLS score; // 棋型得分
    ChessStyle(int s = 0) // 构造函数
        : score(s) // 初始化得分
    {
    }
};

// 所有棋型结构
struct AllChessStyle {
    ChessStyle MyStyle[STYLENUM]; // 我的棋型风格
    ChessStyle EnemyStyle[STYLENUM]; // 敌方棋型风格
    AllChessStyle() // 构造函数
    {
        MyStyle[0].score = 0; // 初始化评分
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

        // 初始化敌方棋型风格
        for (int i = 0; i < STYLENUM; i++) {
            EnemyStyle[i].score = LLS(MyStyle[i].score * STYLE_WEIGHT);
        }
    }
};

// 位置结构
struct Position {
    int row = -1; // 行
    int col = -1; // 列
    LLS score = 0; // 评分
    Position(int r = -1, int c = -1) // 构造函数
    {
        row = r; // 初始化行
        col = c; // 初始化列
    }
};

// 移动列表结构
struct MoveList {
    Position moves[MAX_MOVES]; // 移动数组
    int count = 0; // 移动计数
};

// 交换两个位置
void swapPositions(Position& pos1, Position& pos2)
{
    Position temp = pos1; // 临时存储pos1
    pos1 = pos2; // pos1赋值为pos2
    pos2 = temp; // pos2赋值为temp
}

// 生成64位随机数
ULL random64()
{
    return ((ULL)rand() << 32) | ((ULL)rand() << 16) | (ULL)rand(); // 生成随机数
}

// 查找哈希
LLS lookupHash(ULL hash, int depth, LLS alpha, LLS beta)
{
    HashEntry& entry = hashTable[hash % HASH_SIZE]; // 获取哈希表条目

    // 检查哈希表条目是否有效
    if (entry.key == hash && entry.depth >= depth) {
        if (entry.type == HASH_EXACT)
            return entry.score; // 返回精确评分
        if (entry.type == HASH_ALPHA && entry.score <= alpha)
            return alpha; // 返回alpha值
        if (entry.type == HASH_BETA && entry.score >= beta)
            return beta; // 返回beta值
    }
    return MIN_VAL - 1; // 返回最小值
}

// 存储哈希
void storeHash(ULL hash, int depth, LLS score, int type)
{
    HashEntry& entry = hashTable[hash % HASH_SIZE]; // 获取哈希表条目

    // 更新哈希表条目
    if (entry.depth <= depth) {
        entry.key = hash; // 设置哈希键
        entry.depth = depth; // 设置深度
        entry.score = score; // 设置评分
        entry.type = type; // 设置类型
    }
}

// 游戏状态结构
struct GameState {
    int grid[GRID_DIM][GRID_DIM] = { { 0 } }; // 棋盘网格
    int gamePhase = 0; // 游戏阶段
    ULL hash; // 当前哈希值

    GameState() // 构造函数
    {
        initializeBoard(); // 初始化棋盘
    }

    // 初始化棋盘
    void initializeBoard()
    {
        memset(grid, 0, sizeof(grid)); // 清空棋盘
        grid[CENTER_POS_1][CENTER_POS_1] = PIECE_W; // 设置中心黑棋
        grid[CENTER_POS_2][CENTER_POS_2] = PIECE_W; // 设置中心白棋
        grid[CENTER_POS_2][CENTER_POS_1] = PIECE_B; // 设置侧边黑棋
        grid[CENTER_POS_1][CENTER_POS_2] = PIECE_B; // 设置侧边白棋

        // 计算初始哈希值
        hash = 0; // 初始化哈希
        for (int i = 0; i < GRID_DIM; i++) {
            for (int j = 0; j < GRID_DIM; j++) {
                if (grid[i][j] != BLANK) {
                    hash ^= zobristTable[grid[i][j]][i][j]; // 计算哈希值
                }
            }
        }
    }

    // 更新棋盘
    void updateGrid(int row, int col, int piece)
    {
        // 移除旧棋子的哈希
        hash ^= zobristTable[grid[row][col]][row][col];
        // 添加新棋子的哈希
        hash ^= zobristTable[piece][row][col];
        grid[row][col] = piece; // 更新棋盘
    }
};

GameState gameBoard; // 创建游戏状态对象

// 检查数字是否为偶数
bool isEven2(int number)
{
    return number % 2 == 0; // 判断是否为偶数
}

// 初始化Zobrist哈希表
void initZobrist()
{
    srand(size_t(time(NULL))); // 设置随机种子
    for (int piece = 0; piece < 3; piece++) {
        for (int i = 0; i < GRID_DIM; i++) {
            for (int j = 0; j < GRID_DIM; j++) {
                zobristTable[piece][i][j] = random64(); // 初始化Zobrist哈希表
            }
        }
    }

    // 初始化哈希表
    hashTable = new HashEntry[HASH_SIZE]; // 申请哈希表内存
    memset(hashTable, 0, sizeof(HashEntry) * HASH_SIZE); // 清空哈希表

    // 计算当前哈希值
    currentHash = 0; // 初始化当前哈希
    for (int i = 0; i < GRID_DIM; i++) {
        for (int j = 0; j < GRID_DIM; j++) {
            if (gameBoard.grid[i][j] != BLANK) {
                currentHash ^= zobristTable[gameBoard.grid[i][j]][i][j]; // 计算哈希值
            }
        }
    }
}

// 评分函数
int scoreDirects(GameState* state, Position pos, int dx, int dy, int player)
{
    int count = 1; // 连续棋子计数
    int empty_pos = 0; // 空位计数: 1=单空, 2=双空
    int empty_count = 0; // 空位数量
    int block = 0; // 阻挡计数
    int jump_p = 0; // 跳跃正方向计数
    int jump_n = 0; // 跳跃反方向计数
    int count_j_p = 0; // 正方向跳跃计数
    bool block_j_p = 0; // 正方向阻挡标志
    bool empty_j_p = 0; // 正方向空位标志
    int count_j_n = 0; // 反方向跳跃计数
    bool block_j_n = 0; // 反方向阻挡标志
    bool empty_j_n = 0; // 反方向空位标志

    // 检查正方向
    int tx = pos.row + dx; // 计算新行
    int ty = pos.col + dy; // 计算新列
    bool found_empty = false; // 是否找到空位
    while (tx >= 0 && tx < GRID_DIM && ty >= 0 && ty < GRID_DIM) {
        if (found_empty && !jump_p) { // 如果已找到空位且未跳跃
            if (state->grid[tx][ty] == player) {
                jump_p = 1; // 标记为跳跃
                count_j_p++; // 跳跃计数增加
            } else {
                break; // 结束循环
            }
        } else if (found_empty && jump_p) { // 如果已找到空位且已跳跃
            if (state->grid[tx][ty] == player) {
                count_j_p++; // 跳跃计数增加
            } else if (state->grid[tx][ty] == 3 - player) {
                block_j_p = true; // 标记阻挡
                break; // 结束循环
            } else {
                empty_j_p = true; // 标记空位
                break; // 结束循环
            }
        } else { // 如果未找到空位
            if (state->grid[tx][ty] == BLANK) {
                empty_count++; // 空位计数增加
                empty_pos += 1; // 空位状态更新
                found_empty = true; // 标记已找到空位
            } else if (state->grid[tx][ty] != player) {
                block++; // 阻挡计数增加
                break; // 结束循环
            } else {
                count++; // 连续棋子计数增加
            }
        }
        tx += dx; // 移动到下一个位置
        ty += dy; // 移动到下一个位置
    }

    // 检查反方向
    tx = pos.row - dx; // 计算新行
    ty = pos.col - dy; // 计算新列
    found_empty = false; // 重置空位标志
    while (tx >= 0 && tx < GRID_DIM && ty >= 0 && ty < GRID_DIM) {
        if (found_empty && !jump_n) { // 如果已找到空位且未跳跃
            if (state->grid[tx][ty] == player) {
                jump_n = 1; // 标记为跳跃
                count_j_n++; // 跳跃计数增加
            } else {
                break; // 结束循环
            }
        } else if (found_empty && jump_n) { // 如果已找到空位且已跳跃
            if (state->grid[tx][ty] == player) {
                count_j_n++; // 跳跃计数增加
            } else if (state->grid[tx][ty] == 3 - player) {
                block_j_n = true; // 标记阻挡
                break; // 结束循环
            } else {
                empty_j_n = true; // 标记空位
                break; // 结束循环
            }
        } else { // 如果未找到空位
            if (state->grid[tx][ty] == BLANK) {
                empty_count++; // 空位计数增加
                empty_pos += 2; // 更新双空位状态
                found_empty = true; // 标记已找到空位
            } else if (state->grid[tx][ty] != player) {
                block++; // 阻挡计数增加
                break; // 结束循环
            } else {
                count++; // 连续棋子计数增加
            }
        }

        tx -= dx; // 移动到上一个位置
        ty -= dy; // 移动到上一个位置
    }

    // 评分计算
    if (jump_p && jump_n) {
        // 跳跃情况
    } else if (jump_p) {
        if (count + count_j_p == 3 && empty_pos == 3 && empty_j_p) {
            return JUMPLIVE3; // 跳活三
        } else if (count + count_j_p == 4 && empty_pos == 3 && block_j_p) {
            return JUMPSLEEPING4; // 跳睡四
        } else if (count + count_j_p == 4 && empty_pos == 1 && empty_j_p) {
            return JUMPSLEEPING4; // 跳睡四
        } else if (count + count_j_p == 4 && empty_pos == 3 && empty_j_p) {
            return JUMPLIVE4; // 跳活四
        }
    } else if (jump_n) {
        if (count + count_j_n == 3 && empty_pos == 3 && empty_j_n) {
            return JUMPLIVE3; // 跳活三
        } else if (count + count_j_n == 4 && empty_pos == 3 && block_j_n) {
            return JUMPSLEEPING4; // 跳睡四
        } else if (count + count_j_n == 4 && empty_pos == 2 && empty_j_n) {
            return JUMPSLEEPING4; // 跳睡四
        } else if (count + count_j_n == 4 && empty_pos == 3 && empty_j_n) {
            return JUMPLIVE4; // 跳活四
        }
    }
    if (count >= 5)
        return FIVE5; // 五子连珠

    if (count == 4) {
        if (empty_pos == 3)
            return LIVE4; // 活四
        if (empty_pos > 0)
            return RUSH4; // 冲四
        return 0; // 无评分
    }

    if (count == 3) {
        if (empty_pos == 3)
            return LIVE3; // 活三
        if (empty_pos > 0)
            return SLEEPING3; // 睡三
        return 0; // 无评分
    }

    if (count == 2) {
        if (empty_pos == 3)
            return LIVE2; // 活二
        if (empty_pos > 0)
            return SLEEPING2; // 睡二
        return 0; // 无评分
    }

    return 0; // 无评分
}

// 评分玩家
LLS scorePlayer(GameState* state, Position pos, int player)
{
    AllChessStyle styles; // 创建棋型风格对象
    LLS totalScore = 0; // 总评分初始化为0
    int i;
    for (i = 0; i < 4; i++) { // 遍历四个方向
        int Style = scoreDirects(state, pos, dx[i], dy[i], player); // 获取该方向的棋型评分
        totalScore += styles.MyStyle[Style].score; // 累加评分
    }
    // 如果总评分在特定范围内，强制设置分数
    if (totalScore >= 1400 && totalScore < 1000000) {
        totalScore = 1000000; // 设置为1000000
    }

    return totalScore; // 返回总评分
}

// 评分位置
LLS scorePosts(GameState* state, Position pos)
{
    AllChessStyle styles; // 创建棋型风格对象
    LLS totalScore = 0; // 总评分初始化为0
    int i;
    LLS myScore = 0; // 我方评分
    LLS rivalScore = 0; // 敌方评分
    for (i = 0; i < 4; i++) { // 遍历四个方向
        int myStyle = scoreDirects(state, pos, dx[i], dy[i], myColor); // 计算我方棋型评分
        int rivalStyle = scoreDirects(state, pos, dx[i], dy[i], rivalColor); // 计算敌方棋型评分

        myScore += styles.MyStyle[myStyle].score; // 累加我方评分
        rivalScore += styles.EnemyStyle[rivalStyle].score; // 累加敌方评分
    }
    // 如果敌方评分在特定范围内，强制设置分数
    if (rivalScore >= 1400 && rivalScore < 1000000) {
        rivalScore = 1000000; // 设置为1000000
    }
    // 如果我方评分在特定范围内，强制设置分数
    if (myScore >= 1400 && myScore < 1000000) {
        myScore = 1000000; // 设置为1000000
    }
    totalScore += myScore; // 将我方评分加入总评分
    totalScore += rivalScore; // 将敌方评分加入总评分

    return totalScore; // 返回总评分
}

// 检查是否有后续棋子
int hasNexter(GameState* state, int row, int col)
{
    const int directions[8][2] = { // 定义八个方向
        { -1, 0 }, { 1, 0 }, { 0, -1 }, { 0, 1 },
        { -1, -1 }, { -1, 1 }, { 1, -1 }, { 1, 1 }
    };

    for (int d = 0; d < 8; d++) { // 遍历八个方向
        for (int distance = 1; distance <= 2; distance++) { // 检查1到2个单位的距离
            int ni = row + directions[d][0] * distance; // 计算新行
            int nj = col + directions[d][1] * distance; // 计算新列

            // 如果新位置在棋盘范围内且不是空白
            if (ni >= 0 && ni < GRID_DIM && nj >= 0 && nj < GRID_DIM && state->grid[ni][nj] != BLANK) {
                return 1; // 找到后续棋子，返回1
            }
        }
    }
    return 0; // 未找到后续棋子，返回0
}

// 划分移动
int partitionMoves(Position moveArr[], int startIdx, int endIdx, GameState* state, int player)
{
    Position pivotPos = moveArr[endIdx]; // 选择最后一个元素作为基准
    LLS pivotScore = pivotPos.score; // 基准评分
    int smallerIdx = (startIdx - 1); // 小于基准的元素索引

    for (int currentIdx = startIdx; currentIdx <= endIdx - 1; currentIdx++) { // 遍历数组
        LLS currentScore = moveArr[currentIdx].score; // 当前评分

        if (currentScore > pivotScore) { // 如果当前评分大于基准评分
            smallerIdx++; // 增加小于基准的索引
            swapPositions(moveArr[smallerIdx], moveArr[currentIdx]); // 交换位置
        }
    }
    swapPositions(moveArr[smallerIdx + 1], moveArr[endIdx]); // 将基准放到正确的位置
    return (smallerIdx + 1); // 返回基准的新索引
}

// 快速排序移动
void quickSortMoves(Position moveArr[], int startIdx, int endIdx, GameState* state, int player)
{
    if (startIdx < endIdx) { // 如果开始索引小于结束索引
        int partitionIdx = partitionMoves(moveArr, startIdx, endIdx, state, player); // 划分移动
        quickSortMoves(moveArr, startIdx, partitionIdx - 1, state, player); // 递归排序左半部分
        quickSortMoves(moveArr, partitionIdx + 1, endIdx, state, player); // 递归排序右半部分
    }
}

// 比较两个位置
bool comparePositions(const Position& a, const Position& b)
{
    return a.score > b.score; // 按评分降序比较
}

// 获取可移动位置
void getMoves(GameState* state, struct MoveList* moves, int player)
{
    int i, j; // 行列索引
    moves->count = 0; // 初始化移动计数
    LLS value = 0; // 移动评分
    int first_zero = 1; // 是否为第一个零评分

    for (i = 0; i < GRID_DIM; i++) { // 遍历棋盘行
        for (j = 0; j < GRID_DIM; j++) { // 遍历棋盘列
            if (state->grid[i][j] == BLANK && hasNexter(state, i, j)) { // 如果是空白且有后续棋子
                Position tempPos = { i, j }; // 创建位置对象
                value = scorePosts(state, tempPos); // 计算该位置的评分
                if (value == 0 && first_zero) { // 如果评分为零且是第一个零评分
                    moves->moves[moves->count].row = i; // 记录行
                    moves->moves[moves->count].col = j; // 记录列
                    moves->moves[moves->count].score = value; // 记录评分
                    moves->count++; // 增加计数
                    first_zero = 0; // 设置为非第一个零评分
                } else if (value > 0) { // 如果评分大于零
                    moves->moves[moves->count].row = i; // 记录行
                    moves->moves[moves->count].col = j; // 记录列
                    moves->moves[moves->count].score = value; // 记录评分
                    moves->count++; // 增加计数
                }
            }
        }
    }

    // 如果移动数大于1，进行排序
    if (moves->count > 1) {
        // quickSortMoves(moves->moves, 0, moves->count - 1, state, player); // 快速排序,已废弃
        stable_sort(moves->moves, moves->moves + moves->count, comparePositions); // 稳定排序
    }
    if (moves->count > 6) {
        moves->count = 6; // 限制最大移动数为6
    }
}

// 评分棋盘
LLS scoreBoard(GameState* state)
{
    LLS comScore = 0, humScore = 0; // 初始化计算机和人类评分
    for (int i = 0; i < GRID_DIM; i++) { // 遍历棋盘行
        for (int j = 0; j < GRID_DIM; j++) { // 遍历棋盘列
            Position p = { i, j }; // 创建位置对象
            if (state->grid[i][j] == myColor) {
                comScore += scorePlayer(state, p, myColor); // 计算我方评分
            } else if (state->grid[i][j] == rivalColor) {
                humScore += scorePlayer(state, p, rivalColor); // 计算敌方评分
            }
        }
    }
    return comScore - humScore; // 返回评分差值
}

// Alpha-Beta剪枝算法
LLS AlphaBeta(GameState* state, int depth, LLS alpha, LLS beta, int player, Position lastMove)
{
    // 查找当前状态的哈希值
    LLS hashValue = lookupHash(state->hash, depth, alpha, beta);
    if (hashValue > MIN_VAL - 1) {
        return hashValue; // 返回哈希值
    }

    // 检查获胜情况
    if (player == myColor && scorePlayer(state, lastMove, rivalColor) >= 1000000000) {
        return MIN_VAL + 1; // 返回极小值
    } else if (player == rivalColor && scorePlayer(state, lastMove, myColor) >= 1000000000) {
        return MAX_VAL - 1; // 返回极大值
    }
    if (depth == 0) {
        LLS currentScore = scoreBoard(state); // 计算当前评分
        storeHash(state->hash, depth, currentScore, HASH_EXACT); // 存储哈希
        return currentScore; // 返回评分
    }

    struct MoveList moves; // 创建移动列表
    getMoves(state, &moves, player); // 获取可移动位置
    int hashType = HASH_ALPHA; // 初始化哈希类型

    if (moves.count == 0) {
        LLS currentScore = scoreBoard(state); // 计算当前评分
        storeHash(state->hash, depth, currentScore, HASH_EXACT); // 存储哈希
        return currentScore; // 返回评分
    }

    // 最大化评分
    if (player == myColor) {
        LLS maxEval = MIN_VAL; // 初始化最大评分
        for (int i = 0; i < moves.count; i++) { // 遍历所有移动
            ULL oldHash = state->hash; // 保存当前哈希
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, myColor); // 更新棋盘
            LLS eval = AlphaBeta(state, depth - 1, alpha, beta, 3 - player, moves.moves[i]); // 递归调用
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, BLANK); // 恢复棋盘
            state->hash = oldHash; // 恢复哈希

            if (eval > maxEval) {
                maxEval = eval; // 更新最大评分
                if (eval > alpha) {
                    alpha = eval; // 更新alpha值
                    hashType = HASH_EXACT; // 更新哈希类型
                }
            }
            if (beta <= alpha) {
                hashType = HASH_BETA; // 更新哈希类型
                break; // 剪枝
            }
        }
        storeHash(state->hash, depth, maxEval, hashType); // 存储哈希
        return maxEval; // 返回最大评分
    } else { // 最小化评分
        LLS minEval = MAX_VAL; // 初始化最小评分
        for (int i = 0; i < moves.count; i++) { // 遍历所有移动
            ULL oldHash = state->hash; // 保存当前哈希
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, rivalColor); // 更新棋盘
            LLS eval = AlphaBeta(state, depth - 1, alpha, beta, 3 - player, moves.moves[i]); // 递归调用
            state->updateGrid(moves.moves[i].row, moves.moves[i].col, BLANK); // 恢复棋盘
            state->hash = oldHash; // 恢复哈希

            if (eval < minEval) {
                minEval = eval; // 更新最小评分
                if (eval < beta) {
                    beta = eval; // 更新beta值
                    hashType = HASH_EXACT; // 更新哈希类型
                }
            }
            if (beta <= alpha) {
                hashType = HASH_ALPHA; // 更新哈希类型
                break; // 剪枝
            }
        }
        storeHash(state->hash, depth, minEval, hashType); // 存储哈希
        return minEval; // 返回最小评分
    }
}

// 找到最佳移动
Position findBestShift(GameState* state)
{
    // 清空哈希表
    memset(hashTable, 0, sizeof(HashEntry) * HASH_SIZE);

    struct MoveList moves; // 创建移动列表
    getMoves(state, &moves, myColor); // 获取可移动位置

    LLS bestScore = MIN_VAL; // 初始化最佳评分
    Position bestMove(-1, -1); // 初始化最佳移动

    for (int i = 0; i < moves.count; i++) {
        ULL oldHash = state->hash; // 保存当前哈希

        // 更新棋盘状态
        state->updateGrid(moves.moves[i].row, moves.moves[i].col, myColor);

        LLS score = AlphaBeta(state, MAX_DEPTH, MIN_VAL, MAX_VAL, rivalColor, moves.moves[i]); // 使用Alpha-Beta算法计算评分

        // 恢复棋盘状态
        state->updateGrid(moves.moves[i].row, moves.moves[i].col, BLANK);
        state->hash = oldHash; // 恢复哈希

        if (score > bestScore) {
            bestScore = score; // 更新最佳评分
            bestMove = moves.moves[i]; // 更新最佳移动
        }
    }

    return bestMove; // 返回最佳移动
}

// 检查移动是否合法
bool isValidMove(Position pos)
{
    return pos.row >= 0 && pos.col >= 0 && pos.row < GRID_DIM && pos.col < GRID_DIM; // 检查行列是否在棋盘范围内
}

// 打印棋盘
void printBoard()
{
    printf("    "); // 打印空格
    fflush(stdout);
    for (int col = 0; col < GRID_DIM; col++) {
        printf("%8d ", col); // 打印列号
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
    for (int i = 0; i < GRID_DIM; i++) {
        printf("   +"); // 打印行分隔
        fflush(stdout);
        for (int j = 0; j < GRID_DIM; j++) {
            printf("--------+"); // 打印列分隔
            fflush(stdout);
        }
        printf("\n%2d ", i); // 打印行号
        fflush(stdout);
        for (int j = 0; j < GRID_DIM; j++) {
            printf("|"); // 打印列分隔符
            fflush(stdout);
            if (gameBoard.grid[i][j] == myColor) {
                printf(myColor == PIECE_B ? "   ○   " : "   ●   ");
                fflush(stdout);
            } else if (gameBoard.grid[i][j] == rivalColor) {
                printf(rivalColor == PIECE_B ? "   ○   " : "   ●   ");
                fflush(stdout);
            } else {
                printf("        "); // 打印空白位置
                fflush(stdout);
            }
        }
        printf("|\n"); // 结束行
        fflush(stdout);
    }
    printf("   +"); // 打印行分隔
    fflush(stdout);
    for (int j = 0; j < GRID_DIM; j++) {
        printf("--------+"); // 打印列分隔
        fflush(stdout);
    }
    printf("\n");
    fflush(stdout);
}

// 游戏循环
void gameLoop()
{
    char command[10] = { 0 }; // 命令缓冲区
    Position movePos, currentPos; // 移动位置和当前位置
    Position myturn; // 我的回合
    while (1) { // 无限循环
        memset(command, 0, sizeof(command)); // 清空命令缓冲区
        scanf("%s", command); // 读取命令

        // 处理初始化命令
        if (strcmp(command, CMD_INIT) == 0) {
            scanf("%d", &myColor); // 读取我的颜色
            rivalColor = 3 - myColor; // 计算敌方颜色
            currentPos.row = CENTER_POS_1; // 初始化当前行
            currentPos.col = (myColor == PIECE_B) ? CENTER_POS_2 : CENTER_POS_1; // 初始化当前列
            gameBoard.initializeBoard(); // 初始化棋盘
            printf("OK\n"); // 返回OK
            fflush(stdout);
            if (bugmode) {
                printBoard(); // 如果调试模式开启，打印棋盘
            }
        } else if (strcmp(command, CMD_BUGMODE) == 0) { // 调试模式命令
            bugmode = true; // 开启调试模式
            printf("\n\n\n\n\n\n");
            printf("     WELCOME TO LIBERTY     \n"); // 欢迎信息
            printf("\n\n\n\n\n\n");
        } else if (strcmp(command, CMD_SET) == 0) { // 设置棋子命令
            scanf("%d %d", &movePos.row, &movePos.col); // 读取移动位置
            gameBoard.updateGrid(movePos.row, movePos.col, rivalColor); // 更新棋盘，设置敌方棋子
            if (bugmode) {
                printBoard(); // 如果调试模式开启，打印棋盘
            }
        } else if (strcmp(command, CMD_CRACKER) == 0 && bugmode) { // 破解命令，只有在调试模式下有效
            scanf("%d %d", &movePos.row, &movePos.col); // 读取移动位置
            gameBoard.updateGrid(movePos.row, movePos.col, myColor); // 更新棋盘，设置我的棋子
            if (bugmode) {
                printBoard(); // 如果调试模式开启，打印棋盘
            }
        } else if (strcmp(command, CMD_ERASE) == 0 && bugmode) { // 擦除命令，只有在调试模式下有效
            scanf("%d %d", &movePos.row, &movePos.col); // 读取移动位置
            gameBoard.updateGrid(movePos.row, movePos.col, BLANK); // 更新棋盘，擦除棋子
            if (bugmode) {
                printBoard(); // 如果调试模式开启，打印棋盘
            }
        } else if (strcmp(command, CMD_PLAY) == 0) { // 玩游戏命令
            start_turn = clock(); // 记录当前回合开始时间
            Position nextMove; // 初始化下一步移动位置
            nextMove = findBestShift(&gameBoard); // 计算最佳移动位置
            if (isValidMove(nextMove)) { // 如果移动位置有效
                printf("%d %d\n", nextMove.row, nextMove.col); // 输出移动位置
                fflush(stdout); // 刷新输出
                gameBoard.updateGrid(nextMove.row, nextMove.col, myColor); // 更新棋盘，放置我的棋子
                if (bugmode) {
                    printBoard(); // 如果调试模式开启，打印棋盘
                }
            }
        } else if (strcmp(command, CMD_SHOW_BOARD) == 0 && bugmode) { // 显示棋盘命令，只有在调试模式下有效
            if (bugmode) {
                printBoard(); // 打印棋盘
            }
        } else if (strcmp(command, CMD_LIMIT_CHECK) == 0 && bugmode) { // 限制检查命令，只有在调试模式下有效
            bool failed = false; // 初始化失败标志
            for (int i = 0; i < MAX_MOVES; i++) { // 检查多个回合
                auto start1 = clock(); // 记录开始时间
                Position nextMove; // 初始化下一步移动位置
                nextMove = findBestShift(&gameBoard); // 计算最佳移动位置
                if (isValidMove(nextMove)) { // 如果移动位置有效
                    fflush(stdout); // 刷新输出
                    gameBoard.updateGrid(nextMove.row, nextMove.col, myColor); // 更新棋盘，放置我的棋子
                }
                auto finish = clock(); // 记录结束时间
                auto duration = (double)(finish - start1); // 计算持续时间
                if (duration > TIME_LIMIT) { // 如果超过时间限制
                    failed = true; // 设置失败标志
                    break; // 退出循环
                }
                swap(myColor, rivalColor); // 交换颜色
            }
            if (failed) {
                printf("\n\n\n\n\n\n");
                printf("     TIME CHECK FAILED     \n"); // 时间检查失败信息
                printf("\n\n\n\n\n\n");
            } else {
                printf("\n\n\n\n\n\n");
                printf("     TIME CHECK SUCCESSED     \n"); // 时间检查成功信息
                printf("\n\n\n\n\n\n");
            }
        }
    }
}

// 主函数
int main()
{
    initZobrist(); // 初始化Zobrist哈希表
    gameLoop(); // 启动游戏循环
    return 0; // 返回0，表示程序正常结束
}