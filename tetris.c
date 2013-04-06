#include <windows.h>
#include "tetris.h"

/* 乱数発生関数 */
int random(int max) {
    return (int)(rand() / (RAND_MAX + 1.0) * max);
}

/* ブロックを置いてみる処理 */
BOOL putBlock(STATUS s, BOOL action) {
    int i,j;

    if(board[s.x][s.y] != 0) {
        return FALSE;
    }


    if(action) {
        board[s.x][s.y] = s.type;
    }


    for(i = 0; i < 3; i++) {
        int dx = block[s.type].p[i].x;
        int dy = block[s.type].p[i].y;
        int r = s.rotate % block[s.type].rotate;
        for(j = 0; j < r; j++) {
            int nx = dx, ny = dy;
            dx = ny; dy = -nx;
        }
        if(board[s.x + dx][s.y + dy] != 0) {
            return FALSE;
        }
        if(action) {
            board[s.x + dx][s.y + dy] = s.type;
        }
    }
    if(!action) {
        putBlock(s, TRUE);
    }
    return TRUE;
}

/* カレントブロックを一度消す */
BOOL deleteBlock(STATUS s) {
    int i,j;
    board[s.x][s.y] = 0;

    for(i = 0; i < 3; i++) {
        int dx = block[s.type].p[i].x;
        int dy = block[s.type].p[i].y;
        int r = s.rotate % block[s.type].rotate;
        for(j = 0; j < r; j++) {
            int nx = dx, ny = dy;
            dx = ny; dy = -nx;
        }
        board[s.x + dx][s.y + dy] = 0;
    }

    return TRUE;
}

/* ユーザからのキー入力を取り扱う */
BOOL processInput() {
    BOOL ret = FALSE;
    STATUS n = current;
    if(GetAsyncKeyState(VK_LEFT)) {
        n.x--;
    } else if(GetAsyncKeyState(VK_RIGHT)) {
        n.x++;
    } else if(GetAsyncKeyState(VK_UP)) {
        n.y++;
    } else if(GetAsyncKeyState(VK_SPACE)) {
        n.rotate++;
    } else if(GetAsyncKeyState(VK_DOWN)) {
        n.y--;
        ret = TRUE;
    }

    if(n.x != current.x || n.y != current.y || n.rotate != current.rotate) {
        deleteBlock(current);
        if(putBlock(n, FALSE)) {
            current = n;
        } else {
            putBlock(current, FALSE);
        }
    }
    
    return ret;
}

/* ゲームオーバーになったときに全ブロックを赤く塗る */
void gameOver() {
    int x,y;
    KillTimer(hMainWindow, 100);
    for(x = 1; x <= 10;x++) {
        for(y = 1; y <= 20; y++) {
            if(board[x][y] != 0) {
                board[x][y] = 1;
            }
        }
    }
    InvalidateRect(hMainWindow, NULL, FALSE);
}

/* 段がそろった時に削除する処理 */
void deleteLine() {
    int y,x,i,j;
    for(y = 1; y < 23; y++) {
        BOOL flag = TRUE;
        for(x = 1;x <= 10; x++) {
            if(board[x][y] == 0) {
                flag = FALSE;
            }
        }
        
        if(flag) {
            for(j = y; j < 23; j++) {
                for(i = 1; i <= 10; i++) {
                    board[i][j] = board[i][j + 1];
                }
            }
            y--;
        }
    }
}

/* カレントブロックの自然落下 */
void blockDown() {
    deleteBlock(current);
    current.y--;
    if(!putBlock(current, FALSE)) {
        current.y++;
        putBlock(current, FALSE);
        
        deleteLine();
        
        current.x = 5;
        current.y = 21;
        current.type = random(7) + 1;
        current.rotate = random(4);
        if(!putBlock(current, FALSE)) {
            gameOver();
        }
    }
}

/* 盤面データを描画する */
void showBoard() {
    int x,y;
    for(x = 1; x <= 10; x++) {
        for(y = 1; y <= 20; y++) {
            BitBlt(hMemDC, (x - 1) * 24, (20 -y) * 24, 24, 24, hBlockDC, 0, board[x][y] * 24, SRCCOPY);
        }
    }
}

/**
 * イベント発生時に呼ばれるコールバック関数
 */
LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    switch(msg) {
        case WM_CREATE: {
            int x,y;
            for(x = 0; x < 12; x++) {
                for(y = 0; y < 25; y++) {
                    if(x == 0 || x == 11 || y == 0) {
                        board[x][y] = 1;
                    } else {
                        board[x][y] = 0;
                    }
                }
            }
            
            current.x = 5;
            current.y = 21;
            current.type = random(7) + 1;
            current.rotate = random(4);
            putBlock(current, FALSE);

            HDC hdc = GetDC(hWnd);
            
            hMemDC = CreateCompatibleDC(hdc);
            HBITMAP hBitmap = CreateCompatibleBitmap(hdc, 24 * 10, 24 * 20);
            hMemPrev = (HBITMAP)SelectObject(hMemDC, hBitmap);
            
            hBlockDC = CreateCompatibleDC(hdc);
            hBitmap = LoadBitmap(hInstance, "BLOCKS");
            hBlockPrev = (HBITMAP)SelectObject(hBlockDC, hBitmap);
            
            // debug
            BitBlt(hMemDC, 0, 0, 24, 24, hBlockDC, 0, 0, SRCCOPY);
            
            ReleaseDC(hWnd, hdc);
            break;
        }
        case WM_TIMER: {
            static int w = 0;
            if(w % 2 == 0) {
                if(processInput()) {
                    w = 0;
                }
            }
            if(w % 5 == 0) {
                blockDown();
            }
            w++;
            
            InvalidateRect(hWnd, NULL, FALSE);
            break;
        }

        case WM_PAINT: {
            showBoard();
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hWnd, &ps);
            BitBlt(hdc, 0, 0, 24 * 10, 24 * 20, hMemDC, 0, 0, SRCCOPY);
            EndPaint(hWnd, &ps);
            break;
        }
        case WM_DESTROY: {
            HBITMAP hBitmap = (HBITMAP)SelectObject(hMemDC, hMemPrev);
            DeleteObject(hBitmap);
            DeleteObject(hMemDC);
            
            hBitmap = (HBITMAP)SelectObject(hBlockDC, hBlockPrev);
            DeleteObject(hBitmap);
            DeleteObject(hBlockDC);
            
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hWnd, msg, wParam, lParam);
    }
    return 0;
}

/**
 * アプリケーションの起動処理
 * 中身はほとんどおまじない。
 */
int WINAPI WinMain(HINSTANCE hInst, HINSTANCE hPrevInst, LPSTR cmdLine, int cmdShow) {
    hInstance = hInst;
    WNDCLASSEX wc;
    static LPCTSTR pClassName = "NicoNicoProgramming2";  // クラス名

    wc.cbSize        = sizeof(WNDCLASSEX);               // 構造体サイズ
    wc.style         = CS_HREDRAW | CS_VREDRAW;          // クラススタイル
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;                                // 補足メモリブロック
    wc.cbWndExtra    = 0;                                // 　のサイズ
    wc.hInstance     = hInst;                            // インスタンス
    wc.hIcon         = NULL;                             // アイコン
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);       // カーソル
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);         // 背景色
    wc.lpszMenuName  = NULL;                             // メニュー
    wc.lpszClassName = pClassName;                       // クラス名
    wc.hIconSm       = NULL;                             // 小さいアイコン

    if (!RegisterClassEx(&wc)) return FALSE;             // 登録

    RECT r;
    r.left = r.top = 0;
    r.right = 24 * 10;
    r.bottom = 24 * 20;
    AdjustWindowRectEx(&r, WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION, FALSE, 0);

    hMainWindow = CreateWindow(pClassName, "Nico Nico Programming2", WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU | WS_CAPTION,
        CW_USEDEFAULT, CW_USEDEFAULT, r.right - r.left, r.bottom - r.top,
        NULL, NULL, hInst, NULL);
    
    ShowWindow(hMainWindow, SW_SHOW);

    SetTimer(hMainWindow, 100, 1000 /30, NULL);
    MSG msg;
    while(GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
    
    KillTimer(hMainWindow, 100);

    return 0;
}

