# Proyecto: Kid Ikarus Game in Console

## Descripción

Este proyecto consiste en una adaptación del videojuego clásico **Kid Ikarus** desarrollada completamente en consola utilizando **C++**, **ASCII-Art** y programación concurrente mediante **hilos POSIX / std::thread**. El objetivo principal es permitir que el jugador controle a **Pit** mientras asciende plataformas, derrota enemigos y evita obstáculos dentro de un entorno retro renderizado en tiempo real.

El sistema implementa:

- Renderizado en consola en tiempo real.
- Uso de buffers para evitar parpadeos.
- Física básica y colisiones.
- Enemigos concurrentes.
- HUD dinámico.
- Sistema de puntajes persistente.
- Menús interactivos.
- Tienda de mejoras.
- Pantallas de victoria y Game Over.

El proyecto fue desarrollado para el curso **CC3086 - Programación de Microprocesadores** de la Universidad del Valle de Guatemala.

---

# Estructura del Proyecto

```text
kid_ikarus_game_in_console/
│
├── include/
│   ├── CollisionSystem.h
│   ├── Constants.h
│   ├── EnemyManager.h
│   ├── GameLoop.h
│   ├── GameState.h
│   ├── HUD.h
│   ├── InputHandler.h
│   ├── Level.h
│   ├── Physics.h
│   ├── ProjectileManager.h
│   ├── Renderer.h
│   └── ScoreManager.h
│
├── src/
│   ├── CollisionSystem.cpp
│   ├── EnemyManager.cpp
│   ├── GameLoop.cpp
│   ├── HUD.cpp
│   ├── InputHandler.cpp
│   ├── Level.cpp
│   ├── main.cpp
│   ├── Physics.cpp
│   ├── ProjectilManager.cpp
│   ├── Renderer.cpp
│   └── ScoreManager.cpp
│
├── obj/
│
├── scores.txt
├── Makefile
└── README.md
```

---

# Características Principales

- Juego completamente funcional en consola.
- Representación gráfica utilizando ASCII-Art.
- Renderizado eficiente mediante doble buffer.
- Gestión de enemigos y proyectiles.
- Sistema de físicas y colisiones.
- Menús interactivos.
- Sistema de pausa.
- Tabla de puntajes persistente.
- Tienda para intercambio de corazones por vida.
- Modo CPU automático.
- Compatibilidad multiplataforma.

---

# Representación Visual con ASCII-Art

| Elemento | Representación |
|---|---|
| Pit | `@` |
| Monoeye | `M` |
| Shemum | `S` |
| Reaper | `R` |
| Reapette | `r` |
| Proyectil jugador | `>` |
| Proyectil enemigo | `o` |
| Medusa | `G` |
| Plataforma | `*` |
| Tienda | `$` |

---

# Pantallas del Juego

## Menú Principal

Permite:

- Iniciar partida.
- Activar modo CPU.
- Visualizar instrucciones.
- Consultar puntajes.
- Salir del juego.

---

## Pantalla de Instrucciones

Describe:

- Controles.
- Mecánicas principales.
- Representación de enemigos.
- Objetos y símbolos ASCII.

---

## Pantalla de Puntajes

Lee la información almacenada en `scores.txt` y muestra los mejores resultados del jugador.

---

## Pantalla de Juego

Renderiza dinámicamente:

- Jugador.
- Plataformas.
- HUD.
- Enemigos.
- Proyectiles.
- Objetos interactivos.

---

## Pantalla de Pausa

Permite detener temporalmente la ejecución del juego y continuar posteriormente.

---

## Pantalla de Tienda

Permite intercambiar corazones obtenidos durante la partida por puntos de vida adicionales.

---

## Pantallas Finales

### Victoria
Se muestra al derrotar a Medusa.

### Game Over
Se despliega cuando el jugador pierde todas las vidas.

---

# Diseño en Consola

El sistema divide la terminal en dos áreas principales:

## HUD Superior

Contiene información del jugador:

- Vida
- Puntaje
- Corazones
- Estado actual

## Área de Juego

Contiene:

- Plataformas
- Enemigos
- Movimiento del jugador
- Proyectiles
- Elementos interactivos

### Características Técnicas

- Uso de dimensiones constantes.
- Renderizado selectivo.
- Ocultamiento del cursor.
- Actualización concurrente de entidades.
- Optimización mediante buffers de pantalla.

---

# Arquitectura del Proyecto

| Archivo | Responsabilidad |
|---|---|
| main.cpp | Punto de entrada del programa |
| GameLoop.cpp | Loop principal del juego |
| Renderer.cpp | Renderizado en consola |
| InputHandler.cpp | Captura de entradas |
| Physics.cpp | Movimiento y físicas |
| CollisionSystem.cpp | Detección de colisiones |
| EnemyManager.cpp | Gestión de enemigos |
| ProjectileManager.cpp | Gestión de proyectiles |
| HUD.cpp | Renderizado del HUD |
| ScoreManager.cpp | Manejo de puntajes |
| Level.cpp | Diseño y control del nivel |

---

# Librerías Utilizadas

| Librería | Uso | Compatibilidad |
|---|---|---|
| iostream | Entrada y salida en consola | Multiplataforma |
| cstdio | Renderizado y terminal | Multiplataforma |
| thread | Manejo de hilos | Multiplataforma |
| mutex | Sincronización de datos | Multiplataforma |
| chrono | Control de tiempo y FPS | Multiplataforma |
| cstring | Manejo de buffers | Multiplataforma |

---

# Funciones Principales

| Función | Archivo | Propósito |
|---|---|---|
| `render()` | Renderer.cpp | Dibuja todos los elementos del juego |
| `renderMenu()` | Renderer.cpp | Muestra el menú principal |
| `renderInstructions()` | Renderer.cpp | Despliega instrucciones |
| `renderScores()` | Renderer.cpp | Muestra puntajes guardados |
| `drawHUD()` | Renderer.cpp | Renderiza el HUD |
| `run()` | GameLoop.cpp | Ejecuta el loop principal |
| `processInput()` | InputHandler.cpp | Procesa entradas del jugador |
| `checkAll()` | CollisionSystem.cpp | Gestiona colisiones |
| `threadFunc()` | HUD.cpp | Actualiza información del HUD |

---

# Compilación y Ejecución

## Compilar

```bash
make
```

## Eliminar compilación

```bash
make clean
```

---

## Ejecutar

```bash
./game
```

---

# Controles

| Tecla | Acción |
|---|---|
| A | Mover izquierda |
| D | Mover derecha |
| W | Saltar |
| SPACE | Disparar |
| P | Pausa |
| ESC | Salir |

---

# Sistema Concurrente

El proyecto utiliza programación concurrente para:

- Actualización del HUD.
- Movimiento independiente de enemigos.
- Gestión de proyectiles.
- Renderizado continuo.
- Control del game loop.

Esto permite mantener una experiencia fluida en tiempo real.

---

# Sistema de Puntajes

Los puntajes se almacenan en:

```text
scores.txt
```

El sistema:

- Guarda puntuaciones automáticamente.
- Permite visualizar mejores resultados.
- Mantiene persistencia entre ejecuciones.

---

# Diseño Retro

El proyecto busca recrear la estética clásica de NES mediante:

- ASCII-Art.
- Animaciones en consola.
- Renderizado textual.
- Interfaces minimalistas.
- Colores y separación visual.

---

# Repositorio

Repositorio oficial del proyecto:

```text
https://github.com/Hola2212/kid_ikarus_game_in_console.git
```

---

# Problemas Conocidos y Mejoras Pendientes

Actualmente el proyecto presenta algunos errores y comportamientos pendientes de corregir:

- Problema al atravesar plataformas:
  - Al presionar la tecla `A`, el jugador puede atravesar plataformas, pero el sistema no detiene correctamente la colisión.

- Problema con los proyectiles:
  - Los proyectiles detectan colisión con el jugador, pero actualmente no aplican daño.

- Problema con el sistema de puntaje:
  - El puntaje no incrementa al eliminar enemigos.

- Problema con el movimiento aéreo:
  - La relación entre velocidad horizontal y vertical no está balanceada.
  - La velocidad vertical debería ser menor en proporción con la horizontal para facilitar alcanzar plataformas.

- Validación incompleta:
  - No se comprobó completamente el funcionamiento total de la Fase 2 ni de la Fase 3 del proyecto.

---

# Conclusiones

- El uso de ASCII-Art permitió construir un entorno visual retro funcional.
- La programación concurrente mejoró la fluidez del sistema.
- La separación modular facilita el mantenimiento y escalabilidad.
- El renderizado por buffers optimiza la experiencia visual en consola.
- El proyecto demuestra la integración de estructuras concurrentes, renderizado y control de videojuegos en C++.

---

# Autores

| Nombre | Carné |
|---|---|
| Álvarez Lester | 25196 |
| Juan Pablo Flores | 25454 |
| Esteban Sánchez | 25213 |
| Javier Sánchez | 25341 |

---

# Curso

**CC3086 - Programación de Microprocesadores**  
Universidad del Valle de Guatemala  
Ciclo 2 - 2025 · Sección 30