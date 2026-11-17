package main

import (
	"flag"
	"log"
	"morse-backend/api"
	"morse-backend/hardware"
	"morse-backend/storage"
	"os"
)

func main() {
	defaultPort := os.Getenv("SERIAL_PORT")
	if defaultPort == "" {
		defaultPort = "auto"
	}
	portFlag := flag.String("port", defaultPort, "Последовательный порт Arduino (например, /dev/ttyUSB0, /dev/ttyACM0, COM3 или auto)")
	flag.Parse()

	store := storage.NewStorage()
	port, resolvedPort, err := hardware.InitPort(*portFlag)
	if err != nil {
		log.Fatalf("Не удалось открыть порт (%s): %v", resolvedPort, err)
	}
	defer port.Close()
	log.Printf("Успешно подключено к Arduino на порту: %s (9600 бод)\n", resolvedPort)

	go hardware.StartSerialListener(port, store)
	handlers := api.NewHTTPHandlers(store, port)
	server := api.NewServer(handlers)

	log.Println("HTTP-сервер запущен на http://localhost:9091")
	log.Fatal(server.Run())
}
