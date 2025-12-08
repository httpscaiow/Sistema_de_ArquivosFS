#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <time.h> 
#include "sistema_arq.h"

// FUNÇÕES DE GERENCIAMENTO DE BLOCOS (BITMAP)
/**
 * Aloca o primeiro bloco de dados livre (utilizando o Bitmap).
 * Percorre o BITMAP_BLOCOS_MEMORIA a partir do ID 2, verificando e marcando o bit correspondente.
  O ID do bloco alocado (>= 2), ou 0 se o disco estiver cheio.
 */

int alocar_bloco() {
    // Bloco 0 e 1 são reservados (Superbloco/Inodes). Começamos a alocar do bloco 2.
    for (uint32_t i = 2; i < MAX_BLOCOS; i++) {
        uint32_t byte_index = i / 8;
        uint32_t bit_index = i % 8;
        // Verifica se o bit está LIVRE (0)
        // Se o bit na posição 'bit_index' for 0, o resultado do AND será 0.
        if (!(BITMAP_BLOCOS_MEMORIA[byte_index] & (1 << bit_index))) {// Marca o bit como OCUPADO (1)
            // Usa OR bit a bit para setar o bit específico para 1.
            BITMAP_BLOCOS_MEMORIA[byte_index] |= (1 << bit_index);
    
            printf("[BlkOp] Bloco #%d alocado com sucesso.\n", i);
            return i;
        }
    }
    printf("[BlkOp] ERRO: Disco cheio. Nenhum bloco livre encontrado.\n");
    return 0;
}

/**
 *Libera um bloco de dados (limpa o bit no bitmap).
 *Utiliza a operação AND bit a bit com o complemento (~) para garantir que apenas o bit do bloco_id seja zerado.
 * id_bloco O ID do bloco a ser liberado.
 */
void liberar_bloco(uint32_t id_bloco) {
    if (id_bloco == 0 || id_bloco >= MAX_BLOCOS) return;

    uint32_t byte_index = id_bloco / 8;
    uint32_t bit_index = id_bloco % 8;
    // Limpa o bit (operação AND com o complemento do bit)
    // Ex: Se queremos zerar o 3º bit (1<<3), usamos ~(1<<3), que é 11110111.
    // O AND garante que todos os outros bits permaneçam inalterados.
    BITMAP_BLOCOS_MEMORIA[byte_index] &= ~(1 << bit_index);
    printf("[BlkOp] Bloco #%d liberado no bitmap.\n", id_bloco);
}

// FUNÇÕES DE CRIAÇÃO E DELEÇÃO DE ARQUIVOS
/**
 * Cria um novo arquivo (comando touch).
 * 1. Verifica permissão de escrita no diretório. 2. Aloca e inicializa Inode 3. Adiciona entrada no diretório pai.
 */
int criar_arquivo(const char *nome) {
    if (strlen(nome) >= NOME_ARQ_MAX) {
        printf("[ArqOp] ERRO: Nome do arquivo muito longo.\n");
        return -1;
    }
    // 1. Verificação de Permissão: O usuário deve ter permissão de Escrita (W) no diretório atual (diretório pai) para criar um novo arquivo.
    Inode *dir_pai_inode = &TABELA_INODES_MEMORIA.tabela[ID_DIRETORIO_ATUAL];
    if (!verificar_permissao(dir_pai_inode, PERM_W)) {
        // A função verificar_permissao já imprime a mensagem de erro.
        return -1;
    }

    uint32_t novo_id_inode = alocar_inode();
    if (novo_id_inode == 0) return -1;
    // Definição das Permissões Padrão para arquivo: rw-r--r-- (0644)
    uint16_t permissoes_padrao = (PERM_R | PERM_W) << SHIFT_PROPRIETARIO | 
                                 (PERM_R << SHIFT_GRUPO) | 
                                 (PERM_R << SHIFT_OUTROS);                        
    // 2. Inicialização do Inode: Define tipo, permissões e usuário atual como proprietário.
    inicializar_inode(novo_id_inode, TIPO_ARQUIVO, permissoes_padrao);
    printf("[ArqOp] Novo Inode #%d alocado e inicializado.\n", novo_id_inode);
    // 3. Adiciona a entrada no bloco de dados do diretório pai.
    int resultado = adicionar_entrada_diretorio(ID_DIRETORIO_ATUAL, nome, novo_id_inode);
    if (resultado == 0) {
        printf("[ArqOp] Arquivo '%s' criado com sucesso (Inode #%d).\n", nome, novo_id_inode);
        return 0;
    } else {
        printf("[ArqOp] ERRO: Falha ao adicionar entrada no diretório.\n");
        // Desfaz o Inode alocado, pois não pode ser referenciado.
        TABELA_INODES_MEMORIA.tabela[novo_id_inode].tipo = 0;
        return -1;
    }
}

/**
 * Libera o Inode, os blocos de dados e remove a entrada do diretório (comando rm).
 */
int deletar_arquivo(const char *nome) {
    // 1. Verificar permissão de escrita/remoção (W) no diretório pai
    Inode *dir_pai_inode = &TABELA_INODES_MEMORIA.tabela[ID_DIRETORIO_ATUAL];
    if (!verificar_permissao(dir_pai_inode, PERM_W)) {
        return -1;
    }
    // 2. Remove a entrada do diretório e obtem o ID do Inode que será liberado
    uint32_t id_inode_liberado = remover_entrada_diretorio(ID_DIRETORIO_ATUAL, nome);
    if (id_inode_liberado == 0) {
        return -1; 
    }
    Inode *inode_a_liberar = &TABELA_INODES_MEMORIA.tabela[id_inode_liberado];
    // 3. Liberar Blocos de Dados
    // Itera sobre todos os ponteiros do Inode (diretos, indiretos, etc.)
    for (int i = 0; i < MAX_PONTEIROS_BLOCOS; i++) {
        uint32_t id_bloco = inode_a_liberar->blocos_dados[i];
        if (id_bloco != 0) {
            liberar_bloco(id_bloco); // <-- Libera o bloco no Bitmap
        }
    }
    // 4. Libera o Inode (limpa toda a entrada na Tabela de Inodes, marcando-o como livre)
    memset(inode_a_liberar, 0, sizeof(Inode));
    printf("[ArqOp] Arquivo '%s' deletado com sucesso. Inode #%d liberado.\n", nome, id_inode_liberado);
    return 0;
}

// FUNÇÕES DE ESCRITA E LEITURA (I/O)

/**
 * Escreve conteúdo em um arquivo (comando echo/write_large).
 * Lógica Multi-Bloco:
 * 1. Calcula quantos blocos são necessários.
 * 2. Libera blocos antigos em excesso (se o novo tamanho for menor).
 * 3. Percorre os ponteiros: aloca novos blocos se necessário e copia o conteúdo em pedaços.
 * 4. Atualiza o Inode (tamanho e tempo).
 */
int escrever_arquivo(const char *nome, const char *conteudo) {
    uint32_t id_inode = buscar_entrada(ID_DIRETORIO_ATUAL, nome);
    size_t tamanho_conteudo = strlen(conteudo);
    
    if (id_inode == 0) {
        printf("[ArqOp] ERRO: Arquivo '%s' nao encontrado.\n", nome);
        return -1;
    }

    Inode *inode = &TABELA_INODES_MEMORIA.tabela[id_inode];
    // 1. Verificação de Permissão de Escrita (W)
    if (!verificar_permissao(inode, PERM_W)) {
        return -1;
    }
    // 2. Cálculo dos Blocos Necessários e Verificação de Limite
    // Cálculo Ceiling: (tamanho + TAMANHO_BLOCO - 1) / TAMANHO_BLOCO
    uint32_t blocos_necessarios = (uint32_t)((tamanho_conteudo + TAMANHO_BLOCO - 1) / TAMANHO_BLOCO);
    if (blocos_necessarios > MAX_PONTEIROS_BLOCOS) {
        printf("[ArqOp] ERRO: Conteudo muito grande (%zu bytes). Limite de %d blocos.\n", 
               tamanho_conteudo, MAX_PONTEIROS_BLOCOS);
        return -1;
    }
    // 3. Libera blocos antigos em excesso
    // Se o arquivo tinha 4 blocos e agora precisa de 2, os ponteiros 2 e 3 são zerados.
    for (uint32_t i = blocos_necessarios; i < MAX_PONTEIROS_BLOCOS; i++) {
        if (inode->blocos_dados[i] != 0) {
            liberar_bloco(inode->blocos_dados[i]);
            inode->blocos_dados[i] = 0; // Ponteiro zerado.
        }
    }
    // 4. Alocação ou Reuso e Escrita dos Dados
    const char *ptr_conteudo = conteudo;
    for (uint32_t i = 0; i < blocos_necessarios; i++) {
        uint32_t id_bloco = inode->blocos_dados[i];
        // Se o ponteiro for 0, alocamos um novo bloco (Escrita em Arquivo Novo ou Aumento de Tamanho)
        if (id_bloco == 0) {
            id_bloco = alocar_bloco();
            if (id_bloco == 0) {
                printf("[ArqOp] ERRO: Sem blocos livres para alocar bloco #%d.\n", i);
                // Retornar aqui sem liberar os blocos já escritos causa vazamento.
                return -1; 
            }
            inode->blocos_dados[i] = id_bloco;
        }
        // Determina quantos bytes serão escritos neste bloco (último bloco pode ser parcial)
        size_t bytes_a_escrever = (tamanho_conteudo > TAMANHO_BLOCO) ? TAMANHO_BLOCO : tamanho_conteudo;
        // Copia a parte do conteúdo para o bloco de dados simulado
        memcpy(BLOCOS_DADOS_SIMULACAO[id_bloco], ptr_conteudo, bytes_a_escrever);
        // Atualiza os ponteiros e o tamanho restante para a próxima iteração
        ptr_conteudo += bytes_a_escrever;
        tamanho_conteudo -= bytes_a_escrever;
    }
    // 5. Atualização do Inode (tamanho final e data/hora)
    inode->tamanho = (uint32_t)strlen(conteudo); // Tamanho em bytes
    inode->tempo_modificacao = time(NULL);
    printf("[ArqOp] Conteudo escrito em '%s' (Inode #%d). Tamanho: %u bytes, usando %u blocos.\n", 
           nome, id_inode, inode->tamanho, blocos_necessarios);
    return 0;
}
/**
 *Lê e exibe o conteúdo completo de um arquivo (comando cat).
 * Itera sobre os ponteiros do Inode e concatena o conteúdo na saída.
 */
int ler_arquivo(const char *nome) {
    uint32_t id_inode = buscar_entrada(ID_DIRETORIO_ATUAL, nome);
    
    if (id_inode == 0) {
        printf("[ArqOp] ERRO: Arquivo '%s' nao encontrado.\n", nome);
        return -1;
    }
    Inode *inode = &TABELA_INODES_MEMORIA.tabela[id_inode];
    // 1. Verificação de Permissão de Leitura (R)
    if (!verificar_permissao(inode, PERM_R)) {
        return -1;
    }
    if (inode->tipo != TIPO_ARQUIVO) {
        printf("[ArqOp] ERRO: '%s' nao e um arquivo (Tipo: %d).\n", nome, inode->tipo);
        return -1;
    }
    printf("\n--- Conteúdo de '%s' ---\n", nome);
    size_t bytes_restantes = inode->tamanho;
    // 2. Itera sobre todos os ponteiros de bloco (MAX_PONTEIROS_BLOCOS)
    for (int i = 0; i < MAX_PONTEIROS_BLOCOS; i++) {
        uint32_t id_bloco = inode->blocos_dados[i];
        if (id_bloco != 0 && bytes_restantes > 0) {
            // Calcula quantos bytes leremos neste bloco (o último bloco pode ser incompleto)
            size_t bytes_a_ler = (bytes_restantes > TAMANHO_BLOCO) ? TAMANHO_BLOCO : bytes_restantes;
            // Escreve diretamente no console (stdout) o conteúdo do bloco
            fwrite(BLOCOS_DADOS_SIMULACAO[id_bloco], 1, bytes_a_ler, stdout);
            bytes_restantes -= bytes_a_ler;
        } else if (id_bloco == 0 && bytes_restantes > 0) {
            // Checagem de integridade: Inode diz que o tamanho é maior, mas o ponteiro acabou.
            printf("\n[ArqOp] AVISO: Ponteiro de bloco %d faltando no Inode #%d (Dados perdidos).\n", i, id_inode);
            break;
        }
    }
    printf("\n-------------------------\n");
    return 0;
}

// FUNÇÕES DE MANIPULAÇÃO (CP/MV)

/**
 *Copia um arquivo (comando cp).
 * É uma operação de cópia profunda (deep copy): Aloca um novo Inode e aloca novos blocos de dados para o destino, copiando o conteúdo.
 */
int copiar_arquivo(const char *nome_origem, const char *nome_destino) {
    uint32_t id_inode_origem = buscar_entrada(ID_DIRETORIO_ATUAL, nome_origem);

    if (id_inode_origem == 0) {
        printf("[CpyOp] ERRO: Arquivo '%s' nao encontrado.\n", nome_origem);
        return -1;
    }
    // 1. Verificar permissão de leitura na origem (R)
    Inode *inode_origem = &TABELA_INODES_MEMORIA.tabela[id_inode_origem];
    if (!verificar_permissao(inode_origem, PERM_R)) {
        return -1;
    }
    // 2. Alocar novo Inode
    uint32_t id_inode_destino = alocar_inode();
    if (id_inode_destino == 0) {
        return -1;
    }
    // 3. Inicializar novo Inode
    inicializar_inode(id_inode_destino, inode_origem->tipo, inode_origem->permissoes);
    Inode *inode_destino = &TABELA_INODES_MEMORIA.tabela[id_inode_destino];
    // Cópia dos atRibutos de tamanho
    inode_destino->tamanho = inode_origem->tamanho;
    // 4. Copiar conteúdo dos blocos de dados (Itera sobre todos os ponteiros)
    for (int i = 0; i < MAX_PONTEIROS_BLOCOS; i++) {
        uint32_t id_bloco_origem = inode_origem->blocos_dados[i];
        if (id_bloco_origem != 0) {
            // Aloca um NOVO bloco para o destino (NÃO REUSA o bloco de origem!)
            uint32_t id_bloco_novo = alocar_bloco();
            if (id_bloco_novo == 0) {
                printf("[CpyOp] ERRO: Sem blocos livres para copiar dados (bloco %d).\n", i);
                // NOTA: Em um FS real, precisaria de uma rotina de rollback para liberar alocações parciais.
                TABELA_INODES_MEMORIA.tabela[id_inode_destino].tipo = 0; // Libera Inode
                return -1;
            }
            // Copia o conteúdo bloco por bloco
            memcpy(BLOCOS_DADOS_SIMULACAO[id_bloco_novo], 
                   BLOCOS_DADOS_SIMULACAO[id_bloco_origem], 
                   TAMANHO_BLOCO); 
            
            inode_destino->blocos_dados[i] = id_bloco_novo;
        }
    }
    // 5. Adicionar entrada no diretório atual
    if (adicionar_entrada_diretorio(ID_DIRETORIO_ATUAL, nome_destino, id_inode_destino) == 0) {
        printf("[CpyOp] Arquivo '%s' copiado para '%s' (Inode #%d).\n", 
               nome_origem, nome_destino, id_inode_destino);
        return 0;
    } else {
        printf("[CpyOp] ERRO: Falha ao adicionar entrada no diretorio para '%s'.\n", nome_destino);
        return -1;
    }
}

/**
 * Renomeia um arquivo/diretório no diretório atual (comando mv simplificado).
 * Lógica: Apenas altera a string de nome na entrada do diretório, sem tocar no Inode ou nos blocos de dados.
 */
int renomear_mover(const char *nome_origem, const char *nome_destino) {
    // Busca o bloco de dados do diretório atual (assumindo um único bloco de diretório)
    uint32_t id_bloco_dir = TABELA_INODES_MEMORIA.tabela[ID_DIRETORIO_ATUAL].blocos_dados[0];
    EntradaDiretorio *dir_entradas = (EntradaDiretorio *)BLOCOS_DADOS_SIMULACAO[id_bloco_dir];
    // 1. Verificar permissão de escrita no diretório pai (W) para alterar a entrada
    Inode *dir_inode = &TABELA_INODES_MEMORIA.tabela[ID_DIRETORIO_ATUAL];
    if (!verificar_permissao(dir_inode, PERM_W)) {
        printf("[MvOp] ERRO: Permissao negada para modificar o diretorio atual.\n");
        return -1;
    }
    // 2. Percorrer o bloco de dados do diretório para encontrar a entrada de origem
    for (int i = 0; i < ENTRADAS_POR_BLOCO; i++) {
        if (dir_entradas[i].id_inode != 0 && strcmp(dir_entradas[i].nome_arquivo, nome_origem) == 0) {
            // Entrada encontrada!
            // 3. Renomear (copia o novo nome)
            strncpy(dir_entradas[i].nome_arquivo, nome_destino, NOME_ARQ_MAX);
            dir_entradas[i].nome_arquivo[NOME_ARQ_MAX - 1] = '\0'; // Garantir NULL termination
            // 4. Atualiza o tempo de modificação do diretório, pois seu conteúdo mudou.
            dir_inode->tempo_modificacao = time(NULL);
            printf("[MvOp] '%s' renomeado para '%s' (Inode #%d).\n", 
                   nome_origem, nome_destino, dir_entradas[i].id_inode);
            return 0;
        }
    }
    printf("[MvOp] ERRO: Arquivo ou diretorio '%s' nao encontrado.\n", nome_origem);
    return -1;
}