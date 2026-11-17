package api

import (
	"errors"
	"net/http"

	"github.com/gorilla/mux"
)

type Server struct {
	handlers *HTTPHandlers
}

func NewServer(handlers *HTTPHandlers) *Server {
	return &Server{
		handlers: handlers,
	}
}

func (s *Server) Run() error {
	router := mux.NewRouter().StrictSlash(true)

	router.Path("/api/messages").Methods(http.MethodPost).HandlerFunc(s.handlers.GetSymbol)
	router.Path("/api/messages").Methods(http.MethodGet).HandlerFunc(s.handlers.GetAllSymbols)

	err := http.ListenAndServe(":9091", router)

	if err != nil {
		if errors.Is(err, http.ErrServerClosed) {
			return nil
		}
		return err
	}

	return nil
}
