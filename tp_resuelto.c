// Uso:  ./tetris           corre los tests y despues el juego
//       ./tetris --test    corre solo los tests (valgrind --leak-check=full ./tetris --test)

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

// ========= CONSTANTES DEL JUEGO =========
#define SCREEN_WIDTH 500
#define SCREEN_HEIGHT 800
#define GRID_ROWS 20
#define GRID_COLS 10
#define CELL_SIZE 35
#define OFFSET_X 75
#define OFFSET_Y 50
#define TICK_RATE 30

const int TETROMINOS[7][4][4][4] = {
    { {{0,0,0,0},{1,1,1,1},{0,0,0,0},{0,0,0,0}}, {{0,0,1,0},{0,0,1,0},{0,0,1,0},{0,0,1,0}}, {{0,0,0,0},{0,0,0,0},{1,1,1,1},{0,0,0,0}}, {{0,1,0,0},{0,1,0,0},{0,1,0,0},{0,1,0,0}} },
    { {{2,0,0,0},{2,2,2,0},{0,0,0,0},{0,0,0,0}}, {{0,2,2,0},{0,2,0,0},{0,2,0,0},{0,0,0,0}}, {{0,0,0,0},{2,2,2,0},{0,0,2,0},{0,0,0,0}}, {{0,2,0,0},{0,2,0,0},{2,2,0,0},{0,0,0,0}} },
    { {{0,0,3,0},{3,3,3,0},{0,0,0,0},{0,0,0,0}}, {{0,3,0,0},{0,3,0,0},{0,3,3,0},{0,0,0,0}}, {{0,0,0,0},{3,3,3,0},{3,0,0,0},{0,0,0,0}}, {{3,3,0,0},{0,3,0,0},{0,3,0,0},{0,0,0,0}} },
    { {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}}, {{0,4,4,0},{0,4,4,0},{0,0,0,0},{0,0,0,0}} },
    { {{0,5,5,0},{5,5,0,0},{0,0,0,0},{0,0,0,0}}, {{0,5,0,0},{0,5,5,0},{0,0,5,0},{0,0,0,0}}, {{0,0,0,0},{0,5,5,0},{5,5,0,0},{0,0,0,0}}, {{5,0,0,0},{5,5,0,0},{0,5,0,0},{0,0,0,0}} },
    { {{0,6,0,0},{6,6,6,0},{0,0,0,0},{0,0,0,0}}, {{0,6,0,0},{0,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,0,0,0},{6,6,6,0},{0,6,0,0},{0,0,0,0}}, {{0,6,0,0},{6,6,0,0},{0,6,0,0},{0,0,0,0}} },
    { {{7,7,0,0},{0,7,7,0},{0,0,0,0},{0,0,0,0}}, {{0,0,7,0},{0,7,7,0},{0,7,0,0},{0,0,0,0}}, {{0,0,0,0},{7,7,0,0},{0,7,7,0},{0,0,0,0}}, {{0,7,0,0},{7,7,0,0},{7,0,0,0},{0,0,0,0}} }
};

SDL_Color colors[8] = {
    {30, 30, 30, 255}, {0, 255, 255, 255}, {0, 0, 255, 255}, {255, 165, 0, 255},
    {255, 255, 0, 255}, {0, 255, 0, 255}, {128, 0, 128, 255}, {255, 0, 0, 255}
};

// ========= ESTRUCTURAS DE DATOS =========

// 1. Nodo para la fila del tablero (Lista Enlazada)
typedef struct RowNode {
    int cells[GRID_COLS]; 
    struct RowNode* next; 
} RowNode;

// 2. Nodo para la cola de próximas piezas (Queue)
typedef struct PieceQueueNode {
    int type; 
    struct PieceQueueNode* next;
} PieceQueueNode;

// 3. Nodo para el sistema de partículas 
typedef struct ParticleNode {
    float x, y;
    float vel_x, vel_y;
    int color;
    int life_timer;
    struct ParticleNode* next;
} ParticleNode;

typedef struct ActivePiece {
    int col, row;
    int type;      
    int rotation;  
    int active;
} ActivePiece;

typedef struct GameBoard {
    RowNode* first_row;             
    PieceQueueNode* queue_head;     
    PieceQueueNode* queue_tail;     
    ParticleNode* first_particle;   
    ActivePiece current_piece;
    int score;
    int tick_timer;
} GameBoard;

// ========= VARIABLES GLOBALES =========
SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
GameBoard* game_board = NULL;

// ========= EJERCICIO 1: STRINGS (NO USAR string.h) =========

char* strDuplicate(char* src) {
    int len = 0;
    for(int i = 0; src[i] != '\0'; i++){
        len++;
    }
    char* duplicado = (char*) malloc(sizeof(char)*(len + 1));
    
    for(int i = 0; i < len; i++){
        duplicado[i] = src[i];
    }
    duplicado[len] = '\0';
    return duplicado;
}

int strCompare(char* s1, char* s2) {
    int i = 0;
    int res = 0;
    int seguir = 1;
    while(seguir){
        if ((s1[i] == '\0') && (s2[i] == '\0')){ // si se llega acá, es porque todos lso caracteres previos eran iguales
            seguir = 0;
        } else if(s1[i] < s2[i]){
            res = 1;
            seguir = 0;
        } else if(s1[i] > s2[i]){
            res = -1;
            seguir = 0;
        } else { // son iguales distintos de cero
            i++;
        } 
    }
    return res;
}

char* strConcatenate(char* src1, char* src2) {
    // Precondicion: src1 y src2 vienen de malloc y son punteros distintos.
    int newLen = 0;
    for(int i = 0; src1[i] != '\0'; i++){
        newLen++;
    }
    for(int i = 0; src2[i] != '\0'; i++){
        newLen++;
    }
    char* concatenado = (char*) malloc(sizeof(char)*(newLen + 1));
    int k = 0;
    for(int i = 0; src1[i] != '\0'; i++){
        concatenado[k] = src1[i];
        k++;
    }
    for(int i = 0; src2[i] != '\0'; i++){
        concatenado[k] = src2[i];
        k++;
    }
    concatenado[k] = '\0';
    free(src1);
    free(src2);
    
    return concatenado;
}

// ========= EJERCICIO 2: FUNCIONES A IMPLEMENTAR (EL TP) =========

GameBoard* gameBoardNew() {
    // se pide memoria dinámicamente para el tablero
    GameBoard* board = (GameBoard*) malloc(sizeof(GameBoard));

    RowNode* current_row = NULL;
    // se pide memoria dinámicamente y se incializan los nodos de filas desde el último hasta el primero
    for(int i = 19; i >= 0; i--){
        RowNode* new_row = (RowNode*) malloc(sizeof(RowNode));
        
        // si falla el malloc, se libera la memoria y devuelve NULL
        if(new_row == NULL){
            RowNode* posterior_row = current_row -> next;
            while(posterior_row != NULL){
                free(current_row);
                current_row = posterior_row;
                posterior_row = current_row -> next;
            }
            // este ciclo libera todos los nodos menos el último
            free(current_row);
            return NULL;
        }
        
        // si malloc no falla
        for(int j = 0; j < 10; j++){
            new_row -> cells[j] = 0;
        }
        new_row -> next = current_row;
        current_row = new_row;
    }
    board -> first_row = current_row;

    // se inicializan queue_head, queue_tail y first_particle en NULL
    board -> queue_head = NULL;
    board -> queue_tail = NULL;
    board -> first_particle = NULL;

    // se inicializan el resto de valores
    board -> current_piece.active = 0;
    board -> score = 0;
    board -> tick_timer = 0;

    return board;
}

void gameBoardDelete(GameBoard* board) {
    // debe tolerar board == NULL.
    if(board == NULL){
        return;
    }

    // se recorre y libera todos los RowNode con free().
    RowNode* current_row = board -> first_row;
    RowNode* posterior_row = current_row -> next;
    while(posterior_row != NULL){
        free(current_row);
        current_row = posterior_row;
        posterior_row = current_row -> next;
    }
    free(current_row);

    // se recorre y se libera todos los PieceQueueNode si es que no está vacía
    if(board -> queue_head != NULL){
        PieceQueueNode* current_piece = board -> queue_head;
        PieceQueueNode* posterior_piece = current_piece -> next;
        while(posterior_piece != NULL){
            free(current_piece);
            current_piece = posterior_piece;
            posterior_piece = current_piece -> next;
        }
        free(current_piece);
    }
    

    // se recorre y se libera todos los ParticleNode si es que no está vacía
    if(board -> first_particle != NULL){
        ParticleNode* current_particle = board -> first_particle;
        ParticleNode* posterior_particle = current_particle -> next;
        while(posterior_particle != NULL){
            free(current_particle);
            current_particle = posterior_particle;
            posterior_particle = current_particle -> next;
        }
        free(current_particle);
    }

    // se libera la estructura GameBoard
    free(board);
}

void gameBoardAddParticle(GameBoard* board, float x, float y, int color) {
    // se reserva el espacio de un ParticleNode en memoria dinámica.
    ParticleNode* new_particle = (ParticleNode*) malloc(sizeof(ParticleNode));

    // se inicializan los datos: x, y, velocidades aleatorias y un life_timer.
    new_particle -> x = OFFSET_X + x * CELL_SIZE + CELL_SIZE/2;  // lo mismo que en el juego base
    new_particle -> y = OFFSET_Y + y * CELL_SIZE + CELL_SIZE/2;
    new_particle -> vel_x = ((rand() % 100) / 10.0f) - 5.0f; 
    new_particle -> vel_y = ((rand() % 50) / -10.0f) - 2.0f;
    new_particle -> life_timer = 30 + (rand() % 30);
    new_particle -> color = color;

    // Caso Base: la lista de particulas está vacía
    if(board -> first_particle == NULL){
        board -> first_particle = new_particle;
        new_particle -> next = NULL;
        return;
    }

    // Caso genérico: se enlaza el nuevo nodo al principio de la lista
    new_particle -> next = board -> first_particle;
    board -> first_particle = new_particle;
}

// FUNCIÓN AUXILIAR
int isComplete(int arr[], int size){
    // devuelve 0 si y solo si hay algún 0 en el arreglo. 1 en caso contrario. 
    for(int i = 0; i < size; i++){
        if(arr[i] == 0){
            return 0;
        }
    }
    return 1;
}

int gameBoardCheckAndClearLines(GameBoard* board) {
    int lineas_eliminadas = 0;
    RowNode* previous_row = NULL;
    RowNode* current_row = board -> first_row;
    int current_row_index = 0;
    while(current_row != NULL){ 
        // por cada fila, fijarnos si está llena
        if(isComplete(current_row -> cells, 10)){
            // se genera una particula por celda
            for(int i = 0; i < 10; i++){
                gameBoardAddParticle(board, i, current_row_index, (current_row -> cells)[i]);
            }

            // se borra el nodo de la lista
            if(previous_row == NULL){ // pasa sólo cuando current es el primer nodo de la lista
                board -> first_row = current_row -> next;
                free(current_row);
                current_row = board -> first_row;
            } else {
                previous_row -> next = current_row -> next;
                free(current_row);
                current_row = previous_row -> next;
            }
            
            // se generan fila en blanco arriba del todo
            RowNode* new_row = (RowNode*) malloc(sizeof(RowNode));
            for(int i = 0; i < 10; i++){
                (new_row -> cells)[i] = 0;
            }
            new_row -> next = board -> first_row;
            board -> first_row = new_row;
            
            lineas_eliminadas++;
            current_row_index++;
        } else{
            previous_row = current_row;
            current_row = current_row -> next;
            current_row_index++;
        }
    }
    
    return lineas_eliminadas;
}

void gameBoardEnqueuePiece(GameBoard* board, int type) {
    // TODO: Crear un PieceQueueNode con el 'type'.
    PieceQueueNode* new_piece = (PieceQueueNode*) malloc(sizeof(PieceQueueNode));
    new_piece -> type = type;
    new_piece -> next = NULL;

    if(board -> queue_tail == NULL){ // la cola está vacía
        board -> queue_head = new_piece;
        board -> queue_tail = new_piece;
        return;
    }

    // TODO: Insertarlo al final de la cola (queue_tail) y actualizar punteros.
    board -> queue_tail -> next = new_piece;
    board -> queue_tail = new_piece;
}

int gameBoardDequeuePiece(GameBoard* board, int* out_type) {
    // TODO: Si la cola está vacía, retornar 0.
    if (board -> queue_tail == NULL){
        return 0;
    } else if(board -> queue_head -> next == NULL){ // una sola pieza en la cola 
        *out_type = board -> queue_head -> type;
        free(board -> queue_head);
        board -> queue_head = NULL;
        board -> queue_tail = NULL;
        return 1;
    }

    *out_type = board -> queue_head -> type;
    PieceQueueNode* new_head = board -> queue_head -> next;
    free(board -> queue_head);
    board -> queue_head = new_head;
    return 1;
}

// ========= LOGICA DE DIBUJADO Y COLISIONES (NO MODIFICAR) =========

RowNode* getRowAt(GameBoard* board, int index) {
    RowNode* curr = board->first_row;
    for(int i = 0; i < index && curr != NULL; i++) curr = curr->next;
    return curr;
}

int canMove(GameBoard* board, int target_col, int target_row, int target_rot) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            if (TETROMINOS[board->current_piece.type][target_rot][r][c] != 0) {
                int board_col = target_col + c;
                int board_row = target_row + r;
                if (board_col < 0 || board_col >= GRID_COLS || board_row >= GRID_ROWS) return 0;
                if (board_row >= 0) {
                    RowNode* row_node = getRowAt(board, board_row);
                    if (row_node && row_node->cells[board_col] != 0) return 0;
                }
            }
        }
    }
    return 1;
}

void lockPiece(GameBoard* board) {
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            int val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][r][c];
            if (val != 0) {
                RowNode* row_node = getRowAt(board, board->current_piece.row + r);
                if (row_node) row_node->cells[board->current_piece.col + c] = val;
            }
        }
    }
    board->current_piece.active = 0;
    board->score += gameBoardCheckAndClearLines(board) * 100;
}

// Devuelve 0 si la pieza nueva no entra (GAME OVER), 1 en otro caso.
int spawnNextPiece(GameBoard* board) {
    int type;
    if (gameBoardDequeuePiece(board, &type)) {
        board->current_piece.type = type;
        board->current_piece.rotation = 0;
        board->current_piece.col = GRID_COLS / 2 - 2;
        board->current_piece.row = 0;
        board->current_piece.active = 1;
        gameBoardEnqueuePiece(board, rand() % 7);
        if (!canMove(board, board->current_piece.col, board->current_piece.row, board->current_piece.rotation)) {
            printf("GAME OVER. Puntaje final: %d\n", board->score);
            board->current_piece.active = 0;
            return 0;   // main sale del loop y libera tablero y SDL
        }
    }
    return 1;
}

// Devuelve 0 cuando termina la partida.
int gameBoardUpdate(GameBoard* board) {
    if (!board) return 0;
    if (!board->current_piece.active && !spawnNextPiece(board)) return 0;

    board->tick_timer++;
    if (board->tick_timer >= TICK_RATE) {
        board->tick_timer = 0;
        if (canMove(board, board->current_piece.col, board->current_piece.row + 1, board->current_piece.rotation)) {
            board->current_piece.row++;
        } else {
            lockPiece(board);
        }
    }

    ParticleNode* curr_part = board->first_particle;
    ParticleNode* prev_part = NULL;
    while (curr_part) {
        curr_part->x += curr_part->vel_x;
        curr_part->y += curr_part->vel_y;
        curr_part->vel_y += 0.5f;
        curr_part->life_timer--;

        if (curr_part->life_timer <= 0) {
            ParticleNode* to_delete = curr_part;
            if (prev_part == NULL) {
                board->first_particle = curr_part->next;
                curr_part = board->first_particle;
            } else {
                prev_part->next = curr_part->next;
                curr_part = prev_part->next;
            }
            free(to_delete);
        } else {
            prev_part = curr_part;
            curr_part = curr_part->next;
        }
    }
    return 1;
}

void gameBoardDraw(GameBoard* board) {
    if (!board) return;
    SDL_SetRenderDrawColor(renderer, 30, 30, 30, 255);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
    SDL_Rect board_rect = {OFFSET_X - 2, OFFSET_Y - 2, GRID_COLS * CELL_SIZE + 4, GRID_ROWS * CELL_SIZE + 4};
    SDL_RenderDrawRect(renderer, &board_rect);

    RowNode* current_row = board->first_row;
    int r = 0;
    while (current_row) {
        for (int c = 0; c < GRID_COLS; c++) {
            if (current_row->cells[c] > 0) {
                SDL_Color col = colors[current_row->cells[c]];
                SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                SDL_Rect cell_rect = {OFFSET_X + c * CELL_SIZE, OFFSET_Y + r * CELL_SIZE, CELL_SIZE - 1, CELL_SIZE - 1};
                SDL_RenderFillRect(renderer, &cell_rect);
            }
        }
        current_row = current_row->next;
        r++;
    }

    if (board->current_piece.active) {
        for(int pr = 0; pr < 4; pr++) {
            for(int pc = 0; pc < 4; pc++) {
                int cell_val = TETROMINOS[board->current_piece.type][board->current_piece.rotation][pr][pc];
                if (cell_val != 0) {
                    SDL_Color col = colors[cell_val];
                    SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, 255);
                    SDL_Rect p_rect = {OFFSET_X + (board->current_piece.col + pc) * CELL_SIZE, 
                                       OFFSET_Y + (board->current_piece.row + pr) * CELL_SIZE, 
                                       CELL_SIZE - 1, CELL_SIZE - 1};
                    SDL_RenderFillRect(renderer, &p_rect);
                }
            }
        }
    }

    ParticleNode* curr_part = board->first_particle;
    while (curr_part) {
        SDL_Color col = colors[curr_part->color];
        int alpha = curr_part->life_timer * 8;   // se satura: SDL recibe un Uint8
        if (alpha > 255) alpha = 255;
        SDL_SetRenderDrawColor(renderer, col.r, col.g, col.b, alpha);
        SDL_Rect part_rect = {(int)curr_part->x, (int)curr_part->y, 6, 6};
        SDL_RenderFillRect(renderer, &part_rect);
        curr_part = curr_part->next;
    }
    SDL_RenderPresent(renderer);
}

int inicializar() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) return 0;
    window = SDL_CreateWindow("Tetris Base - TP", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN);
    if (window == NULL) return 0;
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) return 0;
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);
    return 1;
}

// ========= TESTS (COMPLETAR) =========
// función auxiliar para contar las filas
int cantidadFilas(GameBoard* board) {
    int cantidad = 0;
    RowNode* current = board -> first_row;

    while (current != NULL) {
        cantidad++;
        current = current -> next;
    }

    return cantidad;
}

void correrTests() {
    // ===== strDuplicate =====

    // string vacío
    char* s1 = strDuplicate("");
    assert(s1[0] == '\0');
    free(s1);

    // string "x"
    char* s2 = strDuplicate("x");
    assert(s2[0] == 'x');
    assert(s2[1] == '\0');
    free(s2);

    // string con distintos caracteres
    char* s3 = strDuplicate("hola");
    assert(s3[0] == 'h');
    assert(s3[1] == 'o');
    assert(s3[2] == 'l');
    assert(s3[3] == 'a');
    assert(s3[4] == '\0');
    assert(s3);
    free(s3);

    // ===== strCompare =====
    // dos strings vacios
    assert(strCompare("", "") == 0);

    // dos strings iguales de un caracter
    assert(strCompare("a", "a") == 0);

    // primer string menor que el segundo
    assert(strCompare("a", "b") == 1);

    // primer string mayor que el segundo
    assert(strCompare("b", "a") == -1);

    // ====== strConcatenate =====
    // concatenar string vacio con uno de 3 caracteres
    char* c1 = strDuplicate("");
    char* c2 = strDuplicate("abc");

    char* resultado1 = strConcatenate(c1, c2);

    assert(resultado1 != NULL);
    assert(strCompare(resultado1, "abc") == 0);
    free(resultado1);

    // Concatenar dos strings de 5 caracteres
    char* c3 = strDuplicate("hola ");
    char* c4 = strDuplicate("mundo");

    char* resultado2 = strConcatenate(c3, c4);

    assert(resultado2 != NULL);
    assert(strCompare(resultado2, "hola mundo") == 0);
    free(resultado2);

    // ===== Test de la cola =====
    GameBoard* board1 = gameBoardNew();
    assert(board1 != NULL);

    // insertamos cinco piezas
    gameBoardEnqueuePiece(board1, 1);
    gameBoardEnqueuePiece(board1, 2);
    gameBoardEnqueuePiece(board1, 3);
    gameBoardEnqueuePiece(board1, 4);
    gameBoardEnqueuePiece(board1, 5);

    int pieza;

    // extraemos y verificamos el orden FIFO
    assert(gameBoardDequeuePiece(board1, &pieza) == 1);
    assert(pieza == 1);

    assert(gameBoardDequeuePiece(board1, &pieza) == 1);
    assert(pieza == 2);

    assert(gameBoardDequeuePiece(board1, &pieza) == 1);
    assert(pieza == 3);

    assert(gameBoardDequeuePiece(board1, &pieza) == 1);
    assert(pieza == 4);

    assert(gameBoardDequeuePiece(board1, &pieza) == 1);
    assert(pieza == 5);

    // la sexta extraccion debe fallar
    assert(gameBoardDequeuePiece(board1, &pieza) == 0);

    // la cola debe quedar completamente vacia
    assert(board1 -> queue_head == NULL);
    assert(board1 -> queue_tail == NULL);

    // comprobamos que se pueda volver a insertar y extraer
    gameBoardEnqueuePiece(board1, 6);

    assert(gameBoardDequeuePiece(board1, &pieza) == 1);
    assert(pieza == 6);

    assert(board1 -> queue_head == NULL);
    assert(board1 -> queue_tail == NULL);

    gameBoardDelete(board1);

    // ===== Test tablero =====
    // definimos una función auxiliar int cantidadFilas(GameBoard* board) arriba
    GameBoard* board2 = gameBoardNew();
    assert(board2 != NULL);

    // buscamos la ultima fila
    RowNode* fila = board2 -> first_row;

    while(fila -> next != NULL) {
        fila = fila -> next;
    }

    // llenamos todas las celdas de la fila
    for (int i = 0; i < 10; i++) {
        fila -> cells[i] = 1;
    }

    // debe eliminar exactamente una linea
    assert(gameBoardCheckAndClearLines(board2) == 1);

    // la nueva primera fila debe estar vacia
    fila = board2 -> first_row;

    for (int i = 0; i < 10; i++) {
        assert(fila -> cells[i] == 0);
    }

    // el tablero debe seguir teniendo 20 filas
    assert(cantidadFilas(board2) == 20);

    gameBoardDelete(board2);
}

// Compara contra "--test" sin depender de string.h ni del Ejercicio 1.
int esFlagTest(char* arg) {
    char* flag = "--test";
    int i = 0;
    while (arg[i] != '\0' && arg[i] == flag[i]) i++;
    return arg[i] == '\0' && flag[i] == '\0';
}

int main(int argc, char* args[]) {
    srand(time(NULL));

    // Los tests corren antes de SDL. Con --test termina aca, asi valgrind solo
    // reporta la memoria del TP (SDL tiene bloques propios que no se liberan).
    correrTests();
    if (argc > 1 && esFlagTest(args[1])) return 0;

    if (!inicializar()) return 1;

    game_board = gameBoardNew();
    if (game_board == NULL) {
        printf("ADVERTENCIA: Implementa gameBoardNew() para iniciar el juego.\n");
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    }

    for(int i=0; i<3; i++) gameBoardEnqueuePiece(game_board, rand() % 7);

    SDL_Event e;
    int game_over = 0;

    while (!game_over) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) game_over = 1;
            if (e.type == SDL_KEYDOWN && game_board->current_piece.active) {
                if (e.key.keysym.sym == SDLK_LEFT && canMove(game_board, game_board->current_piece.col - 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col--;
                if (e.key.keysym.sym == SDLK_RIGHT && canMove(game_board, game_board->current_piece.col + 1, game_board->current_piece.row, game_board->current_piece.rotation)) game_board->current_piece.col++;
                if (e.key.keysym.sym == SDLK_DOWN && canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                if (e.key.keysym.sym == SDLK_UP) {
                    int next_rot = (game_board->current_piece.rotation + 1) % 4;
                    if (canMove(game_board, game_board->current_piece.col, game_board->current_piece.row, next_rot)) game_board->current_piece.rotation = next_rot;
                }
                if (e.key.keysym.sym == SDLK_SPACE) {
                     while(canMove(game_board, game_board->current_piece.col, game_board->current_piece.row + 1, game_board->current_piece.rotation)) game_board->current_piece.row++;
                     lockPiece(game_board);
                }
            }
        }
        if (!gameBoardUpdate(game_board)) game_over = 1;
        gameBoardDraw(game_board);
        SDL_Delay(16);
    }
    gameBoardDelete(game_board);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
