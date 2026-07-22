#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <io/pad.h>
#include <sysutil/sysutil.h>
#include <tiny3d.h> // Bizim 2D geometrik neon çizim anahtarımız

// Ekran ve Grid Ölçüleri
const int EKRAN_W = 1280;
const int EKRAN_H = 720;
const int BOARD_W = 10;
const int BOARD_H = 20;

// Oyun Ekran Durumları
enum { SCREEN_LOBBY, SCREEN_GAME };
int currentScreen = SCREEN_LOBBY;
int selectedButtonIndex = 0;

// Tetris Matrisleri
int board[BOARD_H][BOARD_W] = {0};
unsigned int colorBoard[BOARD_H][BOARD_W] = {0};

// Kusursuz Parlak Neon Renkler (RGBA formatında)
const unsigned int NEON_COLORS[7] = {
    0x00FFFFFF, // Turkuaz
    0x00FF00FF, // Yeşil
    0xFF00FFFF, // Magenta
    0xFFFF00FF, // Sarı
    0xFF3333FF, // Kırmızı
    0x3333FFFF, // Mavi
    0xFF9900FF  // Turuncu
};

// 7 Temel Tetris Şekli (Maksimum 4x4 matris)
const int SHAPES[7][4][4] = {
    {{1,1,1,1}, {0,0,0,0}, {0,0,0,0}, {0,0,0,0}}, // I
    {{1,0,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, // J
    {{0,0,1,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, // L
    {{1,1,0,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, // O
    {{0,1,1,0}, {1,1,0,0}, {0,0,0,0}, {0,0,0,0}}, // S
    {{0,1,0,0}, {1,1,1,0}, {0,0,0,0}, {0,0,0,0}}, // T
    {{1,1,0,0}, {0,1,1,0}, {0,0,0,0}, {0,0,0,0}}  // Z
};

int currentPiece[4][4];
int currentPieceIndex;
int pieceX = 3, pieceY = 0;
int pieceW = 4, pieceH = 4;

int score = 0;
int isGameOver = 0;
int dropCounter = 0;
int gameSpeed = 30; // Döngü başına düşme hızı eşiği

// Rastgele yeni blok üretme
void spawnPiece() {
    currentPieceIndex = rand() % 7;
    // Şekli kopyala
    for(int r=0; r<4; r++) {
        for(int c=0; c<4; c++) {
            currentPiece[r][c] = SHAPES[currentPieceIndex][r][c];
        }
    }
    pieceX = BOARD_W / 2 - 2;
    pieceY = 0;
}

// Çarpışma Testi
int checkCollision(int nextX, int nextY, int tempPiece[4][4]) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (tempPiece[r][c] != 0) {
                int bX = nextX + c;
                int bY = nextY + r;
                if (bX < 0 || bX >= BOARD_W || bY >= BOARD_H) return 1;
                if (bY >= 0 && board[bY][bX] != 0) return 1;
            }
        }
    }
    return 0;
}

// X Tuşu ile Sağa Döndürme Algoritması
void rotatePiece() {
    int rotated[4][4] = {0};
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            rotated[c][3 - r] = currentPiece[r][c];
        }
    }
    // Döndürme sonrası taşmayı önlemek için otomatik duvardan itme (Wall Kick basitleştirilmiş)
    if (!checkCollision(pieceX, pieceY, rotated)) {
        memcpy(currentPiece, rotated, sizeof(currentPiece));
    }
}

// Bloğu tahtaya sabitleme
void mergePiece() {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (currentPiece[r][c] != 0 && pieceY + r >= 0) {
                board[pieceY + r][pieceX + c] = 1;
                colorBoard[pieceY + r][pieceX + c] = NEON_COLORS[currentPieceIndex];
            }
        }
    }
    // Satır temizleme kontrolü
    for (int r = BOARD_H - 1; r >= 0; r--) {
        int isFull = 1;
        for (int c = 0; c < BOARD_W; c++) {
            if (board[r][c] == 0) { isFull = 0; break; }
        }
        if (isFull) {
            score += 100;
            for (int row = r; row > 0; row--) {
                for(int col=0; col<BOARD_W; col++) {
                    board[row][col] = board[row-1][col];
                    colorBoard[row][col] = colorBoard[row-1][col];
                }
            }
            r++; // Aynı satırı tekrar kontrol et
        }
    }
}

void moveDown() {
    if (!checkCollision(pieceX, pieceY + 1, currentPiece)) {
        pieceY++;
    } else {
        mergePiece();
        spawnPiece();
        if (checkCollision(pieceX, pieceY, currentPiece)) {
            isGameOver = 1;
        }
    }
}

// --- GEOMETRİK NEON ÇİZİM FONKSİYONLARI ---
void drawNeonBlock(float x, float y, float size, unsigned int color) {
    // 1. İç dolgu (Bir tık koyu renk derinlik hissi için)
    tiny3d_Rectangle2D(x + 2, y + 2, x + size - 2, y + size - 2, (color & 0xFFFFFF00) | 0x66, LINK_COLOR);
    // 2. Neon Dış Çizgi Parıltısı (Kalın ve tam opak)
    tiny3d_Rectangle2D(x, y, x + size, y + 2, color, LINK_COLOR);
    tiny3d_Rectangle2D(x, y + size - 2, x + size, y + size, color, LINK_COLOR);
    tiny3d_Rectangle2D(x, y, x + 2, y + size, color, LINK_COLOR);
    tiny3d_Rectangle2D(x + size - 2, y, x + size, y + size, color, LINK_COLOR);
}

int main(int argc, char* argv[]) {
    ioPadInit(7);
    tiny3d_Init(1024*1024);
    spawnPiece();

    while (1) {
        // --- 1. KONTROLLER (DUALSHOCK 3) ---
        padInfo padinfo;
        padData paddata;
        ioPadGetInfo(&padinfo);
        
        if (padinfo.status[0] == PAD_STATUS_CONNECTED) {
            ioPadGetData(0, &paddata);
            
            if (currentScreen == SCREEN_LOBBY) {
                if (paddata.BTN_DOWN || paddata.BTN_UP) {
                    selectedButtonIndex = (selectedButtonIndex == 0) ? 1 : 0;
                }
                if (paddata.BTN_CROSS) { // X tuşuna basınca başla
                    if (selectedButtonIndex == 0) currentScreen = SCREEN_GAME;
                }
            } else {
                // Oyun Ekranı Kontrolleri
                if (paddata.BTN_LEFT) {
                    if (!checkCollision(pieceX - 1, pieceY, currentPiece)) pieceX--;
                }
                if (paddata.BTN_RIGHT) {
                    if (!checkCollision(pieceX + 1, pieceY, currentPiece)) pieceX++;
                }
                if (paddata.BTN_DOWN) {
                    moveDown();
                }
                if (paddata.BTN_CROSS) { // X tuşuyla döndürme istedin!
                    rotatePiece();
                }
            }
        }

        // --- 2. OYUN ZAMANLAMA MOTORU ---
        if (currentScreen == SCREEN_GAME && !isGameOver) {
            dropCounter++;
            if (dropCounter >= gameSpeed) {
                moveDown();
                dropCounter = 0;
            }
        }

        // --- 3. 100x COOL NEON RENDER ALANI ---
        tiny3d_Clear(0x0B0E14FF, TINY3D_CLEAR_ALL); // Koyu arka plan pong teması
        tiny3d_Project2D();

        if (currentScreen == SCREEN_LOBBY) {
            // Lobi Çizimi (Başlıklar ve Neon Butonlar)
            // Arka plana ince sönük çizgiler
            for(int i=0; i<EKRAN_W; i+=60) tiny3d_Rectangle2D(i, 0, i+1, EKRAN_H, 0x121824FF, LINK_COLOR);
            
            // "KITTY TETRIS" büyük panel mantığı
            tiny3d_Rectangle2D(EKRAN_W/2 - 250, 200, EKRAN_W/2 + 250, 205, 0xFF0066FF, LINK_COLOR); // Pembe neon çizgi
            
            // Buton 1: BAŞLA
            unsigned int b1Color = (selectedButtonIndex == 0) ? 0x00F0FFFF : 0x15222EFF;
            tiny3d_Rectangle2D(EKRAN_W/2 - 150, 400, EKRAN_W/2 + 150, 460, b1Color, LINK_COLOR);
        } 
        else {
            // Oyun Ekranı Çizimi
            float blockSize = 30.0f;
            float boardStartX = 450.0f;
            float boardStartY = 60.0f;

            // Neon Oyun Çerçevesi (Pembe Parlama)
            tiny3d_Rectangle2D(boardStartX - 4, boardStartY - 4, boardStartX + (BOARD_W * blockSize) + 4, boardStartY, 0xFF0066FF, LINK_COLOR);
            tiny3d_Rectangle2D(boardStartX - 4, boardStartY + (BOARD_H * blockSize), boardStartX + (BOARD_W * blockSize) + 4, boardStartY + (BOARD_H * blockSize) + 4, 0xFF0066FF, LINK_COLOR);
            tiny3d_Rectangle2D(boardStartX - 4, boardStartY, boardStartX, boardStartY + (BOARD_H * blockSize), 0xFF0066FF, LINK_COLOR);
            tiny3d_Rectangle2D(boardStartX + (BOARD_W * blockSize), boardStartY, boardStartX + (BOARD_W * blockSize) + 4, boardStartY + (BOARD_H * blockSize), 0xFF0066FF, LINK_COLOR);

            // Sabitlenmiş Blokları Çiz
            for (int r = 0; r < BOARD_H; r++) {
                for (int c = 0; c < BOARD_W; c++) {
                    if (board[r][c] != 0) {
                        drawNeonBlock(boardStartX + c * blockSize, boardStartY + r * blockSize, blockSize, colorBoard[r][c]);
                    }
                }
            }

            // Aktif Düşen Bloğu Çiz
            if (!isGameOver) {
                for (int r = 0; r < 4; r++) {
                    for (int c = 0; c < 4; c++) {
                        if (currentPiece[r][c] != 0 && (pieceY + r >= 0)) {
                            drawNeonBlock(boardStartX + (pieceX + c) * blockSize, boardStartY + (pieceY + r) * blockSize, blockSize, NEON_COLORS[currentPieceIndex]);
                        }
                    }
                }
            }
        }

        tiny3d_Flip();
        // PS3 işlemcisini yormamak için mikro gecikme
        sysUtilCheckCallback();
    }

    ioPadEnd();
    return 0;
}
