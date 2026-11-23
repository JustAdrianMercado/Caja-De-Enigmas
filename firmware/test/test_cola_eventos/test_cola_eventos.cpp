// Pruebas de ColaEventos en la PC: orden, cola llena y eventos rechazados.
// Ejecutar desde firmware/: pio test -e pruebas

#include <unity.h>

#include <stdio.h>
#include <string.h>

#include "ColaEventos.h"

namespace {

// Llena la cola con eventos numerados: {"n":desde} ... {"n":hasta}.
void agregarNumerados(ColaEventos& cola, int desde, int hasta) {
    char json[16];
    for (int numero = desde; numero <= hasta; ++numero) {
        snprintf(json, sizeof(json), "{\"n\":%d}", numero);
        TEST_ASSERT_TRUE(cola.agregar(json));
    }
}

}  // namespace

void setUp() {}
void tearDown() {}

void test_cola_nueva_esta_vacia() {
    ColaEventos cola;
    TEST_ASSERT_TRUE(cola.estaVacia());
    TEST_ASSERT_NULL(cola.primero());
    TEST_ASSERT_EQUAL_UINT32(0, cola.descartados());
}

void test_los_eventos_salen_en_el_orden_en_que_entraron() {
    ColaEventos cola;
    agregarNumerados(cola, 1, 3);
    TEST_ASSERT_EQUAL_STRING("{\"n\":1}", cola.primero());
    cola.quitarPrimero();
    TEST_ASSERT_EQUAL_STRING("{\"n\":2}", cola.primero());
    cola.quitarPrimero();
    TEST_ASSERT_EQUAL_STRING("{\"n\":3}", cola.primero());
    cola.quitarPrimero();
    TEST_ASSERT_TRUE(cola.estaVacia());
}

void test_entran_20_eventos_sin_perder_ninguno() {
    ColaEventos cola;
    agregarNumerados(cola, 1, 20);
    TEST_ASSERT_EQUAL_UINT32(20, cola.cantidad());
    TEST_ASSERT_EQUAL_UINT32(0, cola.descartados());
    TEST_ASSERT_EQUAL_STRING("{\"n\":1}", cola.primero());
}

void test_cola_llena_descarta_el_mas_viejo_y_lo_cuenta() {
    ColaEventos cola;
    agregarNumerados(cola, 1, 23);
    TEST_ASSERT_EQUAL_UINT32(20, cola.cantidad());
    TEST_ASSERT_EQUAL_UINT32(3, cola.descartados());
    TEST_ASSERT_EQUAL_STRING("{\"n\":4}", cola.primero());
}

void test_el_orden_se_conserva_al_dar_la_vuelta_al_buffer() {
    ColaEventos cola;
    agregarNumerados(cola, 1, 15);
    for (int quitados = 0; quitados < 10; ++quitados) {
        cola.quitarPrimero();
    }
    agregarNumerados(cola, 16, 30);  // Ocupa las posiciones liberadas al principio.
    TEST_ASSERT_EQUAL_UINT32(20, cola.cantidad());
    for (int esperado = 11; esperado <= 30; ++esperado) {
        char json[16];
        snprintf(json, sizeof(json), "{\"n\":%d}", esperado);
        TEST_ASSERT_EQUAL_STRING(json, cola.primero());
        cola.quitarPrimero();
    }
    TEST_ASSERT_TRUE(cola.estaVacia());
}

void test_json_vacio_o_demasiado_largo_no_se_agrega() {
    ColaEventos cola;
    char largo[EventoJson::TamanoMaximo + 1];
    memset(largo, 'x', sizeof(largo) - 1);
    largo[sizeof(largo) - 1] = '\0';
    TEST_ASSERT_FALSE(cola.agregar(largo));
    TEST_ASSERT_FALSE(cola.agregar(""));
    TEST_ASSERT_FALSE(cola.agregar(nullptr));
    TEST_ASSERT_TRUE(cola.estaVacia());
}

void test_un_evento_rechazado_se_descarta_al_tercer_fallo() {
    ColaEventos cola;
    agregarNumerados(cola, 1, 2);
    TEST_ASSERT_FALSE(cola.registrarFalloDelPrimero());
    TEST_ASSERT_FALSE(cola.registrarFalloDelPrimero());
    TEST_ASSERT_TRUE(cola.registrarFalloDelPrimero());
    TEST_ASSERT_EQUAL_UINT32(1, cola.descartados());
    TEST_ASSERT_EQUAL_STRING("{\"n\":2}", cola.primero());
}

void test_publicar_con_exito_reinicia_la_cuenta_de_fallos() {
    ColaEventos cola;
    agregarNumerados(cola, 1, 2);
    cola.registrarFalloDelPrimero();
    cola.registrarFalloDelPrimero();
    cola.quitarPrimero();  // El primero se publico al fin.
    TEST_ASSERT_FALSE(cola.registrarFalloDelPrimero());  // El segundo empieza de cero.
    TEST_ASSERT_EQUAL_UINT32(0, cola.descartados());
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_cola_nueva_esta_vacia);
    RUN_TEST(test_los_eventos_salen_en_el_orden_en_que_entraron);
    RUN_TEST(test_entran_20_eventos_sin_perder_ninguno);
    RUN_TEST(test_cola_llena_descarta_el_mas_viejo_y_lo_cuenta);
    RUN_TEST(test_el_orden_se_conserva_al_dar_la_vuelta_al_buffer);
    RUN_TEST(test_json_vacio_o_demasiado_largo_no_se_agrega);
    RUN_TEST(test_un_evento_rechazado_se_descarta_al_tercer_fallo);
    RUN_TEST(test_publicar_con_exito_reinicia_la_cuenta_de_fallos);
    return UNITY_END();
}
