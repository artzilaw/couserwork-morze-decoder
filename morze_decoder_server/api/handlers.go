package api

import (
	"encoding/json"
	"fmt"
	"log"
	"morse-backend/storage"
	"net/http"
	"strings"

	"go.bug.st/serial"
)

type HTTPHandlers struct {
	storage *storage.Storage
	port    serial.Port
}

func NewHTTPHandlers(storage *storage.Storage, port serial.Port) *HTTPHandlers {
	return &HTTPHandlers{
		storage: storage,
		port:    port,
	}
}

func (h *HTTPHandlers) GetSymbol(w http.ResponseWriter, r *http.Request) {
	var morsePayloadDTO MorsePayloadDTO
	if err := json.NewDecoder(r.Body).Decode(&morsePayloadDTO); err != nil {
		log.Printf("[HTTP POST Ошибка JSON]: %v\n", err)
		http.Error(w, NewErrorDTO(err.Error()).ToString(), http.StatusBadRequest)
		return
	}

	mode := strings.ToUpper(strings.TrimSpace(morsePayloadDTO.Mode))
	ch := morsePayloadDTO.Char

	if mode == "COMMAND" && strings.EqualFold(ch, "BACKSPACE") {
		if _, err := h.port.Write([]byte{'\b'}); err != nil {
			log.Printf("[Ошибка записи в UART]: %v\n", err)
		}
		h.storage.RemoveLast()
		log.Println("[HTTP POST -> Arduino]: Отправлен BACKSPACE")
	} else if mode == "COMMAND" && strings.EqualFold(ch, "SPACE") {
		if _, err := h.port.Write([]byte{' '}); err != nil {
			log.Printf("[Ошибка записи в UART]: %v\n", err)
		}
		h.storage.Add(*storage.NewMorsePayload("COMMAND", " "))
		log.Println("[HTTP POST -> Arduino]: Отправлен SPACE")
	} else {
		if _, err := h.port.Write([]byte(ch)); err != nil {
			log.Printf("[Ошибка записи в UART]: %v\n", err)
		}
		if mode == "" {
			mode = "SERVER"
		}
		h.storage.Add(*storage.NewMorsePayload(mode, ch))
		log.Printf("[HTTP POST -> Arduino]: Отправлен символ '%s' (Режим: %s)\n", ch, mode)
	}

	w.Header().Set("Content-Type", "application/json")
	w.WriteHeader(http.StatusCreated)
	_ = json.NewEncoder(w).Encode(NewMorsePayloadDTO(mode, ch))
}

func (h *HTTPHandlers) GetAllSymbols(w http.ResponseWriter, r *http.Request) {
	data := h.storage.GetAll()

	responseDTOs := make([]MorsePayloadDTO, 0, len(data))
	for _, item := range data {
		responseDTOs = append(responseDTOs, NewMorsePayloadDTO(item.GetMode(), item.GetChar()))
	}

	w.Header().Set("Content-Type", "application/json")
	b, err := json.MarshalIndent(responseDTOs, "", "\t")
	if err != nil {
		http.Error(w, NewErrorDTO(err.Error()).ToString(), http.StatusInternalServerError)
		return
	}

	if _, err := w.Write(b); err != nil {
		fmt.Println(err)
	}
}
