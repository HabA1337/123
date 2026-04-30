#!/bin/bash

# Запуск серверов.
# На вход:
#   1) каталог data
#
# Для каждого файла из data поднимается отдельный сервер.
# Сервер запускается так:
#   ./server <filename> <port>
#
# Порты начинаются с 40000.
# Во временный файл сохраняется соответствие:
#   port|data_file

set -u

SERVER_BIN="./server"
START_PORT=40000
PORTS_FILE="/tmp/test_server_ports.list"

err() {
	echo "ERROR: $*" >&2
}

require_command() {
	local cmd="$1"
	command -v "$cmd" >/dev/null 2>&1 || {
		err "не найдена команда: $cmd"
		exit 1
	}
}

check_dir_exists() {
	local dir="$1"
	local name="$2"

	if [ ! -e "$dir" ]; then
		err "$name не существует: $dir"
		exit 1
	fi

	if [ ! -d "$dir" ]; then
		err "$name не является каталогом: $dir"
		exit 1
	fi

	if [ ! -r "$dir" ]; then
		err "$name недоступен для чтения: $dir"
		exit 1
	fi
}

if [ "$#" -ne 1 ]; then
	err "нужен 1 аргумент: data_dir"
	echo "Использование: $0 <data_dir>" >&2
	exit 1
fi

data_dir="$1"

check_dir_exists "$data_dir" "каталог data"

if [ ! -e "$SERVER_BIN" ]; then
	err "не найден $SERVER_BIN"
	exit 1
fi

if [ ! -x "$SERVER_BIN" ]; then
	err "$SERVER_BIN не исполняемый"
	exit 1
fi

require_command find
require_command sort
require_command grep

mapfile -t data_files < <(find "$data_dir" -maxdepth 1 -type f | sort)

if [ "${#data_files[@]}" -eq 0 ]; then
	err "в каталоге data нет файлов: $data_dir"
	exit 1
fi

: > "$PORTS_FILE" || {
	err "не удалось создать $PORTS_FILE"
	exit 1
}

echo "Каталог data: $data_dir"
echo "Количество файлов data: ${#data_files[@]}"
echo "Поднимаю серверы с порта $START_PORT"

port=$START_PORT

for data_file in "${data_files[@]}"
do
	log="server-port-${port}.log"
	export LEAKTRACE_FILE="server-port-${port}.leak"

	echo "------------------------------------------------------------"
	echo "DATA_FILE=$data_file"
	echo "PORT=$port"
	echo "LOG=$log"
	echo "LEAK=$LEAKTRACE_FILE"

	"$SERVER_BIN" "$data_file" "$port" >"$log" 2>&1 &
	pid=$!

	sleep 1

	if ! kill -0 "$pid" 2>/dev/null; then
		err "сервер не поднялся для файла $data_file на порту $port"
		err "смотрите лог: $log"
		exit 1
	fi

	echo "${port}" >> "$PORTS_FILE"
	((port++))
done

echo "------------------------------------------------------------"
echo "Серверы подняты"
echo "Список портов и файлов: $PORTS_FILE"
