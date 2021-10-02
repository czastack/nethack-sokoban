// Sokoban.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <conio.h>
#include <locale.h>
#include <stdio.h>
#include <windows.h>
#include <fstream>
#include <iostream>
#include <vector>
#include <memory>

using namespace std;


#define LOG_ERROR(...) fprintf(stderr, __VA_ARGS__)
#define LOG_INFO(...) fprintf(stdout, __VA_ARGS__)

constexpr int ROW = 100;
constexpr int COL = 100;
constexpr int ACTION_COUNT = 10000;
constexpr char ROAD = '.';
constexpr char PLAYER = '@';
constexpr char PIT = '^';

enum Direction : char
{
    UP = 'u',
    DOWN = 'd',
    LEFT = 'l',
    RIGHT = 'r',
};


namespace Sokoban
{

/// <summary>
/// 地图类
/// </summary>
class Map
{
public:
    Map();
    Map(int row, int col);

    bool FromFile(const char* filepath);
    bool Empty() { return row_ == 0 || col_ == 0; }
    char* operator[](int row);
    char& operator[](COORD coord);

    static bool CanMove(char ch);
    static bool CanPush(char ch);

    int row_;
    int col_;
    char data_[ROW][COL];
};

/// <summary>
/// 推箱子游戏
/// </summary>
class Sokoban
{
public:
    Sokoban();
    ~Sokoban();
    bool Init();
    void Move(char action);
    void Undo();
    void Update(COORD pos);
    void Print();
    void PrintActions();
    void ActionFromFile(const char *filepath);

    Map map_;
    char actions_[ACTION_COUNT];
    /// <summary>
    /// 这步操作推的石头
    /// </summary>
    char push_flags_[ACTION_COUNT];
    int action_index_;
    COORD playpos_;
    // 地图左上角坐标
    COORD map_coord_;
    // 打印action的位置
    COORD actions_coord_;


    static COORD GetDelta(char action);

private:
    HANDLE h_console_;
};


Map::Map() : Map(0, 0)
{

}

Map::Map(int row, int col) : row_(row), col_(col), data_{ 0 }
{
}

/// <summary>
/// 从文件读取地图
/// </summary>
/// <param name="filepath"></param>
/// <returns></returns>
bool Map::FromFile(const char* filepath)
{
    ifstream ifile(filepath, ios::in);

    if (!ifile.is_open()) {
        LOG_ERROR("Open %s failed.\n", filepath);
        return false;
    }

    int row = 0;
    int col = 0;
    while (ifile.getline(data_[row], COL))
    {
        if (col == 0)
        {
            col = static_cast<int>(ifile.gcount()) - 1;
        }
        row++;
    }

    row_ = row;
    col_ = col;
    return row != 0 && col != 0;
}

char* Map::operator[](int row)
{
    return data_[row];
}

char& Map::operator[](COORD coord)
{
    return data_[coord.Y][coord.X];
}

bool Map::CanMove(char ch)
{
    switch (ch)
    {
    case '.':
    case '?':
        return true;
    }
    return false;
}

bool Map::CanPush(char ch)
{
    switch (ch)
    {
    case '0':
        return true;
    }
    if ('A' <= ch && ch <= 'Z')
    {
        return true;
    }
    return false;
}

Sokoban::Sokoban()
    : actions_{ 0 }, push_flags_{ 0 }, action_index_{ 0 }, playpos_{ -1, -1 }, map_coord_{ 0, 0 }, actions_coord_{ 0, 0 }
{
    h_console_ = GetStdHandle(STD_OUTPUT_HANDLE); /* 获取标准输出句柄 */
}

Sokoban::~Sokoban()
{
    if (h_console_ != nullptr)
    {
        CloseHandle(h_console_);
        h_console_ = nullptr;
    }
}

/// <summary>
/// 初始化
/// </summary>
/// <returns></returns>
bool Sokoban::Init()
{
    if (map_.Empty())
    {
        return false;
    }
    CONSOLE_SCREEN_BUFFER_INFO scr;
    playpos_ = { -1, -1 };
    for (int i = 0; i < map_.row_; i++)
    {
        char *start = map_[i];
        char *result = std::find(start, start + COL, '@');
        if (result < start + COL)
        {
            playpos_ = { static_cast<short>(result - start), static_cast<short>(i)};
        }
    }
    bool success = playpos_.X != -1 && playpos_.Y != -1;
    if (success)
    {
        GetConsoleScreenBufferInfo(h_console_, &scr);
        map_coord_ = scr.dwCursorPosition;
        Print();
        GetConsoleScreenBufferInfo(h_console_, &scr);
        actions_coord_ = scr.dwCursorPosition;
    }
    else
    {
        LOG_ERROR("地图初始化失败");
    }
    return success;
}

/// <summary>
/// 移动
/// </summary>
/// <param name="action"></param>
void Sokoban::Move(char action)
{
    COORD delta = GetDelta(action);
    COORD next = { playpos_.X + delta.X, playpos_.Y + delta.Y };
    // 判断边界
    if (next.X < 0 || next.X >= map_.col_ || next.Y < 0 || next.Y > map_.row_)
    {
        return;
    }
    COORD next2 = { next.X + delta.X, next.Y + delta.Y };
    char next_char = map_[next];
    char next2_char = map_[next2];
    // 检测是否能推
    bool can_push = Map::CanPush(next_char);
    if (can_push)
    {
        if (next2_char != PIT && !Map::CanMove(next2_char))
        {
            return;
        }
    }
    else if (!Map::CanMove(next_char))
    {
        return;
    }
    map_[playpos_] = ROAD;
    map_[next] = PLAYER;

    Update(playpos_);
    Update(next);
    if (can_push)
    {
        // 判断推石头之后是否是填到洞里了
        map_[next2] = next2_char != PIT ? next_char : ROAD;
        Update(next2);
    }

    playpos_ = next;
    push_flags_[action_index_] = can_push ? next_char : 0;
    actions_[action_index_++] = action;
    PrintActions();
}

/// <summary>
/// 撤销上一步
/// </summary>
void Sokoban::Undo()
{
    if (action_index_ <= 0)
    {
        return;
    }
    char last_action = actions_[--action_index_];
    actions_[action_index_] = 0;

    // 处理map
    COORD delta = GetDelta(last_action);
    COORD prev = { playpos_.X - delta.X, playpos_.Y - delta.Y };
    COORD next = { playpos_.X + delta.X, playpos_.Y + delta.Y };
    char next_char = map_[next];
    bool is_push = push_flags_[action_index_] != 0;
    // 上一步操作是填洞
    bool is_hole = is_push && next_char == ROAD;

    map_[playpos_] = is_push ? push_flags_[action_index_] : ROAD;
    map_[prev] = PLAYER;
    Update(playpos_);
    Update(prev);
    if (is_push)
    {
        map_[next] = is_hole ? PIT : ROAD;
        Update(next);
    }

    playpos_ = prev;

    PrintActions();
}

/// <summary>
/// 更新显示
/// </summary>
/// <param name="coord"></param>
void Sokoban::Update(COORD coord)
{
    DWORD count;
    COORD draw_coord{ map_coord_.X + coord.X, map_coord_.Y + coord.Y };
    FillConsoleOutputCharacter(h_console_, map_[coord.Y][coord.X], 1, draw_coord, &count);
}

/// <summary>
/// 打印初始地图
/// </summary>
void Sokoban::Print()
{
    for (int i = 0; i < map_.row_; i++)
    {
        printf("%s\n", map_[i]);
    }
}

/// <summary>
/// 更新打印动作历史
/// </summary>
void Sokoban::PrintActions()
{
    SetConsoleCursorPosition(h_console_, actions_coord_);
    printf("actions(%05d/%d): %s", action_index_, ACTION_COUNT, actions_);
}

/// <summary>
/// 从文件读取操作
/// </summary>
/// <param name="filepath"></param>
void Sokoban::ActionFromFile(const char* filepath)
{
    ifstream ifile(filepath, ios::in);

    if (!ifile.is_open()) {
        LOG_ERROR("Open %s failed.\n", filepath);
        return;
    }

    char buffer[512];
    while (ifile)
    {
        ifile.read(buffer, sizeof(buffer));
        for (int i = 0; i < ifile.gcount(); i++)
        {
            switch (buffer[i])
            {
            case UP:
            case DOWN:
            case LEFT:
            case RIGHT:
                Move(buffer[i]);
                break;
            default:
                return;
            }
        }
    }
}

/// <summary>
/// 根据操作获取坐标偏移
/// </summary>
/// <param name="action"></param>
/// <returns></returns>
COORD Sokoban::GetDelta(char action)
{
    switch (action)
    {
    case UP: return { 0, -1 };
    case DOWN: return { 0, 1 };
    case LEFT: return { -1, 0 };
    case RIGHT: return { 1, 0 };
    default:
        return { 0, 0 };
    }
    return COORD();
}

}

int main(int argc, char const* argv[])
{
    setlocale(LC_ALL, "");
    unique_ptr<Sokoban::Sokoban> sokoban = make_unique<Sokoban::Sokoban>();
    const char* mapfile = "map-2a.txt";
    if (argc > 1)
    {
        mapfile = argv[1];
    }
    sokoban->map_.FromFile(mapfile);
    if (!sokoban->Init())
    {
        return -1;
    }
    if (argc > 2)
    {
        sokoban->ActionFromFile(argv[2]);
    }
    int ch;
    while ((ch = _getch()) != 0x1B) /* Press ESC to quit... */
    {
        switch (ch)
        {
        case 0xE0:
            switch (ch = _getch())
            {
            case 72: sokoban->Move(UP); break;
            case 80: sokoban->Move(DOWN); break;
            case 75: sokoban->Move(LEFT); break;
            case 77: sokoban->Move(RIGHT); break;
            default:
                break;
            }
            break;
        case 0x08:
            sokoban->Undo();
            break;
        case 'k': sokoban->Move(UP); break;
        case 'j': sokoban->Move(DOWN); break;
        case 'h': sokoban->Move(LEFT); break;
        case 'l': sokoban->Move(RIGHT); break;
        default:
            break;
        }
    }
}
