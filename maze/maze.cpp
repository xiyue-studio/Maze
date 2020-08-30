#include "pch.h"
#include <windows.h>
#include <iostream>
#include <vector>
#include <unordered_set>
#include <math.h>
#include <time.h>
#include <assert.h>

#define MAZE_WIDTH 20
#define MAZE_HEIGHT 20

enum MazeDirection : uint8_t
{
	UP = 1,
	RIGHT = 1 << 1,
	DOWN = 1 << 2,
	LEFT = 1 << 3
};

std::vector<int> visitedHistory;
std::unordered_set<int> closedHistory;

inline bool isVisited(int step)
{
	return closedHistory.find(step) != closedHistory.end() || std::find(visitedHistory.begin(), visitedHistory.end(), step) != visitedHistory.end();
}

bool getMovableSteps(int movableSteps[4], uint8_t* mazeBuffer, int step)
{
	bool movable = false;
	// 测试能否向上移动
	int p = step - MAZE_WIDTH;
	if (p > 0 && mazeBuffer[p] == 0 && !isVisited(p))
		movableSteps[0] = p;
	// 测试能否向右移动
	p = step + 1;
	if (p % MAZE_WIDTH != 0 && mazeBuffer[p] == 0 && !isVisited(p))
		movableSteps[1] = p;
	// 测试能否向下移动
	p = step + MAZE_WIDTH;
	if (p < MAZE_WIDTH * MAZE_HEIGHT && mazeBuffer[p] == 0 && !isVisited(p))
		movableSteps[2] = p;
	// 测试能否向左移动
	p = step - 1;
	if (step % MAZE_WIDTH != 0 && mazeBuffer[p] == 0 && !isVisited(p))
		movableSteps[3] = p;

	for (int i = 0; i < 4; ++i)
	{
		if (movableSteps[i] != -1)
		{
			movable = true;
			break;
		}
	}
	
	return movable;
}

int stepMaze(uint8_t* mazeBuffer, int step)
{
	int movableSteps[] = { -1, -1, -1, -1 };
	if (!getMovableSteps(movableSteps, mazeBuffer, step))
	{
		// 后退一步，换个方向继续前进
		if (visitedHistory.empty())
			return -1;
		int p = visitedHistory.back();
		visitedHistory.pop_back();
		closedHistory.insert(p);
		return p;
	}

	// 随机选择一个方向前进
	int rpIndex = rand() % 4;
	int rp = movableSteps[rpIndex];
	while (rp == -1)
	{
		++rpIndex;
		rp = movableSteps[rpIndex % 4];
		assert(rpIndex < 10);
	}

	// 标记通行方向
	rpIndex = rpIndex % 4;
	mazeBuffer[rp] += (1 << ((rpIndex + 2) % 4));
	mazeBuffer[step] += (1 << rpIndex);

	visitedHistory.push_back(rp);
	return rp;
}

uint8_t* createMaze()
{
	uint8_t* mazeBuffer = (uint8_t*)malloc(sizeof(uint8_t) * MAZE_WIDTH * MAZE_HEIGHT);
	memset(mazeBuffer, 0, sizeof(uint8_t) * MAZE_WIDTH * MAZE_HEIGHT);

	srand((unsigned)time(0));
	visitedHistory.clear();
	closedHistory.clear();
	// 先在地图中间随机选一个位置
	int step = rand() % (MAZE_WIDTH * MAZE_HEIGHT);
	// 搜索地图的路径
	while (step != -1)
		step = stepMaze(mazeBuffer, step);

	// 打通第一个格子和最后一个格子
	mazeBuffer[0] += LEFT;
	mazeBuffer[MAZE_WIDTH * MAZE_HEIGHT - 1] += RIGHT;

	return mazeBuffer;
}

void inline printMazeGrid(HANDLE stdHandle, int mazeData, bool walked, bool finished)
{
	WORD baseAttr = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;
	if (walked)
	{
		if (finished)
			baseAttr |= (BACKGROUND_GREEN | BACKGROUND_INTENSITY);
		else
			baseAttr |= (BACKGROUND_BLUE | BACKGROUND_INTENSITY);
	}
	bool canUp = mazeData & UP;
	bool canRight = mazeData & RIGHT;
	bool canDown = mazeData & DOWN;
	bool canLeft = mazeData & LEFT;

	WORD attr = baseAttr;
	if (!canUp)
		attr |= COMMON_LVB_GRID_HORIZONTAL;
	if (!canDown)
		attr |= COMMON_LVB_UNDERSCORE;
	if (!canLeft)
		attr |= COMMON_LVB_GRID_LVERTICAL;
	SetConsoleTextAttribute(stdHandle, attr);
	printf(" ");

	attr = baseAttr;
	if (!canUp)
		attr |= COMMON_LVB_GRID_HORIZONTAL;
	if (!canDown)
		attr |= COMMON_LVB_UNDERSCORE;
	if (!canRight)
		attr |= COMMON_LVB_GRID_RVERTICAL;
	SetConsoleTextAttribute(stdHandle, attr);
	printf(" ");
}

void printMaze(HANDLE handle, uint8_t* mazeBuffer, std::vector<int>& movedSteps, bool finished)
{
	WORD baseAttr = FOREGROUND_BLUE | FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_INTENSITY;

	system("cls");

	printf("\n->");
	for (int i = 0; i < MAZE_WIDTH; ++i)
	{
		for (int j = 0; j < MAZE_HEIGHT; ++j)
		{
			int index = i * MAZE_WIDTH + j;
			bool walked = std::find(movedSteps.begin(), movedSteps.end(), index) != movedSteps.end();
			printMazeGrid(handle, mazeBuffer[index], walked, finished || movedSteps.back() == index);
		}
		SetConsoleTextAttribute(handle, baseAttr);
		if (i == MAZE_WIDTH - 1)
			printf("->");
		printf("\n  ");
	}

	SetConsoleTextAttribute(handle, baseAttr);
	printf("\n  Use ↑ ↓ ← → to move.\n");
	printf("  Press F5 to restart a new maze.\n");
	printf("  Press ESC to quit.\n");
}

bool move(std::vector<int>& steps, int direction, uint8_t* maze)
{
	int lastStep = steps.back();
	// 判断是否允许朝指定的方向前进
	if ((maze[lastStep] & (1 << direction)) == 0)
		return false;

	int nowStep = lastStep;
	switch (direction)
	{
	case 0:
		nowStep -= MAZE_WIDTH;
		break;
	case 1:
		nowStep += 1;
		break;
	case 2:
		nowStep += MAZE_WIDTH;
		break;
	case 3:
		nowStep -= 1;
		break;
	}

	// 判断是否回退
	if (steps.size() > 1 && nowStep == steps[steps.size() - 2])
		steps.pop_back();
	else
		steps.push_back(nowStep);

	return true;
}

void updateMazeWalker()
{
	HANDLE handle = GetStdHandle(STD_INPUT_HANDLE);
	HANDLE stdOutHandle = GetStdHandle(STD_OUTPUT_HANDLE);
	INPUT_RECORD inputRecord;
	DWORD numRead;
	std::vector<int> movedSteps = { 0 };

	uint8_t* maze = createMaze();
	printMaze(stdOutHandle, maze, movedSteps, false);
	bool isFinished = false;
	for (;;)
	{
		ReadConsoleInput(handle, &inputRecord, 1, &numRead);
		if (inputRecord.EventType != KEY_EVENT)
			continue;

		if (inputRecord.Event.KeyEvent.bKeyDown != 0)
			continue;

		switch (inputRecord.Event.KeyEvent.wVirtualKeyCode)
		{
		case VK_UP:
			if (!isFinished && move(movedSteps, 0, maze))
				printMaze(stdOutHandle, maze, movedSteps, movedSteps.back() == MAZE_WIDTH * MAZE_HEIGHT - 1);
			else if (!isFinished)
				printf("\n  Oops! You can't move that way!\n");
			break;
		case VK_DOWN:
			if (!isFinished && move(movedSteps, 2, maze))
				printMaze(stdOutHandle, maze, movedSteps, movedSteps.back() == MAZE_WIDTH * MAZE_HEIGHT - 1);
			else if (!isFinished)
				printf("\n  Oops! You can't move that way!\n");
			break;
		case VK_LEFT:
			if (!isFinished && move(movedSteps, 3, maze))
				printMaze(stdOutHandle, maze, movedSteps, movedSteps.back() == MAZE_WIDTH * MAZE_HEIGHT - 1);
			else if (!isFinished)
				printf("\n  Oops! You can't move that way!\n");
			break;
		case VK_RIGHT:
			if (!isFinished && move(movedSteps, 1, maze))
				printMaze(stdOutHandle, maze, movedSteps, movedSteps.back() == MAZE_WIDTH * MAZE_HEIGHT - 1);
			else if (!isFinished)
				printf("\n  Oops! You can't move that way!\n");
			break;
		case VK_F5:
			free(maze);
			maze = createMaze();
			movedSteps.clear();
			movedSteps.push_back(0);
			isFinished = false;
			printMaze(stdOutHandle, maze, movedSteps, isFinished);
			break;
		case VK_ESCAPE:
			free(maze);
			CloseHandle(handle);
			CloseHandle(stdOutHandle);
			return;
		}

		bool isLastFinished = isFinished;
		isFinished = movedSteps.back() == MAZE_WIDTH * MAZE_HEIGHT - 1;
		if (!isLastFinished && isFinished)
			printf("Congratulations! Your are so smart as me!\n");
	}
}

int main()
{
	updateMazeWalker();
	return 0;
}
