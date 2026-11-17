package hardware

import (
	"bufio"
	"encoding/json"
	"errors"
	"fmt"
	"log"
	"morse-backend/storage"
	"os"
	"strings"

	"go.bug.st/serial"
)

type incomingData struct {
	Mode string `json:"mode"`
	Char string `json:"char"`
}

// DetectPort автоматически ищет подключенную плату Arduino (Linux: /dev/ttyUSB*, /dev/ttyACM*; Windows: COM*)
func DetectPort() (string, error) {
	ports, err := serial.GetPortsList()
	if err != nil {
		return "", fmt.Errorf("ошибка получения списка портов: %w", err)
	}

	// Для линукса
	for _, p := range ports {
		if strings.HasPrefix(p, "/dev/ttyUSB") || strings.HasPrefix(p, "/dev/ttyACM") {
			return p, nil
		}
	}

	// Для винды
	for _, p := range ports {
		if strings.HasPrefix(strings.ToUpper(p), "COM") {
			return p, nil
		}
	}

	for _, candidate := range []string{"/dev/ttyUSB0", "/dev/ttyUSB1", "/dev/ttyACM0", "/dev/ttyACM1"} {
		if _, statErr := os.Stat(candidate); statErr == nil {
			return candidate, nil
		}
	}

	return "", errors.New("не найдено подключенных устройств Arduino (/dev/ttyUSB*, /dev/ttyACM* или COM*)")
}

func InitPort(portName string) (serial.Port, string, error) {
	resolvedPort := strings.TrimSpace(portName)
	if resolvedPort == "" || strings.EqualFold(resolvedPort, "auto") {
		detected, err := DetectPort()
		if err != nil {
			return nil, "", err
		}
		resolvedPort = detected
	}

	mode := &serial.Mode{
		BaudRate: 9600,
	}
	port, err := serial.Open(resolvedPort, mode)
	if err != nil {
		if os.IsPermission(err) || strings.Contains(strings.ToLower(err.Error()), "permission denied") {
			return nil, resolvedPort, fmt.Errorf(
				"нет прав доступа к порту %s: %w\nПодсказка для Linux: выполните `sudo usermod -aG dialout $USER && sudo chmod 666 %s`",
				resolvedPort, err, resolvedPort,
			)
		}
		return nil, resolvedPort, err
	}
	return port, resolvedPort, nil
}

func StartSerialListener(port serial.Port, store *storage.Storage) {
	log.Println("Слушатель последовательного порта запущен...")
	scanner := bufio.NewScanner(port)

	for scanner.Scan() {
		line := strings.TrimSpace(scanner.Text())
		if line == "" {
			continue
		}

		var data incomingData
		if err := json.Unmarshal([]byte(line), &data); err != nil {
			log.Printf("[Arduino LOG]: %s\n", line)
			continue // Игнорируем обычный текстовый/служебный вывод Arduino
		}

		if data.Mode == "COMMAND" && data.Char == "BACKSPACE" {
			store.RemoveLast()
			log.Println("Удален последний символ (Backspace)")
		} else if data.Mode == "COMMAND" && data.Char == "SPACE" {
			payload := storage.NewMorsePayload("COMMAND", "[SPACE]")
			store.Add(*payload)
			log.Println("Сохранено: [Пробел]")
		} else if data.Mode == "COMMAND" && data.Char == "CLEAR" {
			store.Clear()
			log.Println("Очищен весь буфер сообщений (Clear)")
		} else {
			payload := storage.NewMorsePayload(data.Mode, data.Char)
			store.Add(*payload)
			log.Printf("Сохранено: %s (Шифр: %s)\n", data.Char, data.Mode)
		}
	}

	if err := scanner.Err(); err != nil {
		log.Printf("Ошибка чтения из последовательного порта: %v\n", err)
	}
}
