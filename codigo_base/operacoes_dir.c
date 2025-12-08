// codigo_base/operacoes_dir.c

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "sistema_arq.h"

// ******************************************************
// FUNÇÕES AUXILIARES DE DIRETÓRIO
// ******************************************************

/**
 * @brief Busca uma entrada em um Inode de diretório.
 * @param id_inode_dir O ID do Inode do diretório pai.
 * @param nome O nome do arquivo/diretório a ser buscado.
 * @return O ID do Inode encontrado (ou 0 se não encontrado).
 */
uint32_t buscar_entrada(uint32_t id_inode_dir, const char *nome) {
    if (id_inode_dir == 0) return 0;
    
    // Simulação: o bloco do diretório é acessado diretamente (usando o primeiro ponteiro)
    uint32_t id_bloco = TABELA_INODES_MEMORIA.tabela[id_inode_dir].blocos_dados[0];
    EntradaDiretorio *bloco_dir = (EntradaDiretorio *)BLOCOS_DADOS_SIMULACAO[id_bloco];

    if (id_bloco == 0) return 0; // Bloco não alocado

    for (int i = 0; i < ENTRADAS_POR_BLOCO; i++) {
        if (bloco_dir[i].id_inode != 0 && strcmp(bloco_dir[i].nome_arquivo, nome) == 0) {
            return bloco_dir[i].id_inode;
        }
    }
    return 0; // Não encontrado
}

/**
 * @brief Inicializa o diretório raiz com entradas '.' e '..'.
 */
void inicializar_diretorio_raiz(uint32_t id_inode_raiz) {
    // Aloca um bloco para os dados do diretório
    uint32_t id_bloco_dir = alocar_bloco();
    if (id_bloco_dir == 0) {
        fprintf(stderr, "ERRO FATAL: Nao foi possivel alocar bloco para o diretorio raiz.\n");
        exit(1);
    }
    
    Inode *raiz = &TABELA_INODES_MEMORIA.tabela[id_inode_raiz];
    raiz->blocos_dados[0] = id_bloco_dir;

    EntradaDiretorio *bloco_dir = (EntradaDiretorio *)BLOCOS_DADOS_SIMULACAO[id_bloco_dir];
    memset(bloco_dir, 0, TAMANHO_BLOCO); // Limpa o bloco
    
    // Entrada '.'
    strncpy(bloco_dir[0].nome_arquivo, ".", NOME_ARQ_MAX);
    bloco_dir[0].id_inode = id_inode_raiz;
    printf("[DirOp] Entrada adicionada no Diretorio #%d: . -> Inode #%d\n", id_inode_raiz, id_inode_raiz);

    // Entrada '..'
    strncpy(bloco_dir[1].nome_arquivo, "..", NOME_ARQ_MAX);
    bloco_dir[1].id_inode = id_inode_raiz;
    printf("[DirOp] Entrada adicionada no Diretorio #%d: .. -> Inode #%d\n", id_inode_raiz, id_inode_raiz);
    
    raiz->tamanho = sizeof(EntradaDiretorio) * 2; // Tamanho inicial com '.' e '..'
    printf("[DirOp] Diretorio Raiz (#%d) inicializado com entradas '.' e '..'.\n", id_inode_raiz);
}

/**
 * @brief Adiciona uma entrada (DirEntry) em um Inode de diretório.
 * @return 0 em caso de sucesso.
 */
int adicionar_entrada_diretorio(uint32_t id_inode_dir, const char *nome, uint32_t novo_id_inode) {
    if (id_inode_dir == 0 || novo_id_inode == 0) return -1;

    Inode *dir_inode = &TABELA_INODES_MEMORIA.tabela[id_inode_dir];
    if (dir_inode->tipo != TIPO_DIRETORIO) {
        printf("[DirOp] ERRO: Inode #%d nao e um diretorio.\n", id_inode_dir);
        return -1;
    }

    // O bloco de dados do diretório é o primeiro bloco apontado pelo Inode
    uint32_t id_bloco_dir = dir_inode->blocos_dados[0];
    EntradaDiretorio *bloco_dir = (EntradaDiretorio *)BLOCOS_DADOS_SIMULACAO[id_bloco_dir];
    
    // 1. Verificar se o nome já existe
    if (buscar_entrada(id_inode_dir, nome) != 0) {
        printf("[DirOp] ERRO: Entrada '%s' ja existe no diretorio #%d.\n", nome, id_inode_dir);
        return -1;
    }
    
    // 2. Procura por uma posição vazia
    for (int i = 0; i < ENTRADAS_POR_BLOCO; i++) {
        if (bloco_dir[i].id_inode == 0) {
            // Verifica permissão de escrita antes de adicionar
            if (!verificar_permissao(dir_inode, PERM_W)) {
                printf("[DirOp] ERRO: Permissao negada para escrever no diretorio #%d.\n", id_inode_dir);
                return -1;
            }
            
            strncpy(bloco_dir[i].nome_arquivo, nome, NOME_ARQ_MAX);
            bloco_dir[i].id_inode = novo_id_inode;
            
            // Atualiza tamanho e tempo de modificação do diretório pai
            dir_inode->tamanho += sizeof(EntradaDiretorio);
            dir_inode->tempo_modificacao = time(NULL);
            
            printf("[DirOp] Entrada adicionada no Diretorio #%d: %s -> Inode #%d\n", 
                   id_inode_dir, nome, novo_id_inode);
            return 0;
        }
    }

    printf("[DirOp] ERRO: Diretorio #%d cheio.\n", id_inode_dir);
    return -1;
}

/**
 * @brief Remove uma entrada de um diretório.
 * @return O ID do Inode que foi removido (ou 0 se falhar).
 */
uint32_t remover_entrada_diretorio(uint32_t id_inode_dir, const char *nome) {
    if (id_inode_dir == 0) return 0;
    
    Inode *dir_inode = &TABELA_INODES_MEMORIA.tabela[id_inode_dir];
    
    // 1. Verificar permissão de escrita no diretório pai
    if (!verificar_permissao(dir_inode, PERM_W)) {
        printf("[DirOp] ERRO: Permissao negada para remover entrada do diretorio #%d.\n", id_inode_dir);
        return 0;
    }
    
    uint32_t id_bloco_dir = dir_inode->blocos_dados[0];
    EntradaDiretorio *bloco_dir = (EntradaDiretorio *)BLOCOS_DADOS_SIMULACAO[id_bloco_dir];
    
    // 2. Nomes especiais não podem ser removidos
    if (strcmp(nome, ".") == 0 || strcmp(nome, "..") == 0) {
        printf("[DirOp] ERRO: Nao e possivel remover as entradas '.' ou '..'.\n");
        return 0;
    }
    
    // 3. Procura a entrada
    for (int i = 0; i < ENTRADAS_POR_BLOCO; i++) {
        if (bloco_dir[i].id_inode != 0 && strcmp(bloco_dir[i].nome_arquivo, nome) == 0) {
            uint32_t id_inode_removido = bloco_dir[i].id_inode;
            
            // 4. Limpa a entrada do diretório
            memset(&bloco_dir[i], 0, sizeof(EntradaDiretorio));
            
            // Atualiza tamanho e tempo de modificação do diretório pai
            dir_inode->tamanho -= sizeof(EntradaDiretorio);
            dir_inode->tempo_modificacao = time(NULL);

            printf("[DirOp] Entrada de diretorio '%s' removida do Inode #%d.\n", nome, id_inode_dir);
            return id_inode_removido;
        }
    }
    
    printf("[DirOp] ERRO: Entrada '%s' nao encontrada no diretorio #%d.\n", nome, id_inode_dir);
    return 0;
}


// ******************************************************
// COMANDOS DE DIRETÓRIO
// ******************************************************

/**
 * @brief Cria um novo diretório (comando mkdir).
 */
int criar_diretorio(const char *nome) {
    if (strlen(nome) >= NOME_ARQ_MAX) {
        printf("[DirOp] ERRO: Nome do diretorio muito longo.\n");
        return -1;
    }
    
    // 1. Verificar permissão de escrita no diretório pai
    Inode *dir_pai_inode = &TABELA_INODES_MEMORIA.tabela[ID_DIRETORIO_ATUAL];
    if (!verificar_permissao(dir_pai_inode, PERM_W)) {
        // A mensagem de erro já é tratada por verificar_permissao.
        return -1;
    }

    uint32_t novo_id_inode = alocar_inode();
    if (novo_id_inode == 0) return -1;

    // Permissões padrão para diretório: rwxr-xr-x (0755)
    uint16_t permissoes_padrao = (PERM_R | PERM_W | PERM_X) << SHIFT_PROPRIETARIO | 
                                 (PERM_R | PERM_X) << SHIFT_GRUPO | 
                                 (PERM_R | PERM_X) << SHIFT_OUTROS;
                                 
    inicializar_inode(novo_id_inode, TIPO_DIRETORIO, permissoes_padrao);
    
    // Aloca o primeiro bloco para as entradas do diretório
    uint32_t id_bloco_dir = alocar_bloco();
    if (id_bloco_dir == 0) return -1;
    TABELA_INODES_MEMORIA.tabela[novo_id_inode].blocos_dados[0] = id_bloco_dir;
    
    // Inicializa o novo diretório com '.' e '..'
    EntradaDiretorio *bloco_dir = (EntradaDiretorio *)BLOCOS_DADOS_SIMULACAO[id_bloco_dir];
    memset(bloco_dir, 0, TAMANHO_BLOCO);
    
    // Entrada '.'
    strncpy(bloco_dir[0].nome_arquivo, ".", NOME_ARQ_MAX);
    bloco_dir[0].id_inode = novo_id_inode;

    // Entrada '..' (Aponta para o pai)
    strncpy(bloco_dir[1].nome_arquivo, "..", NOME_ARQ_MAX);
    bloco_dir[1].id_inode = ID_DIRETORIO_ATUAL; // ID do diretório pai

    TABELA_INODES_MEMORIA.tabela[novo_id_inode].tamanho = sizeof(EntradaDiretorio) * 2;
    
    // Adiciona a entrada do novo diretório no diretório pai
    int resultado = adicionar_entrada_diretorio(ID_DIRETORIO_ATUAL, nome, novo_id_inode);

    if (resultado == 0) {
        printf("[DirOp] Diretorio '%s' criado com sucesso (Inode #%d).\n", nome, novo_id_inode);
        return 0;
    } else {
        printf("[DirOp] ERRO: Falha ao adicionar entrada no diretorio pai.\n");
        // Em caso de falha, seria necessário liberar o Inode e o bloco alocados.
        return -1;
    }
}

/**
 * @brief Muda o diretório atual (comando cd).
 */
int mudar_diretorio(const char *nome) {
    uint32_t id_novo_dir = buscar_entrada(ID_DIRETORIO_ATUAL, nome);

    if (id_novo_dir == 0) {
        printf("[DirOp] ERRO: Diretorio '%s' nao encontrado.\n", nome);
        return -1;
    }

    Inode *novo_dir_inode = &TABELA_INODES_MEMORIA.tabela[id_novo_dir];

    if (novo_dir_inode->tipo != TIPO_DIRETORIO) {
        printf("[DirOp] ERRO: '%s' nao e um diretorio.\n", nome);
        return -1;
    }
    
    // 1. Verificar permissão de Execução (acesso/travessia)
    if (!verificar_permissao(novo_dir_inode, PERM_X)) {
        // A mensagem de erro já é tratada por verificar_permissao.
        return -1;
    }

    ID_DIRETORIO_ATUAL = id_novo_dir;
    printf("[DirOp] Diretorio atual alterado para Inode #%d.\n", ID_DIRETORIO_ATUAL);
    return 0;
}

/**
 * @brief Lista o conteúdo do diretório atual (comando ls).
 * Implementa formatação tabular e exibição simbólica de permissões.
 */
int listar_diretorio(uint32_t id_inode_dir) {
    Inode *dir_inode = &TABELA_INODES_MEMORIA.tabela[id_inode_dir];

    // 1. Verificar permissão de Leitura
    if (!verificar_permissao(dir_inode, PERM_R)) {
        // A mensagem de erro já é tratada por verificar_permissao.
        return -1;
    }
    
    uint32_t id_bloco_dir = dir_inode->blocos_dados[0];
    EntradaDiretorio *bloco_dir = (EntradaDiretorio *)BLOCOS_DADOS_SIMULACAO[id_bloco_dir];

    if (dir_inode->tipo != TIPO_DIRETORIO) {
        printf("[DirOp] ERRO: Inode #%d nao e um diretorio.\n", id_inode_dir);
        return -1;
    }

    printf("\n");
    printf("Tipo | Permissões | Proprietário | Tamanho (Bytes) | Inode # | Nome\n");
    printf("-----|------------|--------------|-----------------|---------|--------------------\n");
    
    for (int i = 0; i < ENTRADAS_POR_BLOCO; i++) {
        if (bloco_dir[i].id_inode != 0) {
            Inode *item_inode = &TABELA_INODES_MEMORIA.tabela[bloco_dir[i].id_inode];
            
            // 2. Define o tipo (DIR/ARQ)
            char *tipo_str = (item_inode->tipo == TIPO_DIRETORIO) ? "DIR" : "ARQ";
            
            // 3. Obtém a permissão formatada (rwxrwxrwx)
            char *perm_str = formatar_permissoes(item_inode->permissoes);
            
            // 4. Obtém o nome do proprietário (simplificação, assumindo IDs válidos)
            char *proprietario_nome = (TABELA_USUARIOS[item_inode->id_proprietario].id_usuario == item_inode->id_proprietario) ?
                                      TABELA_USUARIOS[item_inode->id_proprietario].nome : "---";

            // 5. Imprime a linha formatada
            printf("%-4s | %-10s | %-12s | %-15u | %-7u | %s\n", 
                   tipo_str, 
                   perm_str,
                   proprietario_nome,
                   item_inode->tamanho,
                   item_inode->id_inode, 
                   bloco_dir[i].nome_arquivo);
        }
    }
    printf("\n");
    return 0;
}