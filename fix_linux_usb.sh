#!/usr/bin/env bash
set -e


sudo usermod -aG dialout "$USER" 2>/dev/null || true
sudo usermod -aG uucp "$USER" 2>/dev/null || true


UDEV_RULE='/etc/udev/rules.d/99-arduino-serial.rules'
sudo tee "$UDEV_RULE" > /dev/null << 'EOF'
KERNEL=="ttyUSB[0-9]*", MODE="0666", GROUP="dialout"
KERNEL=="ttyACM[0-9]*", MODE="0666", GROUP="dialout"
EOF

sudo udevadm control --reload-rules
sudo udevadm trigger

for dev in /dev/ttyUSB* /dev/ttyACM*; do
    if [ -e "$dev" ]; then
        sudo chmod 666 "$dev"
        echo "Доступ открыт для: $dev"
    fi
done

echo "Готово! Теперь VS Code (PlatformIO) и Go-сервер могут подключаться к Arduino без ошибок."
