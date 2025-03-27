/*
  Servidor básico em C para o projeto "Engenheiros Sem Fronteiras"
  Funcionalidades (F3, F4, F5, F6) demonstradas de forma simplificada.

  Compilação (exemplo):
    gcc -o servidor servidor.c

  Execução:
    ./servidor <porta>

  Depois, testar via telnet (em outro terminal):
    telnet 127.0.0.1 <porta>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

// --------------------------------------------------
// Definições de estruturas e listas ligadas
// --------------------------------------------------

typedef enum {
    VOLUNTARIO,   // Engenheiro voluntário
    ASSOCIACAO,   // Organização sem fins lucrativos
    ADMIN         // Administrador
} UserType;

#define MAX_STR 100

// Estrutura para armazenar os dados do engenheiro
typedef struct Engineer {
    char nomeCompleto[MAX_STR];
    char oeNumber[MAX_STR];
    char especialidade[MAX_STR];
    char instituicao[MAX_STR];
    int  aindaEstudante; // 0 ou 1
    char areasExpertise[MAX_STR];
    char email[MAX_STR];
    char telefone[MAX_STR]; // opcional
    char login[MAX_STR];
    char senha[MAX_STR];
} Engineer;

// Estrutura para armazenar os dados da associação
typedef struct Association {
    char nomeOrganizacao[MAX_STR];
    char nif[MAX_STR];
    char email[MAX_STR];
    char endereco[MAX_STR];
    char descricaoAtividades[MAX_STR];
    char telefone[MAX_STR]; // opcional
    char login[MAX_STR];
    char senha[MAX_STR];
} Association;

// Estrutura para um "desafio"
typedef struct Challenge {
    char nomeDesafio[MAX_STR];
    char descricao[MAX_STR];
    char tipoEngenheiro[MAX_STR];
    int horasEstimadas;

    struct Challenge *next;
} Challenge;

// Estrutura genérica de usuário (pode ser VOLUNTARIO, ASSOCIACAO ou ADMIN)
typedef struct User {
    UserType userType;
    // Guardamos os dados específicos em uniões ou ponteiros para simplicidade
    Engineer engineerData;
    Association assocData;

    struct User *next;
} User;

// Estrutura para armazenar candidaturas
typedef struct Application {
    Challenge *desafio;
    User *engenheiro;
    User *associacao;
    int status; // 0: pendente, 1: aceito, 2: rejeitado
    char mensagem[MAX_STR]; // Mensagem opcional da associação

    struct Application *next;
} Application;

// Listas ligadas para usuários, desafios e candidaturas
User *listaUsuarios = NULL;
Challenge *listaDesafios = NULL;
Application *listaCandidaturas = NULL;


// --------------------------------------------------
// Funções de manipulação de listas
// --------------------------------------------------

// Insere usuário no início da lista (poderia ser no fim, se preferir)
void insereUsuario(User *u) {
    u->next = listaUsuarios;
    listaUsuarios = u;
}

// Localiza usuário pelo login e senha (para "login" no sistema)
User* encontraUsuario(const char* login, const char* senha) {
    User *aux = listaUsuarios;
    while(aux) {
        if (aux->userType == VOLUNTARIO) {
            if (strcmp(aux->engineerData.login, login) == 0 &&
                strcmp(aux->engineerData.senha, senha) == 0) {
                return aux;
            }
        } else if (aux->userType == ASSOCIACAO) {
            if (strcmp(aux->assocData.login, login) == 0 &&
                strcmp(aux->assocData.senha, senha) == 0) {
                return aux;
            }
        } else if (aux->userType == ADMIN) {
            // No exemplo, os dados do admin estão em assocData (pode ser alterado conforme sua necessidade)
            if (strcmp(aux->assocData.login, login) == 0 &&
                strcmp(aux->assocData.senha, senha) == 0) {
                return aux;
            }
        }
        aux = aux->next;
    }
    return NULL;
}


// Adiciona um desafio
void insereDesafio(Challenge *c) {
    c->next = listaDesafios;
    listaDesafios = c;
}

// Lista todos os desafios para engenheiros verem
void listaTodosDesafios(int sockfd) {
    char buffer[1024];
    Challenge *aux = listaDesafios;

    if(!aux) {
        snprintf(buffer, sizeof(buffer), "Nenhum desafio cadastrado no momento.\n");
        send(sockfd, buffer, strlen(buffer), 0);
        return;
    }

    snprintf(buffer, sizeof(buffer), "=== Lista de Desafios ===\n");
    send(sockfd, buffer, strlen(buffer), 0);

    while(aux) {
        snprintf(buffer, sizeof(buffer),
                 "Nome: %s\nDescricao: %s\nTipo de Engenheiro: %s\nHoras Estimadas: %d\n\n",
                 aux->nomeDesafio, aux->descricao,
                 aux->tipoEngenheiro, aux->horasEstimadas);
        send(sockfd, buffer, strlen(buffer), 0);

        aux = aux->next;
    }
}

// Função para criar nova candidatura
void insereCandidatura(Challenge *desafio, User *engenheiro, User *associacao) {
    Application *app = (Application*)malloc(sizeof(Application));
    if(!app) return;

    app->desafio = desafio;
    app->engenheiro = engenheiro;
    app->associacao = associacao;
    app->status = 0; // pendente
    memset(app->mensagem, 0, MAX_STR);

    app->next = listaCandidaturas;
    listaCandidaturas = app;
}

// Função para encontrar um desafio pelo nome
Challenge* encontraDesafio(const char* nome) {
    Challenge *aux = listaDesafios;
    while(aux) {
        if(strcmp(aux->nomeDesafio, nome) == 0) {
            return aux;
        }
        aux = aux->next;
    }
    return NULL;
}

// Função para listar candidaturas de um engenheiro
void listaCandidaturasEngenheiro(int sockfd, User *engenheiro) {
    char buffer[1024];
    Application *aux = listaCandidaturas;
    int encontrou = 0;

    while(aux) {
        if(aux->engenheiro == engenheiro) {
            encontrou = 1;
            snprintf(buffer, sizeof(buffer),
                    "\nDesafio: %s\nStatus: %s\nMensagem: %s\n",
                    aux->desafio->nomeDesafio,
                    aux->status == 0 ? "Pendente" : 
                    aux->status == 1 ? "Aceito" : "Rejeitado",
                    aux->mensagem[0] ? aux->mensagem : "Sem mensagem");
            send(sockfd, buffer, strlen(buffer), 0);
        }
        aux = aux->next;
    }

    if(!encontrou) {
        send(sockfd, "Você não tem candidaturas.\n", 28, 0);
    }
}

// Função para listar candidaturas para uma associação
void listaCandidaturasAssociacao(int sockfd, User *associacao) {
    char buffer[1024];
    Application *aux = listaCandidaturas;
    int encontrou = 0;

    while(aux) {
        if(aux->associacao == associacao && aux->status == 0) {
            encontrou = 1;
            snprintf(buffer, sizeof(buffer),
                    "\nDesafio: %s\nEngenheiro: %s\nStatus: Pendente\n",
                    aux->desafio->nomeDesafio,
                    aux->engenheiro->engineerData.nomeCompleto);
            send(sockfd, buffer, strlen(buffer), 0);
        }
        aux = aux->next;
    }

    if(!encontrou) {
        send(sockfd, "Não há candidaturas pendentes.\n", 31, 0);
    }
}

// Função para processar uma candidatura
void processaCandidatura(Application *app, int aceitar, const char *mensagem) {
    app->status = aceitar ? 1 : 2;
    if(mensagem) {
        strncpy(app->mensagem, mensagem, MAX_STR-1);
        app->mensagem[MAX_STR-1] = 0;
    }
}

// --------------------------------------------------
// Funções de registro (F4) e menu principal (F3)
// --------------------------------------------------

// Função auxiliar para remover \r\n
void removeNewline(char *str) {
    str[strcspn(str, "\r\n")] = 0;
}

// Cadastro de usuário VOLUNTARIO
void cadastrarVoluntario(int sockfd) {
    char buffer[1024];
    User *u = (User *) malloc(sizeof(User));
    if(!u) return;

    u->userType = VOLUNTARIO;

    // Coletando dados
    send(sockfd, "Nome completo: ", 16, 0);
    recv(sockfd, u->engineerData.nomeCompleto, MAX_STR, 0);
    removeNewline(u->engineerData.nomeCompleto);

    send(sockfd, "OE number: ", 11, 0);
    recv(sockfd, u->engineerData.oeNumber, MAX_STR, 0);
    removeNewline(u->engineerData.oeNumber);

    send(sockfd, "Especialidade: ", 16, 0);
    recv(sockfd, u->engineerData.especialidade, MAX_STR, 0);
    removeNewline(u->engineerData.especialidade);

    send(sockfd, "Instituicao de emprego: ", 25, 0);
    recv(sockfd, u->engineerData.instituicao, MAX_STR, 0);
    removeNewline(u->engineerData.instituicao);

    send(sockfd, "Ainda é estudante? (0/1): ", 27, 0);
    recv(sockfd, buffer, 1024, 0);
    removeNewline(buffer);
    u->engineerData.aindaEstudante = atoi(buffer);

    send(sockfd, "Areas de expertise: ", 20, 0);
    recv(sockfd, u->engineerData.areasExpertise, MAX_STR, 0);
    removeNewline(u->engineerData.areasExpertise);

    send(sockfd, "Email: ", 7, 0);
    recv(sockfd, u->engineerData.email, MAX_STR, 0);
    removeNewline(u->engineerData.email);

    send(sockfd, "Telefone (opcional): ", 21, 0);
    recv(sockfd, u->engineerData.telefone, MAX_STR, 0);
    removeNewline(u->engineerData.telefone);

    send(sockfd, "Login desejado: ", 17, 0);
    recv(sockfd, u->engineerData.login, MAX_STR, 0);
    removeNewline(u->engineerData.login);

    send(sockfd, "Senha desejada: ", 17, 0);
    recv(sockfd, u->engineerData.senha, MAX_STR, 0);
    removeNewline(u->engineerData.senha);

    insereUsuario(u);
    send(sockfd, "Voluntario cadastrado com sucesso!\n", 36, 0);
}

// Cadastro de usuário ASSOCIACAO
void cadastrarAssociacao(int sockfd) {
    char buffer[1024];
    User *u = (User *) malloc(sizeof(User));
    if(!u) return;

    u->userType = ASSOCIACAO;

    send(sockfd, "Nome da Organizacao: ", 22, 0);
    recv(sockfd, u->assocData.nomeOrganizacao, MAX_STR, 0);
    removeNewline(u->assocData.nomeOrganizacao);

    send(sockfd, "NIF: ", 6, 0);
    recv(sockfd, u->assocData.nif, MAX_STR, 0);
    removeNewline(u->assocData.nif);

    send(sockfd, "Email: ", 7, 0);
    recv(sockfd, u->assocData.email, MAX_STR, 0);
    removeNewline(u->assocData.email);

    send(sockfd, "Endereco: ", 10, 0);
    recv(sockfd, u->assocData.endereco, MAX_STR, 0);
    removeNewline(u->assocData.endereco);

    send(sockfd, "Descricao de Atividades: ", 25, 0);
    recv(sockfd, u->assocData.descricaoAtividades, MAX_STR, 0);
    removeNewline(u->assocData.descricaoAtividades);

    send(sockfd, "Telefone (opcional): ", 21, 0);
    recv(sockfd, u->assocData.telefone, MAX_STR, 0);
    removeNewline(u->assocData.telefone);

    send(sockfd, "Login desejado: ", 17, 0);
    recv(sockfd, u->assocData.login, MAX_STR, 0);
    removeNewline(u->assocData.login);

    send(sockfd, "Senha desejada: ", 17, 0);
    recv(sockfd, u->assocData.senha, MAX_STR, 0);
    removeNewline(u->assocData.senha);

    insereUsuario(u);
    send(sockfd, "Associacao cadastrada com sucesso!\n", 36, 0);
}

// Menu para voluntário (engenheiro)
void menuVoluntario(int sockfd, User* u) {
    char buffer[1024];
    int sair = 0;

    while(!sair) {
        snprintf(buffer, sizeof(buffer),
                 "\n--- MENU VOLUNTARIO ---\n"
                 "1. Listar desafios disponiveis\n"
                 "2. Candidatar-se a um desafio\n"
                 "3. Ver minhas candidaturas\n"
                 "0. Sair\n"
                 "Escolha: ");
        send(sockfd, buffer, strlen(buffer), 0);

        int bytesRecv = recv(sockfd, buffer, sizeof(buffer), 0);
        if(bytesRecv <= 0) {
            sair = 1;
            break;
        }

        int op = atoi(buffer);
        switch(op) {
            case 1:
                listaTodosDesafios(sockfd);
                break;
            case 2: {
                // F7: Engenheiro se candidata a um desafio
                listaTodosDesafios(sockfd);
                send(sockfd, "\nDigite o nome do desafio que deseja se candidatar: ", 51, 0);
                recv(sockfd, buffer, sizeof(buffer), 0);
                removeNewline(buffer);

                Challenge *desafio = encontraDesafio(buffer);
                if(desafio) {
                    // Encontra a associação que criou o desafio
                    User *aux = listaUsuarios;
                    while(aux) {
                        if(aux->userType == ASSOCIACAO) {
                            insereCandidatura(desafio, u, aux);
                            send(sockfd, "Candidatura enviada com sucesso!\n", 33, 0);
                            break;
                        }
                        aux = aux->next;
                    }
                } else {
                    send(sockfd, "Desafio não encontrado.\n", 25, 0);
                }
                break;
            }
            case 3:
                // F9: Ver status das candidaturas
                listaCandidaturasEngenheiro(sockfd, u);
                break;
            case 0:
            default:
                sair = 1;
                break;
        }
    }
}

// Menu para associação
void menuAssociacao(int sockfd, User* u) {
    char buffer[1024];
    int sair = 0;

    while(!sair) {
        snprintf(buffer, sizeof(buffer),
                 "\n--- MENU ASSOCIACAO ---\n"
                 "1. Adicionar Desafio\n"
                 "2. Listar Desafios\n"
                 "3. Gerenciar Candidaturas\n"
                 "0. Sair\n"
                 "Escolha: ");
        send(sockfd, buffer, strlen(buffer), 0);

        int bytesRecv = recv(sockfd, buffer, sizeof(buffer), 0);
        if(bytesRecv <= 0) {
            sair = 1;
            break;
        }

        int op = atoi(buffer);
        switch(op) {
            case 1: {
                // F6: adicionar desafio
                Challenge *c = (Challenge*)malloc(sizeof(Challenge));
                if(!c) break;
                memset(c, 0, sizeof(Challenge));

                send(sockfd, "Nome do desafio: ", 18, 0);
                recv(sockfd, c->nomeDesafio, MAX_STR, 0);
                removeNewline(c->nomeDesafio);

                send(sockfd, "Descricao do desafio: ", 23, 0);
                recv(sockfd, c->descricao, MAX_STR, 0);
                removeNewline(c->descricao);

                send(sockfd, "Tipo de engenheiro necessario: ", 31, 0);
                recv(sockfd, c->tipoEngenheiro, MAX_STR, 0);
                removeNewline(c->tipoEngenheiro);

                send(sockfd, "Horas estimadas (numero): ", 26, 0);
                recv(sockfd, buffer, 1024, 0);
                removeNewline(buffer);
                c->horasEstimadas = atoi(buffer);

                insereDesafio(c);
                send(sockfd, "Desafio adicionado com sucesso!\n", 33, 0);
                break;
            }
            case 2:
                listaTodosDesafios(sockfd);
                break;
            case 3: {
                // F8: Gerenciar candidaturas
                char mensagem[MAX_STR];
                listaCandidaturasAssociacao(sockfd, u);
                
                send(sockfd, "\nDigite o nome do desafio para processar (ou 0 para voltar): ", 60, 0);
                recv(sockfd, buffer, sizeof(buffer), 0);
                removeNewline(buffer);

                if(strcmp(buffer, "0") == 0) break;

                Challenge *desafio = encontraDesafio(buffer);
                if(!desafio) {
                    send(sockfd, "Desafio não encontrado.\n", 25, 0);
                    break;
                }

                // Encontra a candidatura
                Application *aux = listaCandidaturas;
                while(aux) {
                    if(aux->desafio == desafio && aux->associacao == u && aux->status == 0) {
                        send(sockfd, "Aceitar candidatura? (1: Sim, 2: Não): ", 39, 0);
                        recv(sockfd, buffer, sizeof(buffer), 0);
                        removeNewline(buffer);
                        int aceitar = atoi(buffer) == 1;

                        send(sockfd, "Mensagem para o candidato: ", 27, 0);
                        recv(sockfd, mensagem, MAX_STR, 0);
                        removeNewline(mensagem);

                        processaCandidatura(aux, aceitar, mensagem);
                        send(sockfd, "Candidatura processada com sucesso!\n", 36, 0);
                        break;
                    }
                    aux = aux->next;
                }
                break;
            }
            case 0:
            default:
                sair = 1;
                break;
        }
    }
}

// Menu para administrador (F5)
void menuAdmin(int sockfd, User* u) {
    char buffer[1024];
    int sair = 0;

    while(!sair) {
        snprintf(buffer, sizeof(buffer),
                 "\n--- MENU ADMINISTRADOR ---\n"
                 "1. (Futuro) Validar cadastro de usuarios\n"
                 "2. (Futuro) Remover usuarios\n"
                 "0. Sair\n"
                 "Escolha: ");
        send(sockfd, buffer, strlen(buffer), 0);

        int bytesRecv = recv(sockfd, buffer, sizeof(buffer), 0);
        if(bytesRecv <= 0) {
            sair = 1;
            break;
        }

        int op = atoi(buffer);
        switch(op) {
            case 1:
                // Exemplo de funcionalidade futura
                send(sockfd, "Funcionalidade de validacao ainda nao implementada.\n", 54, 0);
                break;
            case 2:
                // Exemplo de funcionalidade futura
                send(sockfd, "Funcionalidade de remocao ainda nao implementada.\n", 51, 0);
                break;
            case 0:
            default:
                sair = 1;
                break;
        }
    }
}

// Função que envia o menu inicial para o cliente e gerencia login/registro
void menuInicial(int sockfd) {
    char buffer[1024];
    int sair = 0;

    while(!sair) {
        snprintf(buffer, sizeof(buffer),
                 "\n=== BEM-VINDO AO ESF (Engenheiros Sem Fronteiras) ===\n"
                 "1. Login\n"
                 "2. Cadastrar-se como Voluntario\n"
                 "3. Cadastrar-se como Associacao\n"
                 "0. Sair\n"
                 "Escolha: ");
        send(sockfd, buffer, strlen(buffer), 0);

        int bytesRecv = recv(sockfd, buffer, sizeof(buffer), 0);
        if(bytesRecv <= 0) {
            break;
        }

        int op = atoi(buffer);
        switch(op) {
            case 1: {
                // Fazer login
                char login[MAX_STR], senha[MAX_STR];
                send(sockfd, "Login: ", 7, 0);
                recv(sockfd, login, MAX_STR, 0);
                removeNewline(login);

                send(sockfd, "Senha: ", 7, 0);
                recv(sockfd, senha, MAX_STR, 0);
                removeNewline(senha);

                User* userLogado = encontraUsuario(login, senha);
                if(userLogado) {
                    // Se for do tipo
                    if(userLogado->userType == VOLUNTARIO) {
                        menuVoluntario(sockfd, userLogado);
                    } else if(userLogado->userType == ASSOCIACAO) {
                        menuAssociacao(sockfd, userLogado);
                    } else if(userLogado->userType == ADMIN) {
                        menuAdmin(sockfd, userLogado);
                    }
                } else {
                    send(sockfd, "Login ou senha invalidos.\n", 27, 0);
                }
                break;
            }
            case 2:
                cadastrarVoluntario(sockfd);
                break;
            case 3:
                cadastrarAssociacao(sockfd);
                break;
            case 0:
            default:
                sair = 1;
                break;
        }
    }
}

// --------------------------------------------------
// Função principal do servidor (F3: "main server deve enviar menus")
// --------------------------------------------------
int main(int argc, char *argv[])
{
    if(argc < 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    int sockfd, newsockfd;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;

    // Cria socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("Erro ao abrir socket");
        exit(1);
    }

    // Preenche serv_addr
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;  // Recebe de qualquer interface
    serv_addr.sin_port = htons(port);

    // Faz bind
    if(bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Erro no bind");
        close(sockfd);
        exit(1);
    }

    // Listen
    listen(sockfd, 5);
    printf("Servidor rodando na porta %d...\n", port);

    // Para fins de exemplo, criaremos um usuário Admin fixo
    {
        User *admin = (User*)malloc(sizeof(User));
        memset(admin, 0, sizeof(User));
        admin->userType = ADMIN;
        strcpy(admin->assocData.login, "admin");
        strcpy(admin->assocData.senha, "admin");
        admin->next = NULL;
        insereUsuario(admin);
    }

    // Loop infinito aguardando conexões
    while(1) {
        clilen = sizeof(cli_addr);
        newsockfd = accept(sockfd, (struct sockaddr*)&cli_addr, &clilen);
        if(newsockfd < 0) {
            perror("Erro no accept");
            continue;
        }

        // Processa conexão em processo filho (fork) ou thread
        int pid = fork();
        if(pid < 0) {
            perror("Erro no fork");
            close(newsockfd);
            continue;
        }

        if(pid == 0) {
            // Processo filho
            close(sockfd);
            // Envia menu inicial
            menuInicial(newsockfd);

            close(newsockfd);
            exit(0);
        } else {
            // Processo pai
            close(newsockfd);
        }
    }

    close(sockfd);
    return 0;
}
