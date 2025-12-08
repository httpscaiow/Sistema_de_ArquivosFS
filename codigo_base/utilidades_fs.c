// codigo_base/utilidades_fs.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "sistema_arq.h"


// ******************************************************
// TABELAS DE SEGURANÇA (Declaradas extern em sistema_arq.h)
// ******************************************************
// Estas são variáveis externas
extern Usuario TABELA_USUARIOS[MAX_USUARIOS];
extern Grupo TABELA_GRUPOS[MAX_GRUPOS];
extern uint8_t ID_GRUPO_ATUAL;


// ******************************************************
// FUNÇÕES DE SEGURANÇA E PERMISSÃO
// ******************************************************

/**
 * @brief Inicializa as tabelas de Usuários e Grupos.
 * Configura o usuário 'root' (ID 1) e o grupo 'users' (ID 1).
 */
void inicializar_seguranca() {
    // 1. Inicializar Tabela de Grupos
    memset(TABELA_GRUPOS, 0, sizeof(Grupo) * MAX_GRUPOS);
    
    // Grupo 1: 'users' (padrão)
    TABELA_GRUPOS[GRUPO_DEFAULT].id_grupo = GRUPO_DEFAULT;
    strncpy(TABELA_GRUPOS[GRUPO_DEFAULT].nome, "users", 15);

    // 2. Inicializar Tabela de Usuários
    memset(TABELA_USUARIOS, 0, sizeof(Usuario) * MAX_USUARIOS);
    
    // Usuário 1: 'root' (Administrador/Proprietário do FS)
    TABELA_USUARIOS[ID_USUARIO_PROPRIETARIO].id_usuario = ID_USUARIO_PROPRIETARIO;
    strncpy(TABELA_USUARIOS[ID_USUARIO_PROPRIETARIO].nome, "root", 15);
    TABELA_USUARIOS[ID_USUARIO_PROPRIETARIO].id_grupo_principal = GRUPO_DEFAULT;
    
    printf("[SegOp] Seguranca inicializada. Usuario Atual: %s (ID %d, Grupo %d).\n", 
           TABELA_USUARIOS[ID_USUARIO_ATUAL].nome, ID_USUARIO_ATUAL, ID_GRUPO_ATUAL);
}

/**
 * @brief Verifica se o usuário atual tem a permissão necessária (R, W ou X).
 * @param inode O Inode do arquivo/diretório.
 * @param perm_necessaria A permissão a ser verificada (PERM_R, PERM_W, PERM_X).
 * @return 1 se a permissão for concedida, 0 caso contrário.
 */
int verificar_permissao(const Inode *inode, uint8_t perm_necessaria) {
    uint16_t permissoes = inode->permissoes;
    uint8_t id_usuario_atual = ID_USUARIO_ATUAL; 
    uint8_t id_grupo_atual = ID_GRUPO_ATUAL;

    // 1. É o PROPRIETÁRIO?
    if (id_usuario_atual == inode->id_proprietario) {
        uint16_t perm_proprietario = (permissoes >> SHIFT_PROPRIETARIO) & 0b111;
        if (perm_proprietario & perm_necessaria) {
            return 1; // Permissão concedida
        }
    }

    // 2. É do GRUPO? 
    // Simplificação: Verifica se o grupo principal do usuário atual é o mesmo grupo principal do proprietário do arquivo.
    if (id_grupo_atual == TABELA_USUARIOS[inode->id_proprietario].id_grupo_principal) {
        uint16_t perm_grupo = (permissoes >> SHIFT_GRUPO) & 0b111;
        if (perm_grupo & perm_necessaria) {
            return 1; // Permissão concedida
        }
    }

    // 3. É OUTROS?
    uint16_t perm_outros = (permissoes >> SHIFT_OUTROS) & 0b111;
    if (perm_outros & perm_necessaria) {
        return 1; // Permissão concedida
    }

    printf("[Perm] ACESSO NEGADO (User #%d nao tem %c para Inode #%d).\n", 
           id_usuario_atual, 
           (perm_necessaria == PERM_R) ? 'R' : (perm_necessaria == PERM_W) ? 'W' : 'X', 
           inode->id_inode);
    return 0; // Permissão negada
}

/**
 * @brief Altera o campo de permissões de um Inode (comando chmod).
 * Requer que o usuário atual seja o proprietário ou o ROOT.
 * @param id_inode ID do Inode a ser modificado.
 * @param novas_permissoes O valor octal das novas permissões (ex: 0755).
 * @return 0 em caso de sucesso, -1 em caso de erro.
 */
int modificar_permissoes(uint32_t id_inode, uint16_t novas_permissoes) {
    if (id_inode == 0 || id_inode >= MAX_INODES || TABELA_INODES_MEMORIA.tabela[id_inode].tipo == 0) {
        printf("[PermOp] ERRO: Inode invalido ou nao alocado #%d.\n", id_inode);
        return -1;
    }

    Inode *inode = &TABELA_INODES_MEMORIA.tabela[id_inode];

    // O comando CHMOD só pode ser executado pelo Proprietário ou Root
    if (ID_USUARIO_ATUAL != inode->id_proprietario && ID_USUARIO_ATUAL != ID_USUARIO_PROPRIETARIO) {
        printf("[PermOp] ERRO: Apenas o Proprietario (ID %d) ou Root (ID %d) pode usar chmod.\n", 
               inode->id_proprietario, ID_USUARIO_PROPRIETARIO);
        return -1;
    }

    // Aplica as novas permissões (apenas os 9 bits menos significativos são usados)
    inode->permissoes = novas_permissoes & 0b111111111; 
    inode->tempo_modificacao = time(NULL);

    printf("[PermOp] Permissoes do Inode #%d alteradas para %o (octal).\n", 
           id_inode, inode->permissoes);
    return 0;
}

/**
 * @brief Converte o valor numérico (octal) das permissões em uma string simbólica (rwxr-xr-x).
 * @param permissoes O valor uint16_t das permissões.
 * @return Uma string estática formatada.
 */
char *formatar_permissoes(uint16_t permissoes) {
    // Buffer estático para armazenar a string de permissões.
    // O tamanho é 9 (rwxrwxrwx) + 1 (null terminator) = 10.
    static char str_perm[10]; 
    memset(str_perm, '-', 9);
    str_perm[9] = '\0';

    // Máscaras de bits (r=4, w=2, x=1)
    uint8_t masks[3] = {SHIFT_PROPRIETARIO, SHIFT_GRUPO, SHIFT_OUTROS};
    
    for (int i = 0; i < 3; i++) {
        uint8_t shift = masks[i];
        
        // Extrai o trio de bits (rwx) para Proprietário, Grupo ou Outros
        uint8_t p = (permissoes >> shift) & 0b111; 
        
        int offset = i * 3; // 0, 3, 6
        
        // Verifica R (4)
        if (p & PERM_R) str_perm[offset + 0] = 'r';
        
        // Verifica W (2)
        if (p & PERM_W) str_perm[offset + 1] = 'w';
        
        // Verifica X (1)
        if (p & PERM_X) str_perm[offset + 2] = 'x';
    }

    return str_perm;
}

// ******************************************************
// FUNÇÕES UTILITÁRIAS DE BAIXO NÍVEL E FORMATAÇÃO
// ******************************************************

/**
 * @brief Inicializa os campos básicos de um Inode recém-alocado.
 */
void inicializar_inode(uint32_t id_inode, uint8_t tipo, uint16_t perm) {
    Inode *i = &TABELA_INODES_MEMORIA.tabela[id_inode];

    // Garante que o Inode esteja limpo antes de inicializar
    memset(i, 0, sizeof(Inode)); 

    i->id_inode = id_inode;
    i->tipo = tipo;
    i->permissoes = perm;
    i->id_proprietario = ID_USUARIO_ATUAL; 
    i->tamanho = 0;
    i->tempo_criacao = time(NULL);
    i->tempo_modificacao = time(NULL);
    // blocos_dados já zerados pelo memset
}


/**
 * @brief Busca o primeiro Inode livre na tabela.
 * @return O ID do Inode livre (>= 1) ou 0 em caso de falha.
 */
uint32_t alocar_inode() {
    for (uint32_t i = 1; i < MAX_INODES; i++) {
        // Verifica se o Inode está livre (tipo = 0)
        if (TABELA_INODES_MEMORIA.tabela[i].tipo == 0) {
            printf("[InodeOp] Novo Inode #%d alocado.\n", i);
            return i;
        }
    }
    printf("[InodeOp] ERRO: Tabela de Inodes cheia.\n");
    return 0; // Falha na alocação
}

/**
 * @brief Simula a formatação do disco, inicializando Superbloco, Bitmap, e Inode Raiz.
 * @return 0 em caso de sucesso.
 */
int formatar_disco(const char *caminho) {
    FILE *disco = fopen(caminho, "wb");
    if (!disco) {
        perror("Erro ao abrir/criar o arquivo de disco");
        return -1;
    }
    
    // 0. Inicializa Tabela de Inodes, Bitmap, e Seguranca na memória
    memset(&TABELA_INODES_MEMORIA, 0, sizeof(TabelaInodes));
    memset(BITMAP_BLOCOS_MEMORIA, 0, TAMANHO_BITMAP_BLOCOS); 
    
    // Inicializa Usuários e Grupos
    inicializar_seguranca(); 

    // 1. Preenche o disco com zeros (4MB)
    printf("Preenchendo o disco virtual com zeros... (%d bytes)\n", TAMANHO_DISCO_MB * 1024 * 1024);
    char zero_bloco[TAMANHO_BLOCO] = {0};
    // Corrigido para usar MAX_BLOCOS
    for (uint32_t i = 0; i < MAX_BLOCOS; i++) { 
        fwrite(zero_bloco, 1, TAMANHO_BLOCO, disco);
    }
    rewind(disco);

    // 2. Inicializa o Superbloco
    Superbloco sb = {
        .magic_number = 0xDEADC0DE,
        .total_blocos = MAX_BLOCOS,
        .blocos_livres = MAX_BLOCOS - 2, 
        .bloco_dir_raiz = 1 
    };

    // 3. Escreve o Superbloco na primeira posição
    printf("Escrevendo o Superbloco e Metadados iniciais...\n");
    fwrite(&sb, sizeof(Superbloco), 1, disco);

    // 4. Inicializa o primeiro Inode (ID #1) para ser o diretório raiz
    // Permissão: rwxr-xr-x (0755)
    uint16_t permissoes_raiz = (PERM_R | PERM_W | PERM_X) << SHIFT_PROPRIETARIO | 
                              (PERM_R | PERM_X) << SHIFT_GRUPO | 
                              (PERM_R | PERM_X) << SHIFT_OUTROS;
                              
    inicializar_inode(1, TIPO_DIRETORIO, permissoes_raiz); 

    // 5. Inicializa o Diretório Raiz com entradas '.' e '..'
    inicializar_diretorio_raiz(1); 
    
    // 6. Marca Blocos de Metadados/Raiz como ocupados no Bitmap
    BITMAP_BLOCOS_MEMORIA[0] |= 0x01; 
    BITMAP_BLOCOS_MEMORIA[1 / 8] |= (1 << (1 % 8)); 
    printf("[BlkOp] Blocos #0 e #1 marcados como ocupados (Metadados/Raiz).\n"); 

    // 7. Escreve a Tabela de Inodes no disco (simulação)
    fseek(disco, TAMANHO_BLOCO, SEEK_SET); 
    fwrite(&TABELA_INODES_MEMORIA, sizeof(TabelaInodes), 1, disco);

    fclose(disco);

    printf("Disco formatado com sucesso! Arquivo: %s | Inode Raiz: #%d\n", caminho, 1);
    return 0;
}