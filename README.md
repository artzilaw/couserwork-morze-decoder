## Инструкция по запуску, прошивке и тестированию на Linux

### Шаг 1. Открыть доступ к USB-порту (`/dev/ttyUSB0`)
Выполните один раз в терминале (потребуется ввести пароль `sudo`):
```bash
bash ~/Документы/coursework/fix_linux_usb.sh
```

### Шаг 2. Открыть проект в VS Code и загрузить прошивку
1. Запустите VS Code с папкой проекта:
   ```bash
   code ~/Документы/coursework
   ```
2. Для сборки и прошивки используйте меню задач VS Code (`Ctrl+Shift+P` → `Tasks: Run Task`):
   - **`2. Собрать прошивку Arduino (PlatformIO)`** — проверка компиляции.
   - **`3. Загрузить прошивку на Arduino Uno`** — прошивка платы Arduino Uno.
   - **`4. Загрузить прошивку на Arduino Nano (ATmega328P)`** — прошивка платы Arduino Nano.

   Либо напрямую из терминала:
   ```bash
   cd ~/Документы/coursework/morse_decoder_firmware
   ~/.local/bin/pio run -e uno -t upload
   ```
   *(Если у вас плата Arduino Nano, замените `-e uno` на `-e nano` или `-e nano_old`).*

### Шаг 3. Запустить Go-сервер
После того как прошивка загружена (порт `/dev/ttyUSB0` должен быть свободен от других программ):
```bash
cd ~/Документы/coursework/morze_decoder_server
./morse-server
```
Вы увидите:
```text
Успешно подключено к Arduino на порту: /dev/ttyUSB0 (9600 бод)
Слушатель последовательного порта запущен...
HTTP-сервер запущен на http://localhost:9091
```
