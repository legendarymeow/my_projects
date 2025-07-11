#define UNICODE
#define _UNICODE
#include <windows.h>
#include <tchar.h>
#include <vector>
#include <algorithm>

// Game state
int board[3][3] = {0}; // 0=empty, 1=X, 2=O
int currentPlayer = 1;  // 1=X, 2=O
bool gameOver = false;
bool vsAI = false;      // Player vs AI mode

// Constants
const int CELL_SIZE = 200;
const int BOARD_SIZE = CELL_SIZE * 3;

// Minimax result structure
struct MoveScore {
    int row;
    int col;
    int score;
};

// Function prototypes
void DrawBoard(HDC hdc, HWND hwnd);
void DrawX(HDC hdc, int row, int col);
void DrawO(HDC hdc, int row, int col);
void ResetGame();
int CheckWinner();
void ShowWinnerMessage(HWND hwnd, int winner);
void AIMove();
MoveScore Minimax(int board[3][3], bool isMaximizing);
int EvaluateBoard(int board[3][3]);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch (msg) {
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            DrawBoard(hdc, hwnd);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            if (gameOver) {
                ResetGame();
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }
            
            // Handle mode selection buttons
            int xPos = LOWORD(lParam);
            int yPos = HIWORD(lParam);
            
            if (yPos > BOARD_SIZE + 50 && yPos < BOARD_SIZE + 100) {
                if (xPos < 300) {
                    vsAI = false; // PvP mode
                } else {
                    vsAI = true;  // PvAI mode
                }
                ResetGame();
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }
            
            if (currentPlayer == 2 && vsAI) break; // AI's turn
            
            int row = yPos / CELL_SIZE;
            int col = xPos / CELL_SIZE;
            
            if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == 0) {
                board[row][col] = currentPlayer;
                
                int winner = CheckWinner();
                if (winner != 0) {
                    gameOver = true;
                    ShowWinnerMessage(hwnd, winner);
                } else {
                    currentPlayer = 3 - currentPlayer; // Switch player
                    
                    if (currentPlayer == 2 && vsAI && !gameOver) {
                        AIMove();
                        winner = CheckWinner();
                        if (winner != 0) {
                            gameOver = true;
                            ShowWinnerMessage(hwnd, winner);
                        }
                        currentPlayer = 3 - currentPlayer;
                    }
                }
                
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
        
        case WM_KEYDOWN: {
            if (wParam == VK_SPACE && gameOver) {
                ResetGame();
                InvalidateRect(hwnd, NULL, TRUE);
            }
            return 0;
        }
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

void DrawBoard(HDC hdc, HWND hwnd) {
    RECT rect;
    GetClientRect(hwnd, &rect);
    
    // Draw grid
    HPEN hPen = CreatePen(PS_SOLID, 6, RGB(0, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    // Vertical lines
    MoveToEx(hdc, CELL_SIZE, 0, NULL);
    LineTo(hdc, CELL_SIZE, BOARD_SIZE);
    MoveToEx(hdc, CELL_SIZE*2, 0, NULL);
    LineTo(hdc, CELL_SIZE*2, BOARD_SIZE);
    
    // Horizontal lines
    MoveToEx(hdc, 0, CELL_SIZE, NULL);
    LineTo(hdc, BOARD_SIZE, CELL_SIZE);
    MoveToEx(hdc, 0, CELL_SIZE*2, NULL);
    LineTo(hdc, BOARD_SIZE, CELL_SIZE*2);
    
    // Draw X's and O's
    for (int row = 0; row < 3; row++) {
        for (int col = 0; col < 3; col++) {
            if (board[row][col] == 1) {
                DrawX(hdc, row, col);
            } else if (board[row][col] == 2) {
                DrawO(hdc, row, col);
            }
        }
    }
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    // Draw status text
    HFONT hFont = CreateFont(24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
                            OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY,
                            VARIABLE_PITCH, _T("Arial"));
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    
    SetTextColor(hdc, RGB(0, 0, 0));
    TCHAR statusText[50];
    if (gameOver) {
        _stprintf_s(statusText, _T("Game Over! Click to play again."));
    } else {
        _stprintf_s(statusText, _T("Current Player: %s"), currentPlayer == 1 ? _T("X") : _T("O"));
    }
    TextOut(hdc, 10, BOARD_SIZE + 10, statusText, _tcslen(statusText));
    
    // Draw mode buttons
    RECT pvpRect = {10, BOARD_SIZE + 50, 290, BOARD_SIZE + 90};
    RECT pvaiRect = {310, BOARD_SIZE + 50, 590, BOARD_SIZE + 90};
    
    FillRect(hdc, &pvpRect, vsAI ? (HBRUSH)(COLOR_BTNFACE+1) : (HBRUSH)(COLOR_HIGHLIGHT+1));
    FillRect(hdc, &pvaiRect, vsAI ? (HBRUSH)(COLOR_HIGHLIGHT+1) : (HBRUSH)(COLOR_BTNFACE+1));
    
    SetBkMode(hdc, TRANSPARENT);
    DrawText(hdc, _T("Player vs Player"), -1, &pvpRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    DrawText(hdc, _T("Player vs AI"), -1, &pvaiRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void DrawX(HDC hdc, int row, int col) {
    HPEN hPen = CreatePen(PS_SOLID, 6, RGB(255, 0, 0));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    int x = col * CELL_SIZE;
    int y = row * CELL_SIZE;
    int padding = CELL_SIZE / 4;
    
    MoveToEx(hdc, x + padding, y + padding, NULL);
    LineTo(hdc, x + CELL_SIZE - padding, y + CELL_SIZE - padding);
    MoveToEx(hdc, x + CELL_SIZE - padding, y + padding, NULL);
    LineTo(hdc, x + padding, y + CELL_SIZE - padding);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void DrawO(HDC hdc, int row, int col) {
    HPEN hPen = CreatePen(PS_SOLID, 6, RGB(0, 0, 255));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    
    int x = col * CELL_SIZE;
    int y = row * CELL_SIZE;
    int padding = CELL_SIZE / 4;
    
    Ellipse(hdc, x + padding, y + padding, x + CELL_SIZE - padding, y + CELL_SIZE - padding);
    
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
}

void ResetGame() {
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            board[i][j] = 0;
        }
    }
    currentPlayer = 1;
    gameOver = false;
}

int CheckWinner() {
    // Check rows
    for (int i = 0; i < 3; i++) {
        if (board[i][0] != 0 && board[i][0] == board[i][1] && board[i][1] == board[i][2]) {
            return board[i][0];
        }
    }
    
    // Check columns
    for (int j = 0; j < 3; j++) {
        if (board[0][j] != 0 && board[0][j] == board[1][j] && board[1][j] == board[2][j]) {
            return board[0][j];
        }
    }
    
    // Check diagonals
    if (board[0][0] != 0 && board[0][0] == board[1][1] && board[1][1] == board[2][2]) {
        return board[0][0];
    }
    if (board[0][2] != 0 && board[0][2] == board[1][1] && board[1][1] == board[2][0]) {
        return board[0][2];
    }
    
    // Check for draw
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == 0) {
                return 0; // Game continues
            }
        }
    }
    return 3; // Draw
}

void ShowWinnerMessage(HWND hwnd, int winner) {
    TCHAR message[50];
    if (winner == 3) {
        _tcscpy_s(message, _T("It's a draw!"));
    } else {
        _stprintf_s(message, _T("Player %c wins!"), winner == 1 ? 'X' : 'O');
    }
    MessageBox(hwnd, message, _T("Game Over"), MB_OK);
}

void AIMove() {
    MoveScore move = Minimax(board, true);
    board[move.row][move.col] = 2; // AI is O (2)
}

MoveScore Minimax(int board[3][3], bool isMaximizing) {
    // Base cases
    int winner = EvaluateBoard(board);
    if (winner == 1) return {-1, -1, -10};
    if (winner == 2) return {-1, -1, 10};
    if (winner == 3) return {-1, -1, 0};
    
    std::vector<std::pair<int, int>> availableMoves;
    for (int i = 0; i < 3; i++) {
        for (int j = 0; j < 3; j++) {
            if (board[i][j] == 0) {
                availableMoves.emplace_back(i, j);
            }
        }
    }
    
    if (isMaximizing) {
        MoveScore bestMove = {-1, -1, -1000};
        for (const auto& move : availableMoves) {
            board[move.first][move.second] = 2;
            int score = Minimax(board, false).score;
            board[move.first][move.second] = 0;
            
            if (score > bestMove.score) {
                bestMove = {move.first, move.second, score};
            }
        }
        return bestMove;
    } else {
        MoveScore bestMove = {-1, -1, 1000};
        for (const auto& move : availableMoves) {
            board[move.first][move.second] = 1;
            int score = Minimax(board, true).score;
            board[move.first][move.second] = 0;
            
            if (score < bestMove.score) {
                bestMove = {move.first, move.second, score};
            }
        }
        return bestMove;
    }
}

int EvaluateBoard(int board[3][3]) {
    return CheckWinner(); // Reuse the same logic
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = _T("TicTacToeClass");
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);

    if (!RegisterClass(&wc)) {
        MessageBox(NULL, _T("Window Registration Failed!"), _T("Error!"), MB_ICONERROR | MB_OK);
        return 0;
    }

    RECT windowRect = {0, 0, BOARD_SIZE, BOARD_SIZE + 120};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);

    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        _T("Tic Tac Toe (PvP/PvAI)"),
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd) {
        MessageBox(NULL, _T("Window Creation Failed!"), _T("Error!"), MB_ICONERROR | MB_OK);
        return 0;
    }

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}