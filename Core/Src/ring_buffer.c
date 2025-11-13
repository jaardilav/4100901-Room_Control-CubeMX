// Archivo: ring_buffer.c

#include "ring_buffer.h"

void ring_buffer_init(ring_buffer_t *rb, uint8_t *buffer, uint16_t capacity)      //Esta función prepara el buffer para usarse
{
    rb->buffer = buffer;
    rb->capacity = capacity;
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0; // Inicializamos el contador a 0.
}

bool ring_buffer_write(ring_buffer_t *rb, uint8_t data) //Se guarda el nuevo dato en la posición de la cabeza
{
    // Escribe el dato en la posición de la cabeza.
    rb->buffer[rb->head] = data;

    // Avanza la cabeza.
    rb->head = (rb->head + 1) % rb->capacity;

    if (rb->count < rb->capacity) {
        // Si aún no estamos llenos, simplemente incrementamos el contador.
        rb->count++;
    } else {
        // Si ya estábamos llenos, la cabeza ha sobrescrito el dato más antiguo.
        // La cola debe avanzar también para apuntar al nuevo dato más antiguo.
        // El contador permanece en 'capacity'.
        rb->tail = (rb->tail + 1) % rb->capacity;
    }
    
    return true;           //Siempre devuelve true porque el dato se guarda de todas formas (aunque se sobrescriba).
}

bool ring_buffer_read(ring_buffer_t *rb, uint8_t *data)
{
    // Usamos el contador para ver si está vacío.
    if (rb->count == 0) {
        return false;           //si no hay datos , no se puede leer, devuelve false.
    }

    // Lee el dato desde la cola.
    *data = rb->buffer[rb->tail];
    // Avanza la cola.
    rb->tail = (rb->tail + 1) % rb->capacity;
    // Decrementamos el contador.
    rb->count--;

    return true;
}

uint16_t ring_buffer_count(ring_buffer_t *rb)
{
    // La lógica ahora es trivial: solo devolvemos el contador.
    return rb->count;
}

bool ring_buffer_is_empty(ring_buffer_t *rb)
{
    // El búfer está vacío si el contador es 0.
    return rb->count == 0;
}

bool ring_buffer_is_full(ring_buffer_t *rb)
{
    // El búfer está lleno si el contador es igual a la capacidad.
    return rb->count == rb->capacity;
}

void ring_buffer_flush(ring_buffer_t *rb)
{
    // Restablecemos todo al estado inicial.
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}