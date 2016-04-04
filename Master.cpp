#include <windows.h>
#include <stdio.h>
#include <CommCtrl.h>
#include <typeinfo>
#include <time.h>
#include "UCTSample.h"
#include "test.h"

using namespace std;

// �v���C���[�ꗗ
Player* playerList[] = {new UCTSample()};

static bool isPalying = false;
static Board board;
static Player* players[2] = { playerList[0], playerList[0] };
UCTNode result[19*19];
int result_num;

// ����
XY record[19*19+200];
int record_num;

const int MARGIN = 24;
int GRID_SIZE = 9;
const int GRID_WIDTH = 45;
const int INFO_WIDTH = 120;
const int CTRL_HEIGHT = 24;
const int CTRL_MARGIN = 8;

HWND hMainWnd;
DWORD style = WS_OVERLAPPEDWINDOW ^ WS_THICKFRAME ^ WS_MAXIMIZEBOX;

HINSTANCE hInstance;
HBRUSH hBrushBoard;
HPEN hPenBoard;
HPEN hPenLast;
HFONT hFontPlayout;
HFONT hFontPass;

float scaleX, scaleY;

inline int scaledX(const float x) {
	return (int)(x * scaleX);
}
inline int scaledY(const float y) {
	return (int)(y * scaleY);
}

// GTP�p
bool isGTPMode = false;
bool gtp_clear_board = false;
int gtp_boardsize = 0;

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
DWORD WINAPI ThreadProc(LPVOID lpParameter);

#ifndef TEST
int wmain(int argc, wchar_t* argv[]) {
	::hInstance = GetModuleHandle(NULL);
	srand(time(NULL));

	// �I�v�V����
	for (int i = 0; i < argc; i++)
	{
		// -gtp��GTP���[�h
		if (wcscmp(argv[i], L"-gtp") == 0)
		{
			isGTPMode = true;
		}
		if (wcscmp(argv[i], L"-size") == 0)
		{
			if (i + 1 < argc)
			{
				int size = _wtoi(argv[i + 1]);
				if (size >= 9 && size <= 19)
				{
					GRID_SIZE = size;
					board.init(size);
				}
			}
		}
		else {
			// �v���C�A�E�g��
			int n = _wtoi(argv[i]);
			if (n > 0)
			{
				PLAYOUT_MAX = n;
			}
		}
	}

	// GTP���[�h
	if (isGTPMode)
	{
		// �W�����o�͂̃o�b�t�@�����O���I�t�ɂ���
		setbuf(stdout, NULL);
		setbuf(stderr, NULL);  // stderr�ɏ�����GoGui�ɕ\�������B

		// �Ď��X���b�h�N��
		CreateThread(NULL, 0, ThreadProc, NULL, 0, NULL);
	}

	WNDCLASSEX wcex = { sizeof(WNDCLASSEX) };
	wcex.style = 0;
	wcex.lpfnWndProc = WndProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hbrBackground = GetSysColorBrush(COLOR_BTNFACE);
	wcex.lpszMenuName = NULL;
	wcex.hCursor = LoadCursor(NULL, IDI_APPLICATION);
	wcex.lpszClassName = L"GoSample";

	RegisterClassEx(&wcex);

	HDC screen = GetDC(0);
	scaleX = GetDeviceCaps(screen, LOGPIXELSX) / 96.0f;
	scaleY = GetDeviceCaps(screen, LOGPIXELSY) / 96.0f;
	ReleaseDC(0, screen);

	RECT rc = { 0, 0, scaledX(MARGIN + GRID_WIDTH * (GRID_SIZE + 1) + MARGIN + INFO_WIDTH + MARGIN), scaledY(MARGIN + GRID_WIDTH * (GRID_SIZE + 1) + MARGIN) };
	AdjustWindowRect(&rc, style, NULL);

	hMainWnd = CreateWindow(
		L"GoSample",
		L"GoSample",
		style,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rc.right - rc.left, rc.bottom - rc.top,
		NULL, NULL, hInstance, NULL);
	if (hMainWnd == NULL)
	{
		return 0;
	}

	// GDI�I�u�W�F�N�g�쐬
	hBrushBoard = CreateSolidBrush(RGB(190, 160, 60));
	hPenBoard = (HPEN)CreatePen(PS_SOLID, scaledX(2), RGB(0, 0, 0));
	hPenLast = (HPEN)CreatePen(PS_SOLID, scaledX(2), RGB(128, 128, 128));
	HFONT hFont = (HFONT)GetStockObject(DEFAULT_GUI_FONT);
	LOGFONT lf;
	GetObject(hFont, sizeof(lf), &lf);
	lf.lfHeight = -scaledY(12);
	lf.lfWidth = 0;
	hFontPlayout = CreateFontIndirect(&lf);
	// Pass�t�H���g
	lf.lfHeight = -scaledY(16);
	lf.lfWeight = FW_BOLD;
	hFontPass = CreateFontIndirect(&lf);

	ShowWindow(hMainWnd, SW_SHOW);
	UpdateWindow(hMainWnd);

	MSG msg;

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	DeleteObject(hBrushBoard);
	DeleteObject(hPenBoard);
	DeleteObject(hFontPlayout);

	return 0;
}
#endif

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	static HWND staticPlayers[2];
	static HWND cmbPlayers[2];
	static HWND btnStart;
	const int BTN_ID_START = 0;

	switch (uMsg)
	{
	case WM_CREATE:
	{
		int infoX = scaledX(MARGIN + GRID_WIDTH * (GRID_SIZE + 1) + MARGIN);
		int infoY = scaledY(MARGIN);
		// �v���C���[�I���{�b�N�X
		staticPlayers[0] = CreateWindow(WC_STATIC, L"Black:", WS_CHILD | WS_VISIBLE, infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT), hWnd, NULL, hInstance, NULL);
		infoY += scaledY(CTRL_HEIGHT);

		cmbPlayers[0] = CreateWindow(WC_COMBOBOX, L"Player1", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
			infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT * 10),
			hWnd, NULL, hInstance, NULL);
		infoY += scaledY(CTRL_HEIGHT + CTRL_MARGIN * 2);

		staticPlayers[1] = CreateWindow(WC_STATIC, L"White:", WS_CHILD | WS_VISIBLE, infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT), hWnd, NULL, hInstance, NULL);
		infoY += scaledY(CTRL_HEIGHT);

		cmbPlayers[1] = CreateWindow(WC_COMBOBOX, L"Player2", WS_CHILD | WS_VISIBLE | CBS_DROPDOWNLIST,
			infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT * 10),
			hWnd, NULL, hInstance, NULL);
		infoY += scaledY(CTRL_HEIGHT + CTRL_MARGIN * 2);

		for (int i = 0; i < 2; i++)
		{
			for (Player* player : playerList)
			{
				const char* name = typeid(*player).name();
				SendMessageA(cmbPlayers[i], CB_ADDSTRING, NULL, (LPARAM)name + 6);
			}
			SendMessage(cmbPlayers[i], CB_SETCURSEL, 0, NULL);
		}

		// �X�^�[�g�{�^��
		btnStart = CreateWindow(WC_BUTTON, L"Start", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON, infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT),
			hWnd, BTN_ID_START, hInstance, NULL);

		if (isGTPMode)
		{
			EnableWindow(btnStart, FALSE);
		}

		return 0;
	}
	case WM_COMMAND:
	{
		switch (LOWORD(wParam)) {
		case BTN_ID_START:
		{
			// �{�[�h������
			board.init(GRID_SIZE);

			// �v���C���[�擾
			for (int i = 0; i < 2; i++)
			{
				int index = SendMessage(cmbPlayers[i], CB_GETCURSEL, NULL, NULL);
				players[i] = playerList[index];
			}

			// �J�n
			Color color = BLACK;
			Color pre_xy = -1;
			record_num = 0;

			isPalying = true;
			while (isPalying)
			{
				// �ǖʃR�s�[
				Board board_tmp = board;

				// ���I��
				Player* current_player = players[color - 1];
				XY xy = current_player->select_move(board_tmp, color);

				// �΂�ł�
				MoveResult err = board.move(xy, color);

				if (err != SUCCESS)
				{
					break;
				}

				if (xy == PASS && pre_xy == PASS)
				{
					// �I��
					break;
				}

				pre_xy = xy;
				color = opponent(color);
				record[record_num++] = xy; // �����ǉ�

				// �`��X�V
				if (typeid(*current_player) == typeid(UCTSample))
				{
					UCTNode* root = ((UCTSample*)current_player)->root;
					for (int i = 0; i < root->child_num; i++)
					{
						result[i] = root->child[i]; // �l�R�s�[
					}
					result_num = root->child_num;
				}
				InvalidateRect(hWnd, NULL, FALSE);

				// ���b�Z�[�W����
				MSG msg;
				while (PeekMessage(&msg, hWnd, 0, 0, PM_NOREMOVE))
				{
					if (GetMessage(&msg, NULL, 0, 0))
					{
						TranslateMessage(&msg);
						DispatchMessage(&msg);
					}
				}
			}
			isPalying = false;

			// SGF�Ŋ������o��
			printf("(;GM[1]SZ[%d]KM[%.1f]\n", GRID_SIZE, KOMI);
			for (int i = 0; i < record_num; i++) {
				XY xy = record[i];
				int x = get_x(xy);
				int y = get_y(xy);
				const char *sStone[2] = { "B", "W" };
				printf(";%s", sStone[i & 1]);
				if (xy == PASS) {
					printf("[]");
				}
				else {
					printf("[%c%c]", x + 'a' - 1, y + 'a' - 1);
				}
				if (((i + 1) % 10) == 0) printf("\n");
			}
			printf("\n)\n");

			return 0;
		}
		}
	}
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hDC = BeginPaint(hWnd, &ps);

		HBRUSH hPrevBrush = (HBRUSH)SelectObject(hDC, hBrushBoard);

		Rectangle(hDC, scaledX(MARGIN), scaledY(MARGIN), scaledX(MARGIN + GRID_WIDTH * (GRID_SIZE + 1)), scaledY(MARGIN + GRID_WIDTH * (GRID_SIZE + 1)));

		HPEN hPrevPen = (HPEN)SelectObject(hDC, hPenBoard);

		for (int x = 1; x < GRID_SIZE + 1; x++)
		{
			int drawX = scaledX(MARGIN + GRID_WIDTH * x);
			MoveToEx(hDC, drawX, scaledY(MARGIN + GRID_WIDTH), NULL);
			LineTo(hDC, drawX, scaledY(MARGIN + GRID_WIDTH * GRID_SIZE));
		}
		for (int y = 1; y < GRID_SIZE + 1; y++)
		{
			int drawY = scaledY(MARGIN + GRID_WIDTH * y);
			MoveToEx(hDC, scaledY(MARGIN + GRID_WIDTH), drawY, NULL);
			LineTo(hDC, scaledY(MARGIN + GRID_WIDTH * GRID_SIZE), drawY);
		}

		// �΂�`��
		for (XY xy = BOARD_SIZE + 3; xy < BOARD_MAX - (BOARD_SIZE + 3); xy++)
		{
			int x = scaledX(MARGIN + GRID_WIDTH * (get_x(xy) - 0.5f));
			int y = scaledY(MARGIN + GRID_WIDTH * (get_y(xy) - 0.5f));
			if (board[xy] == BLACK)
			{
				SelectObject(hDC, (HBRUSH)GetStockObject(BLACK_BRUSH));
				Ellipse(hDC, x, y, x + GRID_WIDTH, y + GRID_WIDTH);
			}
			else if (board[xy] == WHITE)
			{
				SelectObject(hDC, (HBRUSH)GetStockObject(WHITE_BRUSH));
				Ellipse(hDC, x, y, x + GRID_WIDTH, y + GRID_WIDTH);
			}
		}

		// �Ō�ɑł��ꂽ�΂Ƀ}�[�N
		if (record_num > 0)
		{
			XY xy = record[record_num - 1];
			if (xy != PASS)
			{
				int x = scaledX(MARGIN + GRID_WIDTH * (get_x(xy) - 0.5f));
				int y = scaledY(MARGIN + GRID_WIDTH * (get_y(xy) - 0.5f));
				SelectObject(hDC, GetStockObject(NULL_BRUSH));
				SelectObject(hDC, hPenLast);
				Ellipse(hDC, x, y, x + GRID_WIDTH, y + GRID_WIDTH);
			}
			else {
				SetTextColor(hDC, (record_num & 1) ? RGB(0, 0, 0) : RGB(255, 255, 255));
				SetBkMode(hDC, TRANSPARENT);
				HFONT hPrevFont = (HFONT)SelectObject(hDC, hFontPass);
				int drawX = scaledX(MARGIN + GRID_WIDTH * GRID_SIZE / 2);
				int drawY = scaledY(MARGIN);
				TextOut(hDC, drawX, drawY, L"PASS", 4);
				SelectObject(hDC, hPrevFont);
			}
		}

		// �v���C�A�E�g���ʂ�\��
		if (result_num > 0)
		{
			HFONT hPrevFont = (HFONT)SelectObject(hDC, hFontPlayout);
			SetTextColor(hDC, RGB(255, 0, 0));
			SetBkMode(hDC, TRANSPARENT);
			for (int i = 0; i < result_num; i++)
			{
				int x = get_x(result[i].xy);
				int y = get_y(result[i].xy);
				int drawX = scaledX(MARGIN + GRID_WIDTH * (x - 0.5f));
				int drawY = scaledY(MARGIN + GRID_WIDTH * (y - 0.25f));
				if (x == 0)
				{
					drawX = scaledX(MARGIN);
					drawY = scaledX(MARGIN);
				}
				wchar_t str[20];
				int len = wsprintf(str, L"%3d/%d\n.%d", result[i].win_num, result[i].playout_num, 100 * result[i].win_num / result[i].playout_num);
				RECT rc = { drawX, drawY, drawX + scaledX(GRID_WIDTH), drawY + scaledY(GRID_WIDTH) };
				DrawText(hDC, str, len, &rc, DT_CENTER);
			}
			SelectObject(hDC, hPrevFont);
		}

		SelectObject(hDC, hPrevBrush);
		SelectObject(hDC, hPrevPen);
		EndPaint(hWnd, &ps);

		return 0;
	}
	case WM_SIZE:
	{
		int infoX = scaledX(MARGIN + GRID_WIDTH * (GRID_SIZE + 1) + MARGIN);
		int infoY = scaledY(MARGIN);
		// �v���C���[�I���{�b�N�X
		MoveWindow(staticPlayers[0], infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT), TRUE);
		infoY += scaledY(CTRL_HEIGHT);

		MoveWindow(cmbPlayers[0], infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT * 10), TRUE);
		infoY += scaledY(CTRL_HEIGHT + CTRL_MARGIN * 2);

		MoveWindow(staticPlayers[1], infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT), TRUE);
		infoY += scaledY(CTRL_HEIGHT);

		MoveWindow(cmbPlayers[1], infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT * 10), TRUE);
		infoY += scaledY(CTRL_HEIGHT + CTRL_MARGIN * 2);

		// �X�^�[�g�{�^��
		MoveWindow(btnStart, infoX, infoY, scaledX(INFO_WIDTH), scaledY(CTRL_HEIGHT), TRUE);

		return 0;
	}
	case WM_DESTROY:
	{
		isPalying = false;
		PostQuitMessage(0);
		return 0;
	}
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

DWORD WINAPI ThreadProc(LPVOID lpParameter)
{
	const char *gtp_commands[] = {
		"protocol_version",
		"name",
		"version",
		"known_command",
		"list_commands",
		"quit",
		"boardsize",
		"clear_board",
		"komi",
		"play",
		"genmove"
	};

	const char gtp_axis_x[] = { '\0', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'J', 'k', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T' };

	char line[256];
	char* err;
	//FILE* fp = fopen("out.txt", "a");
	while ((err = gets_s(line, sizeof(line))) != nullptr)
	{
		//fprintf(fp, "%s\n", line);
		//fflush(fp);
		if (strcmp(line, "name") == 0)
		{
			printf("= GoSample\n\n");
		}
		else if (strcmp(line, "protocol_version") == 0)
		{
			printf("= 2\n\n");
		}
		else if (strcmp(line, "version") == 0)
		{
			printf("= 1.0\n\n");
		}
		else if (strcmp(line, "list_commands") == 0)
		{
			printf("= ");
			for (const char* command : gtp_commands)
			{
				printf("%s\n", command);
			}
			printf("\n");
		}
		else if (strncmp(line, "boardsize", 9) == 0)
		{
			GRID_SIZE = atoi(line + 10);
			// �{�[�h������
			board.init(GRID_SIZE);

			RECT rc = { 0, 0, scaledX(MARGIN + GRID_WIDTH * (GRID_SIZE + 1) + MARGIN + INFO_WIDTH + MARGIN), scaledY(MARGIN + GRID_WIDTH * (GRID_SIZE + 1) + MARGIN) };
			AdjustWindowRect(&rc, style, NULL);
			SetWindowPos(hMainWnd, NULL, 0, 0, rc.right - rc.left, rc.bottom - rc.top, SWP_NOMOVE | SWP_NOZORDER);

			printf("= \n\n");
		}
		else if (strcmp(line, "clear_board") == 0)
		{
			board.init(GRID_SIZE);
			record_num = 0;
			printf("= \n\n");
		}
		else if (strncmp(line, "komi", 4) == 0)
		{
			KOMI = atof(line + 5);
			printf("= \n\n");
		}
		else if (strncmp(line, "play", 4) == 0)
		{
			char charColor = line[5];
			Color color = (charColor == 'B') ? BLACK : WHITE;

			XY xy;
			if (strcmp(line + 7, "PASS") != 0)
			{
				char charX = line[7];
				int x;
				for (x = 1; x <= 19; x++)
				{
					if (charX == gtp_axis_x[x])
					{
						break;
					}
				}
				int y = GRID_SIZE - atoi(line + 8) + 1;

				XY xy = x + BOARD_WIDTH * y;

				board.move(xy, color);

				InvalidateRect(hMainWnd, NULL, FALSE);
			}
			else {
				xy = PASS;
			}

			board.move(PASS, color);
			record[record_num++] = xy; // �����ǉ�

			printf("= \n\n");
		}
		else if (strncmp(line, "genmove", 7) == 0)
		{
			char charColor = line[8];
			Color color = (charColor == 'b') ? BLACK : WHITE;

			Player* current_player = players[color - 1];

			XY xy = current_player->select_move(board, color);
			board.move(xy, color);
			record[record_num++] = xy; // �����ǉ�

			if (xy == PASS)
			{
				printf("= pass\n\n");
			}
			else {
				printf("= %c%d\n\n", gtp_axis_x[get_x(xy)], GRID_SIZE - get_y(xy) + 1);
			}

			// UCT�̏�����ۑ�
			if (typeid(*current_player) == typeid(UCTSample))
			{
				UCTNode* root = ((UCTSample*)current_player)->root;
				for (int i = 0; i < root->child_num; i++)
				{
					result[i] = root->child[i]; // �l�R�s�[
				}
				result_num = root->child_num;
			}
			InvalidateRect(hMainWnd, NULL, FALSE);
		}
		else if (strncmp(line, "final_score", 11) == 0)
		{
			printf("? cannot score\n\n");
		}
		else if (strncmp(line, "quit", 4) == 0)
		{
			printf("= \n\n");
			break;
		}
	}
	//fclose(fp);

	return 0;
}