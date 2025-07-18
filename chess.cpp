#define UNICODE           // Use Unicode (for special chess symbols ♔♕)
#define _UNICODE          // Same as above
#define WIN32_LEAN_AND_MEAN // Speeds up compilation by excluding rare Windows APIs
#include <windows.h>      // Core Windows functions (CreateWindow, messages, etc.)
#include <windowsx.h>     // Extra Windows macros (GET_X_LPARAM, etc.)
#include <tchar.h>        // Handles Unicode/ANSI text

using namespace std;

struct ChessMove {
    int fromX, fromY;  // Source position (0-7)
    int toX, toY;      // Destination position (0-7)
    int promotion;      // Piece to promote to (if any)
    int score;          // Used by engine to evaluate moves
    
    // Constructor to initialize all members
    ChessMove() : fromX(0), fromY(0), toX(0), toY(0), promotion(0), score(0) {}
    ChessMove(int fx, int fy, int tx, int ty) : 
        fromX(fx), fromY(fy), toX(tx), toY(ty), promotion(0), score(0) {}
};
ChessMove bestMove = {4, 7, 6, 7};
// Initilaize Pieces
enum Piece {
    EMPTY = 0,
    WHITE_ROOK = 1,
    WHITE_KNIGHT = 2,
    WHITE_BISHOP = 3,
    WHITE_QUEEN = 4,
    WHITE_KING = 5,
    WHITE_PAWN = 6,
    BLACK_ROOK = -1,
    BLACK_KNIGHT = -2,
    BLACK_BISHOP = -3,
    BLACK_QUEEN = -4,
    BLACK_KING = -5,
    BLACK_PAWN = -6
};
enum GameMode { MODE_PVP, MODE_PVAI };
enum GamePhase { OPENING, MIDGAME, ENDGAME };
GameMode currentGameMode = MODE_PVP; // Default to Player vs Player
bool aiThinking = false;
const int AI_TIMER_ID = 1;

// Initilaize Pieces
int board[8][8] = {
    {BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK},
    {BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
    {WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN},    
    {WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK}
};

static bool isPromoting = false; // Global variables for promotion UI
static POINT promotionPos;
static RECT promotionRects[4];  // Stores positions of 4 promotion pieces
static HWND promotionHwnd;  
int currentPlayer = 1; // Switch turns
POINT possibleMoves[64]; // Show all possible moves for the board
int possibleMoveCount = 0; // Calculate possible moves 
bool showMoves = false; // Decide the higlight show or not 
const int squareSize = 80;
int boardStartX = 55;
int boardStartY = 55;
bool gameOver = false; // Check if the game is end
const float REGULAR_PIECE_SCALE = 0.85f;
const float PAWN_SCALE = 0.65f;
POINT whiteKingPos = {4, 7}; 
POINT blackKingPos = {4, 0};
bool whiteKingMoved = false;
bool blackKingMoved = false;
bool whiteRookKingMoved = false;
bool whiteRookQueenMoved = false;
bool blackRookKingMoved = false;
bool blackRookQueenMoved = false;
POINT enPassantTarget = {-1, -1};

// AI's functions
void SortMoves(ChessMove moves[], int moveCount); 
ChessMove FindBestMove(int depth); 
int Minimax(int depth, int alpha, int beta, bool maximizingPlayer);
GamePhase DetectGamePhase();
void GenerateLegalMoves(int player, ChessMove moves[], int &moveCount);
int EvaluatePosition(); 
int CountBishopMoves(int x, int y);
bool IsFileOpen(int x);
int EvaluateKingIndianDefense(int player);
int CalculateMobility(int player);
int CalculateKingSafety(int player);
int QuiescenceSearch(int alpha, int beta, int player);
void SortCaptures(ChessMove moves[], int moveCount);
int CountCenterControl(int player);
int EvaluatePawnStructure(int player);
void GenerateCaptureMoves(int player, ChessMove moves[], int &moveCount);
// Function declarations
const TCHAR* GetPieceSymbol(int piece);
// Drawing and make a piece treat like a rectangle and move pieces and reset game
void DrawBoard(HDC hdc, HWND hwnd);
void DrawPieces(HDC hdc);
void GetPieceRect(int row, int col, RECT* rect, bool isPawn);
void MovePiece(HWND hwnd, int fromCol, int fromRow, int toCol, int toRow);
void DrawPromotionChoice(HDC hdc);
void HandlePromotionClick(int x, int y);
void CreateModeButtons(HWND hwnd);
void PromotePawn(HWND hwnd, int col, int row); // Change the pawn to other pieces
void ResetGame();
// Chess rules and conditions
bool IsValidMoveWithoutCheck(int fromCol, int fromRow, int toCol, int toRow);
bool IsCaptureMove(int fromCol, int fromRow, int toCol, int toRow);
bool IsSquareUnderAttack(int col, int row, int byPlayer);
bool IsInCheck(int player);
bool IsCheckmate(int player);
bool CanCastle(int player, bool kingside);
bool IsStalemate(int player);
bool IsValidMove(int fromCol, int fromRow, int toCol, int toRow);
// Insted of algorithem fuction
int abs(int value);
int Min(int a, int b);
int Max(int a, int b);

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static POINT selectedSquare = {-1, -1};
    
    switch (msg) {
        case WM_COMMAND:
            if (LOWORD(wParam) == 1) { // PvP button
                currentGameMode = MODE_PVP;
                KillTimer(hwnd, AI_TIMER_ID);
                aiThinking = false;
                
                // Update button states
                HWND hPvPButton = GetDlgItem(hwnd, 1);
                HWND hPvAIButton = GetDlgItem(hwnd, 2);
                SendMessage(hPvPButton, BM_SETSTATE, TRUE, 0);
                SendMessage(hPvAIButton, BM_SETSTATE, FALSE, 0);
                
                ResetGame();
                InvalidateRect(hwnd, NULL, TRUE);
            }
            else if (LOWORD(wParam) == 2) { // PvAI button
                currentGameMode = MODE_PVAI;
                
                // Update button states
                HWND hPvPButton = GetDlgItem(hwnd, 1);
                HWND hPvAIButton = GetDlgItem(hwnd, 2);
                SendMessage(hPvPButton, BM_SETSTATE, FALSE, 0);
                SendMessage(hPvAIButton, BM_SETSTATE, TRUE, 0);
                
                ResetGame();
                InvalidateRect(hwnd, NULL, TRUE);
                
                // If AI is black, start thinking immediately
                if (currentPlayer == -1) {
                    SetTimer(hwnd, AI_TIMER_ID, 100, NULL);
                    aiThinking = true;
                }
            }
            break;
            
        case WM_TIMER:
            if (wParam == AI_TIMER_ID && currentGameMode == MODE_PVAI && 
                currentPlayer == -1 && !gameOver) {
                KillTimer(hwnd, AI_TIMER_ID);
                
                // Get the best move
                ChessMove bestMove = FindBestMove(3); // Depth 3 search
                
                // Make the move on the board
                int piece = board[bestMove.fromY][bestMove.fromX];
                board[bestMove.toY][bestMove.toX] = piece;
                board[bestMove.fromY][bestMove.fromX] = EMPTY;
                
                // Update king position if moved
                if (abs(piece) == WHITE_KING) {
                    if (piece > 0) {
                        whiteKingPos.x = bestMove.toX;
                        whiteKingPos.y = bestMove.toY;
                        whiteKingMoved = true;
                    } else {
                        blackKingPos.x = bestMove.toX;
                        blackKingPos.y = bestMove.toY;
                        blackKingMoved = true;
                    }
                }
                
                // Handle promotion
                if (bestMove.promotion != 0) {
                    board[bestMove.toY][bestMove.toX] = bestMove.promotion;
                }
                
                // Switch turns
                currentPlayer = 1;
                
                // Check game state
                if (IsCheckmate(1)) {
                    MessageBox(hwnd, _T("AI wins by checkmate!"), _T("Game Over"), MB_OK);
                    gameOver = true;
                }
                else if (IsStalemate(1)) {
                    MessageBox(hwnd, _T("Stalemate! Game is a draw."), _T("Game Over"), MB_OK);
                    gameOver = true;
                }
                
                InvalidateRect(hwnd, NULL, TRUE);
            }
            break;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
            
        case WM_SIZE: {
            RECT clientRect;
            GetClientRect(hwnd, &clientRect);
            boardStartX = (clientRect.right - 8 * squareSize) / 2;
            boardStartY = (clientRect.bottom - 8 * squareSize) / 2;
            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            DrawBoard(hdc, hwnd);
            
            if (showMoves && possibleMoveCount > 0) {
                for (int i = 0; i < possibleMoveCount; i++) {
                    int row = possibleMoves[i].y;
                    int col = possibleMoves[i].x;
                    
                    bool isCapture = IsCaptureMove(selectedSquare.x, selectedSquare.y, col, row);
                    bool isCastle = (abs(board[selectedSquare.y][selectedSquare.x]) == WHITE_KING && 
                                    abs(selectedSquare.x - col) == 2);
                    
                    if (isCapture || isCastle) {
                        RECT captureRect = {
                            boardStartX + col * squareSize,
                            boardStartY + row * squareSize,
                            boardStartX + (col + 1) * squareSize,
                            boardStartY + (row + 1) * squareSize
                        };
                        HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 100, 100));
                        FillRect(hdc, &captureRect, hRedBrush);
                        DeleteObject(hRedBrush);
                    }
                    else if (board[row][col] == EMPTY) {
                        HBRUSH hGreenBrush = CreateSolidBrush(RGB(100, 255, 100));
                        int centerX = boardStartX + col * squareSize + squareSize/2;
                        int centerY = boardStartY + row * squareSize + squareSize/2;
                        int dotSize = 15;
                        
                        Ellipse(hdc, 
                            centerX - dotSize/2, 
                            centerY - dotSize/2,
                            centerX + dotSize/2, 
                            centerY + dotSize/2);
                        
                        DeleteObject(hGreenBrush);
                    }
                }
            }
            
            DrawPieces(hdc);
            
            if (selectedSquare.x != -1) {
                RECT pieceRect;
                int piece = board[selectedSquare.y][selectedSquare.x];
                bool isPawn = (abs(piece) == WHITE_PAWN);
                GetPieceRect(selectedSquare.y, selectedSquare.x, &pieceRect, isPawn);
                
                HPEN hWhiteBluePen = CreatePen(PS_SOLID, 3, RGB(220, 220, 255));
                HPEN hOldPen = (HPEN)SelectObject(hdc, hWhiteBluePen);
                HBRUSH hOldBrush = (HBRUSH)SelectObject(hdc, GetStockObject(NULL_BRUSH));
                
                int expandBy = isPawn ? 15 : 5;
                Rectangle(hdc, 
                    pieceRect.left - expandBy,
                    pieceRect.top - expandBy,
                    pieceRect.right + expandBy,
                    pieceRect.bottom + expandBy);
                
                SelectObject(hdc, hOldPen);
                SelectObject(hdc, hOldBrush);
                DeleteObject(hWhiteBluePen);
            }
            DrawPromotionChoice(hdc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        
        case WM_LBUTTONDOWN: {
            if (gameOver) {
                ResetGame();
                InvalidateRect(hwnd, NULL, TRUE);
                break;
            }
            
            int xPos = GET_X_LPARAM(lParam);
            int yPos = GET_Y_LPARAM(lParam);

            if (isPromoting) {
                HandlePromotionClick(xPos, yPos);
                return 0;
            }
            
            int col = (xPos - boardStartX) / squareSize;
            int row = (yPos - boardStartY) / squareSize;
            
            if (row < 0 || row >= 8 || col < 0 || col >= 8) {
                selectedSquare.x = -1;
                showMoves = false;
                InvalidateRect(hwnd, NULL, TRUE);
                return 0;
            }
            
            int piece = board[row][col];
            
            if (selectedSquare.x == -1) {
                if (piece != EMPTY && ((currentPlayer == 1 && piece > 0) || (currentPlayer == -1 && piece < 0))) {
                    selectedSquare.x = col;
                    selectedSquare.y = row;
                    showMoves = true;
                    
                    possibleMoveCount = 0;
                    for (int r = 0; r < 8; r++) {
                        for (int c = 0; c < 8; c++) {
                            if (IsValidMove(col, row, c, r)) {
                                possibleMoves[possibleMoveCount].x = c;
                                possibleMoves[possibleMoveCount].y = r;
                                possibleMoveCount++;
                            }
                        }
                    }
                }
            } else {
                if (IsValidMove(selectedSquare.x, selectedSquare.y, col, row)) {
                    MovePiece(hwnd, selectedSquare.x, selectedSquare.y, col, row);
                    
                    // Check for pawn promotion
                    int movedPiece = board[row][col];
                    if (abs(movedPiece) == WHITE_PAWN && (row == 0 || row == 7)) {
                        PromotePawn(hwnd, col, row);
                    }
                    
                    // Switch players
                    currentPlayer = -currentPlayer;
                    
                    // Check for game over conditions
                    if (IsCheckmate(currentPlayer)) {
                        TCHAR message[100];
                        wsprintf(message, _T("%s wins by checkmate!"), 
                                currentPlayer == 1 ? _T("White") : _T("Black"));
                        MessageBox(hwnd, message, _T("Game Over"), MB_OK);
                        gameOver = true;
                    }
                    else if (IsStalemate(currentPlayer)) {
                        MessageBox(hwnd, _T("Stalemate! Game is a draw."), _T("Game Over"), MB_OK);
                        gameOver = true;
                    }
                }
                selectedSquare.x = -1;
                showMoves = false;
            }
            if (currentGameMode == MODE_PVAI && currentPlayer == -1 && !gameOver) {
                SetTimer(hwnd, AI_TIMER_ID, 100, NULL); // 100ms delay before AI moves
            }

            InvalidateRect(hwnd, NULL, TRUE);
            return 0;
        }
        case WM_KEYDOWN:
            if (wParam == VK_SPACE) {  // Space key pressed
                ResetGame();
                InvalidateRect(hwnd, NULL, TRUE);  // Redraw the board
            }
        return 0;
    }
    return DefWindowProc(hwnd, msg, wParam, lParam);
}

const TCHAR* GetPieceSymbol(int piece) {
    switch (piece) {
        case WHITE_PAWN: return _T("♟");
        case WHITE_ROOK: return _T("♜");
        case WHITE_KNIGHT: return _T("♞");
        case WHITE_BISHOP: return _T("♝");
        case WHITE_QUEEN: return _T("♛");
        case WHITE_KING: return _T("♚");
        case BLACK_PAWN: return _T("♟");
        case BLACK_ROOK: return _T("♜");
        case BLACK_KNIGHT: return _T("♞");
        case BLACK_BISHOP: return _T("♝");
        case BLACK_QUEEN: return _T("♛");
        case BLACK_KING: return _T("♚");
        default: return _T("");
    }
}

int abs(int value) {
    return (value < 0) ? -value : value;
}

int Min(int a, int b) {
    return a < b ? a : b;
}

int Max(int a, int b) {
    return a > b ? a : b;
}

void ResetGame() {
    // Reset the board to starting position
    int newBoard[8][8] = {
        {BLACK_ROOK, BLACK_KNIGHT, BLACK_BISHOP, BLACK_QUEEN, BLACK_KING, BLACK_BISHOP, BLACK_KNIGHT, BLACK_ROOK},
        {BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN, BLACK_PAWN},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY, EMPTY},
        {WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN, WHITE_PAWN},    
        {WHITE_ROOK, WHITE_KNIGHT, WHITE_BISHOP, WHITE_QUEEN, WHITE_KING, WHITE_BISHOP, WHITE_KNIGHT, WHITE_ROOK}
    };
    
    memcpy(board, newBoard, sizeof(board));
    
    // Reset game state variables
    currentPlayer = 1;
    possibleMoveCount = 0;
    showMoves = false;
    gameOver = false;
    
    // Reset king positions
    whiteKingPos = {4, 7};
    blackKingPos = {4, 0};
    
    // Reset castling flags
    whiteKingMoved = false;
    blackKingMoved = false;
    whiteRookKingMoved = false;
    whiteRookQueenMoved = false;
    blackRookKingMoved = false;
    blackRookQueenMoved = false;
    
    // Reset en passant target
    enPassantTarget = {-1, -1};
}

void DrawPromotionChoice(HDC hdc) {
    if (!isPromoting) return;

    // Define piece options
    int pieces[4] = { WHITE_QUEEN, WHITE_ROOK, WHITE_BISHOP, WHITE_KNIGHT };
    if (board[promotionPos.y][promotionPos.x] < 0) {  // Black promotion
        pieces[0] = BLACK_QUEEN;
        pieces[1] = BLACK_ROOK;
        pieces[2] = BLACK_BISHOP;
        pieces[3] = BLACK_KNIGHT;
    }

    // Calculate promotion area position
    int startY, endY;
    if (promotionPos.y < 4) {  // Black promotion (show below)
        startY = promotionPos.y + 1;
        endY = promotionPos.y + 5;
    } else {  // White promotion (show above)
        startY = promotionPos.y - 4;
        endY = promotionPos.y;
    }

    // Draw selection background
    HBRUSH hBgBrush = CreateSolidBrush(RGB(164, 164, 164));
    RECT bgRect;
    bgRect.left = boardStartX + promotionPos.x * squareSize;
    bgRect.top = boardStartY + startY * squareSize;
    bgRect.right = boardStartX + (promotionPos.x + 1) * squareSize;
    bgRect.bottom = boardStartY + endY * squareSize;
    FillRect(hdc, &bgRect, hBgBrush);
    DeleteObject(hBgBrush);

    // Draw each option
    for (int i = 0; i < 4; i++) {
        int yPos = (promotionPos.y < 4) ? promotionPos.y + 1 + i : promotionPos.y - 1 - i;
        
        promotionRects[i].left = boardStartX + promotionPos.x * squareSize;
        promotionRects[i].top = boardStartY + yPos * squareSize;
        promotionRects[i].right = boardStartX + (promotionPos.x + 1) * squareSize;
        promotionRects[i].bottom = boardStartY + (yPos + 1) * squareSize;

        // Draw piece
        HFONT hFont = CreateFont(
            squareSize * 0.8, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
            ANTIALIASED_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Arial Unicode MS")
        );
        HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
        
        SetTextColor(hdc, (pieces[i] > 0) ? RGB(255, 255, 255) : RGB(64, 64, 64));
        DrawText(hdc, GetPieceSymbol(pieces[i]), -1, &promotionRects[i], 
                DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        SelectObject(hdc, hOldFont);
        DeleteObject(hFont);
    }
}

void HandlePromotionClick(int x, int y) {
    if (!isPromoting) return;

    for (int i = 0; i < 4; i++) {
        if (x >= promotionRects[i].left && x <= promotionRects[i].right &&
            y >= promotionRects[i].top && y <= promotionRects[i].bottom) {
            // Determine which piece was clicked
            int newPiece;
            switch (i) {
                case 0: newPiece = (board[promotionPos.y][promotionPos.x] > 0) ? 
                                  WHITE_QUEEN : BLACK_QUEEN; break;
                case 1: newPiece = (board[promotionPos.y][promotionPos.x] > 0) ? 
                                  WHITE_ROOK : BLACK_ROOK; break;
                case 2: newPiece = (board[promotionPos.y][promotionPos.x] > 0) ? 
                                  WHITE_BISHOP : BLACK_BISHOP; break;
                case 3: newPiece = (board[promotionPos.y][promotionPos.x] > 0) ? 
                                  WHITE_KNIGHT : BLACK_KNIGHT; break;
            }
            
            board[promotionPos.y][promotionPos.x] = newPiece;
            isPromoting = false;
            InvalidateRect(promotionHwnd, NULL, TRUE);
            break;
        }
    }
}

void PromotePawn(HWND hwnd, int col, int row) {
    promotionPos.x = col;
    promotionPos.y = row;
    promotionHwnd = hwnd;
    isPromoting = true;
    InvalidateRect(hwnd, NULL, TRUE);
}
void GetPieceRect(int row, int col, RECT* rect, bool isPawn) {
    const float scale = isPawn ? PAWN_SCALE : REGULAR_PIECE_SCALE;
    const int offset = (squareSize - static_cast<int>(squareSize * scale)) / 2;
    
    rect->left = boardStartX + col * squareSize + offset;
    rect->top = boardStartY + row * squareSize + offset;
    rect->right = rect->left + static_cast<int>(squareSize * scale);
    rect->bottom = rect->top + static_cast<int>(squareSize * scale);
}

void CreateModeButtons(HWND hwnd) {
    // Create a font for the buttons
    HFONT hFont = CreateFont(
        16, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE,
        DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
        DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Arial")
    );
    
    // Player vs Player button
    HWND hPvPButton = CreateWindow(
        _T("BUTTON"), _T("Player vs Player"),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
        20, 20, 180, 40,
        hwnd, (HMENU)1, NULL, NULL
    );
    SendMessage(hPvPButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Player vs AI button
    HWND hPvAIButton = CreateWindow(
        _T("BUTTON"), _T("Player vs AI"),
        WS_VISIBLE | WS_CHILD | BS_PUSHBUTTON | BS_FLAT,
        220, 20, 180, 40,
        hwnd, (HMENU)2, NULL, NULL
    );
    SendMessage(hPvAIButton, WM_SETFONT, (WPARAM)hFont, TRUE);
    
    // Highlight the current mode
    if (currentGameMode == MODE_PVP) {
        SendMessage(hPvPButton, BM_SETSTATE, TRUE, 0);
    } else {
        SendMessage(hPvAIButton, BM_SETSTATE, TRUE, 0);
    }
}

void DrawPieces(HDC hdc) {
    SetBkMode(hdc, TRANSPARENT);
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            if (board[row][col] != EMPTY) {
                RECT pieceRect;
                bool isPawn = (abs(board[row][col]) == WHITE_PAWN);
                GetPieceRect(row, col, &pieceRect, isPawn);
                
                int fontSize = static_cast<int>(squareSize * (isPawn ? PAWN_SCALE : REGULAR_PIECE_SCALE));
                HFONT hFont = CreateFont(
                    fontSize, 0, 0, 0, 
                    FW_BOLD, FALSE, FALSE, FALSE,
                    DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
                    CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                    DEFAULT_PITCH | FF_DONTCARE, 
                    _T("Arial Unicode MS")
                );
                HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
                
                SetTextColor(hdc, RGB(0, 0, 0));
                OffsetRect(&pieceRect, 1, 1);
                DrawText(hdc, GetPieceSymbol(board[row][col]), 1, &pieceRect, 
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                
                OffsetRect(&pieceRect, -1, -1);
                SetTextColor(hdc, (board[row][col] > 0) ? RGB(255, 255, 255) : RGB(64, 64, 64));
                DrawText(hdc, GetPieceSymbol(board[row][col]), 1, &pieceRect, 
                         DT_CENTER | DT_VCENTER | DT_SINGLELINE);
                
                SelectObject(hdc, hOldFont);
                DeleteObject(hFont);
            }
        }
    }
}

void DrawBoard(HDC hdc, HWND hwnd) {

    RECT clientRect;
    GetClientRect(hwnd, &clientRect);
    
    // Draw a nice background for the buttons area
    HBRUSH hButtonAreaBrush = CreateSolidBrush(RGB(210, 210, 210));
    RECT buttonArea = {0, 0, clientRect.right, 70};
    FillRect(hdc, &buttonArea, hButtonAreaBrush);
    DeleteObject(hButtonAreaBrush);
    
    // Draw a separator line
    HPEN hPen = CreatePen(PS_SOLID, 1, RGB(150, 150, 150));
    HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
    MoveToEx(hdc, 0, 70, NULL);
    LineTo(hdc, clientRect.right, 70);
    SelectObject(hdc, hOldPen);
    DeleteObject(hPen);
    
    HBRUSH hBgBrush = CreateSolidBrush(RGB(121, 76, 39));
    FillRect(hdc, &clientRect, hBgBrush);
    DeleteObject(hBgBrush);
    
    if (IsInCheck(currentPlayer)) {
        POINT kingPos = (currentPlayer == 1) ? whiteKingPos : blackKingPos;
        RECT kingRect = {
            boardStartX + kingPos.x * squareSize,
            boardStartY + kingPos.y * squareSize,
            boardStartX + (kingPos.x + 1) * squareSize,
            boardStartY + (kingPos.y + 1) * squareSize
        };
        HBRUSH hRedBrush = CreateSolidBrush(RGB(255, 150, 150));
        FillRect(hdc, &kingRect, hRedBrush);
        DeleteObject(hRedBrush);
    }
    
    for (int row = 0; row < 8; row++) {
        for (int col = 0; col < 8; col++) {
            // Skip drawing regular square if it's the checked king's square
            if (IsInCheck(currentPlayer) && 
                ((currentPlayer == 1 && row == whiteKingPos.y && col == whiteKingPos.x) ||
                 (currentPlayer == -1 && row == blackKingPos.y && col == blackKingPos.x))) {
                continue;
            }
            
            HBRUSH hSquareBrush = CreateSolidBrush(
                (row + col) % 2 ? RGB(181, 136, 99) : RGB(240, 217, 181)
            );
            
            RECT squareRect = {
                boardStartX + col * squareSize,
                boardStartY + row * squareSize,
                boardStartX + (col + 1) * squareSize,
                boardStartY + (row + 1) * squareSize
            };
            
            FillRect(hdc, &squareRect, hSquareBrush);
            DeleteObject(hSquareBrush);
        }
    }
    
    // Draw coordinates
    HFONT hFont = CreateFont(22, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, 
                            DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, 
                            DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Arial"));
    HFONT hOldFont = (HFONT)SelectObject(hdc, hFont);
    SetBkMode(hdc, TRANSPARENT);
    SetTextColor(hdc, RGB(230, 230, 230));
    
    for (int i = 0; i < 8; i++) {
        TCHAR file = 'a' + i;
        TCHAR rank = '8' - i;
        
        RECT fileRect = {boardStartX + i * squareSize, boardStartY + 8 * squareSize, 
                         boardStartX + (i + 1) * squareSize, boardStartY + 8 * squareSize + 20};
        DrawText(hdc, &file, 1, &fileRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        
        RECT rankRect = {boardStartX - 20, boardStartY + i * squareSize, 
                         boardStartX, boardStartY + (i + 1) * squareSize};
        DrawText(hdc, &rank, 1, &rankRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);

    const TCHAR* turnText = (currentPlayer > 0) ? _T("White's Turn") : _T("Black's Turn");
    RECT turnRect = {boardStartX, boardStartY - 30, boardStartX + 8*squareSize, boardStartY};
    SetTextColor(hdc, (currentPlayer > 0) ? RGB(255, 255, 255) : RGB(0, 0, 0));
    SetBkColor(hdc, (currentPlayer > 0) ? RGB(0, 0, 0) : RGB(255, 255, 255));
    DrawText(hdc, turnText, -1, &turnRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    
    if (IsInCheck(currentPlayer)) {
        const TCHAR* checkText = IsCheckmate(currentPlayer) ? 
            _T("CHECKMATE!") : _T("CHECK!");
        RECT statusRect = {boardStartX, boardStartY + 8*squareSize + 5, 
                          boardStartX + 8*squareSize, boardStartY + 8*squareSize + 30};
        
        SetTextColor(hdc, RGB(255, 0, 0));
        SetBkColor(hdc, RGB(255, 255, 255));
        DrawText(hdc, checkText, -1, &statusRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
    }

    CreateFont(18, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                           DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, _T("Arial"));
    (HFONT)SelectObject(hdc, hFont);
    
    const TCHAR* modeText = (currentGameMode == MODE_PVP) ? 
        _T("Mode: Player vs Player") : _T("Mode: Player vs AI");
    SetTextColor(hdc, RGB(0, 0, 0));
    SetBkMode(hdc, TRANSPARENT);
    
    RECT textRect = {20, 75, clientRect.right, 100};
    DrawText(hdc, modeText, -1, &textRect, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    
    SelectObject(hdc, hOldFont);
    DeleteObject(hFont);
}

void MovePiece(HWND hwnd, int fromCol, int fromRow, int toCol, int toRow) {
    int piece = board[fromRow][fromCol];
    
    // Handle en passant capture
    if (abs(piece) == WHITE_PAWN && toCol == enPassantTarget.x && toRow == enPassantTarget.y) {
        // Remove the captured pawn
        board[fromRow][toCol] = EMPTY;
    }

    // Handle castling
    if (abs(piece) == WHITE_KING && abs(fromCol - toCol) == 2) {
        int row = fromRow;
        
        if (toCol > fromCol) { // Kingside
            board[row][5] = board[row][7];
            board[row][7] = EMPTY;
            // ADDED: Update rook moved flag
            if (piece > 0) whiteRookKingMoved = true;
            else blackRookKingMoved = true;
        } else { // Queenside
            board[row][3] = board[row][0];
            board[row][0] = EMPTY;
            // ADDED: Update rook moved flag
            if (piece > 0) whiteRookQueenMoved = true;
            else blackRookQueenMoved = true;
        }
    }
    
    // Update en passant target
    enPassantTarget.x = -1;
    enPassantTarget.y = -1;
    if (abs(piece) == WHITE_PAWN && abs(fromRow - toRow) == 2) {
        enPassantTarget.x = fromCol;
        enPassantTarget.y = (fromRow + toRow) / 2;
    }
    
    // Update king moved flags - FIXED POSITION UPDATE
    if (abs(piece) == WHITE_KING) {
        if (piece > 0) {
            whiteKingPos.x = toCol;
            whiteKingPos.y = toRow;
            whiteKingMoved = true;  // Moved inside king check
        } else {
            blackKingPos.x = toCol;
            blackKingPos.y = toRow;
            blackKingMoved = true;  // Moved inside king check
        }
    }
    
    // Update rook moved flags
    if (abs(piece) == WHITE_ROOK) {
        if (piece > 0) { // white rook
            if (fromCol == 0 && fromRow == 7) whiteRookQueenMoved = true;  // Fixed row
            if (fromCol == 7 && fromRow == 7) whiteRookKingMoved = true;   // Fixed row
        } else { // black rook
            if (fromCol == 0 && fromRow == 0) blackRookQueenMoved = true;  // Fixed row
            if (fromCol == 7 && fromRow == 0) blackRookKingMoved = true;   // Fixed row
        }
    }
    // Make the move
    board[toRow][toCol] = piece;
    board[fromRow][fromCol] = EMPTY;
    
    InvalidateRect(hwnd, NULL, TRUE);
}

bool IsSquareUnderAttack(int col, int row, int byPlayer) {
    // Temporarily remove the piece at target square to check attacks
    int savedPiece = board[row][col];
    board[row][col] = EMPTY;

    for (int r = 0; r < 8; r++) {
        for (int c = 0; c < 8; c++) {
            int piece = board[r][c];
            if (piece != EMPTY && piece * byPlayer < 0) { // Opponent's piece
                // Special handling for pawn attacks
                if (abs(piece) == WHITE_PAWN) {
                    int direction = (piece > 0) ? -1 : 1; // White pawns move up, black pawns move down
                    if (abs(c - col) == 1 && r + direction == row) {
                        board[row][col] = savedPiece;
                        return true;
                    }
                }
                // For other pieces, use the normal move validation
                else if (IsValidMoveWithoutCheck(c, r, col, row)) {
                    board[row][col] = savedPiece;
                    return true;
                }
            }
        }
    }
    
    board[row][col] = savedPiece;
    return false;
}

bool IsValidMoveWithoutCheck(int fromCol, int fromRow, int toCol, int toRow) {
    int piece = board[fromRow][fromCol];
    int target = board[toRow][toCol];

    if (piece == EMPTY) return false;
    if (fromCol == toCol && fromRow == toRow) return false;
    if (target != EMPTY && piece * target > 0) return false;
    
    int colDiff = toCol - fromCol;
    int rowDiff = toRow - fromRow;
    int absColDiff = abs(colDiff);
    int absRowDiff = abs(rowDiff);
    
    switch (abs(piece)) {
        case WHITE_PAWN: {
            int direction = (piece > 0) ? -1 : 1;
            
            // Forward move
            if (fromCol == toCol && target == EMPTY) {
                if (fromRow + direction == toRow) return true;
                // Double move from starting position
                if ((piece > 0 && fromRow == 6) || (piece < 0 && fromRow == 1)) {
                    if (fromRow + 2*direction == toRow && 
                        board[fromRow + direction][fromCol] == EMPTY) {
                        return true;
                    }
                }
            }
            // Capture
            else if (absColDiff == 1 && fromRow + direction == toRow) {
                // Normal capture
                if (target != EMPTY) return true;
                // En passant
                if (toCol == enPassantTarget.x && toRow == enPassantTarget.y) {
                    return true;
                }
            }
            break;
        }
        case WHITE_KNIGHT: {
            return (absColDiff == 2 && absRowDiff == 1) || 
                   (absColDiff == 1 && absRowDiff == 2);
        }
        case WHITE_BISHOP: {
            if (absColDiff != absRowDiff) return false;
            int colStep = (colDiff > 0) ? 1 : -1;
            int rowStep = (rowDiff > 0) ? 1 : -1;
            for (int i = 1; i < absColDiff; i++) {
                if (board[fromRow + i * rowStep][fromCol + i * colStep] != EMPTY)
                    return false;
            }
            return true;
        }
        case WHITE_ROOK: {
            if (fromCol != toCol && fromRow != toRow) return false;
            if (fromCol == toCol) {
                int step = (rowDiff > 0) ? 1 : -1;
                for (int r = fromRow + step; r != toRow; r += step) {
                    if (board[r][fromCol] != EMPTY) return false;
                }
            } else {
                int step = (colDiff > 0) ? 1 : -1;
                for (int c = fromCol + step; c != toCol; c += step) {
                    if (board[fromRow][c] != EMPTY) return false;
                }
            }
            return true;
        }
        case WHITE_QUEEN: {
            if (fromCol == toCol || fromRow == toRow) {
                if (fromCol == toCol) {
                    int step = (rowDiff > 0) ? 1 : -1;
                    for (int r = fromRow + step; r != toRow; r += step) {
                        if (board[r][fromCol] != EMPTY) return false;
                    }
                } else {
                    int step = (colDiff > 0) ? 1 : -1;
                    for (int c = fromCol + step; c != toCol; c += step) {
                        if (board[fromRow][c] != EMPTY) return false;
                    }
                }
                return true;
            }
            else if (absColDiff == absRowDiff) {
                int colStep = (colDiff > 0) ? 1 : -1;
                int rowStep = (rowDiff > 0) ? 1 : -1;
                for (int i = 1; i < absColDiff; i++) {
                    if (board[fromRow + i * rowStep][fromCol + i * colStep] != EMPTY)
                        return false;
                }
                return true;
            }
            break;
        }
        case WHITE_KING: {
            // Normal king move
            if (absColDiff <= 1 && absRowDiff <= 1) {
                return true;
            }
            // Castling
            if (absColDiff == 2 && rowDiff == 0 && fromRow == (piece > 0 ? 7 : 0)) {
                return CanCastle(piece > 0 ? 1 : -1, colDiff > 0);
            }
            break;
        }
    }
    
    return false;
}

bool IsInCheck(int player) {
    POINT kingPos = (player == 1) ? whiteKingPos : blackKingPos;
    return IsSquareUnderAttack(kingPos.x, kingPos.y, player);
}

bool IsCheckmate(int player) {
    if (!IsInCheck(player)) return false;
    
    // Check all possible moves to see if any get out of check
    for (int fromRow = 0; fromRow < 8; fromRow++) {
        for (int fromCol = 0; fromCol < 8; fromCol++) {
            int piece = board[fromRow][fromCol];
            if (piece != EMPTY && piece * player > 0) { // Player's piece
                for (int toRow = 0; toRow < 8; toRow++) {
                    for (int toCol = 0; toCol < 8; toCol++) {
                        if (IsValidMove(fromCol, fromRow, toCol, toRow)) {
                            return false; // Found a legal move
                        }
                    }
                }
            }
        }
    }
    return true; // No legal moves found
}

bool IsStalemate(int player) {
    // Player must not be in check
    if (IsInCheck(player)) return false;
    
    // Check if player has any legal moves
    for (int fromRow = 0; fromRow < 8; fromRow++) {
        for (int fromCol = 0; fromCol < 8; fromCol++) {
            int piece = board[fromRow][fromCol];
            if (piece != EMPTY && piece * player > 0) { // Player's piece
                for (int toRow = 0; toRow < 8; toRow++) {
                    for (int toCol = 0; toCol < 8; toCol++) {
                        if (IsValidMove(fromCol, fromRow, toCol, toRow)) {
                            return false; // Found a legal move
                        }
                    }
                }
            }
        }
    }
    return true; // No legal moves found
}

bool CanCastle(int player, bool kingside) {
    int row = (player == 1) ? 7 : 0;  // White: bottom row (7), Black: top row (0)
    int kingCol = 4;  // King always starts on column 4 (e-file)
    int rookCol = kingside ? 7 : 0;
    int kingNewCol = kingside ? 6 : 2;  // King's new position after castling
    
    // Check if king and rook have moved
    if (player == 1) { // White
        if (whiteKingMoved) return false;
        if (kingside && whiteRookKingMoved) return false;
        if (!kingside && whiteRookQueenMoved) return false;
    } else { // Black
        if (blackKingMoved) return false;
        if (kingside && blackRookKingMoved) return false;
        if (!kingside && blackRookQueenMoved) return false;
    }
    
    // Check if squares between are empty
    int startCol = Min(kingCol, rookCol) + 1;
    int endCol = Max(kingCol, rookCol);
    for (int col = startCol; col < endCol; col++) {
        if (board[row][col] != EMPTY) return false;
    }
    
    // Check if king is in check
    if (IsInCheck(player)) return false;
    
    // Check intermediate squares for attack
    int step = kingside ? 1 : -1;
    for (int col = kingCol + step; col != kingNewCol + step; col += step) {
        if (IsSquareUnderAttack(col, row, player)) {
            return false;
        }
    }
    
    return true;
}

bool IsValidMove(int fromCol, int fromRow, int toCol, int toRow) {
    int piece = board[fromRow][fromCol];
    int target = board[toRow][toCol];

    // Basic checks
    if (piece == EMPTY) return false;
    if (fromCol == toCol && fromRow == toRow) return false;
    if (target != EMPTY && piece * target > 0) return false;
    if ((currentPlayer == 1 && piece < 0) || (currentPlayer == -1 && piece > 0)) {
        return false;
    }
    
    if (!IsValidMoveWithoutCheck(fromCol, fromRow, toCol, toRow)) {
        return false;
    }
    
    // Special case: castling
    if (abs(piece) == WHITE_KING && abs(fromCol - toCol) == 2) {
        return CanCastle(piece > 0 ? 1 : -1, toCol > fromCol);
    }
    
    // Store original positions for validation
    int savedTarget = target;
 
    // Temporarily make the move
    board[fromRow][fromCol] = EMPTY;
    board[toRow][toCol] = piece;
    
    // Update king position if needed
    POINT savedKingPos;
    if (abs(piece) == WHITE_KING) {
        if (piece > 0) {
            savedKingPos = whiteKingPos;
            whiteKingPos.x = toCol;
            whiteKingPos.y = toRow;
        } else {
            savedKingPos = blackKingPos;
            blackKingPos.x = toCol;
            blackKingPos.y = toRow;
        }
    }
    
    bool inCheckAfterMove = IsInCheck(currentPlayer);
    
    // Undo the move
    board[fromRow][fromCol] = piece;
    board[toRow][toCol] = savedTarget;
    
    // Restore king position if needed
    if (abs(piece) == WHITE_KING) {
        if (piece > 0) {
            whiteKingPos = savedKingPos;
        } else {
            blackKingPos = savedKingPos;
        }
    }
    
    return !inCheckAfterMove;
}
bool IsCaptureMove(int fromCol, int fromRow, int toCol, int toRow) {
    int piece = board[fromRow][fromCol];
    int target = board[toRow][toCol];
    
    // Normal capture
    if (target != EMPTY && piece * target < 0) return true;
    
    // En passant
    if (abs(piece) == WHITE_PAWN && toCol == enPassantTarget.x && toRow == enPassantTarget.y) {
        return true;
    }
    
    return false;
}

void SortMoves(ChessMove moves[], int moveCount) {
    // Simple ordering - prioritize captures
    for (int i = 0; i < moveCount; i++) {
        if (board[moves[i].toY][moves[i].toX] != EMPTY) {
            moves[i].score = 1000; // High score for captures
        } else {
            moves[i].score = 0;
        }
    }
    
    // Bubble sort for simplicity (replace with better sort if needed)
    for (int i = 0; i < moveCount-1; i++) {
        for (int j = i+1; j < moveCount; j++) {
            if (moves[i].score < moves[j].score) {
                ChessMove temp = moves[i];
                moves[i] = moves[j];
                moves[j] = temp;
            }
        }
    }
}

GamePhase DetectGamePhase() {
    int pieceCount = 0;
    int queenCount = 0;
    int minorPieceCount = 0;
    
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int piece = abs(board[y][x]);
            if (piece != EMPTY) {
                pieceCount++;
                if (piece == WHITE_QUEEN) queenCount++;
                if (piece == WHITE_KNIGHT || piece == WHITE_BISHOP) minorPieceCount++;
            }
        }
    }
    
    if (pieceCount > 24) return OPENING;
    if (queenCount > 0 || minorPieceCount > 4) return MIDGAME;
    return ENDGAME;
}

int EvaluatePosition() {
    int score = 0;
    
    // Material evaluation
    const int pieceValues[] = {0, 500, 300, 300, 900, 20000, 100};
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            int piece = board[y][x];
            if (piece != EMPTY) {
                score += pieceValues[abs(piece)] * (piece > 0 ? 1 : -1);
            }
        }
    }
    
    // Strategic components
    score += CalculateMobility(currentPlayer) * 2;
    score += CalculateKingSafety(currentPlayer);
    score += EvaluatePawnStructure(currentPlayer);
    
    // King's Indian Defense evaluation
    score += EvaluateKingIndianDefense(currentPlayer);
    
    // Positional bonuses
    GamePhase phase = DetectGamePhase();
    if (phase == OPENING) {
        // Encourage development and center control
        score += (CountCenterControl(1) - CountCenterControl(-1)) * 10;
    }
    else if (phase == MIDGAME) {
        // Encourage king safety and piece activity
        score += (CalculateKingSafety(1) - CalculateKingSafety(-1));
    }
    else { // ENDGAME
        // Encourage king centralization and pawn promotion
        score += (7 - abs(whiteKingPos.x - 3.5) - abs(whiteKingPos.y - 3.5)) * 5;
        score -= (7 - abs(blackKingPos.x - 3.5) - abs(blackKingPos.y - 3.5)) * 5;
    }
    
    return score;
}

int CalculateMobility(int player) {
    int mobility = 0;
    
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            if (board[y][x] != EMPTY && 
                ((player == 1 && board[y][x] > 0) || 
                 (player == -1 && board[y][x] < 0))) {
                
                for (int ty = 0; ty < 8; ty++) {
                    for (int tx = 0; tx < 8; tx++) {
                        if (IsValidMoveWithoutCheck(x, y, tx, ty)) {
                            mobility++;
                        }
                    }
                }
            }
        }
    }
    return mobility;
}

int CalculateKingSafety(int player) {
    int safety = 0;
    POINT kingPos = (player == 1) ? whiteKingPos : blackKingPos;
    
    // Count pawns near king
    int pawnShield = 0;
    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            int nx = kingPos.x + dx;
            int ny = kingPos.y + (player == 1 ? -dy : dy); // Direction depends on player
            
            if (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
                if (board[ny][nx] == (player == 1 ? WHITE_PAWN : BLACK_PAWN)) {
                    pawnShield++;
                }
            }
        }
    }
    safety += pawnShield * 20;
    
    // Penalize exposed king
    if (pawnShield < 3) {
        safety -= (3 - pawnShield) * 30;
    }
    
    return safety;
}

int CountBishopMoves(int x, int y) {
    int count = 0;
    int directions[4][2] = {{1,1}, {1,-1}, {-1,1}, {-1,-1}};
    
    for (int i = 0; i < 4; i++) {
        int dx = directions[i][0];
        int dy = directions[i][1];
        int nx = x + dx;
        int ny = y + dy;
        
        while (nx >= 0 && nx < 8 && ny >= 0 && ny < 8) {
            count++;
            if (board[ny][nx] != EMPTY) break;
            nx += dx;
            ny += dy;
        }
    }
    
    return count;
}

bool IsFileOpen(int x) {
    for (int y = 0; y < 8; y++) {
        if (board[y][x] == WHITE_PAWN || board[y][x] == BLACK_PAWN) {
            return false;
        }
    }
    return true;
}

int CountCenterControl(int player) {
    int centerControl = 0;
    int centerSquares[4][2] = {{3,3}, {3,4}, {4,3}, {4,4}}; // d4, d5, e4, e5
    
    for (int i = 0; i < 4; i++) {
        int x = centerSquares[i][0];
        int y = centerSquares[i][1];
        if (IsSquareUnderAttack(x, y, player)) {
            centerControl++;
        }
    }
    return centerControl;
}

// Add this function to generate capture moves
void GenerateCaptureMoves(int player, ChessMove moves[], int &moveCount) {
    moveCount = 0;
    
    for (int fromY = 0; fromY < 8; fromY++) {
        for (int fromX = 0; fromX < 8; fromX++) {
            if (board[fromY][fromX] != EMPTY && 
                ((player == 1 && board[fromY][fromX] > 0) || 
                 (player == -1 && board[fromY][fromX] < 0))) {
                
                for (int toY = 0; toY < 8; toY++) {
                    for (int toX = 0; toX < 8; toX++) {
                        if (IsValidMove(fromX, fromY, toX, toY) && 
                            IsCaptureMove(fromX, fromY, toX, toY)) {
                            moves[moveCount].fromX = fromX;
                            moves[moveCount].fromY = fromY;
                            moves[moveCount].toX = toX;
                            moves[moveCount].toY = toY;
                            moves[moveCount].promotion = 0;
                            
                            // Handle pawn promotion
                            if (abs(board[fromY][fromX]) == WHITE_PAWN && 
                                (toY == 0 || toY == 7)) {
                                moves[moveCount].promotion = (player == 1) ? WHITE_QUEEN : BLACK_QUEEN;
                            }
                            
                            moveCount++;
                        }
                    }
                }
            }
        }
    }
}

// Add this function to sort capture moves
void SortCaptures(ChessMove moves[], int moveCount) {
    // Simple ordering - prioritize captures of valuable pieces
    for (int i = 0; i < moveCount; i++) {
        int targetPiece = abs(board[moves[i].toY][moves[i].toX]);
        int attackerPiece = abs(board[moves[i].fromY][moves[i].fromX]);
        
        // Use MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
        moves[i].score = (targetPiece * 10) - attackerPiece;
    }
    
    // Bubble sort by score (high to low)
    for (int i = 0; i < moveCount-1; i++) {
        for (int j = i+1; j < moveCount; j++) {
            if (moves[i].score < moves[j].score) {
                ChessMove temp = moves[i];
                moves[i] = moves[j];
                moves[j] = temp;
            }
        }
    }
}

int QuiescenceSearch(int alpha, int beta, int player) {
    int standPat = EvaluatePosition();
    
    if (player == 1) {
        if (standPat >= beta) return beta;
        alpha = Max(alpha, standPat);
    } else {
        if (standPat <= alpha) return alpha;
        beta = Min(beta, standPat);
    }
    
    // Generate capture moves
    ChessMove captureMoves[64];
    int captureCount = 0;
    GenerateCaptureMoves(player, captureMoves, captureCount);
    
    // Sort captures by most valuable victim first
    SortCaptures(captureMoves, captureCount);
    
    for (int i = 0; i < captureCount; i++) {
        // Make the capture move
        int captured = board[captureMoves[i].toY][captureMoves[i].toX];
        int movedPiece = board[captureMoves[i].fromY][captureMoves[i].fromX];
        board[captureMoves[i].toY][captureMoves[i].toX] = movedPiece;
        board[captureMoves[i].fromY][captureMoves[i].fromX] = EMPTY;
        
        // Recursive call
        int score = -QuiescenceSearch(-beta, -alpha, -player);
        
        // Undo move
        board[captureMoves[i].fromY][captureMoves[i].fromX] = movedPiece;
        board[captureMoves[i].toY][captureMoves[i].toX] = captured;
        
        if (player == 1) {
            if (score >= beta) return beta;
            if (score > alpha) alpha = score;
        } else {
            if (score <= alpha) return alpha;
            if (score < beta) beta = score;
        }
    }
    
    return (player == 1) ? alpha : beta;
}

int Minimax(int depth, int alpha, int beta, bool maximizingPlayer) {
    if (depth == 0) {
        return QuiescenceSearch(alpha, beta, maximizingPlayer ? 1 : -1);
    }

    
    ChessMove moves[256];
    int moveCount = 0;
    GenerateLegalMoves(maximizingPlayer ? 1 : -1, moves, moveCount);
    
    if (moveCount == 0) {
        if (IsInCheck(maximizingPlayer ? 1 : -1)) {
            return maximizingPlayer ? -100000 : 100000; // Checkmate
        }
        return 0; // Stalemate
    }
    
    if (maximizingPlayer) {
        int maxEval = -1000000;
        for (int i = 0; i < moveCount; i++) {
            // Make move
            int captured = board[moves[i].toY][moves[i].toX];
            int movedPiece = board[moves[i].fromY][moves[i].fromX];
            board[moves[i].toY][moves[i].toX] = movedPiece;
            board[moves[i].fromY][moves[i].fromX] = EMPTY;
            
            // Update king position if needed
            if (abs(movedPiece) == WHITE_KING) {
                if (movedPiece > 0) {
                    whiteKingPos.x = moves[i].toX;
                    whiteKingPos.y = moves[i].toY;
                } else {
                    blackKingPos.x = moves[i].toX;
                    blackKingPos.y = moves[i].toY;
                }
            }
            
            int eval = Minimax(depth - 1, alpha, beta, false);
            
            // Undo move
            board[moves[i].fromY][moves[i].fromX] = movedPiece;
            board[moves[i].toY][moves[i].toX] = captured;
            
            if (abs(movedPiece) == WHITE_KING) {
                if (movedPiece > 0) {
                    whiteKingPos.x = moves[i].fromX;
                    whiteKingPos.y = moves[i].fromY;
                } else {
                    blackKingPos.x = moves[i].fromX;
                    blackKingPos.y = moves[i].fromY;
                }
            }
            
            maxEval = Max(maxEval, eval);
            alpha = Max(alpha, eval);
            if (beta <= alpha) break;
        }
        return maxEval;
    } else {
        int minEval = 1000000;
        for (int i = 0; i < moveCount; i++) {
            // Make move (same as above)
            int captured = board[moves[i].toY][moves[i].toX];
            int movedPiece = board[moves[i].fromY][moves[i].fromX];
            board[moves[i].toY][moves[i].toX] = movedPiece;
            board[moves[i].fromY][moves[i].fromX] = EMPTY;
            
            int eval = Minimax(depth - 1, alpha, beta, true);
            
            // Undo move (same as above)
            board[moves[i].fromY][moves[i].fromX] = movedPiece;
            board[moves[i].toY][moves[i].toX] = captured;
            
            if (abs(movedPiece) == WHITE_KING) {
                if (movedPiece > 0) {
                    whiteKingPos.x = moves[i].fromX;
                    whiteKingPos.y = moves[i].fromY;
                } else {
                    blackKingPos.x = moves[i].fromX;
                    blackKingPos.y = moves[i].fromY;
                }
            }
            
            minEval = Min(minEval, eval);
            beta = Min(beta, eval);
            if (beta <= alpha) break;
        }
        return minEval;
    }
}

int EvaluatePawnStructure(int player) {
    int score = 0;
    
    // Detect isolated pawns
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (board[y][x] == (player == 1 ? WHITE_PAWN : BLACK_PAWN)) {
                bool isolated = true;
                
                // Check adjacent files
                for (int dx = -1; dx <= 1; dx += 2) {
                    int nx = x + dx;
                    if (nx >= 0 && nx < 8) {
                        for (int ny = 0; ny < 8; ny++) {
                            if (board[ny][nx] == (player == 1 ? WHITE_PAWN : BLACK_PAWN)) {
                                isolated = false;
                                break;
                            }
                        }
                    }
                }
                
                if (isolated) score -= 20;
            }
        }
    }
    
    // Detect passed pawns
    for (int x = 0; x < 8; x++) {
        for (int y = 0; y < 8; y++) {
            if (board[y][x] == (player == 1 ? WHITE_PAWN : BLACK_PAWN)) {
                bool passed = true;
                int start = Max(0, x-1);
                int end = Min(7, x+1);
                
                for (int nx = start; nx <= end; nx++) {
                    for (int ny = (player == 1) ? y-1 : y+1; 
                         (player == 1) ? ny >= 0 : ny < 8; 
                         (player == 1) ? ny-- : ny++) {
                        
                        if (board[ny][nx] == (player == 1 ? BLACK_PAWN : WHITE_PAWN)) {
                            passed = false;
                            break;
                        }
                    }
                    if (!passed) break;
                }
                
                if (passed) {
                    int advancement = (player == 1) ? (7 - y) : y;
                    score += advancement * 15;
                }
            }
        }
    }
    
    return score;
}

void GenerateLegalMoves(int player, ChessMove moves[], int &moveCount) {
    moveCount = 0;
    
    for (int fromY = 0; fromY < 8; fromY++) {
        for (int fromX = 0; fromX < 8; fromX++) {
            if (board[fromY][fromX] != EMPTY && 
                ((player == 1 && board[fromY][fromX] > 0) || 
                 (player == -1 && board[fromY][fromX] < 0))) {
                
                for (int toY = 0; toY < 8; toY++) {
                    for (int toX = 0; toX < 8; toX++) {
                        if (IsValidMove(fromX, fromY, toX, toY)) {
                            moves[moveCount].fromX = fromX;
                            moves[moveCount].fromY = fromY;
                            moves[moveCount].toX = toX;
                            moves[moveCount].toY = toY;
                            moves[moveCount].promotion = 0;
                            
                            // Handle pawn promotion
                            if (abs(board[fromY][fromX]) == WHITE_PAWN && 
                                (toY == 0 || toY == 7)) {
                                moves[moveCount].promotion = (player == 1) ? WHITE_QUEEN : BLACK_QUEEN;
                            }
                            
                            moveCount++;
                            if (moveCount >= 256) return; // Prevent buffer overflow
                        }
                    }
                }
            }
        }
    }
}

int EvaluateKingIndianDefense(int player) {
    if (player != -1) return 0; // Only for black (KID is a black defense)
    
    int score = 0;
    bool kingsideFianchetto = false;
    bool centerControl = false;
    
    // Check for KID structure
    if (board[0][6] == BLACK_KING &&  // Black king castled kingside
        board[0][5] == BLACK_BISHOP && // Fianchettoed bishop
        board[1][5] == BLACK_PAWN &&   // g-pawn
        board[1][6] == BLACK_PAWN) {   // h-pawn
        kingsideFianchetto = true;
        score += 50;
    }
    
    // Check center control
    int centerControlCount = 0;
    int centerSquares[4][2] = {{3,3}, {3,4}, {4,3}, {4,4}};
    for (int i = 0; i < 4; i++) {
        int x = centerSquares[i][0];
        int y = centerSquares[i][1];
        if (IsSquareUnderAttack(x, y, -1)) centerControlCount++;
    }
    if (centerControlCount >= 2) {
        centerControl = true;
        score += 30;
    }
    
    // Check for KID pawn storm
    if (kingsideFianchetto && centerControl) {
        // Evaluate pawn storm potential
        int pawnStormScore = 0;
        for (int x = 5; x < 7; x++) {
            for (int y = 4; y < 6; y++) {
                if (board[y][x] == BLACK_PAWN) {
                    pawnStormScore += (y - 1) * 10; // Reward advanced pawns
                }
            }
        }
        score += pawnStormScore;
    }
    
    return score;
}


ChessMove FindBestMove(int depth) {
    ChessMove moves[256];
    int moveCount = 0;
    GenerateLegalMoves(currentPlayer, moves, moveCount);

    // King's Indian Defense specific moves
    if (currentPlayer == -1 && DetectGamePhase() == OPENING) {
        // Check if we're in a KID position
        if (board[0][6] == BLACK_KING &&  // Kingside castle
            board[0][5] == BLACK_BISHOP && // Fianchettoed bishop
            board[1][5] == BLACK_PAWN) {   // g-pawn
            
            // Typical KID moves
            ChessMove kidMoves[] = {
                {6, 0, 5, 2},  // Nf6 to g4 (if not already moved)
                {1, 1, 1, 3},   // d7 to d5 (central break)
                {3, 1, 3, 3},   // c7 to c5 (flank attack)
                {5, 1, 5, 3}    // f7 to f5 (kingside attack)
            };
            
            // Try to find a valid KID move
            for (int i = 0; i < 4; i++) {
                ChessMove move = kidMoves[i];
                if (board[move.fromY][move.fromX] == kidMoves[i].promotion && // Using promotion as piece type
                    IsValidMove(move.fromX, move.fromY, move.toX, move.toY)) {
                    return move;
                }
            }
        }
    }

    if (DetectGamePhase() == OPENING) {
        // 1. Develop knights before bishops
        ChessMove knightMoves[32];
        int knightMoveCount = 0;
        
        // 2. Castle early
        if (!whiteKingMoved && currentPlayer == 1) {
            // Try kingside castling
            if (!whiteRookKingMoved && board[7][5] == EMPTY && board[7][6] == EMPTY) {
                if (!IsSquareUnderAttack(4, 7, -1) && 
                    !IsSquareUnderAttack(5, 7, -1) && 
                    !IsSquareUnderAttack(6, 7, -1)) {
                    return {4, 7, 6, 7}; // O-O
                }
            }
            // Try queenside castling
            if (!whiteRookQueenMoved && board[7][1] == EMPTY && 
                board[7][2] == EMPTY && board[7][3] == EMPTY) {
                if (!IsSquareUnderAttack(4, 7, -1) && 
                    !IsSquareUnderAttack(3, 7, -1) && 
                    !IsSquareUnderAttack(2, 7, -1)) {
                    return {4, 7, 2, 7}; // O-O-O
                }
            }
        }
        
        // Similar for black
        if (!blackKingMoved && currentPlayer == -1) {
            // Kingside castling for black
            if (!blackRookKingMoved && board[0][5] == EMPTY && board[0][6] == EMPTY) {
                if (!IsSquareUnderAttack(4, 0, 1) && 
                    !IsSquareUnderAttack(5, 0, 1) && 
                    !IsSquareUnderAttack(6, 0, 1)) {
                    return {4, 0, 6, 0};
                }
            }
            // Queenside castling for black
            if (!blackRookQueenMoved && board[0][1] == EMPTY && 
                board[0][2] == EMPTY && board[0][3] == EMPTY) {
                if (!IsSquareUnderAttack(4, 0, 1) && 
                    !IsSquareUnderAttack(3, 0, 1) && 
                    !IsSquareUnderAttack(2, 0, 1)) {
                    return {4, 0, 2, 0};
                }
            }
        }
        
        // Collect all knight development moves
        for (int y = 0; y < 8; y++) {
            for (int x = 0; x < 8; x++) {
                if (board[y][x] == (currentPlayer == 1 ? WHITE_KNIGHT : BLACK_KNIGHT)) {
                    for (int ty = 0; ty < 8; ty++) {
                        for (int tx = 0; tx < 8; tx++) {
                            if (IsValidMove(x, y, tx, ty)) {
                                knightMoves[knightMoveCount++] = {x, y, tx, ty};
                            }
                        }
                    }
                }
            }
        }
        
        // Prefer knight moves that develop toward center
        if (knightMoveCount > 0) {
            ChessMove bestKnightMove = knightMoves[0];
            int bestScore = -10000;
            
            for (int i = 0; i < knightMoveCount; i++) {
                int centerX = abs(knightMoves[i].toX - 3.5);
                int centerY = abs(knightMoves[i].toY - 3.5);
                int score = 8 - (centerX + centerY);
                
                if (score > bestScore) {
                    bestScore = score;
                    bestKnightMove = knightMoves[i];
                }
            }
            
            return bestKnightMove;
        }
    }
    
    ChessMove bestMove;
    int bestValue = currentPlayer == 1 ? -1000000 : 1000000;
    
    for (int i = 0; i < moveCount; i++) {
        // Make move
        int captured = board[moves[i].toY][moves[i].toX];
        int movedPiece = board[moves[i].fromY][moves[i].fromX];
        board[moves[i].toY][moves[i].toX] = movedPiece;
        board[moves[i].fromY][moves[i].fromX] = EMPTY;
        
        // Update king position if needed
        if (abs(movedPiece) == WHITE_KING) {
            if (movedPiece > 0) {
                whiteKingPos.x = moves[i].toX;
                whiteKingPos.y = moves[i].toY;
            } else {
                blackKingPos.x = moves[i].toX;
                blackKingPos.y = moves[i].toY;
            }
        }
        
        int moveValue = Minimax(depth - 1, -1000000, 1000000, currentPlayer == -1);
        
        // Undo move
        board[moves[i].fromY][moves[i].fromX] = movedPiece;
        board[moves[i].toY][moves[i].toX] = captured;
        
        if (abs(movedPiece) == WHITE_KING) {
            if (movedPiece > 0) {
                whiteKingPos.x = moves[i].fromX;
                whiteKingPos.y = moves[i].fromY;
            } else {
                blackKingPos.x = moves[i].fromX;
                blackKingPos.y = moves[i].fromY;
            }
        }
        
        moves[i].score = moveValue;
        
        if ((currentPlayer == 1 && moveValue > bestValue) || 
            (currentPlayer == -1 && moveValue < bestValue)) {
            bestValue = moveValue;
            bestMove = moves[i];
        }
    }
    
    return bestMove;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {    

    // Define the window class (like a template for the chess window)
    WNDCLASS wc = {};
    wc.lpfnWndProc = WndProc;       // Handles mouse clicks, keyboard, etc.
    wc.hInstance = hInstance;       // The program instance
    wc.lpszClassName = _T("chessclass"); // Class name
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1); // Background color

    // Register the window class (so Windows knows about it)
    if (!RegisterClass(&wc)) {
        MessageBox(NULL, _T("Window Registration Failed!"), _T("Error!"), MB_ICONERROR | MB_OK);
        return 0;
    }

    // Calculate window size (8x8 squares + margins + space for buttons)
    RECT windowRect = {0, 0, 8*squareSize + boardStartX, 8*squareSize + boardStartY + 100};
    AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX, FALSE);

    HWND hwnd = CreateWindow(
        wc.lpszClassName,
        _T("Chess Game"),
        WS_OVERLAPPEDWINDOW & ~WS_THICKFRAME & ~WS_MAXIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT,
        windowRect.right - windowRect.left + 30,
        windowRect.bottom - windowRect.top + 60,
        NULL, NULL, hInstance, NULL
    );

    // Create mode selection buttons
    CreateModeButtons(hwnd);

    // Show the window
    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    // Message loop (keeps the window running)
    MSG msg;
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg); // Handles keyboard input
        DispatchMessage(&msg);  // Sends messages to WndProc
    }
    return 0;
}