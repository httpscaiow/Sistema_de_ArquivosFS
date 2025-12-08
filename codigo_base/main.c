// codigo_base/main.c

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "sistema_arq.h"

// ******************************************************
// VARIÁVEIS GLOBAIS DE ESTADO DO FS
// As definições concretas das variáveis 'extern' estão aqui.
// ******************************************************

// Tabela Inodes
TabelaInodes TABELA_INODES_MEMORIA; 
// Variáveis de estado
uint8_t ID_USUARIO_ATUAL = ID_USUARIO_PROPRIETARIO; 
uint32_t ID_DIRETORIO_ATUAL = 1; 
uint8_t BITMAP_BLOCOS_MEMORIA[TAMANHO_BITMAP_BLOCOS] = {0}; 
// Simulação de dados (I/O)
char BLOCOS_DADOS_SIMULACAO[MAX_BLOCOS][TAMANHO_BLOCO] = {0}; 
// Tabelas de Segurança
Usuario TABELA_USUARIOS[MAX_USUARIOS];
Grupo TABELA_GRUPOS[MAX_GRUPOS];
uint8_t ID_GRUPO_ATUAL = GRUPO_DEFAULT; 

// ******************************************************
// FUNÇÃO PRINCIPAL
// ******************************************************

int main() {
    printf("### Simulador de Sistema de Arquivos (FS) ###\n");
    printf("Digite 'format' para iniciar o disco.\n");
    
    char comando[256];
    
    // Inicializa o nome do root e grupo padrão
    strncpy(TABELA_USUARIOS[ID_USUARIO_PROPRIETARIO].nome, "root", NOME_USUARIO_MAX);
    TABELA_USUARIOS[ID_USUARIO_PROPRIETARIO].id_usuario = ID_USUARIO_PROPRIETARIO;
    TABELA_USUARIOS[ID_USUARIO_PROPRIETARIO].id_grupo_principal = GRUPO_DEFAULT;
    
    while (1) {
        // --- PROMPT DE COMANDO ---
        char *nome_usuario = (TABELA_USUARIOS[ID_USUARIO_ATUAL].id_usuario == ID_USUARIO_ATUAL) ? 
                             TABELA_USUARIOS[ID_USUARIO_ATUAL].nome : "unknown";
                             
        printf("FS (%s, Inode #%d)> ", nome_usuario, ID_DIRETORIO_ATUAL);
        
        if (fgets(comando, sizeof(comando), stdin) == NULL) break;
        
        // Remove a quebra de linha do final
        comando[strcspn(comando, "\n")] = 0; 
        
        // --- ANÁLISE DE COMANDO ---
        char *token = strtok(comando, " "); 
        char *arg_resto = strtok(NULL, ""); // CAPTURA TUDO QUE SOBROU DA LINHA

        if (token == NULL) {
            continue;
        }
        
        // --- COMANDOS BÁSICOS DE ESTRUTURA ---
        if (strcmp(token, "format") == 0) {
            formatar_disco("my_disk.img");
            ID_DIRETORIO_ATUAL = 1; // Volta para o root
        } 
        else if (strcmp(token, "ls") == 0) {
            listar_diretorio(ID_DIRETORIO_ATUAL);
        }
        else if (strcmp(token, "touch") == 0) {
            if (arg_resto) criar_arquivo(arg_resto);
            else printf("Uso: touch <nome_arquivo>\n");
        }
        else if (strcmp(token, "mkdir") == 0) {
            if (arg_resto) criar_diretorio(arg_resto);
            else printf("Uso: mkdir <nome_diretorio>\n");
        }
        else if (strcmp(token, "cd") == 0) {
            if (arg_resto) mudar_diretorio(arg_resto);
            else printf("Uso: cd <nome_diretorio>\n");
        }
        else if (strcmp(token, "rm") == 0) {
            if (arg_resto) deletar_arquivo(arg_resto);
            else printf("Uso: rm <nome_arquivo>\n");
        }
        
        // --- COMANDOS DE MANIPULAÇÃO DE ARQUIVOS (I/O e CÓPIA) ---
        else if (strcmp(token, "cat") == 0) { 
            if (arg_resto) {
                ler_arquivo(arg_resto);
            } else {
                printf("Uso: cat <nome_arquivo>\n");
            }
        }
        
        // Comando ECHO: Espera o formato "echo <conteúdo> <nome_arquivo>"
        else if (strcmp(token, "echo") == 0) { 
            char *nome_arq_str = NULL;
            char *conteudo_str = NULL;

            if (arg_resto) {
                // Procura o último espaço para separar o nome do arquivo do conteúdo
                char *ultimo_espaco = strrchr(arg_resto, ' ');
                
                if (ultimo_espaco) {
                    nome_arq_str = ultimo_espaco + 1;
                    *ultimo_espaco = '\0'; // Separa o conteúdo do nome
                    conteudo_str = arg_resto; 
                }
            } 
            
            if (conteudo_str && nome_arq_str) {
                // Remove aspas duplas, se presentes
                if (conteudo_str[0] == '"') conteudo_str++;
                size_t len = strlen(conteudo_str);
                if (len > 0 && conteudo_str[len - 1] == '"') {
                     conteudo_str[len - 1] = '\0';
                }
                
                escrever_arquivo(nome_arq_str, conteudo_str); 
                
            } else {
                printf("Uso: echo \"<conteudo>\" <nome_arquivo>\n");
                printf("Exemplo: echo \"Teste de escrita\" meu_arq.txt\n");
            }
        }
        
        // Comando CP: Espera o formato "cp <origem> <destino>"
        else if (strcmp(token, "cp") == 0) {
            if (arg_resto) {
                char *origem = strtok(arg_resto, " ");
                char *destino = strtok(NULL, "");
                if (origem && destino) copiar_arquivo(origem, destino);
                else printf("Uso: cp <origem> <destino>\n");
            } else {
                printf("Uso: cp <origem> <destino>\n");
            }
        }
        
        // Comando MV: Espera o formato "mv <origem> <destino>"
        else if (strcmp(token, "mv") == 0) {
            if (arg_resto) {
                char *origem = strtok(arg_resto, " ");
                char *destino = strtok(NULL, "");
                if (origem && destino) renomear_mover(origem, destino);
                else printf("Uso: mv <origem> <destino>\n");
            } else {
                printf("Uso: mv <origem> <destino>\n");
            }
        }
        
        // --- COMANDOS DE SEGURANÇA E TESTE ---
        
        // Comando CHMOD: Espera o formato "chmod <permissao_octal> <nome_arq>"
        else if (strcmp(token, "chmod") == 0) { 
            if (arg_resto) {
                char *str_permissoes = strtok(arg_resto, " ");
                char *nome_arq = strtok(NULL, ""); // Resto é o nome do arquivo

                if (str_permissoes && nome_arq) {
                    // Converte a string octal (ex: "755") para uint16_t
                    uint16_t novas_permissoes = (uint16_t)strtol(str_permissoes, NULL, 8); 

                    uint32_t id_inode = buscar_entrada(ID_DIRETORIO_ATUAL, nome_arq);

                    if (id_inode != 0) {
                        modificar_permissoes(id_inode, novas_permissoes);
                    } else {
                        printf("[Chmod] ERRO: Arquivo ou diretorio '%s' nao encontrado.\n", nome_arq);
                    }
                } else {
                    printf("Uso: chmod <permissao_octal> <nome_arquivo_ou_dir>\n");
                }
            } else {
                printf("Uso: chmod <permissao_octal> <nome_arquivo_ou_dir>\n");
            }
        }
        
        // Comando TEMPORÁRIO SETUSER (Simula login/troca de usuário)
        else if (strcmp(token, "setuser") == 0) { 
            if (arg_resto) {
                uint8_t novo_id = (uint8_t)atoi(arg_resto);
                
                if (novo_id > 0 && novo_id < MAX_USUARIOS) {
                    // Inicializa o usuário se ele não existir
                    if (TABELA_USUARIOS[novo_id].id_usuario == 0) {
                        TABELA_USUARIOS[novo_id].id_usuario = novo_id;
                        TABELA_USUARIOS[novo_id].id_grupo_principal = GRUPO_DEFAULT;
                        char nome_usr[16];
                        sprintf(nome_usr, "user%d", novo_id);
                        strncpy(TABELA_USUARIOS[novo_id].nome, nome_usr, NOME_USUARIO_MAX);
                    }
                    
                    ID_USUARIO_ATUAL = novo_id;
                    ID_GRUPO_ATUAL = TABELA_USUARIOS[novo_id].id_grupo_principal; 
                    
                    printf("[SimOp] Usuario atual mudado para ID %d (%s). Grupo atual: %d.\n", 
                           ID_USUARIO_ATUAL, TABELA_USUARIOS[ID_USUARIO_ATUAL].nome, ID_GRUPO_ATUAL);
                } else {
                    printf("ID de usuario invalido (1 a %d).\n", MAX_USUARIOS - 1);
                }
            } else {
                printf("Uso: setuser <ID>\n");
            }
        }

        // Comando de TESTE: write_large (para escrita de múltiplos blocos)
        else if (strcmp(token, "write_large") == 0) {
            if (!arg_resto) {
                printf("Uso: write_large <nome_arquivo> <tamanho_em_bytes>\n");
                printf("Exemplo: write_large grande.txt 8192\n");
                continue;
            }

            char *nome_arq = strtok(arg_resto, " ");
            char *str_tamanho = strtok(NULL, "");
            
            if (nome_arq && str_tamanho) {
                size_t tamanho = (size_t)atoi(str_tamanho);
                
                // Limite de teste (por exemplo, 2x o tamanho do bloco para testar dois ponteiros)
                if (tamanho == 0 || tamanho > MAX_PONTEIROS_BLOCOS * TAMANHO_BLOCO) {
                    printf("[Teste] ERRO: Tamanho de teste invalido. Use um valor entre 1 e %u.\n", 
                           MAX_PONTEIROS_BLOCOS * TAMANHO_BLOCO);
                    continue;
                }
                
                // 1. Cria a string de teste (aloca 1 byte extra para o \0)
                char *conteudo_teste = (char *)malloc(tamanho + 1);
                if (!conteudo_teste) {
                    printf("[Teste] ERRO: Falha na alocacao de memoria para teste.\n");
                    continue;
                }
                
                // Preenche a string com caracteres 'X'
                memset(conteudo_teste, 'X', tamanho);
                conteudo_teste[tamanho] = '\0'; // Terminador nulo
                
                // 2. Chama a função de escrita
                int resultado = escrever_arquivo(nome_arq, conteudo_teste);
                
                free(conteudo_teste);

                if (resultado == 0) {
                    printf("[Teste] Escrita de %zu bytes concluida com sucesso.\n", tamanho);
                }
                
            } else {
                 printf("Uso: write_large <nome_arquivo> <tamanho_em_bytes>\n");
            }
        }
        
        // --- COMANDO DE SAÍDA ---
        else if (strcmp(token, "exit") == 0) {
            printf("Saindo do simulador.\n");
            break;
        } 
        
        // --- COMANDO DESCONHECIDO ---
        else {
            printf("Comando desconhecido. Comandos disponiveis: format, touch, mkdir, cd, ls, rm, echo, cat, chmod, setuser, cp, mv, write_large, exit.\n");
        }
    }
    return 0;
}