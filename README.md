(README.md): Sistema de Arquivos Simulador FS - https://github.com/httpscaiow/Sistema_de_ArquivosFS/tree/master (Link de Acesso)

1.0 Introdução: 

1.1 Objetivos Gerais: 
Este projeto consiste em um simulador de sistema de arquivos simples (FS) implementado em C, desenhado para demonstrar e reforçar os conceitos teóricos de Gerenciamento de Sistemas de Arquivos vistos na disciplina de Sistemas Operacionais.
O simulador implementa estruturas de metadados inspiradas em sistemas UNIX (como Inodes e permissões RWX) e operações básicas de gerenciamento de diretórios e arquivos.

1.2 Introdução de Compilação e Execução: 
Pré Requisitos: 
Ferramenta de Compilação C e Makefile. 
Compilação: 
Para compilar o simulador, utilize o Makefile fornecido no diretório raiz do projeto:
make all
Este comando gera o executável principal do shell do sistema de arquivos 
(simulador_fs).
Execução:
O simulador deve ser inicializado com o comando de formatação na primeira vez, seguido da execução do shell:
Formatar o Disco Virtual (Inicialização): ./simulador_fs format meu_disco.img
Esta ação chama a função formatar_disco em utilidades_fs.c, inicializando o  Superbloco, o Bitmap e o Inode Raiz.

- > /simulador_fs shell - Isso inicia a interface de linha de comando, permitindo o uso de comandos como ls, mkdir, cd, touch, chmod, etc.

1.3. Escolhas de Design e Estruturas de Dados
As estruturas de dados foram projetadas para replicar o layout de metadados de um sistema de arquivos. Utilizamos a lógica de Inodes para separar logicamente os dados do arquivo de seus atributos.

Estruturas FCB / Inode Simulado: 
Estas estruturas de dados são fundamentais para o funcionamento do sistema de arquivos, pois elas definem onde o arquivo está, quem pode acessá-lo e quais são seus atributos.

Inode (struct Inode): É o File Control Block (FCB) ou o "RG do Arquivo". Armazena todos os atributos sobre o arquivo (tamanho, permissões, proprietário, tipo) e os endereços dos blocos de dados. É a estrutura central, pois permite que o sistema encontre o conteúdo físico do arquivo sem depender do seu nome.	

Superbloco (struct Superbloco): É o "Cabeçalho" e o gerenciador do Sistema. Contém dados essenciais sobre o disco virtual, como o tamanho total do disco, a localização da Tabela de Inodes e o Bitmap, que rastreia quais blocos de dados estão livres ou ocupados.

Entrada de Diretório (struct EntradaDiretorio): É o mapeamento de Nomes. É a estrutura que liga o nome visível pelo usuário (o nome lógico, como relatorio.txt) ao Inode ID (o número do FCB) correspondente.

Conexão: O sistema usa o Nome (Entrada de Diretório) para obter o Inode ID, que aponta para o Inode (FCB), que, por sua vez, localiza os Blocos de Dados do arquivo.

Estrutura de Diretórios em Árvore e Vantagens
A organização dos arquivos no simulador é hierárquica, replicando a estrutura de pastas (folders) de sistemas operacionais.
Implementação: Cada diretório é um Inode (tipo = TIPO_DIRETORIO) cujo bloco de dados armazena uma tabela de Entradas de Diretório.
Hierarquia: A navegação é garantida pelas entradas especiais . (aponta para o próprio diretor) e (aponta para o diretor pai).
Vantagens: O modelo em árvore permite o agrupamento lógico de arquivos e facilita a flexibilidade de nomeação, pois nomes de arquivos podem se repetir em diferentes pastas. Além disso, a busca se torna mais eficiente ao seguir o caminho hierárquico.
O projeto do simulador de sistema de arquivos foi estruturado em módulos distintos, o que é fundamental para a organização, manutenção e escalabilidade do código. Cada arquivo C possui uma responsabilidade clara, tornando fácil identificar onde uma determinada funcionalidade está implementada.
As definições estruturais básicas do sistema são centralizadas no arquivo de cabeçalho definicoes/sistema_arq.h, que declara as structs principais (Inode, Superbloco) e as constantes essenciais (tamanho de bloco, permissões), servindo como a interface primária do sistema de arquivos.
A execução e o tratamento da linha de comando ficam a cargo de codigo_base/main.c, que contém a função main() e o loop do shell, responsável por receber a entrada do usuário e delegar a execução para as funções específicas.
As funções de infraestrutura e gerenciamento de disco de baixo nível estão em codigo_base/utilidades_fs.c. Este módulo é responsável por tarefas como formatar o disco, ler e escrever blocos diretamente no arquivo de imagem e gerenciar o bitmap de blocos livres, além de funções de utilidade como alocar_inode() e modificar_permissoes().
A hierarquia e a manipulação da estrutura de diretórios são tratadas por codigo_base/operacoes_dir.c. Este arquivo implementa funções como criar_diretorio(), mudar_diretorio() (cd), listar_diretorio() (ls) e remover_diretorio() (rmdir), lidando diretamente com o mapeamento de Inode ID/Nome e a travessia da árvore de diretórios.
Por fim, o módulo codigo_base/operacoes_arq.c é dedicado à manipulação do conteúdo e da existência de arquivos, com funções como criar_arquivo() (touch), escrever_arquivo() (write), ler_arquivo() (read) e remover_arquivo() (rm), gerenciando diretamente a alocação e liberação dos blocos de dados. 
1.4. Demonstração dos Conceitos Teóricos

 Conceito de Arquivo e Seus Atributos: O conceito de arquivo é abstraído pela sua entrada na Tabela de Inodes. O Inode (struct Inode) contém todos os atributos essenciais:

Identificação: id_inode (endereço do FCB).
Tipo: tipo (distingue Arquivo de Diretório).
Propriedade e Segurança: id_proprietario e permissoes.
Tempo: tempo_criacao e tempo_modificacao (time_t).
Estrutura Lógica: Mapeamento do nome (na entrada de diretório) para o Inode.

O Mecanismo de Proteção de Acesso (RWX e chmod)
A proteção de acesso é implementada seguindo o modelo Proprietário/Grupo/Outros .

Implementação de chmod: A função modificar_permissoes (utilidades_fs.c) aceita um valor octal (ex: 0755) e armazena os 9 bits de permissão no campo permissoes do Inode. A modificação é restrita ao proprietário do arquivo ou ao Root.
Bitmasks: A função verificar_permissao utiliza bitmasks e shifts (>> SHIFT_PROPRIETARIO, & 0b111) para isolar o trio de bits correto de permissão (R=4, W=2, X=1) e verificar se o usuário atual (Proprietário, Grupo ou Outro) possui a permissão necessária.



Simulação da Alocação de Blocos
A alocação de espaço é simulada através de dois mecanismos:
Bitmap de Blocos: O array BITMAP_BLOCOS_MEMORIA gerencia a alocação de espaço livre em disco. Cada bit representa o status (livre/ocupado) de um bloco no disco virtual.

Alocação Indexada Simplificada: O array blocos_dados dentro da estrutura Inode armazena os endereços (IDs) dos blocos de dados. Este arranjo simula a lógica de um bloco de índice.

Tutorial de Uso do Simulador FS: 
1. Inicialização do Sistema de Arquivos
Antes de usar qualquer comando de arquivo ou diretório, o disco virtual deve ser formatado.
Formatar o Disco: O primeiro passo sempre deve ser formatar o disco virtual. Isso inicializa o Superbloco, o Bitmap e cria o Inode Raiz (#1).
./simulador_fs -> format  
2. Comandos de Gerenciamento de Diretórios
Estes comandos manipulam a estrutura de pastas do sistema.
ls: Lista o conteúdo do diretório atual.
ls <caminho>: Lista o conteúdo de um diretório específico.
cd <diretorio>: Muda o diretório de trabalho atual.
cd ..: Volta para o diretório pai.
cd /: Volta para o diretório raiz.
mkdir <nome>: Cria um novo diretório no local atual.
rmdir <nome>: Remove um diretório, mas somente se estiver vazio.

3. Comandos de Arquivo e Conteúdo
Estes comandos manipulam arquivos e seus metadados.
touch <nome_arquivo>: Cria um novo arquivo vazio.
rm <nome_arquivo>: Deleta (remove) um arquivo.
write <nome_arquivo> "<conteudo>": Escreve o conteúdo fornecido (entre aspas) no arquivo.
read <nome_arquivo>: Exibe o conteúdo do arquivo.
cat <nome_arquivo>: Sinônimo de read.

4. Comandos de Segurança e Metadados
Estes comandos alteram os atributos (FCB/Inode) dos arquivos e diretórios.

chmod <octal> <nome>: Altera as permissões de acesso (R=4, W=2, X=1) do arquivo ou diretório. (Exemplo: chmod 777 meu_arquivo concede todas as permissões a todos.)

stat <nome>: Exibe os metadados (atributos) do arquivo/diretório, incluindo Inode ID, tamanho, permissões octais e IDs de proprietário/grupo.

chown <id_usuario> <nome>: Altera o ID do proprietário do arquivo ou diretório.

5. Comandos de Saída e Informação
exit: Encerra a sessão do shell do simulador e salva o estado do disco.

Exemplo de Fluxo (Root)
Para iniciar rapidamente um teste, você pode usar esta sequência:
./simulador_fs format disco_dev.img
./simulador_fs shell
mkdir documentos
touch documentos/relatorio.txt
cd documentos
ls
stat relatorio.txt
chmod 600 relatorio.txt
write relatorio.txt "Dados confidenciais."
cat relatorio.txt
cd ..
ls
rm documentos/relatorio.txt
rmdir documentos
quit
