// definicoes/sistema_arq.h

#include <stdint.h>
#include <time.h>

// --- DEFINIÇÕES DE TAMANHO E CONSTANTES ---
#define TAMANHO_DISCO_MB 4
#define TAMANHO_BLOCO 4096 
#define MAX_BLOCOS (TAMANHO_DISCO_MB * 1024 * 1024 / TAMANHO_BLOCO)
#define NUM_BLOCOS_DADOS MAX_BLOCOS
#define TAMANHO_BITMAP_BLOCOS (MAX_BLOCOS / 8)
#define NOME_ARQ_MAX 28
#define MAX_INODES 100
#define MAX_PONTEIROS_BLOCOS 4


// Garante que ENTRADAS_POR_BLOCO seja definido *após* EntradaDiretorio
#define TIPO_ARQUIVO 1
#define TIPO_DIRETORIO 2

// --- PERMISSÕES (Bitmasks) ---
#define PERM_R 0b100 
#define PERM_W 0b010 
#define PERM_X 0b001 

#define SHIFT_PROPRIETARIO 6
#define SHIFT_GRUPO 3
#define SHIFT_OUTROS 0

// --- CONSTANTES E VARIÁVEIS DE GRUPO E USUÁRIO ---
#define MAX_USUARIOS 10
#define MAX_GRUPOS 10
#define ID_USUARIO_PROPRIETARIO 1 // Root
#define NOME_USUARIO_MAX 16 // Tamanho do campo 'nome' na struct Usuario (15 + \0)
#define GRUPO_DEFAULT 1

// --- ESTRUTURAS DE USUÁRIO E GRUPO (Simulação) ---
typedef struct {
    uint8_t id_usuario;
    char nome[16];
    uint8_t id_grupo_principal; 
} Usuario;

typedef struct {
    uint8_t id_grupo;
    char nome[16];
} Grupo;

// --- ESTRUTURAS DO SISTEMA DE ARQUIVOS ---
typedef struct {
    uint32_t id_inode;
    uint8_t tipo;
    uint16_t permissoes;
    uint8_t id_proprietario;
    uint32_t tamanho;
    time_t tempo_criacao;
    time_t tempo_modificacao;
    uint32_t blocos_dados[MAX_PONTEIROS_BLOCOS];
} Inode;

typedef struct {
    uint32_t magic_number;
    uint32_t total_blocos;
    uint32_t blocos_livres;
    uint32_t bloco_dir_raiz;
} Superbloco;

typedef struct {
    char nome_arquivo[NOME_ARQ_MAX];
    uint32_t id_inode; 
} EntradaDiretorio;

// Tabela Inodes
typedef struct {
    Inode tabela[MAX_INODES];
} TabelaInodes; 

// Garante que a constante do bloco de diretório seja definida por último
#define ENTRADAS_POR_BLOCO (TAMANHO_BLOCO / sizeof(EntradaDiretorio))

// --- DECLARAÇÕES GLOBAIS EXTERN (Variáveis de estado) ---

extern TabelaInodes TABELA_INODES_MEMORIA; 
extern uint8_t ID_USUARIO_ATUAL; 
extern uint32_t ID_DIRETORIO_ATUAL; 
extern uint8_t BITMAP_BLOCOS_MEMORIA[TAMANHO_BITMAP_BLOCOS]; // Adicionado
extern char BLOCOS_DADOS_SIMULACAO[MAX_BLOCOS][TAMANHO_BLOCO];
extern Usuario TABELA_USUARIOS[MAX_USUARIOS];
extern Grupo TABELA_GRUPOS[MAX_GRUPOS];
extern uint8_t ID_GRUPO_ATUAL; 

// --- PROTÓTIPOS DAS FUNÇÕES ---

// Operações de Arquivos/Blocos
int alocar_bloco();
void liberar_bloco(uint32_t id_bloco);
void inicializar_inode(uint32_t id_inode, uint8_t tipo, uint16_t perm);
uint32_t alocar_inode();
int criar_arquivo(const char *nome);
int deletar_arquivo(const char *nome);
int escrever_arquivo(const char *nome, const char *conteudo);
int ler_arquivo(const char *nome);
int copiar_arquivo(const char *nome_origem, const char *nome_destino);   // <-- NOVO PROTÓTIPO
int renomear_mover(const char *nome_origem, const char *nome_destino); // <-- NOVO PROTÓTIPO

// Operações de Diretório
void inicializar_diretorio_raiz(uint32_t id_inode_raiz);
int adicionar_entrada_diretorio(uint32_t id_inode_dir, const char *nome, uint32_t novo_id_inode);
uint32_t remover_entrada_diretorio(uint32_t id_inode_dir, const char *nome);
uint32_t buscar_entrada(uint32_t id_inode_dir, const char *nome);
int criar_diretorio(const char *nome);
int mudar_diretorio(const char *nome);
int listar_diretorio(uint32_t id_inode_dir);

// Utilidades e Segurança
int formatar_disco(const char *caminho);
int verificar_permissao(const Inode *inode, uint8_t perm_necessaria);
int modificar_permissoes(uint32_t id_inode, uint16_t novas_permissoes); // <-- NOVO PROTÓTIPO
char *formatar_permissoes(uint16_t permissoes);