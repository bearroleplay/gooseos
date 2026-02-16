#include "chess.h"
#include "vga.h"
#include "keyboard.h"
#include "libc.h"
#include "terminal.h"
#include "libc.h"

#define ABS(x) ((x) < 0 ? -(x) : (x))
#define CHESS_BOARD_SIZE 8

// ========== СТРУКТУРЫ И ПЕРЕМЕННЫЕ ==========

typedef enum {
    PIECE_NONE = 0,
    PIECE_PAWN,
    PIECE_ROOK,
    PIECE_KNIGHT,
    PIECE_BISHOP,
    PIECE_QUEEN,
    PIECE_KING
} PieceType;

typedef enum {
    COLOR_NONE = 0,
    COLOR_WHITE,
    COLOR_BLACK
} PieceColor;

typedef struct {
    PieceType type;
    PieceColor color;
    int moved;
} ChessPiece;

typedef struct {
    ChessPiece board[CHESS_BOARD_SIZE][CHESS_BOARD_SIZE];
    PieceColor current_player;
    int selected_x, selected_y;
    int cursor_x, cursor_y;
    int game_over;
    char message[64];
} ChessGame;

static ChessGame current_game;
static int need_redraw = 1;
static int redraw_board = 1;
static char last_board[8][8];

// Символы фигур (Unicode)
static const char* piece_chars[2][7] = {
    {" ", "♙", "♖", "♘", "♗", "♕", "♔"},
    {" ", "♟", "♜", "♞", "♝", "♛", "♚"}
};

// ========== ВСПОМОГАТЕЛЬНЫЕ ФУНКЦИИ ==========

static void chess_init_game(void) {
    memset(&current_game, 0, sizeof(ChessGame));
    memset(last_board, 0, sizeof(last_board));
    
    // Чёрные фигуры (верх)
    current_game.board[0][0] = (ChessPiece){PIECE_ROOK, COLOR_BLACK, 0};
    current_game.board[0][1] = (ChessPiece){PIECE_KNIGHT, COLOR_BLACK, 0};
    current_game.board[0][2] = (ChessPiece){PIECE_BISHOP, COLOR_BLACK, 0};
    current_game.board[0][3] = (ChessPiece){PIECE_QUEEN, COLOR_BLACK, 0};
    current_game.board[0][4] = (ChessPiece){PIECE_KING, COLOR_BLACK, 0};
    current_game.board[0][5] = (ChessPiece){PIECE_BISHOP, COLOR_BLACK, 0};
    current_game.board[0][6] = (ChessPiece){PIECE_KNIGHT, COLOR_BLACK, 0};
    current_game.board[0][7] = (ChessPiece){PIECE_ROOK, COLOR_BLACK, 0};
    
    // Чёрные пешки
    for(int i = 0; i < 8; i++) {
        current_game.board[1][i] = (ChessPiece){PIECE_PAWN, COLOR_BLACK, 0};
    }
    
    // Белые фигуры (низ)
    current_game.board[7][0] = (ChessPiece){PIECE_ROOK, COLOR_WHITE, 0};
    current_game.board[7][1] = (ChessPiece){PIECE_KNIGHT, COLOR_WHITE, 0};
    current_game.board[7][2] = (ChessPiece){PIECE_BISHOP, COLOR_WHITE, 0};
    current_game.board[7][3] = (ChessPiece){PIECE_QUEEN, COLOR_WHITE, 0};
    current_game.board[7][4] = (ChessPiece){PIECE_KING, COLOR_WHITE, 0};
    current_game.board[7][5] = (ChessPiece){PIECE_BISHOP, COLOR_WHITE, 0};
    current_game.board[7][6] = (ChessPiece){PIECE_KNIGHT, COLOR_WHITE, 0};
    current_game.board[7][7] = (ChessPiece){PIECE_ROOK, COLOR_WHITE, 0};
    
    // Белые пешки
    for(int i = 0; i < 8; i++) {
        current_game.board[6][i] = (ChessPiece){PIECE_PAWN, COLOR_WHITE, 0};
    }
    
    current_game.current_player = COLOR_WHITE;
    current_game.cursor_x = 4;
    current_game.cursor_y = 7;
    current_game.selected_x = -1;
    current_game.selected_y = -1;
    
    strcpy(current_game.message, "Белые начинают. WASD-ход, Enter-выбор, Space-отмена");
    
    need_redraw = 1;
    redraw_board = 1;
}

// ========== ОТРИСОВКА ==========

static void chess_draw_board_once(void) {
    if (!redraw_board) return;
    
    // Заголовок
    terminal_print_at("=== GOOSE CHESS v0.1 ===", 25, 0, VGA_COLOR_YELLOW);
    terminal_print_at("ESC-Выход F1-Помощь F2-Новая игра", 20, 1, VGA_COLOR_LIGHT_GRAY);
    
    // Шахматная доска
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            int screen_x = 10 + col * 7;
            int screen_y = 4 + row * 2;
            
            uint8_t cell_color = ((row + col) % 2) ? 
                vga_make_color(VGA_COLOR_BLACK, VGA_COLOR_BROWN) : 
                vga_make_color(VGA_COLOR_BLACK, VGA_COLOR_YELLOW);
            
            for(int dx = 0; dx < 5; dx++) {
                vga_putchar(' ', cell_color, screen_x + dx, screen_y);
                vga_putchar(' ', cell_color, screen_x + dx, screen_y + 1);
            }
            
            if(col == 0) {
                char rank = '8' - row;
                vga_putchar(rank, VGA_COLOR_WHITE, screen_x - 2, screen_y + 1);
            }
            
            if(row == 7) {
                char file = 'a' + col;
                vga_putchar(file, VGA_COLOR_WHITE, screen_x + 2, screen_y + 3);
            }
        }
    }
    
    // Легенда
    terminal_print_at("♔♕♖♗♘♙ - Белые | ♚♛♜♝♞♟ - Чёрные", 20, 21, VGA_COLOR_CYAN);
    
    // Панель справа
    terminal_print_at("Сообщение:", 55, 5, VGA_COLOR_YELLOW);
    terminal_print_at("Текущий игрок:", 55, 8, VGA_COLOR_YELLOW);
    terminal_print_at("Счёт:", 55, 11, VGA_COLOR_YELLOW);
    terminal_print_at("Белые: 0  Чёрные: 0", 55, 12, VGA_COLOR_LIGHT_GRAY);
    
    redraw_board = 0;
}

static void chess_update_pieces(void) {
    for(int row = 0; row < 8; row++) {
        for(int col = 0; col < 8; col++) {
            ChessPiece piece = current_game.board[row][col];
            char current_char = piece.type ? piece_chars[piece.color-1][piece.type][0] : ' ';
            
            if (need_redraw || last_board[row][col] != current_char) {
                int screen_x = 12 + col * 7;
                int screen_y = 4 + row * 2;
                
                uint8_t cell_color = ((row + col) % 2) ? 
                    vga_make_color(VGA_COLOR_BLACK, VGA_COLOR_BROWN) : 
                    vga_make_color(VGA_COLOR_BLACK, VGA_COLOR_YELLOW);
                
                vga_putchar(' ', cell_color, screen_x, screen_y);
                
                if (piece.type != PIECE_NONE) {
                    const char* symbol = piece_chars[piece.color-1][piece.type];
                    uint8_t piece_color = (piece.color == COLOR_WHITE) ? 
                        VGA_COLOR_WHITE : VGA_COLOR_LIGHT_GRAY;
                    
                    terminal_print_at(symbol, screen_x, screen_y, piece_color);
                }
                
                last_board[row][col] = current_char;
            }
        }
    }
}

static void chess_update_cursor(void) {
    static int last_cursor_x = -1, last_cursor_y = -1;
    static int last_selected_x = -1, last_selected_y = -1;
    
    // Убираем старый курсор
    if (last_cursor_x != -1 && last_cursor_y != -1) {
        int old_x = 12 + last_cursor_x * 7;
        int old_y = 4 + last_cursor_y * 2;
        
        ChessPiece piece = current_game.board[last_cursor_y][last_cursor_x];
        if (piece.type != PIECE_NONE) {
            const char* symbol = piece_chars[piece.color-1][piece.type];
            uint8_t piece_color = (piece.color == COLOR_WHITE) ? 
                VGA_COLOR_WHITE : VGA_COLOR_LIGHT_GRAY;
            
            terminal_print_at(symbol, old_x, old_y, piece_color);
        } else {
            uint8_t cell_color = ((last_cursor_y + last_cursor_x) % 2) ? 
                vga_make_color(VGA_COLOR_BLACK, VGA_COLOR_BROWN) : 
                vga_make_color(VGA_COLOR_BLACK, VGA_COLOR_YELLOW);
            vga_putchar(' ', cell_color, old_x, old_y);
        }
    }
    
    // Убираем старое выделение
    if (last_selected_x != -1 && last_selected_y != -1) {
        int sel_x = 12 + last_selected_x * 7;
        int sel_y = 4 + last_selected_y * 2;
        
        ChessPiece piece = current_game.board[last_selected_y][last_selected_x];
        if (piece.type != PIECE_NONE) {
            const char* symbol = piece_chars[piece.color-1][piece.type];
            uint8_t piece_color = (piece.color == COLOR_WHITE) ? 
                VGA_COLOR_WHITE : VGA_COLOR_LIGHT_GRAY;
            
            terminal_print_at(symbol, sel_x, sel_y, piece_color);
        }
    }
    
    // Рисуем новый курсор
    int cursor_screen_x = 12 + current_game.cursor_x * 7;
    int cursor_screen_y = 4 + current_game.cursor_y * 2;
    
    uint8_t cursor_color = vga_make_color(VGA_COLOR_RED, VGA_COLOR_BLACK);
    if (current_game.selected_x >= 0) {
        cursor_color = vga_make_color(VGA_COLOR_GREEN, VGA_COLOR_BLACK);
    }
    
    vga_putchar('[', cursor_color, cursor_screen_x - 1, cursor_screen_y);
    vga_putchar(']', cursor_color, cursor_screen_x + 1, cursor_screen_y);
    
    // Рисуем выделение выбранной фигуры
    if (current_game.selected_x >= 0) {
        int selected_x = 12 + current_game.selected_x * 7;
        int selected_y = 4 + current_game.selected_y * 2;
        
        for(int dx = -1; dx <= 1; dx++) {
            if (dx == 0) continue;
            vga_putchar('*', VGA_COLOR_LIGHT_GREEN, selected_x + dx, selected_y - 1);
            vga_putchar('*', VGA_COLOR_LIGHT_GREEN, selected_x + dx, selected_y + 1);
        }
    }
    
    // Запоминаем текущие позиции
    last_cursor_x = current_game.cursor_x;
    last_cursor_y = current_game.cursor_y;
    last_selected_x = current_game.selected_x;
    last_selected_y = current_game.selected_y;
}

static void chess_update_status(void) {
    static char last_message[64] = "";
    static PieceColor last_player = COLOR_NONE;
    
    if (need_redraw || strcmp(last_message, current_game.message) != 0) {
        for (int x = 55; x < 80; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, 6);
        }
        terminal_print_at(current_game.message, 55, 6, VGA_COLOR_WHITE);
        strcpy(last_message, current_game.message);
    }
    
    if (need_redraw || last_player != current_game.current_player) {
        for (int x = 55; x < 65; x++) {
            vga_putchar(' ', VGA_COLOR_BLACK, x, 9);
        }
        
        if (current_game.current_player == COLOR_WHITE) {
            terminal_print_at("БЕЛЫЕ ♔", 55, 9, VGA_COLOR_WHITE);
        } else {
            terminal_print_at("ЧЁРНЫЕ ♚", 55, 9, VGA_COLOR_LIGHT_GRAY);
        }
        last_player = current_game.current_player;
    }
}

// ========== ЛОГИКА ИГРЫ ==========

static int chess_is_valid_move(int from_x, int from_y, int to_x, int to_y) {
    ChessPiece piece = current_game.board[from_y][from_x];
    ChessPiece target = current_game.board[to_y][to_x];
    
    if (piece.type == PIECE_NONE) return 0;
    if (piece.color != current_game.current_player) return 0;
    if (target.color == piece.color) return 0;
    
    switch(piece.type) {
        case PIECE_PAWN: {
            int direction = (piece.color == COLOR_WHITE) ? -1 : 1;
            if (from_x == to_x) {
                if (to_y == from_y + direction) {
                    return target.type == PIECE_NONE;
                }
                if (!piece.moved && to_y == from_y + 2*direction) {
                    return target.type == PIECE_NONE && 
                           current_game.board[from_y + direction][from_x].type == PIECE_NONE;
                }
            } else if (ABS(to_x - from_x) == 1 && to_y == from_y + direction) {
                return target.type != PIECE_NONE && target.color != piece.color;
            }
            return 0;
        }
        case PIECE_ROOK:
            return (from_x == to_x || from_y == to_y);
        case PIECE_KNIGHT: {
            int dx = ABS(to_x - from_x);
            int dy = ABS(to_y - from_y);
            return (dx == 1 && dy == 2) || (dx == 2 && dy == 1);
        }
        case PIECE_BISHOP:
            return ABS(to_x - from_x) == ABS(to_y - from_y);
        case PIECE_QUEEN:
            return (from_x == to_x || from_y == to_y) || 
                   (ABS(to_x - from_x) == ABS(to_y - from_y));
        case PIECE_KING:
            return ABS(to_x - from_x) <= 1 && ABS(to_y - from_y) <= 1;
        default:
            return 0;
    }
}

static int chess_move_piece(int from_x, int from_y, int to_x, int to_y) {
    if (!chess_is_valid_move(from_x, from_y, to_x, to_y)) {
        strcpy(current_game.message, "Недопустимый ход!");
        return 0;
    }
    
    ChessPiece piece = current_game.board[from_y][from_x];
    
    last_board[from_y][from_x] = 0;
    last_board[to_y][to_x] = 0;
    
    current_game.board[to_y][to_x] = piece;
    current_game.board[from_y][from_x] = (ChessPiece){PIECE_NONE, COLOR_NONE, 0};
    current_game.board[to_y][to_x].moved = 1;
    
    if (current_game.board[to_y][to_x].type == PIECE_KING) {
        current_game.game_over = 1;
        if (piece.color == COLOR_WHITE) {
            strcpy(current_game.message, "МАТ! Белые выиграли!");
        } else {
            strcpy(current_game.message, "МАТ! Чёрные выиграли!");
        }
    } else {
        current_game.current_player = (current_game.current_player == COLOR_WHITE) ? 
                                      COLOR_BLACK : COLOR_WHITE;
        strcpy(current_game.message, "Ход сделан. ");
        strcat(current_game.message, (current_game.current_player == COLOR_WHITE) ? 
               "Ход белых" : "Ход чёрных");
    }
    
    return 1;
}

static void chess_handle_input(char key) {
    if (current_game.game_over) {
        if (key == 'n' || key == 'N' || key == 0x3D) {
            chess_init_game();
            need_redraw = 1;
        }
        return;
    }
    
    switch(key) {
        case 'w': case 'W':
            if (current_game.cursor_y > 0) current_game.cursor_y--;
            break;
        case 's': case 'S':
            if (current_game.cursor_y < 7) current_game.cursor_y++;
            break;
        case 'a': case 'A':
            if (current_game.cursor_x > 0) current_game.cursor_x--;
            break;
        case 'd': case 'D':
            if (current_game.cursor_x < 7) current_game.cursor_x++;
            break;
            
        case '\n':  // Enter
            if (current_game.selected_x == -1) {
                ChessPiece piece = current_game.board[current_game.cursor_y][current_game.cursor_x];
                if (piece.type != PIECE_NONE && piece.color == current_game.current_player) {
                    current_game.selected_x = current_game.cursor_x;
                    current_game.selected_y = current_game.cursor_y;
                    strcpy(current_game.message, "Фигура выбрана. Выберите клетку для хода.");
                } else {
                    strcpy(current_game.message, "Здесь нет вашей фигуры!");
                }
            } else {
                if (chess_move_piece(current_game.selected_x, current_game.selected_y, 
                                     current_game.cursor_x, current_game.cursor_y)) {
                    current_game.selected_x = -1;
                    current_game.selected_y = -1;
                }
            }
            break;
            
        case ' ':  // Space
            current_game.selected_x = -1;
            current_game.selected_y = -1;
            strcpy(current_game.message, "Выбор отменен.");
            break;
            
        case 0x3B:  // F1 - помощь
            strcpy(current_game.message, "WASD-курсор, Enter-выбор, Space-отмена, ESC-выход");
            break;
            
        case 0x3D:  // F2 - новая игра
            chess_init_game();
            need_redraw = 1;
            break;
    }
}

// ========== ГЛАВНАЯ ФУНКЦИЯ ==========

void chess_play(void) {
    chess_init_game();
    
    vga_clear();
    chess_draw_board_once();
    chess_update_pieces();
    chess_update_cursor();
    chess_update_status();
    need_redraw = 0;
    
    while(1) {
        char key = keyboard_getch();
        if (key == 27) break;  // ESC
        
        if (key) {
            chess_handle_input(key);
        }
        
        if (need_redraw) {
            vga_clear();
            chess_draw_board_once();
        }
        
        chess_update_pieces();
        chess_update_cursor();
        chess_update_status();
        
        for (volatile int i = 0; i < 5000; i++);
    }
    
    terminal_clear_with_banner();

}
