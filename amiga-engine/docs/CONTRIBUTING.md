# Contribuir al amiga-engine

## Reglas

- Toda nueva primitiva gráfica o de escena debe incluir **al menos un test host** en `tests/host/test_runner.cpp` (o un archivo añadido al `Makefile` del motor).
- No introducir dependencias que rompan la compilación **freestanding** del resto del repositorio.
- El puente Amiga en C debe mantenerse delgado: lógica reutilizable en C++ bajo `include/` / `src/cxx/`.

## Estilo C++

- C++20, sin excepciones en el núcleo pensado para enlazado futuro con Amiga (`-fno-exceptions` recomendado en destino).
- Preferir `std::span`, tipos fuertes y RAII; evitar punteros crudos en la API pública del motor.
