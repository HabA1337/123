#!/bin/bash

# Тестирование серверов.
# На вход:
#   1) каталог data
#   2) файл insert
#   3) файл commands
#
# Логика:
#   1. Проверяем, что серверов столько же, сколько файлов в data.
#   2. Для каждого сервера один раз отправляем insert_file как есть.
#   3. Для каждого сервера запускаем несколько клиентов одновременно.
#      Все они выполняют один и тот же commands_file.
#   4. Сравниваем результаты через diff.
#   5. Отправляем stop на каждый сервер.

set -u

CLIENT_BIN="./client"
HOST="127.0.0.1"
PORTS_FILE="/tmp/test_server_ports.list"
TMP_DIR="/tmp/test_clients"

err() {
	echo "ERROR: $*" >&2
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

check_file_exists() {
	local file="$1"
	local name="$2"

	if [ ! -e "$file" ]; then
		err "$name не существует: $file"
		exit 1
	fi

	if [ ! -f "$file" ]; then
		err "$name не является файлом: $file"
		exit 1
	fi

	if [ ! -r "$file" ]; then
		err "$name недоступен для чтения: $file"
		exit 1
	fi
}

count_files() {
	local dir="$1"
	find "$dir" -maxdepth 1 -type f | wc -l
}

send_stop() {
	local port="$1"
	printf 'stop;\n' | "$CLIENT_BIN" "$HOST" "$port" >/dev/null 2>"$TMP_DIR/stop-${port}.err"
}

if [ "$#" -ne 3 ]; then
	err "нужно 3 аргумента: data_dir insert_file commands_file"
	echo "Использование: $0 <data_dir> <insert_file> <commands_file>" >&2
	exit 1
fi

data_dir="$1"
insert_file="$2"
commands_file="$3"

check_dir_exists "$data_dir" "каталог data"
check_file_exists "$insert_file" "файл insert"
check_file_exists "$commands_file" "файл commands"

if [ ! -e "$CLIENT_BIN" ]; then
	err "не найден $CLIENT_BIN"
	exit 1
fi

if [ ! -x "$CLIENT_BIN" ]; then
	err "$CLIENT_BIN не исполняемый"
	exit 1
fi

if [ ! -f "$PORTS_FILE" ]; then
	err "не найден файл со списком портов: $PORTS_FILE"
	err "сначала запустите run_server.sh"
	exit 1
fi

if [ ! -s "$PORTS_FILE" ]; then
	err "файл со списком портов пустой: $PORTS_FILE"
	exit 1
fi

if [ ! -s "$insert_file" ]; then
	err "файл insert пустой: $insert_file"
	exit 1
fi

if [ ! -s "$commands_file" ]; then
	err "файл commands пустой: $commands_file"
	exit 1
fi

data_count=$(count_files "$data_dir")
if [ "$data_count" -le 0 ]; then
	err "в каталоге data нет файлов: $data_dir"
	exit 1
fi

mkdir -p "$TMP_DIR" || {
	err "не удалось создать $TMP_DIR"
	exit 1
}

port_count=$(wc -l < "$PORTS_FILE")
if [ "$port_count" -ne "$data_count" ]; then
	err "число серверов ($port_count) не совпадает с количеством файлов data ($data_count)"
	exit 1
fi

if ! command -v diff >/dev/null 2>&1; then
	err "не найдена команда diff"
	exit 1
fi

if ! command -v grep >/dev/null 2>&1; then
	err "не найдена команда grep"
	exit 1
fi

if ! command -v wc >/dev/null 2>&1; then
	err "не найдена команда wc"
	exit 1
fi

echo "------------------------------------------------------------"
echo "DATA_DIR=$data_dir"
echo "INSERT_FILE=$insert_file"
echo "COMMANDS_FILE=$commands_file"
echo "SERVERS=$data_count"
echo "CLIENTS_PER_SERVER=$data_count"
echo "------------------------------------------------------------"

# Один раз выполняем insert для каждого сервера
while IFS='|' read -r port data_file
do
	[ -z "${port:-}" ] && continue

	echo "Insert для сервера port=$port file=$data_file"

	if ! "$CLIENT_BIN" "$HOST" "$port" < "$insert_file" > "$TMP_DIR/insert-${port}.out" 2> "$TMP_DIR/insert-${port}.err"
	then
		err "ошибка insert на порту $port"
		err "stderr: $TMP_DIR/insert-${port}.err"
		exit 1
	fi
done < "$PORTS_FILE"

# Для каждого сервера одновременно запускаем несколько клиентов
while IFS='|' read -r port data_file
do
	[ -z "${port:-}" ] && continue

	(
		echo "Запуск клиентов для port=$port"

		for ((i=0; i<data_count; i++))
		do
			"$CLIENT_BIN" "$HOST" "$port" < "$commands_file" > "$TMP_DIR/res-${port}-${i}" 2> "$TMP_DIR/res-${port}-${i}.err" &
		done

		wait

		for ((i=0; i<data_count; i++))
		do
			if [ ! -f "$TMP_DIR/res-${port}-${i}" ]; then
				err "нет результата клиента: $TMP_DIR/res-${port}-${i}"
				exit 1
			fi
		done

		for ((i=1; i<data_count; i++))
		do
			if ! diff -q "$TMP_DIR/res-${port}-0" "$TMP_DIR/res-${port}-${i}" >/dev/null 2>&1
			then
				err "результаты на порту $port различаются: 0 и $i"
				diff -q "$TMP_DIR/res-${port}-0" "$TMP_DIR/res-${port}-${i}"
				exit 1
			fi
		done

		result=$(grep '^[[:space:]]*Student' "$TMP_DIR/res-${port}-0" | wc -l)
		echo "OK: port=$port answer=$result"
		echo "------------------------------------------------------------"
	) &
done < "$PORTS_FILE"

wait

# Останавливаем серверы
while IFS='|' read -r port data_file
do
	[ -z "${port:-}" ] && continue

	echo "Останавливаю сервер port=$port"
	if ! send_stop "$port"; then
		err "не удалось отправить stop на порт $port"
		exit 1
	fi
done < "$PORTS_FILE"

echo "Все тесты завершены"
