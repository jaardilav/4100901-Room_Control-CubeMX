#include "keypad_driver.h"

// Mapa de caracteres del teclado matricial 4x4
static const char keypad_map[KEYPAD_ROWS][KEYPAD_COLS] = {            //Aquí se guarda qué carácter corresponde a cada posición del teclado.
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

/**
 * @brief Inicializa el estado del teclado.
 * Configura todas las filas como salidas y las pone en estado BAJO.
 * Esto prepara el teclado para que, al presionar una tecla, se genere
 * una interrupción por flanco de bajada en el pin de la columna correspondiente.
 * @param keypad Puntero al manejador del teclado.
 */
void keypad_init(keypad_handle_t* keypad) {     //prepara el teclado para “escuchar” una tecla.
    // Se asume que los pines ya están configurados como Salida/Entrada en el main.c (CubeMX)
    // Filas como Salida (Output Push-Pull)
    // Columnas como Entrada con Pull-up (Input with Pull-up) y configuradas para interrupción externa.
    
    // Ponemos todas las filas en BAJO para habilitar la detección de interrupciones en las columnas.
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_RESET);
    }
}

/**
 * @brief Escanea el teclado para determinar qué tecla fue presionada.
 * Esta función debe ser llamada desde la rutina de interrupción de los pines de columna.
 * @param keypad Puntero al manejador del teclado.
 * @param col_pin El pin de la columna que generó la interrupción.
 * @return El caracter de la tecla presionada o '\0' si no se detecta ninguna.
 */
char keypad_scan(keypad_handle_t* keypad, uint16_t col_pin) {     //cuando el usuario presiona una tecla
    char key_pressed = '\0';
    int col_index = -1;

    // --- 1. Debounce ---
    //Pequeña pausa (50 ms) para evitar detectar la misma tecla varias veces por el “rebote” mecánico del botón.
    HAL_Delay(50);

    // --- 2. Identificar columna ---
    // Itera sobre los pines de columna para encontrar el índice que corresponde al pin
    // que causó la interrupción.
    for (int i = 0; i < KEYPAD_COLS; i++) {
        if (keypad->col_pins[i] == col_pin) {
            col_index = i;
            break;
        }
    }

    // Si por alguna razón el pin no corresponde a ninguna columna, salir.
    if (col_index == -1) {
        return '\0';
    }

    // --- 3. Identificar fila --- Este es el paso clave para saber exactamente qué tecla fue presionada.
    // Pon todas las filas en ALTO
    for (int i = 0; i < KEYPAD_ROWS; i++) {
        HAL_GPIO_WritePin(keypad->row_ports[i], keypad->row_pins[i], GPIO_PIN_SET);
    }

    // Itera sobre las filas, poniendo una a una en BAJO para encontrar la que está conectada
    // a la columna activa.
    for (int row = 0; row < KEYPAD_ROWS; row++) {
        // Pone la fila actual en BAJO
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_RESET);

        // Lee el pin de la columna. Si está en BAJO, hemos encontrado la tecla.
        if (HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET) {
            
            // Guarda el caracter correspondiente del mapa.
            key_pressed = keypad_map[row][col_index];
            
            // Espera hasta que se suelte la tecla (vuelva a ser ALTA) para evitar lecturas múltiples.
            while(HAL_GPIO_ReadPin(keypad->col_ports[col_index], keypad->col_pins[col_index]) == GPIO_PIN_RESET);
            
            // Sal del bucle, ya encontramos la tecla.
            break; 
        }

        // Restaura la fila actual a ALTO antes de probar la siguiente.
        HAL_GPIO_WritePin(keypad->row_ports[row], keypad->row_pins[row], GPIO_PIN_SET);
    }

    // --- 4. Restaurar ---
    // Llama a keypad_init() para poner todas las filas en BAJO de nuevo,
    // preparando el sistema para la próxima interrupción.
    keypad_init(keypad);

    // --- 5. Return ---
    // Devuelve el carácter que se detectó
    return key_pressed;
}