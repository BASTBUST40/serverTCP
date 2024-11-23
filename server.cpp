#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <fstream>
#include <string>
#include <signal.h>
#include <sys/stat.h> 
#include <sys/types.h>
#include <sys/select.h>

#define BUF_SIZE 1024 

// Структура для хранения данных о файле
struct FileData {
    char filename[256]; // Имя файла (максимум 256 символов)
    int filesize; // Размер файла в байтах
};

// Структура для сообщения об успехе/неудаче
struct SuccessMessage {
    bool success; // Флаг успешного завершения операции (true - успех, false - ошибка)
};

// Обработчик сигнала для завершения сервера
void signalHandler(int signum) {
    std::cout << "Получен сигнал " << signum << ". Завершение сервера..." << std::endl;
    exit(0);
}

int main(int argc, char *argv[]) {
    // Определение порта и директории для сохранения файлов 
    // (можете использовать аргументы командной строки)
    if (argc != 3) {
        std::cerr << "Использование: " << argv[0] << " <порт> <директория_для_сохранения>" << std::endl;
        return 1;
    }
    int port = std::stoi(argv[1]); // Преобразование строки в целое число
    std::string saveDirectory = argv[2]; 

    // Установка обработчика сигнала для завершения
    signal(SIGINT, signalHandler); // Обработка сигнала Ctrl+C
    signal(SIGTERM, signalHandler); // Обработка сигнала завершения процесса

    // Создание TCP-сокета
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        std::cerr << "Ошибка создания сокета" << std::endl;
        return 1;
    }

    // Заполнение структуры адреса
    struct sockaddr_in serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr)); // Обнуление структуры
    serverAddr.sin_family = AF_INET; // IPv4
    serverAddr.sin_addr.s_addr = INADDR_ANY; // Любой доступный адрес
    serverAddr.sin_port = htons(port); // Преобразование порта в сетевой порядок байтов

    // Привязка сокета к адресу
    if (bind(sockfd, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        std::cerr << "Ошибка привязки сокета" << std::endl;
        return 1;
    }

    // Прослушивание на сокете
    if (listen(sockfd, 5) < 0) {
        std::cerr << "Ошибка прослушивания на сокете" << std::endl;
        return 1;
    }

    std::cout << "Сервер запущен на порту " << port << " и сохраняет файлы в " << saveDirectory << std::endl;

    fd_set readfds;
    int max_sd;
    int activity;
    int new_socket;
    int addrlen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;

    // Массив для хранения клиентских сокетов
    int client_sockets[30];
    for (int i = 0; i < 30; i++) {
        client_sockets[i] = 0;
    }

    while (true) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        max_sd = sockfd;

        // Добавление клиентских сокетов в набор для мониторинга
        for (int i = 0; i < 30; i++) {
            int sd = client_sockets[i];
            if (sd > 0) {
                FD_SET(sd, &readfds);
            }
            if (sd > max_sd) {
                max_sd = sd;
            }
        }

        // Ожидание активности на сокетах
        activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
        if ((activity < 0) && (errno != EINTR)) {
            std::cerr << "Ошибка в select" << std::endl;
        }

        // Если есть новое соединение
        if (FD_ISSET(sockfd, &readfds)) {
            if ((new_socket = accept(sockfd, (struct sockaddr*)&clientAddr, (socklen_t*)&addrlen)) < 0) {
                std::cerr << "Ошибка принятия соединения" << std::endl;
                continue;
            }

            std::cout << "Новое соединение, сокет: " << new_socket << ", IP: " << inet_ntoa(clientAddr.sin_addr) << ", порт: " << ntohs(clientAddr.sin_port) << std::endl;

            // Добавление нового сокета в массив
            for (int i = 0; i < 30; i++) {
                if (client_sockets[i] == 0) {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
        }

        // Проверка других сокетов на наличие данных
        for (int i = 0; i < 30; i++) {
            int sd = client_sockets[i];

            if (FD_ISSET(sd, &readfds)) {
                struct FileData fileData;
                if (recv(sd, &fileData, sizeof(fileData), 0) <= 0) {
                    getpeername(sd, (struct sockaddr*)&clientAddr, (socklen_t*)&addrlen);
                    std::cout << "Отключение клиента, IP: " << inet_ntoa(clientAddr.sin_addr) << ", порт: " << ntohs(clientAddr.sin_port) << std::endl;

                    // Закрытие сокета и удаление из массива
                    close(sd);
                    client_sockets[i] = 0;
                } else {
                    // Извлечение имени файла без "jpeg/"
                    std::string filename(fileData.filename); 
                    size_t pos = filename.find("jpeg/"); // Поиск подстроки "jpeg/"
                    if (pos != std::string::npos) {
                        filename = filename.substr(pos + 5); // Удаление "jpeg/" из имени файла
                    }

                    // Создание полного пути к файлу
                    std::string fullPath = saveDirectory + "/" + filename;

                    // Создание файла для записи
                    std::ofstream file(fullPath, std::ios::binary); 
                    if (!file.is_open()) {
                        std::cerr << "Ошибка создания файла: " << fullPath << std::endl;
                        // Отправляем сообщение об ошибке клиенту
                        SuccessMessage successMsg;
                        successMsg.success = false;
                        send(sd, &successMsg, sizeof(successMsg), 0);
                        close(sd); // Закрываем соединение с клиентом
                        client_sockets[i] = 0;
                        continue; // Переход к следующему файлу
                    }

                    // Получение данных файла от клиента
                    char buffer[BUF_SIZE];
                    int bytesReceived = 0;
                    while (bytesReceived < fileData.filesize) {
                        int bytesRead = recv(sd, buffer, BUF_SIZE, 0);
                        if (bytesRead < 0) {
                            std::cerr << "Ошибка получения данных файла" << std::endl;
                            // Отправляем сообщение об ошибке клиенту
                            SuccessMessage successMsg;
                            successMsg.success = false;
                            send(sd, &successMsg, sizeof(successMsg), 0);
                            file.close(); // Закрываем файл, если произошла ошибка
                            close(sd); // Закрываем соединение с клиентом
                            client_sockets[i] = 0;
                            continue; // Переходим к следующему файлу
                        }
                        file.write(buffer, bytesRead); // Запись полученных данных в файл
                        bytesReceived += bytesRead; // Обновление количества полученных байт
                    }

                    // Проверка целостности
                    if (bytesReceived != fileData.filesize) {
                        std::cerr << "Ошибка получения файла: получено не все данные" << std::endl;
                        // Отправляем сообщение об ошибке клиенту
                        SuccessMessage successMsg;
                        successMsg.success = false;
                        send(sd, &successMsg, sizeof(successMsg), 0);
                        file.close(); // Закрываем файл, если произошла ошибка
                        close(sd); // Закрываем соединение с клиентом
                        client_sockets[i] = 0;
                        continue; // Переходим к следующему файлу
                    }

                    // Закрываем файл
                    file.close(); 

                    // Отправляем сообщение об успехе клиенту
                    SuccessMessage successMsg;
                    successMsg.success = true;
                    send(sd, &successMsg, sizeof(successMsg), 0);

                    // Закрываем соединение с клиентом
                    close(sd);
                    client_sockets[i] = 0;
                }
            }
        }
    }

    close(sockfd);
    return 0;
}