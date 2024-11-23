<H1>Сервер для передачи данных (фотографий) через udp-протокол. </H1>
Решение является элементом комплексной задачи (прием-передача файлов большого размера) в совокупности с проектом [Клиент](https://github.com/flametoxic/ImageClient/tree/main)

Для запуска сервера используйте операционную систему Linux Debian.
Далее необходимо проделать шаги:
1. [Скачайте исходный код](https://github.com/flametoxic/ImageServer/archive/refs/heads/main.zip);
2. Распакуйте zip-архив в отдельную папку и откройте;
3. Если у вас не установлены build-essential и cmake, то выполните шаги 4-7, иначе перейдите к 8 шагу;
4. Зайдите в свойства файла [start.sh](https://github.com/flametoxic/ImageServer/blob/main/start.sh);
5. Переключите тумблер "Исполняемый как приложение";
6. Нажмите правой клавишей мыши по файлу start.sh. В контекстном меню выберите "Запустить как приложение";
7. Согласитесь со всеми установками в терминале, если они потребуют "Д" илл "Y";
8. Зайдите в свойства файла start_build.sh и повторите шаг 5;
9. Нажмите по файлу [start.build.sh](https://github.com/flametoxic/ImageServer/blob/main/start_build.sh) правой кнопкой мыши и выберите пункт "Запустить как приложение";
10. После сборки решения, перейдите в папку build;
11. Нажмите правую клавишу мыши и выберите пункт "Открыть в терминале";
12. Вводите в терминал команду:
 "./server <Порт на котором работает сервер> <Путь для сохранения файлов>".