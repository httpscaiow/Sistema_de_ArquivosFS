# Define o nome do executável final
TARGET = simulador_fs

# Define os arquivos fonte (.c)
SRCS = codigo_base/main.c \
       codigo_base/utilidades_fs.c \
       codigo_base/operacoes_arq.c \
       codigo_base/operacoes_dir.c

# Define os arquivos objeto (.o)
OBJS = $(SRCS:.c=.o)

# Flags do compilador: -I aponta para o diretório de cabeçalhos
CC = gcc
CFLAGS = -g -Wall -Idefinicoes

# Regra padrão: compilar o executável
all: $(TARGET)

# Como construir o alvo final (o executável)
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET)

# Regra para compilar arquivos .c em .o
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Limpa arquivos compilados (.o e o executável)
clean:
	rm -f $(TARGET) $(OBJS)
	rm -f my_disk.img
