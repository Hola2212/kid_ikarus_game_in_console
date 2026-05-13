# Investigado y colocado para realizar compilacion con solo colocar make
# Compilador
CXX = g++
#Daba error
#CXXFLAGS = -std=c++17 -Wall -Iinclude
CXXFLAGS = -std=c++20 -Wall -Iinclude

# Carpetas
SRC_DIR = src
OBJ_DIR = obj
BIN = game

# Archivos fuente
SRCS = $(wildcard $(SRC_DIR)/*.cpp)

# Archivos objeto
OBJS = $(patsubst $(SRC_DIR)/%.cpp,$(OBJ_DIR)/%.o,$(SRCS))

# Regla principal
all: $(BIN)

# Linkeo final
$(BIN): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(BIN) $(OBJS) -lpthread

# Compilar cada .cpp → .o
$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Limpiar
clean:
	rm -rf $(OBJ_DIR) $(BIN)

# Ejecutar
run: all
	./$(BIN)