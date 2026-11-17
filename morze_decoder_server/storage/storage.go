package storage

import (
	"sync"
)

type MorsePayload struct {
	mode string
	char string
}

func NewMorsePayload(mode string, char string) *MorsePayload {
	return &MorsePayload{
		mode: mode,
		char: char,
	}
}

type Storage struct {
	messages []MorsePayload
	mu       sync.RWMutex
}

func NewStorage() *Storage {
	return &Storage{
		messages: make([]MorsePayload, 0),
	}
}

func (s *Storage) Add(p MorsePayload) {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.messages = append(s.messages, p)
}

func (s *Storage) GetAll() []MorsePayload {
	s.mu.RLock()
	defer s.mu.RUnlock()

	tmp := make([]MorsePayload, len(s.messages))
	for k, v := range s.messages {
		tmp[k] = v
	}
	return tmp
}

func (m *MorsePayload) GetMode() string {
	return m.mode
}

func (m *MorsePayload) GetChar() string {
	return m.char
}

func (s *Storage) RemoveLast() {
	s.mu.Lock()
	defer s.mu.Unlock()

	if len(s.messages) > 0 {
		s.messages = s.messages[:len(s.messages)-1]
	}
}

func (s *Storage) Clear() {
	s.mu.Lock()
	defer s.mu.Unlock()

	s.messages = make([]MorsePayload, 0)
}
