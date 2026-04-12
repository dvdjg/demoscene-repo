/*
 * Prueba mínima de C++23 freestanding (sin RTTI/excepciones/libstdc++).
 * Atributo [[assume]] (C++23): pista al optimizador, sin coste en tiempo de ejecución.
 * Ver TOOLCHAIN.md.
 */
extern "C" void DemosceneCxxSmoke(void) {
  int x = 1;
  [[assume(x != 0)]];
}
