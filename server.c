/*
 * Servidor simples para demonstração no projeto ESF
 * Compilação:  gcc server.c -o server
 * Execução:    ./server <porta>
 * Exemplo:     ./server 12345
 *
 * Depois, em outro terminal, use:
 * telnet 127.0.0.1 12345
 *
 * Autor: Exemplo de referência
 * Data:  2025
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>

#define BACKLOG 5       // Número de conexões pendentes
#define BUF_SIZE 1024   // Tamanho do buffer de leitura/escrita

// Funções para imprimir menus e manipular opções:
void send_main_menu(int client_sock);
void handle_engineer_menu(int client_sock);
void handle_ngo_menu(int client_sock);
void handle_admin_menu(int client_sock);

// Função auxiliar para enviar dados (strings) ao cliente
void send_msg(int client_sock, const char *msg) {
    send(client_sock, msg, strlen(msg), 0);
}

// Função principal que lida com cada conexão em separado
void handle_client(int client_sock) {
    char buffer[BUF_SIZE];
    int bytes_read;
    
    // Envia um cabeçalho de boas-vindas
    send_msg(client_sock, "\nBem-vindo ao servidor ESF (Engenheiros Sem Fronteiras)!\n");
    send_main_menu(client_sock);

    // Loop principal de interação
    while (1) {
        memset(buffer, 0, BUF_SIZE);
        bytes_read = recv(client_sock, buffer, BUF_SIZE - 1, 0);
        
        if (bytes_read <= 0) {
            // Se bytes_read <= 0, significa que o cliente encerrou ou houve erro
            printf("Cliente desconectado.\n");
            break;
        }

        // Remover \r\n do final (se vier do Telnet)
        buffer[strcspn(buffer, "\r\n")] = 0;

        // Verificar opção selecionada
        if (strcmp(buffer, "1") == 0) {
            handle_engineer_menu(client_sock);
            send_main_menu(client_sock);
        } 
        else if (strcmp(buffer, "2") == 0) {
            handle_ngo_menu(client_sock);
            send_main_menu(client_sock);
        }
        else if (strcmp(buffer, "3") == 0) {
            handle_admin_menu(client_sock);
            send_main_menu(client_sock);
        }
        else if (strcmp(buffer, "4") == 0) {
            send_msg(client_sock, "Encerrando conexão...\n");
            break;
        }
        else {
            send_msg(client_sock, "Opção inválida. Tente novamente.\n\n");
            send_main_menu(client_sock);
        }
    }

    close(client_sock);
}

// Menu principal
void send_main_menu(int client_sock) {
    char menu[] =
        "\n--- Menu Principal ---\n"
        "1) Sou Engenheiro Voluntário\n"
        "2) Sou Organização (ONG)\n"
        "3) Sou Administrador\n"
        "4) Sair\n"
        "Selecione a opção: ";
    send_msg(client_sock, menu);
}

// Menu do engenheiro voluntário
void handle_engineer_menu(int client_sock) {
    // Aqui podemos exibir subopções relacionadas ao engenheiro
    // Exemplo: Registrar, Listar Desafios, etc.
    char menu[] =
        "\n--- Menu Engenheiro ---\n"
        "1) Registrar engenheiro (futuro)\n"
        "2) Listar desafios (futuro)\n"
        "3) Voltar ao menu principal\n"
        "Digite qualquer tecla para voltar...\n";
    send_msg(client_sock, menu);

    // Espera alguma entrada para voltar
    char buffer[BUF_SIZE];
    recv(client_sock, buffer, BUF_SIZE - 1, 0);
}

// Menu da ONG
void handle_ngo_menu(int client_sock) {
    char menu[] =
        "\n--- Menu Organização (ONG) ---\n"
        "1) Registrar ONG (futuro)\n"
        "2) Adicionar desafio (futuro)\n"
        "3) Ver voluntários inscritos (futuro)\n"
        "4) Voltar ao menu principal\n"
        "Digite qualquer tecla para voltar...\n";
    send_msg(client_sock, menu);

    char buffer[BUF_SIZE];
    recv(client_sock, buffer, BUF_SIZE - 1, 0);
}

// Menu do administrador
void handle_admin_menu(int client_sock) {
    char menu[] =
        "\n--- Menu Administrador ---\n"
        "1) Gerenciar engenheiros (futuro)\n"
        "2) Gerenciar organizações (futuro)\n"
        "3) Ver logs do servidor (futuro)\n"
        "4) Voltar ao menu principal\n"
        "Digite qualquer tecla para voltar...\n";
    send_msg(client_sock, menu);

    char buffer[BUF_SIZE];
    recv(client_sock, buffer, BUF_SIZE - 1, 0);
}

int main(int argc, char *argv[]) {
    int sockfd, new_fd;
    struct sockaddr_in my_addr;
    struct sockaddr_in their_addr;
    socklen_t sin_size;
    int port;

    if (argc != 2) {
        fprintf(stderr, "Uso: %s <porta>\n", argv[0]);
        exit(1);
    }

    port = atoi(argv[1]);

    // Cria socket
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Erro ao criar socket");
        exit(1);
    }

    // Configura struct de endereço
    memset(&my_addr, 0, sizeof(my_addr));
    my_addr.sin_family = AF_INET;
    my_addr.sin_port = htons(port);
    my_addr.sin_addr.s_addr = INADDR_ANY;

    // Faz bind
    if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1) {
        perror("Erro no bind");
        close(sockfd);
        exit(1);
    }

    // Começa a escutar
    if (listen(sockfd, BACKLOG) == -1) {
        perror("Erro no listen");
        close(sockfd);
        exit(1);
    }

    printf("Servidor rodando na porta %d. Aguardando conexões...\n", port);

    while (1) {
        sin_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &sin_size);
        if (new_fd == -1) {
            perror("Erro no accept");
            continue;
        }

        printf("Conexão recebida de %s\n", inet_ntoa(their_addr.sin_addr));

        // Aqui podemos criar um processo ou thread para lidar com cada cliente
        // Para simplicidade, vamos tratar de modo sequencial
        // Em um projeto real, você poderia usar fork() ou pthread_create()
        
        handle_client(new_fd);
    }

    close(sockfd);
    return 0;
}
