package api

type MorsePayloadDTO struct {
	Mode string `json:"mode"`
	Char string `json:"char"`
}

func NewMorsePayloadDTO(mode string, char string) MorsePayloadDTO {
	return MorsePayloadDTO{
		Mode: mode,
		Char: char,
	}
}
